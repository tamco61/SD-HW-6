#pragma once

#include <string_view>
#include <userver/components/component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/utils/uuid4.hpp>
#include "../rate_limiter.hpp"
#include "../redis_cache.hpp"
#include "../storage_mongo.hpp"
#include "../storage_pg.hpp"

namespace blablacar_service {

class LoginUserHandler final
    : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-login-user";
  LoginUserHandler(const userver::components::ComponentConfig& config,
                   const userver::components::ComponentContext& context);
  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext&) const override;

 private:
  StoragePgComponent& pg_storage_;
  StorageMongoComponent& mongo_storage_;
  RateLimiterComponent& rate_limiter_;
};

class CreateUserHandler final
    : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-create-user";
  CreateUserHandler(const userver::components::ComponentConfig& config,
                    const userver::components::ComponentContext& context);
  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext&) const override;

 private:
  StoragePgComponent& storage_;
  RedisCacheComponent& cache_;
};

class GetUserHandler final : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-get-user";
  GetUserHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);
  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext&) const override;

 private:
  StoragePgComponent& storage_;
  RedisCacheComponent& cache_;
};

class SearchUsersHandler final
    : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-search-users";
  SearchUsersHandler(const userver::components::ComponentConfig& config,
                     const userver::components::ComponentContext& context);
  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext&) const override;

 private:
  StoragePgComponent& storage_;
};

class CreateRouteHandler final
    : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-create-route";
  CreateRouteHandler(const userver::components::ComponentConfig& config,
                     const userver::components::ComponentContext& context);
  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext&) const override;

 private:
  StoragePgComponent& storage_;
  RedisCacheComponent& cache_;
};

class GetRoutesHandler final
    : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-get-routes";
  GetRoutesHandler(const userver::components::ComponentConfig& config,
                   const userver::components::ComponentContext& context);
  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext&) const override;

 private:
  StoragePgComponent& storage_;
  RedisCacheComponent& cache_;
};

class CreateTripHandler final
    : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-create-trip";
  CreateTripHandler(const userver::components::ComponentConfig& config,
                    const userver::components::ComponentContext& context);
  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext&) const override;

 private:
  StoragePgComponent& pg_storage_;
  StorageMongoComponent& mongo_storage_;
};

class AddUserToTripHandler final
    : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-add-user-to-trip";
  AddUserToTripHandler(const userver::components::ComponentConfig& config,
                       const userver::components::ComponentContext& context);
  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext&) const override;

 private:
  StorageMongoComponent& mongo_storage_;
};

class GetTripHandler final : public userver::server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-get-trip";
  GetTripHandler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context);
  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext&) const override;

 private:
  StorageMongoComponent& mongo_storage_;
};

void AppendApiHandlers(userver::components::ComponentList& component_list);

}  // namespace blablacar_service
