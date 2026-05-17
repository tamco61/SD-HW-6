#pragma once

#include <cstdint>
#include <string>

#include <userver/components/component_list.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/storages/redis/client.hpp>

namespace blablacar_service {

/// Результат проверки rate limit.
struct RateLimitResult {
  bool allowed;         ///< true — запрос разрешен, false — превышен лимит
  std::int64_t current; ///< Текущее количество запросов в окне
  std::int64_t ttl;     ///< Секунд до сброса окна (TTL ключа)
  std::int64_t limit;   ///< Максимальное количество запросов в окне
};

/// @brief Компонент rate limiting на базе Redis.
///
/// Реализует алгоритм Fixed Window Counter с помощью Lua-скрипта,
/// выполняемого атомарно на стороне Redis.
///
/// Lua-скрипт за один вызов выполняет INCR + EXPIRE + TTL,
/// что гарантирует атомарность и устраняет race conditions.
class RateLimiterComponent final
    : public userver::components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "rate-limiter";

  RateLimiterComponent(const userver::components::ComponentConfig& config,
                       const userver::components::ComponentContext& context);

  /// Проверить, разрешён ли запрос для данного ключа (например, IP-адреса).
  /// @param key     Идентификатор клиента (IP, login и т.п.)
  /// @param limit   Максимум запросов в окне
  /// @param window_seconds  Размер окна в секундах
  RateLimitResult Check(const std::string& key, std::int64_t limit,
                        std::int64_t window_seconds) const;

 private:
  std::shared_ptr<userver::storages::redis::Client> redis_client_;

  /// Lua-скрипт для атомарной проверки rate limit (Fixed Window Counter).
  static constexpr const char* kRateLimitScript = R"(
    local current = redis.call("INCR", KEYS[1])
    if current == 1 then
        redis.call("EXPIRE", KEYS[1], ARGV[2])
    end
    local ttl = redis.call("TTL", KEYS[1])
    if ttl < 0 then
        ttl = tonumber(ARGV[2])
    end
    if current > tonumber(ARGV[1]) then
        return {0, current, ttl}
    else
        return {1, current, ttl}
    end
  )";
};

void AppendRateLimiter(userver::components::ComponentList& component_list);

}  // namespace blablacar_service
