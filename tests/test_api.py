import pytest

async def test_user_flow(service_client):
    # 1. Create a user
    response = await service_client.post(
        '/v1/users',
        json={
            'login': 'test_user',
            'first_name': 'Test',
            'last_name': 'User',
            'password': 'password123'
        }
    )
    assert response.status == 200
    assert response.json() == {'status': 'ok'}

    # 2. Try creating the same user again (should be conflict)
    response = await service_client.post(
        '/v1/users',
        json={
            'login': 'test_user',
            'first_name': 'Test2',
            'last_name': 'User2',
            'password': 'password123'
        }
    )
    assert response.status == 409
    assert response.json() == {'error': 'User already exists'}

    # 3. Get the user
    response = await service_client.get('/v1/users', params={'login': 'test_user'})
    assert response.status == 200
    assert response.json() == {
        'login': 'test_user',
        'first_name': 'Test',
        'last_name': 'User'
    }

    # 3.5 Get non-existent user
    response = await service_client.get('/v1/users', params={'login': 'not_found_user'})
    assert response.status == 404
    assert response.json() == {'error': 'User not found'}

    # 4. Search user
    response = await service_client.get('/v1/users/search', params={'mask': 'Test'})
    assert response.status == 200
    assert len(response.json()) >= 1
    assert response.json()[0]['login'] == 'test_user'

async def test_auth_and_routes(service_client):
    # Create user first
    await service_client.post(
        '/v1/users',
        json={
            'login': 'route_user',
            'first_name': 'Route',
            'last_name': 'User',
            'password': 'secure123'
        }
    )

    # Login
    response = await service_client.post(
        '/v1/login',
        json={
            'login': 'route_user',
            'password': 'secure123'
        }
    )
    assert response.status == 200
    token = response.json()['token']

    # Login with bad credentials
    response = await service_client.post(
        '/v1/login',
        json={
            'login': 'route_user',
            'password': 'wrong'
        }
    )
    assert response.status == 401
    
    # Try creating a route without auth
    response = await service_client.post(
        '/v1/routes',
        json={
            'owner_login': 'route_user',
            'points': 'A -> B'
        }
    )
    assert response.status == 401

    # Create a route with auth
    response = await service_client.post(
        '/v1/routes',
        json={
            'owner_login': 'route_user',
            'points': 'A -> B'
        },
        headers={'Authorization': f'Bearer {token}'}
    )
    assert response.status == 200
    route_id = response.json()['id']

    # Get user routes
    response = await service_client.get('/v1/routes', params={'login': 'route_user'})
    assert response.status == 200
    routes = response.json()
    assert len(routes) == 1
    assert routes[0]['id'] == route_id

async def test_trips_flow(service_client):
    # 1. Setup user and get token
    await service_client.post('/v1/users', json={'login': 'trip_user', 'first_name': 'F', 'last_name': 'L', 'password': 'p'})
    res = await service_client.post('/v1/login', json={'login': 'trip_user', 'password': 'p'})
    token = res.json()['token']

    # 2. Setup route
    res = await service_client.post('/v1/routes', json={'owner_login': 'trip_user', 'points': 'X->Y'}, headers={'Authorization': f'Bearer {token}'})
    route_id = res.json()['id']

    # 3. Create trip
    response = await service_client.post(
        '/v1/trips',
        json={'route_id': route_id},
        headers={'Authorization': f'Bearer {token}'}
    )
    assert response.status == 200
    trip_id = response.json()['id']

    # 4. Add participant (trip_user will add another user to trip)
    # create participant
    await service_client.post('/v1/users', json={'login': 'participant1', 'first_name': 'P1', 'last_name': 'L', 'password': 'p'})
    
    response = await service_client.post(
        '/v1/trips/add-user',
        json={'trip_id': trip_id, 'login': 'participant1'}
        # doesn't require auth according to static_config.yaml, but maybe it should?
    )
    assert response.status == 200
    assert response.json() == {'status': 'ok'}

    # Re-adding the same participant should stay idempotent because Mongo uses $addToSet
    response = await service_client.post(
        '/v1/trips/add-user',
        json={'trip_id': trip_id, 'login': 'participant1'}
    )
    assert response.status == 200
    assert response.json() == {'status': 'ok'}

    # 5. Get trip
    response = await service_client.get('/v1/trips', params={'id': trip_id})
    assert response.status == 200
    trip_data = response.json()
    assert trip_data['id'] == trip_id
    assert trip_data['route_id'] == route_id
    assert 'participant1' in trip_data['participants']
    assert trip_data['participants'].count('participant1') == 1

    # 6. Get non-existent trip
    response = await service_client.get(
        '/v1/trips',
        params={'id': '507f1f77bcf86cd799439012'}
    )
    assert response.status == 404
    assert response.json() == {'error': 'Trip not found'}

    # 7. Add participant to non-existent trip
    response = await service_client.post(
        '/v1/trips/add-user',
        json={'trip_id': 'not-existing-trip-id', 'login': 'participant1'}
    )
    assert response.status == 400
    assert 'error' in response.json()

    # 8. Existing-format but missing Mongo trip should return 404
    response = await service_client.get(
        '/v1/trips',
        params={'id': '507f1f77bcf86cd799439011'}
    )
    assert response.status == 404
    assert response.json() == {'error': 'Trip not found'}

    # 9. Invalid Mongo id should return 400
    response = await service_client.get('/v1/trips', params={'id': 'bad-trip-id'})
    assert response.status == 400
    assert 'error' in response.json()

    response = await service_client.post(
        '/v1/trips/add-user',
        json={'trip_id': 'bad-trip-id', 'login': 'participant1'}
    )
    assert response.status == 400
    assert 'error' in response.json()

async def test_create_trip_with_missing_route(service_client):
    await service_client.post(
        '/v1/users',
        json={
            'login': 'trip_route_guard',
            'first_name': 'Trip',
            'last_name': 'Guard',
            'password': 'p'
        }
    )
    res = await service_client.post(
        '/v1/login',
        json={'login': 'trip_route_guard', 'password': 'p'}
    )
    token = res.json()['token']

    response = await service_client.post(
        '/v1/trips',
        json={'route_id': 'd290f1ee-6c54-4b01-90e6-d701748f0851'},
        headers={'Authorization': f'Bearer {token}'}
    )
    assert response.status == 404
    assert response.json() == {'error': 'Route not found'}

async def test_bad_requests(service_client):
    # 1. Missing required JSON fields (e.g. missing 'password' when creating user)
    response = await service_client.post(
        '/v1/users',
        json={
            'login': 'bad_user',
            'first_name': 'Bad',
            'last_name': 'User'
        }
    )
    assert response.status == 400
    assert 'error' in response.json()

    # 2. Malformed JSON
    response = await service_client.post(
        '/v1/users',
        data='{malformed_json:',
        headers={'Content-Type': 'application/json'}
    )
    assert response.status == 400

    # 3. Missing query parameter
    response = await service_client.get('/v1/users/search') # missing 'mask'
    assert response.status == 400
    assert 'error' in response.json()

    response = await service_client.get('/v1/trips') # missing 'id'
    assert response.status == 400
    assert 'error' in response.json()

    response = await service_client.get('/v1/users') # missing 'login'
    assert response.status == 400
    assert 'error' in response.json()
