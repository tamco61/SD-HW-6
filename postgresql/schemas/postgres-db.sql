-- Enable UUID extension
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

-- Таблица пользователей (users)
CREATE TABLE IF NOT EXISTS users (
    login VARCHAR(50) PRIMARY KEY,
    first_name VARCHAR(100) NOT NULL,
    last_name VARCHAR(100) NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    
    CONSTRAINT users_first_name_check CHECK (char_length(first_name) > 0),
    CONSTRAINT users_last_name_check CHECK (char_length(last_name) > 0),
    CONSTRAINT users_password_check CHECK (char_length(password_hash) >= 8)
);

-- Индекс для поиска пользователей по имени (для операции SearchUsersByName)
-- Нужен для быстрого выполнения LIKE запросов при поиске по маске
CREATE INDEX idx_users_first_name ON users USING btree (first_name);
CREATE INDEX idx_users_last_name ON users USING btree (last_name);

-- Составной индекс для поиска по full name (first_name + last_name)
CREATE INDEX idx_users_full_name ON users USING btree (first_name, last_name);

-- Таблица токенов авторизации (auth_tokens)
CREATE TABLE IF NOT EXISTS auth_tokens (
    token VARCHAR(255) PRIMARY KEY,
    login VARCHAR(50) NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    expires_at TIMESTAMP WITH TIME ZONE NOT NULL,
    
    CONSTRAINT fk_auth_tokens_user FOREIGN KEY (login)
        REFERENCES users(login) ON DELETE CASCADE,
    CONSTRAINT auth_tokens_expires_check CHECK (expires_at > created_at)
);

-- Индекс для быстрого поиска токена по login
CREATE INDEX idx_auth_tokens_login ON auth_tokens USING btree (login);

-- Индекс для удаления просроченных токенов
CREATE INDEX idx_auth_tokens_expires_at ON auth_tokens USING btree (expires_at);

-- Таблица маршрутов (routes)
CREATE TABLE IF NOT EXISTS routes (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    owner_login VARCHAR(50) NOT NULL,
    points TEXT NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    
    CONSTRAINT fk_routes_owner FOREIGN KEY (owner_login)
        REFERENCES users(login) ON DELETE CASCADE,
    CONSTRAINT routes_points_check CHECK (char_length(points) > 0)
);

-- Индекс для быстрого поиска маршрутов по владельцу (для GetRoutesByUser)
CREATE INDEX idx_routes_owner_login ON routes USING btree (owner_login);

-- Индекс для поиска маршрутов по точкам (для будущего поиска по маршруту)
CREATE INDEX idx_routes_points ON routes USING gin (to_tsvector('russian', points));

-- Таблица поездок (trips)
CREATE TABLE IF NOT EXISTS trips (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    route_id UUID NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    departure_time TIMESTAMP WITH TIME ZONE,
    status VARCHAR(20) DEFAULT 'active' CHECK (status IN ('active', 'completed', 'cancelled')),
    
    CONSTRAINT fk_trips_route FOREIGN KEY (route_id)
        REFERENCES routes(id) ON DELETE CASCADE
);

-- Индекс для поиска поездок по маршруту
CREATE INDEX idx_trips_route_id ON trips USING btree (route_id);

-- Индекс для фильтрации по статусу
CREATE INDEX idx_trips_status ON trips USING btree (status);

-- Индекс для поиска поездок по времени отправления
CREATE INDEX idx_trips_departure_time ON trips USING btree (departure_time);

-- Таблица участников поездок (trip_participants)
-- Связь многие-ко-многим между trips и users
CREATE TABLE IF NOT EXISTS trip_participants (
    trip_id UUID NOT NULL,
    login VARCHAR(50) NOT NULL,
    joined_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    
    PRIMARY KEY (trip_id, login),
    
    CONSTRAINT fk_trip_participants_trip FOREIGN KEY (trip_id)
        REFERENCES trips(id) ON DELETE CASCADE,
    CONSTRAINT fk_trip_participants_user FOREIGN KEY (login)
        REFERENCES users(login) ON DELETE CASCADE
);

-- Индекс для поиска участников по trip_id (для GetTrip)
CREATE INDEX idx_trip_participants_trip_id ON trip_participants USING btree (trip_id);

-- Индекс для поиска поездок пользователя по login
CREATE INDEX idx_trip_participants_login ON trip_participants USING btree (login);

-- Триггер для автоматического обновления updated_at
CREATE OR REPLACE FUNCTION update_updated_at_column()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = CURRENT_TIMESTAMP;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER update_users_updated_at
    BEFORE UPDATE ON users
    FOR EACH ROW
    EXECUTE FUNCTION update_updated_at_column();

-- Представления (Views) для упрощения запросов

-- Представление: Поездка с информацией о маршруте и владельце
CREATE OR REPLACE VIEW trip_details AS
SELECT 
    t.id AS trip_id,
    t.route_id,
    t.status,
    t.departure_time,
    t.created_at AS trip_created_at,
    r.owner_login,
    r.points,
    u.first_name AS owner_first_name,
    u.last_name AS owner_last_name
FROM trips t
JOIN routes r ON t.route_id = r.id
JOIN users u ON r.owner_login = u.login;

-- Представление: Участники поездки с их именами
CREATE OR REPLACE VIEW trip_participants_details AS
SELECT 
    tp.trip_id,
    tp.login,
    tp.joined_at,
    u.first_name,
    u.last_name
FROM trip_participants tp
JOIN users u ON tp.login = u.login;

-- Комментарии к таблицам
COMMENT ON TABLE users IS 'Пользователи системы BlablaCar';
COMMENT ON TABLE auth_tokens IS 'Токены авторизации пользователей';
COMMENT ON TABLE routes IS 'Маршруты, созданные пользователями';
COMMENT ON TABLE trips IS 'Поездки, привязанные к маршрутам';
COMMENT ON TABLE trip_participants IS 'Участники поездок (связь многие-ко-многим)';
