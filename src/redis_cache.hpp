#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include <userver/components/component_list.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/storages/redis/client.hpp>

namespace blablacar_service {

/// @brief Компонент кеширования на базе Redis (стратегия Cache-Aside).
///
/// Используется для кеширования результатов GET-запросов,
/// чтобы снизить нагрузку на PostgreSQL и MongoDB.
class RedisCacheComponent final
    : public userver::components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "redis-cache";

  RedisCacheComponent(const userver::components::ComponentConfig& config,
                      const userver::components::ComponentContext& context);

  /// Получить значение из кеша. Возвращает nullopt при промахе.
  std::optional<std::string> Get(const std::string& key) const;

  /// Положить значение в кеш с заданным TTL (в секундах).
  void Set(const std::string& key, const std::string& value,
           std::int64_t ttl_seconds) const;

  /// Удалить конкретный ключ из кеша (точечная инвалидация).
  void Invalidate(const std::string& key) const;

 private:
  std::shared_ptr<userver::storages::redis::Client> redis_client_;
};

void AppendRedisCache(userver::components::ComponentList& component_list);

}  // namespace blablacar_service
