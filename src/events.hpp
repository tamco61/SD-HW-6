#pragma once

#include <string>
#include <string_view>
#include <memory>

#include <userver/components/component_list.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/rabbitmq.hpp>

#include "models.hpp"

namespace blablacar_service {

class EventPublisherComponent final
    : public userver::components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "event-publisher";

  EventPublisherComponent(const userver::components::ComponentConfig& config,
                          const userver::components::ComponentContext& context);

  void PublishUserCreated(const User& user) const;
  void PublishRouteCreated(const Route& route) const;
  void PublishTripCreated(const std::string& trip_id, const Route& route) const;
  void PublishTripParticipantJoined(const std::string& trip_id,
                                    const std::string& login) const;

 private:
  void Publish(std::string_view event_type,
               std::string_view routing_key,
               userver::formats::json::ValueBuilder payload) const;

  std::shared_ptr<userver::urabbitmq::Client> rabbit_client_;
};

class EventConsumerComponent final
    : public userver::urabbitmq::ConsumerComponentBase {
 public:
  static constexpr std::string_view kName = "event-consumer";

  EventConsumerComponent(const userver::components::ComponentConfig& config,
                         const userver::components::ComponentContext& context);

 private:
  void Process(std::string message) override;
};

void AppendEventComponents(userver::components::ComponentList& component_list);

}  // namespace blablacar_service
