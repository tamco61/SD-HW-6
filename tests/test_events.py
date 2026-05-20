import pytest

async def test_user_creation_event_published(service_client, testpoint):
    @testpoint('event-consumed')
    def event_consumed_hook(data):
        return

    # Создаем пользователя, что должно триггернуть событие UserCreated
    response = await service_client.post(
        '/v1/users',
        json={
            'login': 'event_user',
            'first_name': 'Event',
            'last_name': 'User',
            'password': 'password123'
        }
    )
    assert response.status == 200

    # Ждем, пока EventConsumerComponent получит событие из RabbitMQ и вызовет TESTPOINT
    call = await event_consumed_hook.wait_call()
    
    # Проверяем структуру события
    event_data = call['data']
    assert event_data['event_type'] == 'UserCreated'
    assert event_data['producer'] == 'blablacar-api'
    
    # Проверяем payload
    payload = event_data['payload']
    assert payload['login'] == 'event_user'
    assert payload['first_name'] == 'Event'
    assert payload['last_name'] == 'User'

async def test_route_creation_event_published(service_client, testpoint):
    @testpoint('event-consumed')
    def event_consumed_hook(data):
        pass

    # Создаем юзера и логинимся
    await service_client.post('/v1/users', json={'login': 'route_event_user', 'first_name': 'F', 'last_name': 'L', 'password': 'p'})
    res = await service_client.post('/v1/login', json={'login': 'route_event_user', 'password': 'p'})
    token = res.json()['token']

    # Очищаем вызовы от создания юзера
    while event_consumed_hook.has_calls:
        event_consumed_hook.next_call()

    # Создаем маршрут
    response = await service_client.post(
        '/v1/routes',
        json={'owner_login': 'route_event_user', 'points': 'A -> B'},
        headers={'Authorization': f'Bearer {token}'}
    )
    assert response.status == 200
    route_id = response.json()['id']

    # Ждем события RouteCreated
    call = await event_consumed_hook.wait_call()
    event_data = call['data']
    
    assert event_data['event_type'] == 'RouteCreated'
    assert event_data['payload']['route_id'] == route_id
    assert event_data['payload']['owner_login'] == 'route_event_user'
