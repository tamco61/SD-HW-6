# Performance Design: Кеширование и Rate Limiting

## 1. Анализ производительности

### 1.1. Hot Paths (часто выполняемые операции)

| Endpoint | Метод | Частота | Описание |
|---|---|---|---|
| `GET /v1/users?login=X` | GET | Очень высокая | Просмотр профиля пользователя — вызывается при каждом отображении профиля |
| `GET /v1/routes?login=X` | GET | Высокая | Список маршрутов пользователя — вызывается при навигации по профилю |
| `GET /v1/trips?id=X` | GET | Высокая | Просмотр деталей поездки |
| `GET /v1/users/search?mask=X` | GET | Средняя | Поиск пользователей (зависит от активности) |
| `POST /v1/login` | POST | Средняя | Авторизация, но может стать целью brute-force атак |

### 1.2. Медленные операции

| Операция | Источник задержки | Среднее время |
|---|---|---|
| Запрос к PostgreSQL | Сетевой round-trip + выполнение SQL | 2–15 мс |
| Запрос к MongoDB | Сетевой round-trip + BSON parsing | 3–20 мс |
| `SearchUsersByName` (ILIKE) | Full-scan / index scan с ILIKE | 5–50 мс |
| `GetTrip` (MongoDB) | Чтение документа с embedded данными | 3–10 мс |

### 1.3. Требования к производительности

| Метрика | Целевое значение |
|---|---|
| Время отклика (p95) для GET | < 50 мс |
| Время отклика (p95) для POST | < 100 мс |
| Пропускная способность | > 500 RPS (на один экземпляр) |
| Доступность | > 99.9% |

---

## 2. Стратегия кеширования

### 2.1. Выбор данных для кеширования

#### Критерии отбора:
1. **Часто читаемые данные** — профили пользователей и списки маршрутов запрашиваются многократно.
2. **Редко изменяемые данные** — профиль пользователя создается один раз и практически не меняется.
3. **Повторяемые запросы** — один и тот же пользователь / маршрут запрашивается из разных контекстов.

#### Кешируемые данные:

| Данные | Endpoint | Обоснование |
|---|---|---|
| Профиль пользователя | `GET /v1/users?login=X` | Read-heavy, данные меняются крайне редко |
| Маршруты пользователя | `GET /v1/routes?login=X` | Read-heavy, маршруты добавляются нечасто |

#### Не кешируемые данные:

| Данные | Причина |
|---|---|
| Результаты поиска (`/v1/users/search`) | Высокая кардинальность ключей (каждая маска уникальна), низкий cache hit rate |
| Поездки (`/v1/trips`) | Часто обновляются (добавление участников), нужна актуальность |
| Авторизация (`/v1/login`) | Каждый запрос уникален, результат не повторяется |

### 2.2. Стратегия: Cache-Aside (Lazy Loading)

```
┌─────────┐     1. GET key     ┌─────────┐
│  Client  │ ─────────────────> │  Redis   │
│ (Handler)│ <───────────────── │  Cache   │
│          │   2a. HIT: return  │          │
│          │                    └─────────┘
│          │   2b. MISS
│          │ ─────────────────> ┌─────────┐
│          │                    │PostgreSQL│
│          │ <───────────────── │    DB    │
│          │   3. data          └─────────┘
│          │
│          │ ─────────────────> ┌─────────┐
│          │   4. SET key data  │  Redis   │
└─────────┘     (with TTL)     │  Cache   │
                                └─────────┘
```

**Почему Cache-Aside:**
- **Простота** — приложение полностью контролирует, что и когда кешируется.
- **Устойчивость** — при падении Redis запросы просто идут напрямую в БД (fail-open).
- **Гибкость** — разный TTL для разных типов данных.
- **Lazy Loading** — данные попадают в кеш только при первом запросе, нет лишнего расхода памяти.

**Альтернативы (отклоненные):**
- **Read-Through** — требует промежуточный слой (cache provider), усложняет архитектуру.
- **Write-Through** — синхронная запись в кеш при каждом изменении; избыточно, т.к. данные создаются редко.
- **Write-Back** — отложенная запись; рискованно для основных данных.

### 2.3. TTL (Time To Live)

| Данные | TTL | Обоснование |
|---|---|---|
| Профиль пользователя | 300 с (5 мин) | Данные меняются очень редко, 5 минут — разумный компромисс |
| Маршруты пользователя | 180 с (3 мин) | Маршруты добавляются чаще, чем меняется профиль |

### 2.4. Стратегия инвалидации

Применяется **гибридная стратегия**: TTL + event-driven инвалидация.

| Событие | Инвалидируемый ключ | Механизм |
|---|---|---|
| `POST /v1/users` (создание пользователя) | `cache:user:{login}` | Явный `DEL` после успешного INSERT |
| `POST /v1/routes` (создание маршрута) | `cache:routes:{owner_login}` | Явный `DEL` после успешного INSERT |
| Естественное истечение | Все ключи | TTL автоматически удаляет устаревшие данные |

**Формат ключей:**
- `cache:user:{login}` — JSON с профилем пользователя
- `cache:routes:{login}` — JSON-массив маршрутов пользователя

---

## 3. Стратегия Rate Limiting

### 3.1. Endpoints, требующие rate limiting

| Endpoint | Причина | Риск |
|---|---|---|
| `POST /v1/login` | Защита от brute-force атак на пароли | Высокий |
| `POST /v1/users` | Защита от массовой регистрации (spam) | Средний |
| `GET /v1/users/search` | Предотвращение перегрузки БД (ILIKE) | Средний |

> В текущей реализации rate limiting применен к `POST /v1/login` как к наиболее критичному endpoint.

### 3.2. Алгоритм: Fixed Window Counter

```
Окно 60 секунд:
┌──────────────────────────────────────────────┐
│  Запрос 1  Запрос 2  ...  Запрос 10  │ БЛОК │
│  ✓         ✓              ✓          │  ✗   │
└──────────────────────────────────────────────┘
                                          ↑
                                    Превышен лимит
                                    HTTP 429
```

**Почему Fixed Window Counter:**
- **Простота реализации** — один INCR + EXPIRE в Redis.
- **Атомарность** — Lua-скрипт выполняется атомарно на стороне Redis.
- **Низкая задержка** — один round-trip к Redis (< 1 мс).
- **Минимальное потребление памяти** — один ключ на клиента.

**Альтернативы (отклоненные):**
- **Token Bucket** — более сложная реализация, избыточна для учебного проекта.
- **Sliding Window Log** — хранит timestamp каждого запроса, высокое потребление памяти.
- **Leaking Bucket** — сглаживает трафик, но не подходит для защиты от brute-force (нужен жесткий отсечение).

### 3.3. Lua-скрипт (атомарная операция)

```lua
-- KEYS[1]: ключ rate limit (например, "rate_limit:192.168.1.1")
-- ARGV[1]: максимум запросов (например, "10")
-- ARGV[2]: размер окна в секундах (например, "60")

local current = redis.call("INCR", KEYS[1])
if current == 1 then
    redis.call("EXPIRE", KEYS[1], ARGV[2])
end
local ttl = redis.call("TTL", KEYS[1])
if ttl < 0 then
    ttl = tonumber(ARGV[2])
end
if current > tonumber(ARGV[1]) then
    return {0, current, ttl}  -- denied
else
    return {1, current, ttl}  -- allowed
end
```

### 3.4. Конфигурация лимитов

| Endpoint | Ключ | Лимит | Окно | Обоснование |
|---|---|---|---|---|
| `POST /v1/login` | `rate_limit:{client_ip}` | 10 запросов | 60 секунд | Защита от brute-force: 10 попыток в минуту достаточно для легитимного пользователя |

### 3.5. HTTP заголовки

При каждом ответе от `POST /v1/login` устанавливаются заголовки:

| Заголовок | Описание | Пример |
|---|---|---|
| `X-RateLimit-Limit` | Максимум запросов в окне | `10` |
| `X-RateLimit-Remaining` | Оставшиеся запросы | `7` |
| `X-RateLimit-Reset` | Секунд до сброса окна | `45` |

При превышении лимита:
- HTTP статус: **429 Too Many Requests**
- Тело ответа: `{"error": "Too Many Requests"}`

### 3.6. Fail-Open стратегия

При недоступности Redis rate limiter **пропускает** запросы (fail-open).
Это предотвращает отказ в обслуживании при сбое инфраструктуры кеширования:

```
Redis DOWN → rate_limiter_.Check() → catch exception → return {allowed: true}
```

---

## 4. Реализация

### 4.1. Компонент кеширования (`RedisCacheComponent`)

Файлы: `src/redis_cache.hpp`, `src/redis_cache.cpp`

Методы:
- `Get(key)` → `std::optional<std::string>` — чтение из Redis (GET)
- `Set(key, value, ttl)` — запись с TTL (SET EX)
- `Invalidate(key)` — удаление ключа (DEL)

Все операции обернуты в try-catch — при ошибке Redis запрос продолжает работу без кеша.

### 4.2. Компонент rate limiting (`RateLimiterComponent`)

Файлы: `src/rate_limiter.hpp`, `src/rate_limiter.cpp`

Использует Lua-скрипт через `redis_client_->Eval()` для атомарной проверки.
Возвращает `RateLimitResult` с полями `allowed`, `current`, `ttl`, `limit`.

### 4.3. Кешируемые endpoints

#### `GET /v1/users?login=X`
```
1. cache_key = "cache:user:" + login
2. cached = cache.Get(cache_key)
3. if cached → return cached (HIT)
4. user = pg_storage.GetUserByLogin(login)
5. if !user → return 404
6. result = serialize(user)
7. cache.Set(cache_key, result, 300s)
8. return result
```

#### `GET /v1/routes?login=X`
```
1. cache_key = "cache:routes:" + login
2. cached = cache.Get(cache_key)
3. if cached → return cached (HIT)
4. routes = pg_storage.GetRoutesByUser(login)
5. result = serialize(routes)
6. cache.Set(cache_key, result, 180s)
7. return result
```

### 4.4. Инвалидация

#### `POST /v1/users`
```
1. storage.AddUser(user)
2. cache.Invalidate("cache:user:" + user.login)
```

#### `POST /v1/routes`
```
1. storage.AddRoute(route)
2. cache.Invalidate("cache:routes:" + owner_login)
```

---

## 5. Анализ влияния на производительность

### 5.1. Кеширование

#### Ожидаемые улучшения:

| Метрика | Без кеша | С кешем | Улучшение |
|---|---|---|---|
| Время отклика GET /v1/users (p95) | ~10 мс | ~1 мс | **~90%** |
| Время отклика GET /v1/routes (p95) | ~15 мс | ~1 мс | **~93%** |
| Нагрузка на PostgreSQL | 100% запросов | ~20% запросов* | **~80% снижение** |

\* При cache hit rate ~80%, что типично для профильных данных.

#### Снижение нагрузки на БД:
- Каждый cache hit экономит один SQL-запрос к PostgreSQL.
- При 1000 RPS и 80% hit rate — 800 запросов/сек обслуживаются Redis (< 1 мс), только 200 идут в PostgreSQL.

### 5.2. Rate Limiting

#### Защита от злоупотреблений:
- Brute-force на `/v1/login`: ограничен 10 попытками в минуту на IP.
- При DDoS-атаке сервер не перегружается — Redis отсекает запросы до их обработки.

#### Накладные расходы:
- Один дополнительный round-trip к Redis (~0.5 мс) для каждого POST /v1/login.
- Lua-скрипт выполняется за O(1) — не масштабируется с количеством запросов.

### 5.3. Метрики мониторинга

| Метрика | Описание | Цель |
|---|---|---|
| **Cache Hit Rate** | % запросов, обслуженных из кеша | > 70% |
| **Cache Miss Rate** | % промахов кеша | < 30% |
| **Redis Latency (p99)** | Задержка Redis операций | < 5 мс |
| **Rate Limit Rejections** | Количество отклоненных запросов (429) | Мониторинг аномалий |
| **DB Query Reduction** | Снижение запросов к PostgreSQL | > 50% |

### 5.4. Измерение эффективности кеширования (Cache Hit Rate)

**Формула:**
```
Cache Hit Rate = Cache Hits / (Cache Hits + Cache Misses) × 100%
```

**Способы измерения:**
1. **Логирование** — в `RedisCacheComponent` логируются HIT и MISS события (уровень DEBUG).
2. **Redis INFO** — команда `redis-cli INFO stats` показывает `keyspace_hits` и `keyspace_misses`.
3. **Prometheus метрики** — userver поддерживает экспорт метрик, можно добавить кастомные счетчики.

**Пример мониторинга через Redis CLI:**
```bash
redis-cli INFO stats | grep keyspace
# keyspace_hits:12345
# keyspace_misses:2345
# → Hit Rate = 12345 / (12345 + 2345) = 84.1%
```

---

## 6. Архитектура

```
                    ┌────────────────────────────┐
                    │       HTTP Request          │
                    └─────────────┬──────────────┘
                                  │
                    ┌─────────────▼──────────────┐
                    │     Rate Limiter (Redis)    │
                    │   Fixed Window Counter      │
                    │   Lua script (INCR+EXPIRE)  │
                    └─────────────┬──────────────┘
                                  │
                        ┌─────────▼─────────┐
                        │   allowed?        │
                        │   yes      no     │
                        └──┬──────────┬─────┘
                           │          │
                           │    ┌─────▼──────┐
                           │    │ HTTP 429   │
                           │    │ + headers  │
                           │    └────────────┘
                    ┌──────▼─────────────────┐
                    │   Redis Cache Lookup    │
                    │   GET cache:key         │
                    └──────┬─────────────────┘
                           │
                  ┌────────▼────────┐
                  │   HIT?         │
                  │  yes      no   │
                  └──┬────────┬────┘
                     │        │
              ┌──────▼──┐  ┌──▼──────────────┐
              │ Return  │  │ Query DB        │
              │ cached  │  │ (PostgreSQL /   │
              │ data    │  │  MongoDB)       │
              └─────────┘  └──┬──────────────┘
                              │
                       ┌──────▼──────────┐
                       │ SET cache:key   │
                       │ (with TTL)      │
                       └──┬──────────────┘
                          │
                   ┌──────▼──────┐
                   │ Return data │
                   └─────────────┘
```

---

## 7. Конфигурация Redis

### Docker Compose
```yaml
redis:
  image: redis:7-alpine
  container_name: blablacar-redis
  ports:
    - "6379:6379"
  volumes:
    - redis_data:/data
  healthcheck:
    test: ["CMD", "redis-cli", "ping"]
    interval: 10s
    timeout: 5s
    retries: 5
```

### userver static_config.yaml
```yaml
redis-blabla:
  groups:
    - config_name: redis-blabla
      db: redis-blabla
      sharding_strategy: RedisStandalone
  thread_pools:
    redis_thread_pool_size: 4
    sentinel_thread_pool_size: 1
```

### Secdist (configs/secdist.json)
```json
{
  "redis_settings": {
    "redis-blabla": {
      "password": "",
      "sentinels": [{"host": "localhost", "port": 6379}],
      "shards": [{"name": "shard0"}]
    }
  }
}
```
