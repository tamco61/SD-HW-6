#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include <userver/components/component_list.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/storages/mongo/pool.hpp>  // Подключаем пул MongoDB

#include "models.hpp"

namespace blablacar_service {

enum class JoinTripResult {
  kJoined,
  kAlreadyJoined,
  kNotFound,
  kInvalidId,
  kError,
};

class StorageMongoComponent final
    : public userver::components::LoggableComponentBase {
 public:
  // Имя компонента ДОЛЖНО совпадать с ключом в static_config.yaml
  static constexpr std::string_view kName = "mongo-storage";

  StorageMongoComponent(const userver::components::ComponentConfig& config,
                        const userver::components::ComponentContext& context);

  // --- Токены авторизации ---

  // Сохраняет сгенерированный токен с привязкой к логину (с учетом TTL)
  void InsertAuthToken(const std::string& token,
                       const std::string& login) const;

  // Ищет логин по токену. Возвращает nullopt, если токен не найден или
  // просрочен
  std::optional<std::string> GetLoginByToken(const std::string& token) const;

  // --- Поездки (Trips) в MongoDB ---
  // Реализация для ДЗ №4: Работа с коллекцией trips

  // Создает новую поездку (объединенную с маршрутом) и возвращает ее OID.
  std::string CreateTrip(const Route& route,
                         double distance_km,
                         std::chrono::system_clock::time_point departure_time) const;

  std::optional<MongoTrip> GetTripById(const std::string& trip_id_str) const;

  // Добавляет участника в существующую поездку (по OID)
  JoinTripResult JoinTrip(const std::string& trip_id_str,
                          const std::string& login) const;

 private:
  // Указатель на пул соединений с MongoDB.
  // Через него мы будем получать коллекции и делать запросы.
  userver::storages::mongo::PoolPtr mongo_pool_;
};

// Функция для регистрации компонента в main.cpp
void AppendStorageMongo(userver::components::ComponentList& component_list);

}  // namespace blablacar_service
