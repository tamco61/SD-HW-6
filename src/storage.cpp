#include "storage.hpp"

#include <algorithm>
#include <mutex>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/utils/uuid4.hpp>

namespace blablacar_service {

StorageComponent::StorageComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : LoggableComponentBase(config, context) {}

std::optional<std::string> StorageComponent::LoginUser(
    const std::string& login, const std::string& password) {
  {
    std::shared_lock lock(users_mutex_);
    auto it = users_.find(login);
    if (it == users_.end() || it->second.password != password) {
      return std::nullopt;
    }
  }

  std::string token = userver::utils::generators::GenerateUuid();
  {
    std::unique_lock lock(tokens_mutex_);
    tokens_[token] = login;
  }
  return token;
}

std::optional<std::string> StorageComponent::GetLoginByToken(
    const std::string& token) const {
  std::shared_lock lock(tokens_mutex_);
  if (auto it = tokens_.find(token); it != tokens_.end()) {
    return it->second;
  }
  return std::nullopt;
}

bool StorageComponent::AddUser(const User& user) {
  std::unique_lock lock(users_mutex_);
  if (users_.count(user.login)) {
    return false;
  }
  users_[user.login] = user;
  return true;
}

std::optional<User> StorageComponent::GetUserByLogin(
    const std::string& login) const {
  std::shared_lock lock(users_mutex_);
  if (auto it = users_.find(login); it != users_.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::vector<User> StorageComponent::SearchUsersByName(
    const std::string& mask) const {
  std::shared_lock lock(users_mutex_);
  std::vector<User> result;

  for (const auto& [login, user] : users_) {
    if (user.first_name.find(mask) != std::string::npos ||
        user.last_name.find(mask) != std::string::npos) {
      result.push_back(user);
    }
  }
  return result;
}

std::string StorageComponent::AddRoute(Route route) {
  route.id = userver::utils::generators::GenerateUuid();

  std::unique_lock lock(routes_mutex_);
  routes_[route.id] = route;
  return route.id;
}

std::vector<Route> StorageComponent::GetRoutesByUser(
    const std::string& login) const {
  std::shared_lock lock(routes_mutex_);
  std::vector<Route> result;

  for (const auto& [id, route] : routes_) {
    if (route.owner_login == login) {
      result.push_back(route);
    }
  }
  return result;
}

std::string StorageComponent::AddTrip(Trip trip) {
  trip.id = userver::utils::generators::GenerateUuid();

  std::unique_lock lock(trips_mutex_);
  trips_[trip.id] = trip;
  return trip.id;
}

bool StorageComponent::AddUserToTrip(const std::string& trip_id,
                                     const std::string& login) {
  std::unique_lock lock(trips_mutex_);
  auto it = trips_.find(trip_id);
  if (it == trips_.end()) {
    return false;
  }

  auto& participants = it->second.participants_logins;
  if (std::find(participants.begin(), participants.end(), login) ==
      participants.end()) {
    participants.push_back(login);
  }
  return true;
}

std::optional<Trip> StorageComponent::GetTrip(
    const std::string& trip_id) const {
  std::shared_lock lock(trips_mutex_);
  if (auto it = trips_.find(trip_id); it != trips_.end()) {
    return it->second;
  }
  return std::nullopt;
}

void AppendStorage(userver::components::ComponentList& component_list) {
  component_list.Append<StorageComponent>();
}

}  // namespace blablacar_service