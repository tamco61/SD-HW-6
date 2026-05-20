#include "api_handlers.hpp"

#include <chrono>
#include <stdexcept>
#include <userver/components/component_context.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <utility>


namespace blablacar_service {

namespace {
using userver::formats::json::FromString;
using userver::formats::json::ToString;
using userver::formats::json::ValueBuilder;
using userver::server::http::HttpStatus;

/// TTL кеша для профиля пользователя (секунды)
constexpr std::int64_t kUserCacheTtl = 300;  // 5 минут

/// TTL кеша для маршрутов пользователя (секунды)
constexpr std::int64_t kRoutesCacheTtl = 180;  // 3 минуты

/// Rate limit: максимум запросов на /v1/login за окно
constexpr std::int64_t kLoginRateLimit = 10;

/// Rate limit: размер окна в секундах
constexpr std::int64_t kLoginRateWindow = 60;

template <typename Func>
std::string SafeHandle(const userver::server::http::HttpRequest& request,
                       Func&& f) {
  try {
    return f();
  } catch (const userver::formats::json::Exception& e) {
    request.SetResponseStatus(HttpStatus::kBadRequest);
    return "{\"error\": \"Invalid JSON or missing fields\"}";
  } catch (const std::invalid_argument& e) {
    request.SetResponseStatus(HttpStatus::kBadRequest);
    return "{\"error\": \"Bad request\"}";
  } catch (const std::exception& e) {
    request.SetResponseStatus(HttpStatus::kInternalServerError);
    return "{\"error\": \"Internal server error\"}";
  }
}

/// Установить rate limit заголовки в ответ.
void SetRateLimitHeaders(const userver::server::http::HttpRequest& request,
                         const RateLimitResult& rl) {
  auto& response = request.GetHttpResponse();
  response.SetHeader(std::string{"X-RateLimit-Limit"}, std::to_string(rl.limit));
  const auto remaining = std::max(std::int64_t{0}, rl.limit - rl.current);
  response.SetHeader(std::string{"X-RateLimit-Remaining"}, std::to_string(remaining));
  response.SetHeader(std::string{"X-RateLimit-Reset"}, std::to_string(rl.ttl));
}

}  // namespace

// ===================== LoginUserHandler =====================

LoginUserHandler::LoginUserHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_storage_(context.FindComponent<StoragePgComponent>()),
      mongo_storage_(context.FindComponent<StorageMongoComponent>()),
      rate_limiter_(context.FindComponent<RateLimiterComponent>()) {}

std::string LoginUserHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  // --- Rate Limiting (Fixed Window Counter) ---
  // Используем IP-адрес клиента как ключ для rate limit.
  const auto& client_ip = request.GetRemoteAddress().PrimaryAddressString();
  auto rl = rate_limiter_.Check(client_ip, kLoginRateLimit, kLoginRateWindow);

  // Устанавливаем заголовки rate limit в любом случае
  SetRateLimitHeaders(request, rl);

  if (!rl.allowed) {
    request.SetResponseStatus(HttpStatus{429});
    return "{\"error\": \"Too Many Requests\"}";
  }

  return SafeHandle(request, [&]() {
    auto json = FromString(request.RequestBody());
    auto login = json["login"].As<std::string>();
    auto password = json["password"].As<std::string>();

    // 1. Идем в Postgres и проверяем логин/пароль
    bool is_valid_user = pg_storage_.LoginUser(login, password);

    if (!is_valid_user) {
      request.SetResponseStatus(HttpStatus::kUnauthorized);
      return std::string("{\"error\": \"Invalid credentials\"}");
    }

    // 2. Если пароль верный — генерируем токен
    std::string token = userver::utils::generators::GenerateUuid();

    // 3. Сохраняем токен в MongoDB (с автоматическим TTL)
    mongo_storage_.InsertAuthToken(token, login);

    // 4. Отдаем токен клиенту
    ValueBuilder response;
    response["token"] = token;
    return ToString(response.ExtractValue());
  });
}

// ===================== CreateUserHandler =====================

CreateUserHandler::CreateUserHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<StoragePgComponent>()),
      cache_(context.FindComponent<RedisCacheComponent>()),
      events_(context.FindComponent<EventPublisherComponent>()) {}

std::string CreateUserHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  return SafeHandle(request, [&]() {
    auto json = FromString(request.RequestBody());
    User user{json["login"].As<std::string>(),
              json["first_name"].As<std::string>(),
              json["last_name"].As<std::string>(),
              json["password"].As<std::string>()};

    if (!storage_.AddUser(user)) {
      request.SetResponseStatus(HttpStatus::kConflict);
      return std::string("{\"error\": \"User already exists\"}");
    }

    // Инвалидируем кеш пользователя, чтобы следующий GET получил свежие данные
    cache_.Invalidate("cache:user:" + user.login);
    events_.PublishUserCreated(user);

    return std::string("{\"status\": \"ok\"}");
  });
}

// ===================== GetUserHandler =====================

GetUserHandler::GetUserHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<StoragePgComponent>()),
      cache_(context.FindComponent<RedisCacheComponent>()) {}

std::string GetUserHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  return SafeHandle(request, [&]() {
    if (!request.HasArg("login") || request.GetArg("login").empty()) {
      request.SetResponseStatus(HttpStatus::kBadRequest);
      return std::string("{\"error\": \"Missing login argument\"}");
    }
    const auto& login = request.GetArg("login");
    const std::string cache_key = "cache:user:" + login;

    // 1. Проверяем кеш (Cache-Aside: сначала кеш, потом БД)
    auto cached = cache_.Get(cache_key);
    if (cached) {
      return *cached;  // Cache HIT — возвращаем сразу
    }

    // 2. Cache MISS — идем в PostgreSQL
    auto user = storage_.GetUserByLogin(login);

    if (!user) {
      request.SetResponseStatus(HttpStatus::kNotFound);
      return std::string("{\"error\": \"User not found\"}");
    }

    ValueBuilder builder;
    builder["login"] = user->login;
    builder["first_name"] = user->first_name;
    builder["last_name"] = user->last_name;
    std::string result = ToString(builder.ExtractValue());

    // 3. Сохраняем в кеш для следующих запросов
    cache_.Set(cache_key, result, kUserCacheTtl);

    return result;
  });
}

// ===================== SearchUsersHandler =====================

SearchUsersHandler::SearchUsersHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<StoragePgComponent>()) {}

std::string SearchUsersHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  return SafeHandle(request, [&]() {
    if (!request.HasArg("mask") || request.GetArg("mask").empty()) {
      request.SetResponseStatus(HttpStatus::kBadRequest);
      return std::string("{\"error\": \"Missing mask argument\"}");
    }
    const auto& mask = request.GetArg("mask");
    auto users = storage_.SearchUsersByName(mask);

    ValueBuilder builder(userver::formats::json::Type::kArray);
    for (const auto& u : users) {
      ValueBuilder item;
      item["login"] = u.login;
      item["first_name"] = u.first_name;
      item["last_name"] = u.last_name;
      builder.PushBack(std::move(item));
    }
    return ToString(builder.ExtractValue());
  });
}

// ===================== CreateRouteHandler =====================

CreateRouteHandler::CreateRouteHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<StoragePgComponent>()),
      cache_(context.FindComponent<RedisCacheComponent>()),
      events_(context.FindComponent<EventPublisherComponent>()) {}

std::string CreateRouteHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  return SafeHandle(request, [&]() {
    auto json = FromString(request.RequestBody());
    Route route{"", json["owner_login"].As<std::string>(),
                json["points"].As<std::string>()};

    std::string id = storage_.AddRoute(route);
    if (id.empty()) {
      request.SetResponseStatus(HttpStatus::kInternalServerError);
      return std::string("{\"error\": \"Internal server error\"}");
    }
    route.id = id;
    ValueBuilder builder;
    builder["id"] = id;

    // Инвалидируем кеш маршрутов пользователя,
    // чтобы GET /v1/routes вернул актуальный список
    auto owner = json["owner_login"].As<std::string>();
    cache_.Invalidate("cache:routes:" + owner);
    events_.PublishRouteCreated(route);

    return ToString(builder.ExtractValue());
  });
}

// ===================== GetRoutesHandler =====================

GetRoutesHandler::GetRoutesHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      storage_(context.FindComponent<StoragePgComponent>()),
      cache_(context.FindComponent<RedisCacheComponent>()) {}

std::string GetRoutesHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  return SafeHandle(request, [&]() {
    if (!request.HasArg("login") || request.GetArg("login").empty()) {
      request.SetResponseStatus(HttpStatus::kBadRequest);
      return std::string("{\"error\": \"Missing login argument\"}");
    }
    const auto& login = request.GetArg("login");
    const std::string cache_key = "cache:routes:" + login;

    // 1. Проверяем кеш (Cache-Aside)
    auto cached = cache_.Get(cache_key);
    if (cached) {
      return *cached;  // Cache HIT
    }

    // 2. Cache MISS — идем в PostgreSQL
    auto routes = storage_.GetRoutesByUser(login);

    ValueBuilder builder(userver::formats::json::Type::kArray);
    for (const auto& r : routes) {
      ValueBuilder item;
      item["id"] = r.id;
      item["owner_login"] = r.owner_login;
      item["points"] = r.points;
      builder.PushBack(std::move(item));
    }
    std::string result = ToString(builder.ExtractValue());

    // 3. Сохраняем в кеш
    cache_.Set(cache_key, result, kRoutesCacheTtl);

    return result;
  });
}

// ===================== CreateTripHandler =====================

CreateTripHandler::CreateTripHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_storage_(context.FindComponent<StoragePgComponent>()),
      mongo_storage_(context.FindComponent<StorageMongoComponent>()),
      events_(context.FindComponent<EventPublisherComponent>()) {}

std::string CreateTripHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  return SafeHandle(request, [&]() {
    auto json = FromString(request.RequestBody());
    const auto route_id = json["route_id"].As<std::string>();
    auto route = pg_storage_.GetRouteById(route_id);
    if (!route) {
      request.SetResponseStatus(HttpStatus::kNotFound);
      return std::string("{\"error\": \"Route not found\"}");
    }

    auto departure_time = std::chrono::system_clock::now();
    std::string id = mongo_storage_.CreateTrip(*route, 0.0, departure_time);
    events_.PublishTripCreated(id, *route);
    ValueBuilder builder;
    builder["id"] = id;
    return ToString(builder.ExtractValue());
  });
}

// ===================== AddUserToTripHandler =====================

AddUserToTripHandler::AddUserToTripHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      mongo_storage_(context.FindComponent<StorageMongoComponent>()),
      events_(context.FindComponent<EventPublisherComponent>()) {}

std::string AddUserToTripHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  return SafeHandle(request, [&]() {
    auto json = FromString(request.RequestBody());
    auto trip_id = json["trip_id"].As<std::string>();
    auto login = json["login"].As<std::string>();

    switch (mongo_storage_.JoinTrip(trip_id, login)) {
      case JoinTripResult::kJoined:
        events_.PublishTripParticipantJoined(trip_id, login);
        return std::string("{\"status\": \"ok\"}");
      case JoinTripResult::kAlreadyJoined:
        return std::string("{\"status\": \"ok\"}");
      case JoinTripResult::kNotFound:
        request.SetResponseStatus(HttpStatus::kNotFound);
        return std::string("{\"error\": \"Trip not found\"}");
      case JoinTripResult::kInvalidId:
        request.SetResponseStatus(HttpStatus::kBadRequest);
        return std::string("{\"error\": \"Bad request\"}");
      case JoinTripResult::kError:
        request.SetResponseStatus(HttpStatus::kInternalServerError);
        return std::string("{\"error\": \"Internal server error\"}");
    }

    request.SetResponseStatus(HttpStatus::kInternalServerError);
    return std::string("{\"error\": \"Internal server error\"}");
  });
}

// ===================== GetTripHandler =====================

GetTripHandler::GetTripHandler(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      mongo_storage_(context.FindComponent<StorageMongoComponent>()) {}

std::string GetTripHandler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  return SafeHandle(request, [&]() {
    if (!request.HasArg("id") || request.GetArg("id").empty()) {
      request.SetResponseStatus(HttpStatus::kBadRequest);
      return std::string("{\"error\": \"Missing id argument\"}");
    }
    const auto& id = request.GetArg("id");
    auto trip = mongo_storage_.GetTripById(id);

    if (!trip) {
      request.SetResponseStatus(HttpStatus::kNotFound);
      return std::string("{\"error\": \"Trip not found\"}");
    }

    ValueBuilder builder;
    builder["id"] = trip->id;
    builder["route_id"] = trip->route_id;

    ValueBuilder participants(userver::formats::json::Type::kArray);
    for (const auto& p : trip->participants_logins) {
      participants.PushBack(p);
    }
    builder["participants"] = participants;

    return ToString(builder.ExtractValue());
  });
}

// ===================== Registration =====================

void AppendApiHandlers(userver::components::ComponentList& component_list) {
  component_list.Append<CreateUserHandler>();
  component_list.Append<GetUserHandler>();
  component_list.Append<LoginUserHandler>();
  component_list.Append<SearchUsersHandler>();
  component_list.Append<CreateRouteHandler>();
  component_list.Append<GetRoutesHandler>();
  component_list.Append<CreateTripHandler>();
  component_list.Append<AddUserToTripHandler>();
  component_list.Append<GetTripHandler>();
}

}  // namespace blablacar_service
