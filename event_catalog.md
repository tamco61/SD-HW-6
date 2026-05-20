# Event catalog

## Общий формат сообщения

Все события публикуются в exchange `blablacar.events` с типом `topic`.

```json
{
  "event_id": "uuid",
  "event_type": "EventName",
  "schema_version": 1,
  "occurred_at": "2026-05-17T12:00:00Z",
  "producer": "blablacar-api",
  "correlation_id": "uuid",
  "payload": {}
}
```

Гарантия доставки для всех событий: `at-least-once`. Потребители должны
обрабатывать повторную доставку идемпотентно по `event_id`.

## UserCreated

| Field | Value |
| --- | --- |
| Routing key | `user.created` |
| Producer | `blablacar-api`, handler `POST /v1/users` |
| Delivery | `at-least-once` |
| Current consumer | `event-consumer` |
| Potential consumers | `analytics-service`, `read-model-projector`, `notification-service` |

Payload:

```json
{
  "login": "ivan",
  "first_name": "Ivan",
  "last_name": "Petrov"
}
```

Назначение: зафиксировать регистрацию нового пользователя. Событие не содержит
пароль.

## RouteCreated

| Field | Value |
| --- | --- |
| Routing key | `route.created` |
| Producer | `blablacar-api`, handler `POST /v1/routes` |
| Delivery | `at-least-once` |
| Current consumer | `event-consumer` |
| Potential consumers | `read-model-projector`, `cache-invalidator`, `analytics-service` |

Payload:

```json
{
  "route_id": "3b6b55ff-6cb8-48f7-a5a0-4f522ae18d4f",
  "owner_login": "ivan",
  "points": "Moscow -> Tver -> Saint Petersburg"
}
```

Назначение: уведомить систему, что пользователь создал новый маршрут.

## TripCreated

| Field | Value |
| --- | --- |
| Routing key | `trip.created` |
| Producer | `blablacar-api`, handler `POST /v1/trips` |
| Delivery | `at-least-once` |
| Current consumer | `event-consumer` |
| Potential consumers | `read-model-projector`, `notification-service`, `analytics-service` |

Payload:

```json
{
  "trip_id": "66544cb83aa674a1d2e65b0e",
  "route_id": "3b6b55ff-6cb8-48f7-a5a0-4f522ae18d4f",
  "owner_login": "ivan",
  "status": "active"
}
```

Назначение: создать или обновить read projection поездки и запустить внешние
процессы, связанные с новой поездкой.

## TripParticipantJoined

| Field | Value |
| --- | --- |
| Routing key | `trip.participant_joined` |
| Producer | `blablacar-api`, handler `POST /v1/trips/add-user` |
| Delivery | `at-least-once` |
| Current consumer | `event-consumer` |
| Potential consumers | `read-model-projector`, `notification-service`, `analytics-service` |

Payload:

```json
{
  "trip_id": "66544cb83aa674a1d2e65b0e",
  "login": "petr"
}
```

Назначение: уведомить владельца поездки, обновить список участников в read
model, пересчитать аналитику заполненности поездок.
