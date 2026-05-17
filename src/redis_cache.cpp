#include "redis_cache.hpp"

#include <userver/components/component_context.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/redis/component.hpp>

namespace blablacar_service {

RedisCacheComponent::RedisCacheComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : LoggableComponentBase(config, context) {
  auto& redis =
      context.FindComponent<userver::components::Redis>("redis-blabla");
  redis_client_ = redis.GetClient("redis-blabla");
}

std::optional<std::string> RedisCacheComponent::Get(
    const std::string& key) const {
  try {
    auto result =
        redis_client_->Get(key, userver::storages::redis::CommandControl{})
            .Get();

    if (result) {
      LOG_DEBUG() << "Redis cache HIT for key: " << key;
      return *result;
    }

    LOG_DEBUG() << "Redis cache MISS for key: " << key;
    return std::nullopt;
  } catch (const std::exception& e) {
    LOG_WARNING() << "Redis GET failed for key " << key << ": " << e.what();
    return std::nullopt;
  }
}

void RedisCacheComponent::Set(const std::string& key,
                              const std::string& value,
                              std::int64_t ttl_seconds) const {
  try {
    redis_client_
        ->Set(key, value, std::chrono::seconds{ttl_seconds},
              userver::storages::redis::CommandControl{})
        .Get();

    LOG_DEBUG() << "Redis cache SET key: " << key << " TTL: " << ttl_seconds
                << "s";
  } catch (const std::exception& e) {
    LOG_WARNING() << "Redis SET failed for key " << key << ": " << e.what();
  }
}

void RedisCacheComponent::Invalidate(const std::string& key) const {
  try {
    redis_client_->Del(key, userver::storages::redis::CommandControl{}).Get();
    LOG_DEBUG() << "Redis cache INVALIDATED key: " << key;
  } catch (const std::exception& e) {
    LOG_WARNING() << "Redis DEL failed for key " << key << ": " << e.what();
  }
}

void AppendRedisCache(userver::components::ComponentList& component_list) {
  component_list.Append<RedisCacheComponent>();
}

}  // namespace blablacar_service
