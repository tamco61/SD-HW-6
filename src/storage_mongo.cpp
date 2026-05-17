#include "storage_mongo.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

#include <userver/components/component_context.hpp>
#include <userver/formats/bson/inline.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/mongo/component.hpp>
#include <userver/utils/datetime.hpp>

namespace blablacar_service {

namespace {

constexpr auto kLogComponent = "mongo-storage";

std::string TrimCopy(std::string value) {
  value.erase(value.begin(),
              std::find_if(value.begin(), value.end(), [](unsigned char ch) {
                return !std::isspace(ch);
              }));
  value.erase(
      std::find_if(value.rbegin(), value.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
      }).base(),
      value.end());
  return value;
}

std::vector<std::string> SplitRoutePoints(const std::string& raw_points) {
  std::vector<std::string> points;
  std::string normalized = raw_points;

  for (const std::string delimiter : {"->", ",", ";"}) {
    std::size_t pos = 0;
    while ((pos = normalized.find(delimiter, pos)) != std::string::npos) {
      normalized.replace(pos, delimiter.size(), "|");
      pos += 1;
    }
  }

  std::size_t start = 0;
  while (start <= normalized.size()) {
    const auto end = normalized.find('|', start);
    auto token = TrimCopy(normalized.substr(start, end - start));
    if (!token.empty()) {
      points.push_back(std::move(token));
    }
    if (end == std::string::npos) break;
    start = end + 1;
  }

  if (points.empty()) {
    points.push_back(raw_points);
  }
  if (points.size() == 1) {
    points.push_back(points.front());
  }

  return points;
}

MongoTrip ParseMongoTripDocument(const userver::formats::bson::Document& doc) {
  MongoTrip trip;
  trip.id = doc["_id"].As<userver::formats::bson::Oid>().ToString();
  trip.owner_login = doc["owner_login"].As<std::string>();
  trip.route_id = doc["route_id"].As<std::string>();
  trip.status = doc["status"].As<std::string>();
  trip.participants_logins = doc["participants"].As<std::vector<std::string>>();

  const auto route = doc["route"];
  trip.route.route_id = trip.route_id;
  trip.route.owner_login = trip.owner_login;
  trip.route.points = route["points"].As<std::vector<std::string>>();
  trip.route.raw_points = route["raw_points"].As<std::string>();
  trip.route.distance_km = route["distance_km"].As<double>();

  if (trip.route.raw_points.empty() && !trip.route.points.empty()) {
    trip.route.raw_points = trip.route.points.front();
    for (std::size_t i = 1; i < trip.route.points.size(); ++i) {
      trip.route.raw_points += " -> " + trip.route.points[i];
    }
  }

  return trip;
}

userver::formats::bson::Oid ParseTripOidOrThrow(const std::string& trip_id_str) {
  try {
    return userver::formats::bson::Oid{trip_id_str};
  } catch (const std::exception&) {
    throw std::invalid_argument("Invalid trip id");
  }
}

}  // namespace

StorageMongoComponent::StorageMongoComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : LoggableComponentBase(config, context) {
  // Получаем пул соединений компонента Монги по его имени (kName =
  // "mongo-blabla")
  mongo_pool_ =
      context.FindComponent<userver::components::Mongo>("mongo-blabla")
          .GetPool();
}

// --- Токены авторизации ---

void StorageMongoComponent::InsertAuthToken(const std::string& token,
                                            const std::string& login) const {
  try {
    auto auth_collection = mongo_pool_->GetCollection("auth_tokens");

    auto now = userver::utils::datetime::Now();
    auto expires_at = now + std::chrono::hours(24);

    auth_collection.InsertOne(userver::formats::bson::MakeDoc(
        "_id", token, "login", login, "created_at", now, "expires_at",
        expires_at));
  } catch (const std::exception& e) {
    LOG_ERROR() << "Failed to insert auth token to Mongo: " << e.what();
    // Здесь мы просто логируем ошибку, но в реальном проде можно
    // бросать кастомное исключение, чтобы HTTP-ручка вернула 500
  }
}

std::optional<std::string> StorageMongoComponent::GetLoginByToken(
    const std::string& token) const {
  try {
    auto auth_collection = mongo_pool_->GetCollection("auth_tokens");

    // Ищем токен с проверкой, что время его жизни еще не истекло
    auto doc = auth_collection.FindOne(userver::formats::bson::MakeDoc(
        "_id", token, "expires_at",
        userver::formats::bson::MakeDoc("$gt",
                                        userver::utils::datetime::Now())));

    if (!doc) {
      return std::nullopt;
    }

    return (*doc)["login"].As<std::string>();
  } catch (const std::exception& e) {
    LOG_ERROR() << "Failed to get login by token from Mongo: " << e.what();
    return std::nullopt;
  }
}

// --- Поездки (Trips) ---

std::string StorageMongoComponent::CreateTrip(
    const Route& route,
    double distance_km,
    std::chrono::system_clock::time_point departure_time) const {
  try {
    auto trips_collection = mongo_pool_->GetCollection("trips");

    auto now = userver::utils::datetime::Now();
    const auto route_points = SplitRoutePoints(route.points);
    const auto trip_oid = userver::formats::bson::Oid{};

    auto route_doc = userver::formats::bson::MakeDoc(
        "points", route_points,
        "raw_points", route.points,
        "distance_km", distance_km);

    auto doc = userver::formats::bson::MakeDoc(
        "_id", trip_oid,
        "owner_login", route.owner_login,
        "route_id", route.id,
        "route", route_doc,
        "departure_time", departure_time,
        "status", "active",
        "participants", userver::formats::bson::MakeArray(),
        "created_at", now);

    trips_collection.InsertOne(doc);
    return trip_oid.ToString();
  } catch (const std::exception& e) {
    LOG_ERROR() << "Failed to create trip in Mongo: " << e.what();
    throw std::runtime_error("MongoDB Insert Error");
  }
}

std::optional<MongoTrip> StorageMongoComponent::GetTripById(
    const std::string& trip_id_str) const {
  try {
    const auto trip_oid = ParseTripOidOrThrow(trip_id_str);
    auto trips_collection = mongo_pool_->GetCollection("trips");
    auto doc =
        trips_collection.FindOne(userver::formats::bson::MakeDoc("_id", trip_oid));
    if (!doc) {
      return std::nullopt;
    }
    return ParseMongoTripDocument(*doc);
  } catch (const std::exception& e) {
    if (dynamic_cast<const std::invalid_argument*>(&e) != nullptr) {
      throw std::invalid_argument("Invalid trip id");
    }
    LOG_ERROR() << "Failed to get trip from Mongo: " << e.what();
    return std::nullopt;
  }
}

JoinTripResult StorageMongoComponent::JoinTrip(const std::string& trip_id_str,
                                               const std::string& login) const {
  try {
    auto trips_collection = mongo_pool_->GetCollection("trips");

    const auto trip_oid = ParseTripOidOrThrow(trip_id_str);

    auto existing = trips_collection.FindOne(
        userver::formats::bson::MakeDoc("_id", trip_oid, "participants", login));
    if (existing) {
      return JoinTripResult::kAlreadyJoined;
    }

    auto result = trips_collection.UpdateOne(
        userver::formats::bson::MakeDoc("_id", trip_oid),
        userver::formats::bson::MakeDoc(
            "$addToSet", userver::formats::bson::MakeDoc("participants", login)));

    if (result.MatchedCount() == 0) {
      return JoinTripResult::kNotFound;
    }

    return result.ModifiedCount() > 0 ? JoinTripResult::kJoined
                                      : JoinTripResult::kAlreadyJoined;
  } catch (const std::exception& e) {
    if (dynamic_cast<const std::invalid_argument*>(&e) != nullptr) {
      return JoinTripResult::kInvalidId;
    }
    LOG_ERROR() << "Failed to join trip in Mongo: " << e.what();
    return JoinTripResult::kError;
  }
}

// --- Регистрация компонента ---

void AppendStorageMongo(userver::components::ComponentList& component_list) {
  component_list.Append<StorageMongoComponent>();
}

}  // namespace blablacar_service
