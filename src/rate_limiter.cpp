#include "rate_limiter.hpp"

#include <userver/components/component_context.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/redis/component.hpp>

namespace blablacar_service {

RateLimiterComponent::RateLimiterComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : LoggableComponentBase(config, context) {
  auto& redis =
      context.FindComponent<userver::components::Redis>("redis-blabla");
  redis_client_ = redis.GetClient("redis-blabla");
}

RateLimitResult RateLimiterComponent::Check(const std::string& key,
                                            std::int64_t limit,
                                            std::int64_t window_seconds) const {
  RateLimitResult result{true, 0, window_seconds, limit};

  try {
    const std::string full_key = "rate_limit:" + key;

    // Выполняем Lua-скрипт атомарно на стороне Redis.
    // Скрипт возвращает массив: {allowed(0/1), current_count, ttl}
    auto reply = redis_client_
                     ->Eval(kRateLimitScript, {full_key},
                            {std::to_string(limit),
                             std::to_string(window_seconds)},
                            userver::storages::redis::CommandControl{})
                     .Get();

    // Парсим ответ из Redis (массив из 3 элементов)
    auto arr = reply.GetArray();
    if (arr.size() == 3) {
      result.allowed = (arr[0].GetInt() == 1);
      result.current = arr[1].GetInt();
      result.ttl = arr[2].GetInt();
    }

    LOG_DEBUG() << "Rate limit check for " << key << ": "
                << (result.allowed ? "ALLOWED" : "DENIED") << " ("
                << result.current << "/" << result.limit << ", reset in "
                << result.ttl << "s)";
  } catch (const std::exception& e) {
    // При ошибке Redis — пропускаем запрос (fail-open),
    // чтобы не блокировать пользователей при сбое кеша.
    LOG_WARNING() << "Rate limiter Redis error for key " << key << ": "
                  << e.what();
    result.allowed = true;
  }

  return result;
}

void AppendRateLimiter(userver::components::ComponentList& component_list) {
  component_list.Append<RateLimiterComponent>();
}

}  // namespace blablacar_service
