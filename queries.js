// MongoDB CRUD and aggregation examples for trips

db = db.getSiblingDB('blablacar_db');

print('==== 1. CREATE ====');

const newTrip = {
  owner_login: 'ivan_petrov',
  route_id: 'route-new-001',
  route: {
    points: ['Moscow', 'Tula'],
    raw_points: 'Moscow -> Tula',
    distance_km: 180,
  },
  departure_time: new Date(new Date().getTime() + 86400000 * 6),
  status: 'active',
  participants: [],
  created_at: new Date(),
};

const insertResult = db.trips.insertOne(newTrip);
print('Inserted trip id: ' + insertResult.insertedId);

print('\n==== 2. READ ====');

const ivanTrips = db.trips
  .find({ owner_login: 'ivan_petrov', status: { $eq: 'active' } })
  .toArray();
printjson(ivanTrips.map((trip) => ({ id: trip._id, route: trip.route.points })));

const longTrips = db.trips.find({ 'route.distance_km': { $gt: 500 } }).toArray();
print('Trips longer than 500 km: ' + longTrips.length);

const mariaTrips = db.trips.find({ participants: { $in: ['maria_ivanova'] } }).toArray();
print('Trips with maria_ivanova: ' + mariaTrips.length);

const cityTrips = db.trips
  .find({
    $and: [
      { 'route.points': { $in: ['Moscow', 'Kazan'] } },
      { status: { $ne: 'cancelled' } },
    ],
  })
  .toArray();
print('Trips via Moscow or Kazan (not cancelled): ' + cityTrips.length);

print('\n==== 3. UPDATE ====');

const tripToUpdate = db.trips.findOne({ owner_login: 'ivan_petrov', status: 'active' });
if (tripToUpdate) {
  db.trips.updateOne(
    { _id: tripToUpdate._id },
    { $addToSet: { participants: 'anna_morozova' } },
  );
  print('Added anna_morozova to trip ' + tripToUpdate._id);

  db.trips.updateOne(
    { _id: tripToUpdate._id },
    { $pull: { participants: 'anna_morozova' } },
  );
  print('Removed anna_morozova from trip ' + tripToUpdate._id);
}

const updateResult = db.trips.updateMany(
  { status: 'active', departure_time: { $lt: new Date() } },
  { $set: { status: 'completed' } },
);
print('Completed expired active trips: ' + updateResult.modifiedCount);

print('\n==== 4. DELETE ====');

const deleteResult = db.trips.deleteMany({
  status: 'cancelled',
  created_at: { $lt: new Date(new Date().getTime() - 86400000 * 5) },
});
print('Deleted cancelled old trips: ' + deleteResult.deletedCount);

print('\n==== 5. AGGREGATION ====');

const aggPipeline = [
  { $match: { status: 'completed' } },
  { $addFields: { passengers_count: { $size: '$participants' } } },
  {
    $group: {
      _id: '$owner_login',
      total_trips: { $sum: 1 },
      total_distance_km: { $sum: '$route.distance_km' },
      total_passengers: { $sum: '$passengers_count' },
    },
  },
  {
    $lookup: {
      from: 'users',
      localField: '_id',
      foreignField: '_id',
      as: 'user_info',
    },
  },
  { $unwind: '$user_info' },
  {
    $project: {
      _id: 0,
      login: '$_id',
      name: { $concat: ['$user_info.first_name', ' ', '$user_info.last_name'] },
      total_trips: 1,
      total_distance_km: 1,
      total_passengers: 1,
    },
  },
  { $sort: { total_distance_km: -1 } },
];

printjson(db.trips.aggregate(aggPipeline).toArray());
