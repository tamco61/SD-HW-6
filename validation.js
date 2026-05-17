// MongoDB schema validation for trips collection

db = db.getSiblingDB('blablacar_db');

db.trips.drop();

db.createCollection('trips', {
  validator: {
    $jsonSchema: {
      bsonType: 'object',
      required: ['owner_login', 'route_id', 'route', 'departure_time', 'status'],
      properties: {
        owner_login: {
          bsonType: 'string',
          description: 'Trip owner login',
        },
        route_id: {
          bsonType: 'string',
          description: 'PostgreSQL route id kept for API compatibility',
        },
        route: {
          bsonType: 'object',
          required: ['points', 'raw_points'],
          properties: {
            points: {
              bsonType: 'array',
              minItems: 2,
              items: { bsonType: 'string' },
              description: 'Normalized route points array',
            },
            raw_points: {
              bsonType: 'string',
              minLength: 1,
              description: 'Original route text from PostgreSQL',
            },
            distance_km: {
              bsonType: 'number',
              minimum: 0,
              description: 'Optional route distance in kilometers',
            },
          },
        },
        departure_time: {
          bsonType: 'date',
        },
        status: {
          bsonType: 'string',
          enum: ['active', 'completed', 'cancelled'],
        },
        participants: {
          bsonType: 'array',
          items: { bsonType: 'string' },
        },
        created_at: {
          bsonType: 'date',
        },
      },
    },
  },
  validationLevel: 'strict',
  validationAction: 'error',
});

print("Collection 'trips' created with validation.");

try {
  db.trips.insertOne({
    owner_login: 'test_user',
    route_id: 'route-1',
    route: {
      points: ['Moscow'],
      raw_points: 'Moscow',
    },
    departure_time: new Date(),
  });
  print('ERROR: invalid document was inserted.');
} catch (e) {
  print('Validation rejected invalid document: ' + e.message);
}
