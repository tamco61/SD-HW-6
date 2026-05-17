#pragma once

#include <string>
#include <vector>

namespace blablacar_service {

struct User {
  std::string login;
  std::string first_name;
  std::string last_name;
  std::string password;
};

struct Route {
  std::string id;
  std::string owner_login;
  std::string points;
};

struct Trip {
  std::string id;
  std::string route_id;
  std::vector<std::string> participants_logins;
};

struct MongoRouteSnapshot {
  std::string route_id;
  std::string owner_login;
  std::string raw_points;
  std::vector<std::string> points;
  double distance_km{0.0};
};

struct MongoTrip {
  std::string id;
  std::string owner_login;
  std::string route_id;
  MongoRouteSnapshot route;
  std::string status;
  std::vector<std::string> participants_logins;
};

}  // namespace blablacar_service
