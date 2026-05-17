
-- Тестовые данные для таблицы users (12 записей)
INSERT INTO users (login, first_name, last_name, password_hash) VALUES
('ivan_petrov', 'Ivan', 'Petrov', 'hash_ivan_pass123'),
('maria_sidorova', 'Maria', 'Sidorova', 'hash_maria_pass456'),
('alexey_smirnov', 'Alexey', 'Smirnov', 'hash_alexey_pass789'),
('olga_kuznetsova', 'Olga', 'Kuznetsova', 'hash_olga_pass012'),
('dmitry_popov', 'Dmitry', 'Popov', 'hash_dmitry_pass345'),
('elena_volkova', 'Elena', 'Volkova', 'hash_elena_pass678'),
('sergey_novikov', 'Sergey', 'Novikov', 'hash_sergey_pass901'),
('anna_morozova', 'Anna', 'Morozova', 'hash_anna_pass234'),
('nikita_fedorov', 'Nikita', 'Fedorov', 'hash_nikita_pass567'),
('tatiana_lebedeva', 'Tatiana', 'Lebedeva', 'hash_tatiana_pass890'),
('artem_kozlov', 'Artem', 'Kozlov', 'hash_artem_pass123'),
('natasha_orlova', 'Natasha', 'Orlova', 'hash_natasha_pass456');

-- Тестовые данные для таблицы auth_tokens (10 записей)
INSERT INTO auth_tokens (token, login, created_at, expires_at) VALUES
('token_ivan_active_001', 'ivan_petrov', CURRENT_TIMESTAMP - INTERVAL '1 hour', CURRENT_TIMESTAMP + INTERVAL '23 hours'),
('token_maria_active_002', 'maria_sidorova', CURRENT_TIMESTAMP - INTERVAL '2 hours', CURRENT_TIMESTAMP + INTERVAL '22 hours'),
('token_alexey_active_003', 'alexey_smirnov', CURRENT_TIMESTAMP - INTERVAL '30 minutes', CURRENT_TIMESTAMP + INTERVAL '23.5 hours'),
('token_olga_active_004', 'olga_kuznetsova', CURRENT_TIMESTAMP - INTERVAL '3 hours', CURRENT_TIMESTAMP + INTERVAL '21 hours'),
('token_dmitry_active_005', 'dmitry_popov', CURRENT_TIMESTAMP - INTERVAL '4 hours', CURRENT_TIMESTAMP + INTERVAL '20 hours'),
('token_elena_active_006', 'elena_volkova', CURRENT_TIMESTAMP - INTERVAL '1.5 hours', CURRENT_TIMESTAMP + INTERVAL '22.5 hours'),
('token_sergey_active_007', 'sergey_novikov', CURRENT_TIMESTAMP - INTERVAL '5 hours', CURRENT_TIMESTAMP + INTERVAL '19 hours'),
('token_anna_active_008', 'anna_morozova', CURRENT_TIMESTAMP - INTERVAL '6 hours', CURRENT_TIMESTAMP + INTERVAL '18 hours'),
('token_nikita_active_009', 'nikita_fedorov', CURRENT_TIMESTAMP - INTERVAL '45 minutes', CURRENT_TIMESTAMP + INTERVAL '23.25 hours'),
('token_tatiana_active_010', 'tatiana_lebedeva', CURRENT_TIMESTAMP - INTERVAL '2.5 hours', CURRENT_TIMESTAMP + INTERVAL '21.5 hours'),
('token_expired_old_011', 'artem_kozlov', CURRENT_TIMESTAMP - INTERVAL '2 days', CURRENT_TIMESTAMP - INTERVAL '1 day'),
('token_expired_old_012', 'natasha_orlova', CURRENT_TIMESTAMP - INTERVAL '3 days', CURRENT_TIMESTAMP - INTERVAL '2 days');

-- Тестовые данные для таблицы routes (12 записей)
INSERT INTO routes (owner_login, points) VALUES
('ivan_petrov', 'Moscow -> Tver -> Veliky Novgorod -> Saint Petersburg'),
('ivan_petrov', 'Moscow -> Vladimir -> Nizhny Novgorod'),
('maria_sidorova', 'Saint Petersburg -> Pskov -> Riga'),
('alexey_smirnov', 'Moscow -> Tula -> Orel -> Kursk'),
('alexey_smirnov', 'Moscow -> Ryazan -> Penza -> Samara'),
('olga_kuznetsova', 'Saint Petersburg -> Novgorod -> Pskov'),
('dmitry_popov', 'Moscow -> Yaroslavl -> Vologda -> Arkhangelsk'),
('elena_volkova', 'Moscow -> Kaluga -> Bryansk -> Gomel'),
('sergey_novikov', 'Saint Petersburg -> Murmansk -> Kirovsk'),
('anna_morozova', 'Moscow -> Smolensk -> Minsk'),
('nikita_fedorov', 'Moscow -> Ivanovo -> Kostroma -> Vologda'),
('tatiana_lebedeva', 'Saint Petersburg -> Petrozavodsk -> Murmansk');

-- Тестовые данные для таблицы trips (15 записей)
INSERT INTO trips (route_id, departure_time, status) VALUES
-- Поездки Ивана (2 маршрута)
((SELECT id FROM routes WHERE owner_login = 'ivan_petrov' ORDER BY created_at LIMIT 1), CURRENT_TIMESTAMP + INTERVAL '1 day', 'active'),
((SELECT id FROM routes WHERE owner_login = 'ivan_petrov' ORDER BY created_at LIMIT 1 OFFSET 1), CURRENT_TIMESTAMP + INTERVAL '2 days', 'active'),
((SELECT id FROM routes WHERE owner_login = 'ivan_petrov' ORDER BY created_at LIMIT 1), CURRENT_TIMESTAMP + INTERVAL '5 days', 'active'),
-- Поездки Марии (1 маршрут)
((SELECT id FROM routes WHERE owner_login = 'maria_sidorova' ORDER BY created_at LIMIT 1), CURRENT_TIMESTAMP + INTERVAL '1 day', 'active'),
((SELECT id FROM routes WHERE owner_login = 'maria_sidorova' ORDER BY created_at LIMIT 1), CURRENT_TIMESTAMP + INTERVAL '3 days', 'active'),
-- Поездки Алексея (2 маршрута)
((SELECT id FROM routes WHERE owner_login = 'alexey_smirnov' ORDER BY created_at LIMIT 1), CURRENT_TIMESTAMP + INTERVAL '2 days', 'active'),
((SELECT id FROM routes WHERE owner_login = 'alexey_smirnov' ORDER BY created_at LIMIT 1 OFFSET 1), CURRENT_TIMESTAMP + INTERVAL '4 days', 'active'),
-- Поездки Ольги (1 маршрут)
((SELECT id FROM routes WHERE owner_login = 'olga_kuznetsova' ORDER BY created_at LIMIT 1), CURRENT_TIMESTAMP + INTERVAL '1 day', 'active'),
-- Поездки Дмитрия (1 маршрут)
((SELECT id FROM routes WHERE owner_login = 'dmitry_popov' ORDER BY created_at LIMIT 1), CURRENT_TIMESTAMP + INTERVAL '6 days', 'active'),
-- Поездки Елены (1 маршрут)
((SELECT id FROM routes WHERE owner_login = 'elena_volkova' ORDER BY created_at LIMIT 1), CURRENT_TIMESTAMP - INTERVAL '1 day', 'completed'),
-- Поездки Сергея (1 маршрут)
((SELECT id FROM routes WHERE owner_login = 'sergey_novikov' ORDER BY created_at LIMIT 1), CURRENT_TIMESTAMP - INTERVAL '2 days', 'completed'),
-- Поездки Анны (1 маршрут)
((SELECT id FROM routes WHERE owner_login = 'anna_morozova' ORDER BY created_at LIMIT 1), CURRENT_TIMESTAMP - INTERVAL '3 days', 'cancelled'),
-- Поездки Никиты (1 маршрут)
((SELECT id FROM routes WHERE owner_login = 'nikita_fedorov' ORDER BY created_at LIMIT 1), CURRENT_TIMESTAMP + INTERVAL '7 days', 'active'),
-- Поездки Татьяны (1 маршрут)
((SELECT id FROM routes WHERE owner_login = 'tatiana_lebedeva' ORDER BY created_at LIMIT 1), CURRENT_TIMESTAMP + INTERVAL '8 days', 'active'),
-- Ещё одна поездка для тестирования
((SELECT id FROM routes WHERE owner_login = 'ivan_petrov' ORDER BY created_at LIMIT 1), CURRENT_TIMESTAMP - INTERVAL '5 days', 'completed');

-- Тестовые данные для таблицы trip_participants (20 записей)
INSERT INTO trip_participants (trip_id, login) VALUES
-- Поездка 1: Иван (владелец) + участники
((SELECT id FROM trips ORDER BY created_at LIMIT 1 OFFSET 0), 'ivan_petrov'),
((SELECT id FROM trips ORDER BY created_at LIMIT 1 OFFSET 0), 'maria_sidorova'),
((SELECT id FROM trips ORDER BY created_at LIMIT 1 OFFSET 0), 'alexey_smirnov'),
-- Поездка 2: Иван (владелец) + другие участники
((SELECT id FROM trips ORDER BY created_at LIMIT 1 OFFSET 1), 'ivan_petrov'),
((SELECT id FROM trips ORDER BY created_at LIMIT 1 OFFSET 1), 'olga_kuznetsova'),
-- Поездка 3: Иван (владелец) + участники
((SELECT id FROM trips ORDER BY created_at LIMIT 1 OFFSET 2), 'ivan_petrov'),
((SELECT id FROM trips ORDER BY created_at LIMIT 1 OFFSET 2), 'dmitry_popov'),
((SELECT id FROM trips ORDER BY created_at LIMIT 1 OFFSET 2), 'elena_volkova'),
-- Поездка 4: Мария (владелец) + участники
((SELECT id FROM trips ORDER BY created_at LIMIT 1 OFFSET 3), 'maria_sidorova'),
((SELECT id FROM trips ORDER BY created_at LIMIT 1 OFFSET 3), 'sergey_novikov'),
-- Поездка 5: Мария (владелец) + участники
((SELECT id FROM trips ORDER BY created_at LIMIT 1 OFFSET 4), 'maria_sidorova'),
((SELECT id FROM trips ORDER BY created_at LIMIT 1 OFFSET 4), 'anna_morozova'),
((SELECT id FROM trips ORDER BY created_at LIMIT 1 OFFSET 4), 'nikita_fedorov'),
-- Поездка 6: Алексей (владелец) + участники
((SELECT id FROM trips ORDER BY created_at LIMIT 1 OFFSET 5), 'alexey_smirnov'),
((SELECT id FROM trips ORDER BY created_at LIMIT 1 OFFSET 5), 'tatiana_lebedeva'),
-- Поездка 7: Алексей (владелец) + участники
((SELECT id FROM trips ORDER BY created_at LIMIT 1 OFFSET 6), 'alexey_smirnov'),
((SELECT id FROM trips ORDER BY created_at LIMIT 1 OFFSET 6), 'artem_kozlov'),
-- Поездка 8: Ольга (владелец) + участники
((SELECT id FROM trips ORDER BY created_at LIMIT 1 OFFSET 7), 'olga_kuznetsova'),
((SELECT id FROM trips ORDER BY created_at LIMIT 1 OFFSET 7), 'natasha_orlova'),
-- Поездка 9: Дмитрий (владелец) + участники
((SELECT id FROM trips ORDER BY created_at LIMIT 1 OFFSET 8), 'dmitry_popov'),
((SELECT id FROM trips ORDER BY created_at LIMIT 1 OFFSET 8), 'ivan_petrov');
