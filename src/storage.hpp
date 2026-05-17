#pragma once

#include <optional>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

#include <userver/components/component_list.hpp>
#include <userver/components/loggable_component_base.hpp>
#include "models.hpp"

namespace blablacar_service {

class StorageComponent final
    : public userver::components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "in-memory-storage";
  StorageComponent(const userver::components::ComponentConfig& config,
                   const userver::components::ComponentContext& context);

  bool AddUser(const User& user);
  std::optional<std::string> LoginUser(const std::string& login,
                                       const std::string& password);
  std::optional<std::string> GetLoginByToken(const std::string& token) const;
  std::optional<User> GetUserByLogin(const std::string& login) const;
  std::vector<User> SearchUsersByName(const std::string& mask) const;

  std::string AddRoute(Route route);
  std::vector<Route> GetRoutesByUser(const std::string& login) const;

  std::string AddTrip(Trip trip);
  bool AddUserToTrip(const std::string& trip_id, const std::string& login);
  std::optional<Trip> GetTrip(const std::string& trip_id) const;

 private:
  mutable std::shared_mutex users_mutex_;
  std::unordered_map<std::string, User> users_;

  mutable std::shared_mutex tokens_mutex_;
  std::unordered_map<std::string, std::string> tokens_;

  mutable std::shared_mutex routes_mutex_;
  std::unordered_map<std::string, Route> routes_;

  mutable std::shared_mutex trips_mutex_;
  std::unordered_map<std::string, Trip> trips_;
};

void AppendStorage(userver::components::ComponentList& component_list);

}  // namespace blablacar_service