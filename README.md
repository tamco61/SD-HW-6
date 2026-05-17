BlablaCar-Service — это REST API приложения на С++ с использованием `userver`, реализующего микросервис для управления пользователями, маршрутами и поездками.

В данном проекте реализована полнофункциональная реляционная база данных на PostgreSQL с оптимизированной схемой, индексами и тестовыми данными.

---

## Схема базы данных

### Сущности и связи

```
┌─────────────┐         ┌──────────────┐         ┌─────────────┐
│   users     │◄────────│  auth_tokens │         │   routes    │
├─────────────┤         ├──────────────┤         ├─────────────┤
│ login (PK)  │         │ token (PK)   │         │ id (PK)     │
│ first_name  │         │ login (FK)   │         │ owner_login │◄───┐
│ last_name   │         │ created_at   │         │ points      │    │
│ password    │         │ expires_at   │         │ created_at  │    │
│ created_at  │         └──────────────┘         └─────────────┘    │
│ updated_at  │                                                    │
└──────┬──────┘                                                    │
       │                                                           │
       │                              ┌─────────────┐              │
       │                              │   trips     │              │
       │                              ├─────────────┤              │
       │                              │ id (PK)     │              │
       │                              │ route_id(FK)├──────────────┘
       │                              │ status      │
       │                              │ departure   │
       │                              │ created_at  │
       │                              └──────┬──────┘
       │                                     │
       │               ┌─────────────────────┘
       │               │
       │    ┌──────────────────────┐
       │    │ trip_participants    │
       │    ├──────────────────────┤
       └────│ login (FK)           │
            │ trip_id (FK)         │
            │ joined_at            │
            └──────────────────────┘
```

### Таблицы

1. **users** - Пользователи системы
   - Первичный ключ: `login`
   - Индексы: по `first_name`, `last_name`, составной по полному имени
   - Ограничения: NOT NULL, CHECK на длину имени/пароля

2. **auth_tokens** - Токены авторизации
   - Первичный ключ: `token`
   - Внешний ключ: `login` → `users(login)` с CASCADE DELETE
   - Индексы: по `login`, по `expires_at` для очистки просроченных

3. **routes** - Маршруты
   - Первичный ключ: `id` (UUID)
   - Внешний ключ: `owner_login` → `users(login)` с CASCADE DELETE
   - Индексы: по `owner_login`, полнотекстовый по `points` (GIN)

4. **trips** - Поездки
   - Первичный ключ: `id` (UUID)
   - Внешний ключ: `route_id` → `routes(id)` с CASCADE DELETE
   - Индексы: по `route_id`, `status`, `departure_time`
   - Ограничения: CHECK status IN ('active', 'completed', 'cancelled')

5. **trip_participants** - Участники поездок (связь многие-ко-многим)
   - Составной первичный ключ: `(trip_id, login)`
   - Внешние ключи: `trip_id` → `trips(id)`, `login` → `users(login)`
   - Индексы: по `trip_id`, по `login`

### Представления (Views)

1. **trip_details** - Поездка с информацией о маршруте и владельце
2. **trip_participants_details** - Участники с их именами

---

## Инструкции по запуску

### Вариант 1: Запуск через Docker Compose (Рекомендуемый)

Этот вариант запускает и PostgreSQL, и API сервис с автоматической инициализацией базы данных:

```bash
docker-compose up --build -d
```

**Что произойдет:**

1. Создается volume `postgres_data` для хранения данных
2. Запускается PostgreSQL 16 Alpine
3. Автоматически выполняются `schema.sql` и `data.sql` при первом запуске
4. Запускается API сервис с подключением к PostgreSQL
5. API будет доступен на порту `8080`
6. PostgreSQL будет доступен на порту `5432`

Проверка статуса:

```bash
docker-compose ps
docker-compose logs postgres
```

### Вариант 2: Запуск только PostgreSQL (для разработки БД)

```bash
docker-compose up -d postgres
```

После запуска можно подключиться к базе:

**Linux/Mac:**

```bash
psql -h localhost -p 5432 -U postgres -d blablacar_db
```

**Windows (PowerShell/CMD):**

```cmd
init_db.bat verify
init_db.bat test
```

Или с установленным PostgreSQL клиентом:

```cmd
psql -h localhost -p 5432 -U postgres -d blablacar_db
Password: postgres_password
```

### Вариант 3: Локальный PostgreSQL + ручная инициализация

Если PostgreSQL уже установлен локально:

**Linux/Mac:**

```bash
# Создание базы данных и схемы
./init_db.sh setup

# Проверка
./init_db.sh verify

# Тестовые запросы
./init_db.sh test

# EXPLAIN анализ
./init_db.sh explain

# Сброс базы данных
./init_db.sh reset
```

**Windows:**

```cmd
REM Создание базы данных и схемы
init_db.bat setup

REM Проверка
init_db.bat verify

REM Тестовые запросы
init_db.bat test

REM EXPLAIN анализ
init_db.bat explain

REM Сброс базы данных
init_db.bat reset
```

### Вариант 4: Запуск Swagger UI для отправки запросов к API

Для интерактивной работы с API и просмотра документации:

```bash
docker-compose -f docker-compose.swagger.yml up -d
```

Swagger UI будет доступен по адресу: http://localhost:8081

### Вариант 5: Сборка исходников и запуск тестов локально (вне Docker)

Для локального запуска и проверки тестов:

```bash
make test-debug
make start-debug
```

---

## Структура файлов БД

```
├── schema.sql              # SQL скрипт для создания схемы БД (таблицы, индексы, ограничения)
├── data.sql                # SQL скрипт для вставки тестовых данных (10+ записей в каждой таблице)
├── queries.sql             # SQL запросы для всех операций из варианта задания
├── optimization.md         # Описание оптимизаций с планами выполнения (EXPLAIN ANALYZE)
├── init_db.sh              # Bash скрипт для инициализации БД (Linux/Mac)
├── init_db.bat             # Batch скрипт для инициализации БД (Windows)
├── docker-compose.yaml     # Docker Compose с PostgreSQL + API
└── README.md               # Данный файл
```

---

## Индексы и оптимизации

### Основные индексы

| Таблица           | Индекс                          | Тип                | Назначение                     |
| ----------------- | ------------------------------- | ------------------ | ------------------------------ |
| users             | `users_pkey`                    | B-tree (PK)        | Быстрый поиск по login         |
| users             | `idx_users_first_name`          | B-tree             | Поиск по имени                 |
| users             | `idx_users_last_name`           | B-tree             | Поиск по фамилии               |
| users             | `idx_users_full_name`           | B-tree (composite) | Поиск по полному имени         |
| auth_tokens       | `auth_tokens_pkey`              | B-tree (PK)        | Валидация токена               |
| auth_tokens       | `idx_auth_tokens_login`         | B-tree (FK)        | Поиск токенов пользователя     |
| auth_tokens       | `idx_auth_tokens_expires_at`    | B-tree             | Очистка просроченных           |
| routes            | `routes_pkey`                   | B-tree (PK)        | Поиск по ID                    |
| routes            | `idx_routes_owner_login`        | B-tree (FK)        | Маршруты пользователя          |
| routes            | `idx_routes_points`             | GIN (full-text)    | Полнотекстовый поиск маршрутов |
| trips             | `trips_pkey`                    | B-tree (PK)        | Поиск по ID                    |
| trips             | `idx_trips_route_id`            | B-tree (FK)        | Поиск по маршруту              |
| trips             | `idx_trips_status`              | B-tree             | Фильтрация по статусу          |
| trips             | `idx_trips_departure_time`      | B-tree             | Сортировка по времени          |
| trip_participants | `trip_participants_pkey`        | B-tree (PK)        | Уникальность участника         |
| trip_participants | `idx_trip_participants_trip_id` | B-tree (FK)        | Участники поездки              |
| trip_participants | `idx_trip_participants_login`   | B-tree (FK)        | Поездки пользователя           |

### Результаты оптимизации

Подробный анализ с EXPLAIN ANALYZE доступен в файле [`optimization.md`](optimization.md).

**Ключевые улучшения:**

- Поиск по маске имени: **54-57% быстрее** с индексами
- Активные поездки с сортировкой: **66% быстрее** с composite индексами
- Поездка с участниками: **21% быстрее** с индексами на FK

---

## Тестовые данные

В базе данных созданы следующие тестовые данные:

| Таблица           | Количество записей | Описание                                                      |
| ----------------- | ------------------ | ------------------------------------------------------------- |
| users             | 12                 | Пользователи с разными именами                                |
| auth_tokens       | 12                 | Токены (10 активных, 2 просроченных)                          |
| routes            | 12                 | Маршруты между городаами России                               |
| trips             | 15                 | Поездки (10 активных, 3 завершенных, 1 отмененный, 1 будущий) |
| trip_participants | 20                 | Участники различных поездок                                   |

---

## Примеры SQL запросов

Все запросы из варианта задания доступны в файле [`queries.sql`](queries.sql):

1. **Создание пользователя** - `INSERT INTO users ... ON CONFLICT`
2. **Авторизация** - Проверка credentials + создание токена
3. **Поиск по login** - `SELECT ... WHERE login = ?`
4. **Поиск по маске** - `SELECT ... WHERE first_name ILIKE ?`
5. **Создание маршрута** - `INSERT INTO routes ... RETURNING`
6. **Маршруты пользователя** - `SELECT ... WHERE owner_login = ?`
7. **Создание поездки** - `INSERT INTO trips ... RETURNING`
8. **Получение поездки** - JOIN с участниками и маршрутом
9. **Добавление участника** - `INSERT INTO trip_participants ...`
10. **Валидация токена** - JOIN tokens + users с проверкой expires_at

### Пример: Получение поездки с участниками

```sql
SELECT
    td.trip_id,
    td.owner_login,
    td.points,
    td.owner_first_name || ' ' || td.owner_last_name AS owner_name,
    td.status,
    td.departure_time,
    array_agg(tpd.login) AS participants,
    array_agg(tpd.first_name || ' ' || tpd.last_name) AS participant_names
FROM trip_details td
LEFT JOIN trip_participants_details tpd ON td.trip_id = tpd.trip_id
WHERE td.trip_id = '<trip_id>'
GROUP BY td.trip_id, td.owner_login, td.points,
         td.owner_first_name, td.owner_last_name,
         td.status, td.departure_time;
```

---

## Подключение API к базе данных

API полностью интегрировано с PostgreSQL через компонент userver:

### Архитектура интеграции

```
┌─────────────────────────────────────────────────────────┐
│                    HTTP Handlers                        │
│  (LoginUser, CreateUser, CreateRoute, etc.)            │
└────────────────────┬────────────────────────────────────┘
                     │
                     │ вызывают
                     ▼
┌─────────────────────────────────────────────────────────┐
│              StoragePgComponent                         │
│  (storage_pg.hpp / storage_pg.cpp)                     │
│  - Использует userver::postgresql::Cluster             │
│  - Асинхронные корутинные запросы (co_await)           │
│  - Prepared statements для безопасности                │
└────────────────────┬────────────────────────────────────┘
                     │
                     │ подключается к
                     ▼
┌─────────────────────────────────────────────────────────┐
│             PostgreSQL Database                         │
│  (postgres:5432 в Docker / localhost:5432 локально)    │
└─────────────────────────────────────────────────────────┘
```

### Ключевые файлы интеграции:

1. **`src/storage_pg.hpp/cpp`** - PostgreSQL storage компонент
   - Заменяет in-memory `StorageComponent`
   - Использует `userver::postgresql::Cluster` для подключения
   - Все методы асинхронные (co_await)

2. **`src/handlers/api_handlers.cpp`** - Обновленные handlers
   - Все handlers теперь используют `StoragePgComponent`
   - Интерфейс остался тем же (абстракция хранения)

3. **`src/main.cpp`** - Регистрация компонентов

   ```cpp
   .Append<userver::postgresql::Cluster>("postgres-db")
   blablacar_service::AppendStoragePg(component_list);
   ```

4. **`configs/static_config.yaml`** - Конфигурация PostgreSQL

   ```yaml
   postgres-db:
     shards:
       - hosts:
           - host: $db-host
             port: $db-port
         dbname: $db-name
         user: $db-user
         password: $db-password
   ```

5. **`CMakeLists.txt`** - Линковка PostgreSQL библиотеки
   ```cmake
   target_link_libraries(${PROJECT_NAME}_objs PUBLIC userver::core userver::postgresql)
   ```

### Пример запроса в storage_pg.cpp:

```cpp
std::optional<User> StoragePgComponent::GetUserByLogin(
    const std::string& login) const {
  try {
    auto result = co_await pg_cluster_->Execute(
        "SELECT login, first_name, last_name, password_hash "
        "FROM users WHERE login = $1",
        login);

    if (result.IsEmpty()) {
      co_return std::nullopt;
    }

    User user;
    user.login = result[0]["login"].As<std::string>();
    user.first_name = result[0]["first_name"].As<std::string>();
    user.last_name = result[0]["last_name"].As<std::string>();
    user.password = result[0]["password_hash"].As<std::string>();

    co_return user;
  } catch (const std::exception& e) {
    LOG_ERROR() << "Failed to get user by login: " << e.what();
    co_return std::nullopt;
  }
}
```

### Соответствие запросов API и SQL:

| API Endpoint                  | SQL Query в StoragePgComponent                                               |
| ----------------------------- | ---------------------------------------------------------------------------- |
| `POST /v1/users`              | `INSERT INTO users ... ON CONFLICT`                                          |
| `POST /v1/login`              | `SELECT ... FROM users WHERE login AND password` + `INSERT INTO auth_tokens` |
| `GET /v1/users?login=X`       | `SELECT ... FROM users WHERE login = $1`                                     |
| `GET /v1/users/search?mask=X` | `SELECT ... FROM users WHERE first_name ILIKE $1`                            |
| `POST /v1/routes`             | `INSERT INTO routes ... RETURNING id`                                        |
| `GET /v1/routes?login=X`      | `SELECT ... FROM routes WHERE owner_login = $1`                              |
| `POST /v1/trips`              | `INSERT INTO trips ... RETURNING id`                                         |
| `GET /v1/trips?id=X`          | `SELECT ... FROM trips` + `SELECT ... FROM trip_participants`                |
| `POST /v1/trips/add-user`     | `INSERT INTO trip_participants ... ON CONFLICT`                              |

---

## Домашнее задание №4: MongoDB

В рамках четвертого домашнего задания в проект была добавлена документоориентированная база данных MongoDB. 
В MongoDB была частично перенесена логика управления токенами (уже интегрировано) и спроектирована модель для поездок (`trips`) и пользователей (`users`).

### Новые файлы:
- `schema_design.md` — Описание спроектированной документной модели с обоснованием выбора `embedded` и `references`.
- `validation.js` — Скрипт для создания коллекции `trips` с применением `$jsonSchema` валидации.
- `data.js` — Скрипт для вставки тестовых данных в MongoDB.
- `queries.js` — Скрипт с примерами CRUD операций и пайплайном агрегации.

### Интеграция API:
В `src/storage_mongo.hpp` и `src/storage_mongo.cpp` добавлены методы `CreateTrip` и `JoinTrip`, реализующие работу с MongoDB через C++ драйвер фреймворка `userver`. Данные методы используют встроенный документ `route` и массивы `participants` в соответствии с новой схемой.

Актуальное состояние интеграции:
- `auth_tokens` и `trips` работают через MongoDB.
- `users` и `routes` остаются в PostgreSQL.
- Внешний API для `trips` сохранён совместимым: `POST /v1/trips` по-прежнему принимает `route_id`, а `GET /v1/trips` возвращает `id`, `route_id`, `participants`.
- При создании поездки сервис читает маршрут из PostgreSQL и сохраняет в MongoDB совместимый документ: `route_id` + embedded `route`.

### Инструкции по запуску MongoDB:

1. MongoDB запускается автоматически в `docker-compose` (контейнер `mongo-blabla`).
   ```bash
   docker-compose up -d
   ```

2. Для выполнения скриптов (создание валидации, вставка данных, выполнение запросов) подключитесь к контейнеру с `mongosh`:
   ```bash
   docker exec -it mongo-blabla mongosh
   ```

3. Внутри `mongosh` выполните скрипты последовательно. Скрипты уже примонтированы в контейнер в `/app`, поэтому можно использовать `load()` напрямую:
   ```javascript
   // 1. Создание схемы и валидации
   load("/app/validation.js") // Или просто вставьте код
   
   // 2. Вставка данных
   load("/app/data.js")
   
   // 3. Запуск запросов
   load("/app/queries.js")
   ```
*(Примечание: чтобы `load` сработал, можно пробросить файлы в контейнер через volumes в `docker-compose.yaml` или просто вставлять содержимое файлов в консоль `mongosh`).*

---

## Домашнее задание №5: Кеширование и Rate Limiting (Redis)

В рамках пятого домашнего задания в проект интегрирован **Redis 7** для реализации кеширования часто запрашиваемых данных и защиты от злоупотреблений через rate limiting.

### Что реализовано

#### 1. Кеширование (Cache-Aside / Lazy Loading)

Кеширование применено к двум GET-endpoints, обращающимся к PostgreSQL:

| Endpoint | Ключ кеша | TTL | Инвалидация |
|---|---|---|---|
| `GET /v1/users?login=X` | `cache:user:{login}` | 300с (5 мин) | При `POST /v1/users` |
| `GET /v1/routes?login=X` | `cache:routes:{login}` | 180с (3 мин) | При `POST /v1/routes` |

**Логика работы:**
1. Handler проверяет Redis кеш (`GET cache:user:ivan`)
2. Если **HIT** — возвращает данные из кеша (< 1 мс)
3. Если **MISS** — запрашивает PostgreSQL, сохраняет результат в кеш с TTL

**Инвалидация:**
- Гибридная стратегия: автоматическое истечение по TTL + явное удаление ключа при создании/изменении данных.

#### 2. Rate Limiting (Fixed Window Counter)

Rate limiting реализован для `POST /v1/login` с использованием Lua-скрипта в Redis:

| Endpoint | Ключ | Лимит | Окно | Алгоритм |
|---|---|---|---|---|
| `POST /v1/login` | `rate_limit:{client_ip}` | 10 запросов | 60 секунд | Fixed Window Counter |

**При каждом ответе устанавливаются HTTP-заголовки:**
- `X-RateLimit-Limit: 10`
- `X-RateLimit-Remaining: 7`
- `X-RateLimit-Reset: 45`

**При превышении лимита:**
- HTTP статус: `429 Too Many Requests`
- Тело: `{"error": "Too Many Requests"}`

#### 3. Fail-Open

При недоступности Redis:
- Кеш-промахи обрабатываются прозрачно — запрос идет напрямую в БД.
- Rate limiter пропускает запрос — защита от отказа в обслуживании при сбое инфраструктуры.

### Новые файлы

| Файл | Описание |
|---|---|
| `src/redis_cache.hpp/cpp` | Компонент кеширования (Cache-Aside через Redis) |
| `src/rate_limiter.hpp/cpp` | Компонент rate limiting (Fixed Window Counter через Lua) |
| `configs/secdist.json` | Настройки подключения к Redis (локально) |
| `configs/secdist.docker.json` | Настройки подключения к Redis (Docker) |
| `performance_design.md` | Полный документ: стратегия кеширования, rate limiting, анализ производительности |

### Измененные файлы

| Файл | Изменения |
|---|---|
| `docker-compose.yaml` | Добавлен сервис `redis` (Redis 7 Alpine) |
| `CMakeLists.txt` | Добавлен `userver::redis`, новые source-файлы |
| `Dockerfile` | Включен `USERVER_FEATURE_REDIS`, Docker config_vars |
| `configs/static_config.yaml` | Компоненты `secdist`, `redis-blabla`, `redis-cache`, `rate-limiter` |
| `configs/config_vars.yaml` | Добавлен `secdist-path` |
| `configs/config_vars.docker.yaml` | Добавлен `secdist-path`, `dbconnection` |
| `src/main.cpp` | Регистрация Redis, Secdist, Cache, RateLimiter компонентов |
| `src/handlers/api_handlers.hpp/cpp` | Интеграция кеша и rate limiter в handlers |

### Архитектура с Redis

```
┌─────────────────────────────────────────────────────────────────┐
│                       HTTP Handlers                             │
│  LoginUser (rate limit) │ GetUser (cache) │ GetRoutes (cache)  │
│  CreateUser (invalidate) │ CreateRoute (invalidate)            │
└───────────┬─────────────────┬──────────────────┬───────────────┘
            │                 │                  │
            ▼                 ▼                  ▼
┌─────────────────┐  ┌──────────────────┐  ┌──────────────┐
│  RateLimiter    │  │  RedisCache      │  │  Storage     │
│  (Lua script)   │  │  (Cache-Aside)   │  │  (PG/Mongo)  │
└────────┬────────┘  └────────┬─────────┘  └──────┬───────┘
         │                    │                    │
         ▼                    ▼                    ▼
┌─────────────────────────────────┐       ┌──────────────┐
│           Redis 7               │       │ PostgreSQL / │
│  rate_limit:* │ cache:user:*   │       │   MongoDB    │
│               │ cache:routes:* │       └──────────────┘
└─────────────────────────────────┘
```

### Запуск

```bash
# Полный стек (PostgreSQL + MongoDB + Redis + API)
docker-compose up --build -d

# Проверка Redis
docker exec -it blablacar-redis redis-cli PING
# → PONG

# Мониторинг кеша
docker exec -it blablacar-redis redis-cli INFO stats | grep keyspace
```

