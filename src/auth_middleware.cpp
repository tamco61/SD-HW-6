#include "auth_middleware.hpp"
#include <userver/components/component_context.hpp>
#include <userver/server/http/http_response.hpp>
#include <userver/server/request/request_context.hpp>

namespace blablacar_service {

AuthMiddleware::AuthMiddleware(StorageMongoComponent& mongo_storage,
                               bool enabled)
    : mongo_storage_(mongo_storage), enabled_(enabled) {}

void AuthMiddleware::HandleRequest(
    userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& context) const {
  if (!enabled_) {
    Next(request, context);
    return;
  }

  const auto& auth_header = request.GetHeader("Authorization");
  const std::string bearer_prefix = "Bearer ";

  if (auth_header.starts_with(bearer_prefix)) {
    std::string token = auth_header.substr(bearer_prefix.length());

    // Идем в Монгу проверять токен
    auto login = mongo_storage_.GetLoginByToken(token);
    if (login) {
      context.SetData("X-User-Login", *login);
      Next(request, context);
      return;
    }
  }

  request.SetResponseStatus(userver::server::http::HttpStatus::kUnauthorized);
  request.GetHttpResponse().SetData("{\"error\": \"Unauthorized\"}");
}

AuthMiddlewareFactory::AuthMiddlewareFactory(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpMiddlewareFactoryBase(config, context),
      // Находим компонент Монги при инициализации фабрики
      mongo_storage_(context.FindComponent<StorageMongoComponent>()) {}

std::unique_ptr<userver::server::middlewares::HttpMiddlewareBase>
AuthMiddlewareFactory::Create(
    const userver::server::handlers::HttpHandlerBase& /*handler*/,
    userver::yaml_config::YamlConfig middleware_config) const {
  bool is_enabled = !middleware_config.IsMissing();

  // Передаем компонент Монги в создаваемый middleware
  return std::make_unique<AuthMiddleware>(mongo_storage_, is_enabled);
}

void AppendAuthMiddleware(userver::components::ComponentList& component_list) {
  component_list.Append<AuthMiddlewareFactory>();
}

}  // namespace blablacar_service