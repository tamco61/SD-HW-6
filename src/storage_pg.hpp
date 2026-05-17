#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

// Provide coroutine support for Clang
#if defined(__clang__)
#if __has_include(<coroutine>)
#include <coroutine>
#elif __has_include(<experimental/coroutine>)
#include <experimental/coroutine>
namespace std {
using std::experimental::coroutine_handle;
using std::experimental::noop_coroutine;
using std::experimental::suspend_always;
using std::experimental::suspend_never;
template <class Promise = void>
using coroutine_traits = std::experimental::coroutine_traits<Promise>;
}  // namespace std
#endif
#else
#include <coroutine>
#endif

#include <userver/components/component_list.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/cluster_types.hpp>

#include "models.hpp"

namespace blablacar_service {

class StoragePgComponent final
    : public userver::components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "postgres-storage";

  StoragePgComponent(const userver::components::ComponentConfig& config,
                     const userver::components::ComponentContext& context);

  // Users
  bool AddUser(const User& user);
  bool LoginUser(const std::string& login, const std::string& password);
  std::optional<User> GetUserByLogin(const std::string& login) const;
  std::vector<User> SearchUsersByName(const std::string& mask) const;

  // Routes
  std::string AddRoute(Route route);
  std::vector<Route> GetRoutesByUser(const std::string& login) const;
  std::optional<Route> GetRouteById(const std::string& route_id) const;

  // Trips
  std::string AddTrip(Trip trip);
  bool AddUserToTrip(const std::string& trip_id, const std::string& login);
  std::optional<Trip> GetTrip(const std::string& trip_id) const;

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

void AppendStoragePg(userver::components::ComponentList& component_list);

}  // namespace blablacar_service
