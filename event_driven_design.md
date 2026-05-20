# Event-Driven design

## Контекст системы

Сервис поиска попутчиков реализует API для пользователей, маршрутов и поездок.
Основные команды изменяют состояние системы, а события уведомляют другие части
системы о факте уже совершенного изменения.

## Команды и события

| Command | HTTP API | Event | Producer |
| --- | --- | --- | --- |
| `CreateUser` | `POST /v1/users` | `UserCreated` | `blablacar-api` |
| `CreateRoute` | `POST /v1/routes` | `RouteCreated` | `blablacar-api` |
| `CreateTrip` | `POST /v1/trips` | `TripCreated` | `blablacar-api` |
| `JoinTrip` | `POST /v1/trips/add-user` | `TripParticipantJoined` | `blablacar-api` |

Событие публикуется только после успешного выполнения команды. Если пользователь
уже был участником поездки, повторное подключение не публикует
`TripParticipantJoined`, потому что новое доменное изменение не произошло.

## Producers и consumers

**Event producers**

- REST handlers сервиса `blablacar-api`;
- в коде публикация сосредоточена в `EventPublisherComponent`.

**Event consumers**

- `event-consumer` внутри текущего сервиса: демонстрационный audit consumer,
  который читает события из RabbitMQ и пишет лог обработки;
- `notification-service`: отправка уведомлений водителю и пассажирам;
- `analytics-service`: подсчет созданных маршрутов, поездок и присоединений;
- `read-model-projector`: обновление read-моделей для быстрых запросов;
- `cache-invalidator`: инвалидация кешей Redis по событиям.

## RabbitMQ topology

Выбран RabbitMQ, потому что для этой системы удобно маршрутизировать доменные
события по routing key.

| Entity | Value |
| --- | --- |
| Exchange | `blablacar.events` |
| Exchange type | `topic` |
| Durable | yes |
| Queue | `blablacar.audit.events` |
| Binding | `#` |
| Delivery guarantee | `at-least-once` |

Routing keys:

- `user.created`;
- `route.created`;
- `trip.created`;
- `trip.participant_joined`.

## Message format

Все события используют единый JSON envelope:

```json
{
  "event_id": "d6d9859f-9b11-40b8-a62f-4a83f22b881b",
  "event_type": "TripCreated",
  "schema_version": 1,
  "occurred_at": "2026-05-17T12:00:00Z",
  "producer": "blablacar-api",
  "correlation_id": "7e8a9e15-f15b-4c28-8294-4f8e19fdb8bd",
  "payload": {
    "trip_id": "66544cb83aa674a1d2e65b0e",
    "route_id": "3b6b55ff-6cb8-48f7-a5a0-4f522ae18d4f",
    "owner_login": "ivan",
    "status": "active"
  }
}
```

`event_id` нужен для идемпотентной обработки, `schema_version` позволяет
развивать контракт событий, `correlation_id` связывает сообщения одного
пользовательского сценария.

## Event flow

1. Клиент вызывает REST command endpoint.
2. Handler валидирует запрос и выполняет запись в PostgreSQL или MongoDB.
3. Handler вызывает `EventPublisherComponent`.
4. Publisher формирует JSON envelope.
5. Publisher отправляет событие в `blablacar.events` через
   `PublishReliable`, то есть ждет publisher confirm от RabbitMQ.
6. RabbitMQ маршрутизирует событие в очереди по routing key.
7. `event-consumer` получает сообщение из `blablacar.audit.events`, логирует
   `event_type`, `event_id` и payload.

## Delivery guarantees

Текущая реализация использует `at-least-once`:

- publisher применяет reliable publish с подтверждением брокера;
- durable exchange и durable queue переживают перезапуск RabbitMQ;
- сообщения публикуются как persistent;
- consumer подтверждает сообщение после успешного `Process`;
- при ошибке обработки userver/RabbitMQ может доставить сообщение повторно.

Потребители должны быть идемпотентными и хранить обработанные `event_id`, если
их действие нельзя безопасно повторять.

Exactly-once не заявляется. Для production-версии нужен transactional outbox:
команда записывает данные и событие в одной транзакции БД, отдельный publisher
читает outbox и доставляет событие в RabbitMQ.

## CQRS

CQRS применим, потому что операции записи и чтения имеют разные требования.

**Write side**

- `POST /v1/users`;
- `POST /v1/routes`;
- `POST /v1/trips`;
- `POST /v1/trips/add-user`.

Write side отвечает за проверку инвариантов и запись в основное хранилище.

**Read side**

- `GET /v1/users`;
- `GET /v1/users/search`;
- `GET /v1/routes`;
- `GET /v1/trips`.

Read side использует PostgreSQL, MongoDB и Redis cache-aside для быстрых
запросов. События могут синхронизировать отдельные read projections:

- `UserCreated` обновляет user/search projection;
- `RouteCreated` обновляет список маршрутов пользователя и инвалидирует кеш;
- `TripCreated` создает trip projection;
- `TripParticipantJoined` обновляет список участников поездки.

В учебной реализации consumer демонстрирует обработку события логами. В
production read-model projector должен обновлять отдельную материализованную
модель чтения.
