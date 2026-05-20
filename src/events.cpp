#include "events.hpp"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include <userver/components/component_context.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/uuid4.hpp>
#include <userver/testsuite/testpoint.hpp>

namespace blablacar_service {

namespace {

constexpr std::string_view kRabbitComponentName = "rabbitmq-blabla";
constexpr std::string_view kExchangeName = "blablacar.events";
constexpr std::string_view kAuditQueueName = "blablacar.audit.events";
constexpr std::string_view kAuditBinding = "#";
constexpr std::string_view kProducerName = "blablacar-api";
constexpr std::chrono::seconds kRabbitDeadline{3};

userver::engine::Deadline RabbitDeadline() {
  return userver::engine::Deadline::FromDuration(kRabbitDeadline);
}

std::string ToIso8601Utc(std::chrono::system_clock::time_point time_point) {
  const auto time = std::chrono::system_clock::to_time_t(time_point);
  std::tm utc_time{};
#if defined(_WIN32)
  gmtime_s(&utc_time, &time);
#else
  gmtime_r(&time, &utc_time);
#endif
  std::ostringstream out;
  out << std::put_time(&utc_time, "%Y-%m-%dT%H:%M:%SZ");
  return out.str();
}

void DeclareEventTopology(
    const std::shared_ptr<userver::urabbitmq::Client>& rabbit_client) {
  const userver::urabbitmq::Exchange exchange{std::string{kExchangeName}};
  const userver::urabbitmq::Queue queue{std::string{kAuditQueueName}};

  rabbit_client->DeclareExchange(
      exchange,
      userver::urabbitmq::Exchange::Type::kTopic,
      userver::utils::Flags<userver::urabbitmq::Exchange::Flags>{
          userver::urabbitmq::Exchange::Flags::kDurable},
      RabbitDeadline());
  rabbit_client->DeclareQueue(
      queue,
      userver::utils::Flags<userver::urabbitmq::Queue::Flags>{
          userver::urabbitmq::Queue::Flags::kDurable},
      RabbitDeadline());
  rabbit_client->BindQueue(exchange, queue, std::string{kAuditBinding},
                           RabbitDeadline());
}

}  // namespace

EventPublisherComponent::EventPublisherComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : LoggableComponentBase(config, context),
      rabbit_client_(
          context.FindComponent<userver::components::RabbitMQ>(
                     std::string{kRabbitComponentName})
              .GetClient()) {
  DeclareEventTopology(rabbit_client_);
  LOG_INFO() << "RabbitMQ event topology is ready: exchange=" << kExchangeName
             << ", audit_queue=" << kAuditQueueName;
}

void EventPublisherComponent::PublishUserCreated(const User& user) const {
  userver::formats::json::ValueBuilder payload;
  payload["login"] = user.login;
  payload["first_name"] = user.first_name;
  payload["last_name"] = user.last_name;
  Publish("UserCreated", "user.created", std::move(payload));
}

void EventPublisherComponent::PublishRouteCreated(const Route& route) const {
  userver::formats::json::ValueBuilder payload;
  payload["route_id"] = route.id;
  payload["owner_login"] = route.owner_login;
  payload["points"] = route.points;
  Publish("RouteCreated", "route.created", std::move(payload));
}

void EventPublisherComponent::PublishTripCreated(const std::string& trip_id,
                                                 const Route& route) const {
  userver::formats::json::ValueBuilder payload;
  payload["trip_id"] = trip_id;
  payload["route_id"] = route.id;
  payload["owner_login"] = route.owner_login;
  payload["status"] = "active";
  Publish("TripCreated", "trip.created", std::move(payload));
}

void EventPublisherComponent::PublishTripParticipantJoined(
    const std::string& trip_id,
    const std::string& login) const {
  userver::formats::json::ValueBuilder payload;
  payload["trip_id"] = trip_id;
  payload["login"] = login;
  Publish("TripParticipantJoined", "trip.participant_joined",
          std::move(payload));
}

void EventPublisherComponent::Publish(
    std::string_view event_type,
    std::string_view routing_key,
    userver::formats::json::ValueBuilder payload) const {
  try {
    userver::formats::json::ValueBuilder envelope;
    envelope["event_id"] = userver::utils::generators::GenerateUuid();
    envelope["event_type"] = std::string{event_type};
    envelope["schema_version"] = 1;
    envelope["occurred_at"] =
        ToIso8601Utc(std::chrono::system_clock::now());
    envelope["producer"] = std::string{kProducerName};
    envelope["correlation_id"] = userver::utils::generators::GenerateUuid();
    envelope["payload"] = payload.ExtractValue();

    const auto message =
        userver::formats::json::ToString(envelope.ExtractValue());
    rabbit_client_->PublishReliable(
        userver::urabbitmq::Exchange{std::string{kExchangeName}},
        std::string{routing_key}, message,
        userver::urabbitmq::MessageType::kPersistent, RabbitDeadline());

    LOG_INFO() << "Published event type=" << event_type
               << " routing_key=" << routing_key;
  } catch (const std::exception& e) {
    LOG_ERROR() << "Failed to publish event type=" << event_type
                << " routing_key=" << routing_key << ": " << e.what();
  }
}

EventConsumerComponent::EventConsumerComponent(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& context)
    : ConsumerComponentBase(config, context) {}

void EventConsumerComponent::Process(std::string message) {
  try {
    const auto json = userver::formats::json::FromString(message);
    TESTPOINT("event-consumed", json);
    LOG_INFO() << "Consumed event type="
               << json["event_type"].As<std::string>()
               << " event_id=" << json["event_id"].As<std::string>()
               << " payload=" << userver::formats::json::ToString(json["payload"]);
  } catch (const std::exception& e) {
    LOG_ERROR() << "Malformed event message was acknowledged and skipped: "
                << e.what() << "; message=" << message;
  }
}

void AppendEventComponents(userver::components::ComponentList& component_list) {
  component_list.Append<EventPublisherComponent>();
  component_list.Append<EventConsumerComponent>();
}

}  // namespace blablacar_service
