#include "storage_pg.hpp"

#include <stdexcept>
#include <userver/components/component_context.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/utils/uuid4.hpp>

namespace blablacar_service {

namespace {

constexpr auto kLogComponent = "postgres-storage";

}  // namespace

StoragePgComponent::StoragePgComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : LoggableComponentBase(config, context) {
  pg_cluster_ =
      context.FindComponent<userver::components::Postgres>("postgres-db")
          .GetCluster();
}

// Users

bool StoragePgComponent::AddUser(const User& user) {
  try {
    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        userver::storages::postgres::Query{
            "INSERT INTO users (login, first_name, last_name, password_hash) "
            "VALUES ($1, $2, $3, $4) ON CONFLICT (login) DO NOTHING"},
        user.login, user.first_name, user.last_name, user.password);

    return result.RowsAffected() == 1;
  } catch (const std::exception& e) {
    LOG_ERROR() << "Failed to add user: " << e.what();
    return false;
  }
}

bool StoragePgComponent::LoginUser(const std::string& login,
                                   const std::string& password) {
  try {
    // Просто проверяем, есть ли такой пользователь с таким паролем
    auto check_result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        userver::storages::postgres::Query{
            "SELECT login FROM users WHERE login = $1 AND password_hash = $2"},
        login, password);

    // Если выборка не пустая, значит логин и пароль верные
    return !check_result.IsEmpty();
  } catch (const std::exception& e) {
    LOG_ERROR() << "Failed to check user credentials: " << e.what();
    return false;
  }
}

std::optional<User> StoragePgComponent::GetUserByLogin(
    const std::string& login) const {
  try {
    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        userver::storages::postgres::Query{
            "SELECT login, first_name, last_name, password_hash "
            "FROM users WHERE login = $1"},
        login);

    if (result.IsEmpty()) {
      return std::nullopt;
    }

    User user;
    user.login = result[0]["login"].As<std::string>();
    user.first_name = result[0]["first_name"].As<std::string>();
    user.last_name = result[0]["last_name"].As<std::string>();
    user.password = result[0]["password_hash"].As<std::string>();

    return user;
  } catch (const std::exception& e) {
    LOG_ERROR() << "Failed to get user by login: " << e.what();
    return std::nullopt;
  }
}

std::vector<User> StoragePgComponent::SearchUsersByName(
    const std::string& mask) const {
  try {
    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        userver::storages::postgres::Query{
            "SELECT login, first_name, last_name, password_hash "
            "FROM users "
            "WHERE first_name ILIKE $1 OR last_name ILIKE $1 OR login ILIKE $1 "
            "ORDER BY first_name, last_name"},
        "%" + mask + "%");

    std::vector<User> users;
    for (const auto& row : result) {
      User user;
      user.login = row["login"].As<std::string>();
      user.first_name = row["first_name"].As<std::string>();
      user.last_name = row["last_name"].As<std::string>();
      user.password = row["password_hash"].As<std::string>();
      users.push_back(user);
    }

    return users;
  } catch (const std::exception& e) {
    LOG_ERROR() << "Failed to search users: " << e.what();
    return {};
  }
}

// Routes

std::string StoragePgComponent::AddRoute(Route route) {
  try {
    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        userver::storages::postgres::Query{
            "INSERT INTO routes (owner_login, points) "
            "VALUES ($1, $2) RETURNING id"},
        route.owner_login, route.points);

    return result[0]["id"].As<std::string>();
  } catch (const std::exception& e) {
    LOG_ERROR() << "Failed to add route: " << e.what();
    return "";
  }
}

std::vector<Route> StoragePgComponent::GetRoutesByUser(
    const std::string& login) const {
  try {
    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        userver::storages::postgres::Query{
            "SELECT id, owner_login, points, created_at "
            "FROM routes WHERE owner_login = $1 "
            "ORDER BY created_at DESC"},
        login);

    std::vector<Route> routes;
    for (const auto& row : result) {
      Route route;
      route.id = row["id"].As<std::string>();
      route.owner_login = row["owner_login"].As<std::string>();
      route.points = row["points"].As<std::string>();
      routes.push_back(route);
    }

    return routes;
  } catch (const std::exception& e) {
    LOG_ERROR() << "Failed to get routes by user: " << e.what();
    return {};
  }
}

std::optional<Route> StoragePgComponent::GetRouteById(
    const std::string& route_id) const {
  try {
    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        userver::storages::postgres::Query{
            "SELECT id, owner_login, points "
            "FROM routes WHERE id = $1"},
        route_id);

    if (result.IsEmpty()) {
      return std::nullopt;
    }

    Route route;
    route.id = result[0]["id"].As<std::string>();
    route.owner_login = result[0]["owner_login"].As<std::string>();
    route.points = result[0]["points"].As<std::string>();
    return route;
  } catch (const std::exception& e) {
    LOG_ERROR() << "Failed to get route by id: " << e.what();
    return std::nullopt;
  }
}

// Trips

std::string StoragePgComponent::AddTrip(Trip trip) {
  try {
    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        userver::storages::postgres::Query{
            "INSERT INTO trips (route_id) VALUES ($1) RETURNING id"},
        trip.route_id);

    return result[0]["id"].As<std::string>();
  } catch (const std::exception& e) {
    LOG_ERROR() << "Failed to add trip: " << e.what();
    return "";
  }
}

bool StoragePgComponent::AddUserToTrip(const std::string& trip_id,
                                       const std::string& login) {
  try {
    auto result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        userver::storages::postgres::Query{
            "INSERT INTO trip_participants (trip_id, login) "
            "VALUES ($1, $2) ON CONFLICT (trip_id, login) DO NOTHING"},
        trip_id, login);

    return result.RowsAffected() == 1;
  } catch (const std::exception& e) {
    LOG_ERROR() << "Failed to add user to trip: " << e.what();
    return false;
  }
}

std::optional<Trip> StoragePgComponent::GetTrip(
    const std::string& trip_id) const {
  try {
    // Get trip info
    auto trip_result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        userver::storages::postgres::Query{
            "SELECT id, route_id, status, departure_time "
            "FROM trips WHERE id = $1"},
        trip_id);

    if (trip_result.IsEmpty()) {
      return std::nullopt;
    }

    Trip trip;
    trip.id = trip_result[0]["id"].As<std::string>();
    trip.route_id = trip_result[0]["route_id"].As<std::string>();

    // Get participants
    auto participants_result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        userver::storages::postgres::Query{
            "SELECT login FROM trip_participants WHERE trip_id = $1"},
        trip_id);

    for (const auto& row : participants_result) {
      trip.participants_logins.push_back(row["login"].As<std::string>());
    }

    return trip;
  } catch (const std::exception& e) {
    LOG_ERROR() << "Failed to get trip: " << e.what();
    return std::nullopt;
  }
}

void AppendStoragePg(userver::components::ComponentList& component_list) {
  component_list.Append<StoragePgComponent>();
}

}  // namespace blablacar_service
