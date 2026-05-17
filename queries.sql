-- ОПЕРАЦИЯ 1: Создание пользователя (CreateUser)
-- API: POST /v1/users
INSERT INTO users (login, first_name, last_name, password_hash)
VALUES ('new_user', 'New', 'User', 'hashed_password_123')
ON CONFLICT (login) DO NOTHING;

-- Проверка результата
SELECT login, first_name, last_name 
FROM users 
WHERE login = 'new_user';

-- ОПЕРАЦИЯ 2: Авторизация пользователя (LoginUser)
-- API: POST /v1/login

-- Шаг 1: Проверка credentials
SELECT login 
FROM users 
WHERE login = 'ivan_petrov' 
  AND password_hash = 'hash_ivan_pass123';

-- Шаг 2: Создание токена (если credentials верны)
INSERT INTO auth_tokens (token, login, created_at, expires_at)
VALUES (
    'new_auth_token_' || extract(epoch from now())::bigint,
    'ivan_petrov',
    CURRENT_TIMESTAMP,
    CURRENT_TIMESTAMP + INTERVAL '24 hours'
)
RETURNING token;

-- ОПЕРАЦИЯ 3: Поиск пользователя по login (GetUserByLogin)
-- API: GET /v1/users?login=ivan_petrov
SELECT login, first_name, last_name
FROM users
WHERE login = 'ivan_petrov';

-- ОПЕРАЦИЯ 4: Поиск пользователей по маске имени (SearchUsersByName)
-- API: GET /v1/users/search?mask=Ivan

-- Вариант 4.1: Поиск по точному совпадению first_name
SELECT login, first_name, last_name
FROM users
WHERE first_name ILIKE 'Ivan%' 
   OR last_name ILIKE 'Ivan%'
   OR login ILIKE 'Ivan%';

-- Вариант 4.2: Поиск по частичному совпадению (LIKE с %)
SELECT login, first_name, last_name
FROM users
WHERE first_name ILIKE '%Ivan%' 
   OR last_name ILIKE '%Ivan%'
   OR login ILIKE '%Ivan%';

-- Вариант 4.3: Полнотекстовый поиск (более эффективный для больших данных)
SELECT login, first_name, last_name
FROM users
WHERE to_tsvector('russian', first_name || ' ' || last_name || ' ' || login) 
      @@ to_tsquery('russian', 'Ivan:*');

-- ОПЕРАЦИЯ 5: Создание маршрута (CreateRoute)
-- API: POST /v1/routes
INSERT INTO routes (owner_login, points)
VALUES ('ivan_petrov', 'Moscow -> Kazan -> Yekaterinburg')
RETURNING id, owner_login, points;

-- ОПЕРАЦИЯ 6: Получение маршрутов пользователя (GetRoutesByUser)
-- API: GET /v1/routes?login=ivan_petrov

-- Вариант 6.1: Все маршруты пользователя
SELECT id, owner_login, points, created_at
FROM routes
WHERE owner_login = 'ivan_petrov'
ORDER BY created_at DESC;

-- Вариант 6.2: Маршруты пользователя с количеством поездок
SELECT 
    r.id,
    r.owner_login,
    r.points,
    r.created_at,
    COUNT(t.id) AS trip_count
FROM routes r
LEFT JOIN trips t ON r.id = t.route_id
WHERE r.owner_login = 'ivan_petrov'
GROUP BY r.id, r.owner_login, r.points, r.created_at
ORDER BY r.created_at DESC;

-- ОПЕРАЦИЯ 7: Создание поездки (CreateTrip)
-- API: POST /v1/trips
INSERT INTO trips (route_id, departure_time, status)
VALUES (
    (SELECT id FROM routes WHERE owner_login = 'ivan_petrov' ORDER BY created_at LIMIT 1),
    CURRENT_TIMESTAMP + INTERVAL '3 days',
    'active'
)
RETURNING id, route_id;

-- ОПЕРАЦИЯ 8: Получение поездки по ID (GetTrip)
-- API: GET /v1/trips?id=<trip_id>

-- Вариант 8.1: Базовая информация о поездке
SELECT 
    t.id,
    t.route_id,
    t.status,
    t.departure_time,
    t.created_at
FROM trips t
WHERE t.id = (SELECT id FROM trips ORDER BY created_at LIMIT 1);

-- Вариант 8.2: Полная информация о поездке с маршрутом и участниками
SELECT 
    td.trip_id,
    td.route_id,
    td.owner_login,
    td.points,
    td.owner_first_name,
    td.owner_last_name,
    td.status,
    td.departure_time,
    array_agg(tpd.login) AS participants,
    array_agg(tpd.first_name || ' ' || tpd.last_name) AS participant_names
FROM trip_details td
LEFT JOIN trip_participants_details tpd ON td.trip_id = tpd.trip_id
WHERE td.trip_id = (SELECT id FROM trips ORDER BY created_at LIMIT 1)
GROUP BY td.trip_id, td.route_id, td.owner_login, td.points, 
         td.owner_first_name, td.owner_last_name, td.status, 
         td.departure_time, td.trip_created_at;

-- ОПЕРАЦИЯ 9: Добавление участника в поездку (AddUserToTrip)
-- API: POST /v1/trips/add-user

-- Вариант 9.1: Добавление участника (с проверкой существования поездки)
INSERT INTO trip_participants (trip_id, login)
VALUES (
    (SELECT id FROM trips ORDER BY created_at LIMIT 1),
    'new_participant'
)
ON CONFLICT (trip_id, login) DO NOTHING
RETURNING trip_id, login;

-- Вариант 9.2: Добавление участника с полной проверкой
WITH trip_check AS (
    SELECT t.id, t.status, r.owner_login
    FROM trips t
    JOIN routes r ON t.route_id = r.id
    WHERE t.id = (SELECT id FROM trips ORDER BY created_at LIMIT 1)
)
INSERT INTO trip_participants (trip_id, login)
SELECT tc.id, 'sergey_novikov'
FROM trip_check tc
WHERE tc.status = 'active'
  AND tc.owner_login != 'sergey_novikov'
ON CONFLICT (trip_id, login) DO NOTHING
RETURNING trip_id, login;

-- ОПЕРАЦИЯ 10: Проверка валидности токена (ValidateToken)
-- Используется middleware для защищенных маршрутов
SELECT at.login, u.first_name, u.last_name
FROM auth_tokens at
JOIN users u ON at.login = u.login
WHERE at.token = 'token_ivan_active_001'
  AND at.expires_at > CURRENT_TIMESTAMP;

-- ДОПОЛНИТЕЛЬНЫЕ ЗАПРОСЫ

-- ДОП. 1: Поиск поездок по маршруту
SELECT 
    t.id AS trip_id,
    t.status,
    t.departure_time,
    r.points,
    u.login AS owner_login,
    u.first_name AS owner_first_name,
    u.last_name AS owner_last_name,
    COUNT(tp.login) AS participant_count
FROM trips t
JOIN routes r ON t.route_id = r.id
JOIN users u ON r.owner_login = u.login
LEFT JOIN trip_participants tp ON t.id = tp.trip_id
WHERE r.points ILIKE '%Moscow%'
GROUP BY t.id, t.status, t.departure_time, r.points, 
         u.login, u.first_name, u.last_name
HAVING COUNT(tp.login) > 0
ORDER BY t.departure_time;

-- ДОП. 2: Поиск попутчиков для пользователя
SELECT DISTINCT
    u2.login,
    u2.first_name,
    u2.last_name,
    t.id AS trip_id,
    r.points
FROM trip_participants tp1
JOIN trips t ON tp1.trip_id = t.id
JOIN trip_participants tp2 ON t.id = tp2.trip_id
JOIN users u1 ON tp1.login = u1.login
JOIN users u2 ON tp2.login = u2.login
JOIN routes r ON t.route_id = r.id
WHERE u1.login = 'ivan_petrov'
  AND u2.login != 'ivan_petrov'
  AND t.status = 'active'
ORDER BY u2.last_name, u2.first_name;

-- ДОП. 3: Статистика пользователя (количество поездок, маршрутов, попутчиков)
SELECT 
    u.login,
    u.first_name,
    u.last_name,
    (SELECT COUNT(*) FROM routes WHERE owner_login = u.login) AS routes_count,
    (SELECT COUNT(DISTINCT t.id) 
     FROM trips t 
     JOIN routes r ON t.route_id = r.id 
     WHERE r.owner_login = u.login) AS owned_trips_count,
    (SELECT COUNT(DISTINCT t.id) 
     FROM trip_participants tp 
     JOIN trips t ON tp.trip_id = t.id 
     WHERE tp.login = u.login) AS participated_trips_count,
    (SELECT COUNT(DISTINCT tp2.login)
     FROM trip_participants tp1
     JOIN trips t ON tp1.trip_id = t.id
     JOIN trip_participants tp2 ON t.id = tp2.trip_id
     WHERE tp1.login = u.login AND tp2.login != u.login) AS unique_coworkers_count
FROM users u
WHERE u.login = 'ivan_petrov';

-- ДОП. 4: Активные поездки с участниками (для главной страницы)
SELECT 
    t.id AS trip_id,
    t.status,
    t.departure_time,
    r.points AS route,
    u_owner.login AS owner_login,
    u_owner.first_name || ' ' || u_owner.last_name AS owner_name,
    COUNT(tp.login) AS current_participants,
    COUNT(tp.login) FILTER (WHERE tp.login != r.owner_login) AS passengers_count
FROM trips t
JOIN routes r ON t.route_id = r.id
JOIN users u_owner ON r.owner_login = u_owner.login
LEFT JOIN trip_participants tp ON t.id = tp.trip_id
WHERE t.status = 'active'
  AND t.departure_time > CURRENT_TIMESTAMP
GROUP BY t.id, t.status, t.departure_time, r.points, 
         u_owner.login, u_owner.first_name, u_owner.last_name
ORDER BY t.departure_time
LIMIT 20;

-- ДОП. 5: Удаление просроченных токенов (очистка)
DELETE FROM auth_tokens
WHERE expires_at < CURRENT_TIMESTAMP;

-- ДОП. 6: Обновление статуса завершенных поездок
UPDATE trips
SET status = 'completed'
WHERE status = 'active'
  AND departure_time < CURRENT_TIMESTAMP - INTERVAL '1 day';
