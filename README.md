# BlablaCar Service — Event-Driven Architecture (ДЗ-06)

REST API сервис поиска попутчиков на C++ (`userver`) с событийно-ориентированной архитектурой на RabbitMQ.

---

## Документация (ДЗ-06)

| Файл | Описание |
|---|---|
| [`event_driven_design.md`](event_driven_design.md) | Проектирование Event-Driven архитектуры, CQRS, delivery guarantees |
| [`event_catalog.md`](event_catalog.md) | Каталог событий: payload, producers, consumers, routing keys |
| [`src/events.hpp`](src/events.hpp) / [`src/events.cpp`](src/events.cpp) | Реализация publisher и consumer (RabbitMQ через userver) |
| [`docker-compose.yml`](docker-compose.yml) | Стек: API + RabbitMQ + PostgreSQL + MongoDB + Redis |

---

## Архитектура событий

| Command | HTTP | Event | Routing key |
|---|---|---|---|
| `CreateUser` | `POST /v1/users` | `UserCreated` | `user.created` |
| `CreateRoute` | `POST /v1/routes` | `RouteCreated` | `route.created` |
| `CreateTrip` | `POST /v1/trips` | `TripCreated` | `trip.created` |
| `JoinTrip` | `POST /v1/trips/add-user` | `TripParticipantJoined` | `trip.participant_joined` |

**Producer:** `EventPublisherComponent` в `blablacar-api`  
**Consumer:** `EventConsumerComponent` (audit log) — читает из `blablacar.audit.events`  
**Exchange:** `blablacar.events` (topic, durable), **Delivery:** `at-least-once`

---

## Запуск

### Полный стек

```bash
docker compose up --build -d
```

Сервисы после запуска:

| Сервис | Адрес |
|---|---|
| API | http://localhost:8080 |
| RabbitMQ Management UI | http://localhost:15672 (`guest` / `guest`) |
| PostgreSQL | localhost:5432 |
| Redis | localhost:6379 |
| MongoDB | localhost:27017 |

Проверка готовности:

```bash
curl http://localhost:8080/ping
docker compose ps
```

### Проверка RabbitMQ topology

```bash
docker exec -it blablacar-rabbitmq rabbitmqctl list_exchanges name type durable
docker exec -it blablacar-rabbitmq rabbitmqctl list_queues name messages durable
```

---

## Пример сценария (producer → consumer)

```bash
# 1. Создать пользователя → публикуется UserCreated
curl -X POST http://localhost:8080/v1/users \
  -H 'Content-Type: application/json' \
  -d '{"login":"alice","first_name":"Alice","last_name":"Smith","password":"pass123"}'

# 2. Залогиниться → получить токен
TOKEN=$(curl -s -X POST http://localhost:8080/v1/login \
  -H 'Content-Type: application/json' \
  -d '{"login":"alice","password":"pass123"}' | jq -r '.token')

# 3. Создать маршрут → RouteCreated
ROUTE_ID=$(curl -s -X POST http://localhost:8080/v1/routes \
  -H "Authorization: Bearer $TOKEN" \
  -H 'Content-Type: application/json' \
  -d '{"owner_login":"alice","points":"Moscow -> Saint Petersburg"}' | jq -r '.id')

# 4. Создать поездку → TripCreated
TRIP_ID=$(curl -s -X POST http://localhost:8080/v1/trips \
  -H "Authorization: Bearer $TOKEN" \
  -H 'Content-Type: application/json' \
  -d "{\"route_id\":\"$ROUTE_ID\"}" | jq -r '.id')

# 5. Добавить участника → TripParticipantJoined
curl -X POST http://localhost:8080/v1/trips/add-user \
  -H 'Content-Type: application/json' \
  -d "{\"trip_id\":\"$TRIP_ID\",\"login\":\"alice\"}"

# 6. Проверить логи consumer
docker logs blablacar-api 2>&1 | grep -E 'Published event|Consumed event'
```

---

## Локальная сборка и тесты (без Docker)

```bash
make test-debug    # сборка и запуск тестов
make start-debug   # запуск сервиса локально
```

---

## API

Полная спецификация — [`openapi.yaml`](openapi.yaml).

| Метод | Endpoint | Описание |
|---|---|---|
| `POST` | `/v1/users` | Создание пользователя |
| `GET` | `/v1/users?login=X` | Поиск по логину |
| `GET` | `/v1/users/search?mask=X` | Поиск по маске имени/фамилии |
| `POST` | `/v1/routes` | Создание маршрута |
| `GET` | `/v1/routes?login=X` | Маршруты пользователя |
| `POST` | `/v1/trips` | Создание поездки |
| `POST` | `/v1/trips/add-user` | Добавление участника в поездку |
| `GET` | `/v1/trips?id=X` | Информация о поездке |
