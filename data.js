// Seed data for MongoDB homework

db = db.getSiblingDB('blablacar_db');

db.users.drop();

const usersData = [
  { _id: 'ivan_petrov', first_name: 'Ivan', last_name: 'Petrov', created_at: new Date() },
  { _id: 'sergey_novikov', first_name: 'Sergey', last_name: 'Novikov', created_at: new Date() },
  { _id: 'maria_ivanova', first_name: 'Maria', last_name: 'Ivanova', created_at: new Date() },
  { _id: 'alex_smirnov', first_name: 'Alex', last_name: 'Smirnov', created_at: new Date() },
  { _id: 'dmitry_sokolov', first_name: 'Dmitry', last_name: 'Sokolov', created_at: new Date() },
  { _id: 'elena_kuznetsova', first_name: 'Elena', last_name: 'Kuznetsova', created_at: new Date() },
  { _id: 'anna_morozova', first_name: 'Anna', last_name: 'Morozova', created_at: new Date() },
  { _id: 'pavel_volkov', first_name: 'Pavel', last_name: 'Volkov', created_at: new Date() },
  { _id: 'olga_lebedeva', first_name: 'Olga', last_name: 'Lebedeva', created_at: new Date() },
  { _id: 'nikita_kozlov', first_name: 'Nikita', last_name: 'Kozlov', created_at: new Date() },
];

db.users.insertMany(usersData);

const tripsData = [
  {
    owner_login: 'ivan_petrov',
    route_id: 'route-001',
    route: { points: ['Moscow', 'Kazan', 'Ufa'], raw_points: 'Moscow -> Kazan -> Ufa', distance_km: 1350 },
    departure_time: new Date(new Date().getTime() + 86400000 * 2),
    status: 'active',
    participants: ['sergey_novikov', 'alex_smirnov'],
    created_at: new Date(),
  },
  {
    owner_login: 'maria_ivanova',
    route_id: 'route-002',
    route: { points: ['St. Petersburg', 'Moscow'], raw_points: 'St. Petersburg -> Moscow', distance_km: 700 },
    departure_time: new Date(new Date().getTime() + 86400000),
    status: 'active',
    participants: ['dmitry_sokolov'],
    created_at: new Date(),
  },
  {
    owner_login: 'alex_smirnov',
    route_id: 'route-003',
    route: { points: ['Novosibirsk', 'Omsk'], raw_points: 'Novosibirsk -> Omsk', distance_km: 650 },
    departure_time: new Date(new Date().getTime() - 86400000 * 5),
    status: 'completed',
    participants: ['elena_kuznetsova', 'anna_morozova', 'pavel_volkov'],
    created_at: new Date(new Date().getTime() - 86400000 * 10),
  },
  {
    owner_login: 'dmitry_sokolov',
    route_id: 'route-004',
    route: { points: ['Yekaterinburg', 'Chelyabinsk'], raw_points: 'Yekaterinburg -> Chelyabinsk', distance_km: 210 },
    departure_time: new Date(new Date().getTime() + 86400000 * 5),
    status: 'active',
    participants: [],
    created_at: new Date(),
  },
  {
    owner_login: 'ivan_petrov',
    route_id: 'route-005',
    route: { points: ['Ufa', 'Samara'], raw_points: 'Ufa -> Samara', distance_km: 460 },
    departure_time: new Date(new Date().getTime() - 86400000 * 2),
    status: 'cancelled',
    participants: ['olga_lebedeva'],
    created_at: new Date(new Date().getTime() - 86400000 * 7),
  },
  {
    owner_login: 'elena_kuznetsova',
    route_id: 'route-006',
    route: { points: ['Kazan', 'Nizhny Novgorod', 'Moscow'], raw_points: 'Kazan -> Nizhny Novgorod -> Moscow', distance_km: 820 },
    departure_time: new Date(new Date().getTime() + 86400000 * 3),
    status: 'active',
    participants: ['ivan_petrov', 'maria_ivanova'],
    created_at: new Date(),
  },
  {
    owner_login: 'pavel_volkov',
    route_id: 'route-007',
    route: { points: ['Rostov-on-Don', 'Krasnodar', 'Sochi'], raw_points: 'Rostov-on-Don -> Krasnodar -> Sochi', distance_km: 550 },
    departure_time: new Date(new Date().getTime() + 86400000 * 10),
    status: 'active',
    participants: ['nikita_kozlov'],
    created_at: new Date(),
  },
  {
    owner_login: 'olga_lebedeva',
    route_id: 'route-008',
    route: { points: ['Vladivostok', 'Khabarovsk'], raw_points: 'Vladivostok -> Khabarovsk', distance_km: 760 },
    departure_time: new Date(new Date().getTime() - 86400000 * 30),
    status: 'completed',
    participants: ['alex_smirnov', 'dmitry_sokolov'],
    created_at: new Date(new Date().getTime() - 86400000 * 40),
  },
  {
    owner_login: 'nikita_kozlov',
    route_id: 'route-009',
    route: { points: ['Samara', 'Saratov'], raw_points: 'Samara -> Saratov', distance_km: 380 },
    departure_time: new Date(new Date().getTime() + 86400000 * 4),
    status: 'active',
    participants: [],
    created_at: new Date(),
  },
  {
    owner_login: 'anna_morozova',
    route_id: 'route-010',
    route: { points: ['Voronezh', 'Moscow'], raw_points: 'Voronezh -> Moscow', distance_km: 520 },
    departure_time: new Date(new Date().getTime() + 86400000),
    status: 'active',
    participants: ['ivan_petrov', 'sergey_novikov', 'pavel_volkov'],
    created_at: new Date(),
  },
  {
    owner_login: 'sergey_novikov',
    route_id: 'route-011',
    route: { points: ['Moscow', 'Tver'], raw_points: 'Moscow -> Tver', distance_km: 180 },
    departure_time: new Date(new Date().getTime() + 86400000 * 7),
    status: 'active',
    participants: [],
    created_at: new Date(),
  },
  {
    owner_login: 'maria_ivanova',
    route_id: 'route-012',
    route: { points: ['Moscow', 'Vladimir', 'Nizhny Novgorod'], raw_points: 'Moscow -> Vladimir -> Nizhny Novgorod', distance_km: 420 },
    departure_time: new Date(new Date().getTime() + 86400000 * 12),
    status: 'active',
    participants: ['elena_kuznetsova'],
    created_at: new Date(),
  },
];

db.trips.insertMany(tripsData);

print(
  'Inserted ' +
    db.users.countDocuments() +
    ' users and ' +
    db.trips.countDocuments() +
    ' trips.',
);
