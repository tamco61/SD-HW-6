#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/clients/http/middlewares/pipeline_component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/storages/mongo/component.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/redis/component.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include "auth_middleware.hpp"
#include "handlers/api_handlers.hpp"
#include "rate_limiter.hpp"
#include "redis_cache.hpp"
#include "storage_mongo.hpp"
#include "storage_pg.hpp"

int main(int argc, char* argv[]) {
  auto component_list =
      userver::components::MinimalServerComponentList()
          .Append<userver::server::handlers::Ping>()
          .Append<userver::components::TestsuiteSupport>()
          .Append<userver::components::HttpClientCore>()
          .Append<userver::components::HttpClient>()
          .Append<userver::clients::http::MiddlewarePipelineComponent>()
          .Append<userver::server::handlers::TestsControl>()
          .Append<userver::clients::dns::Component>()
          .Append<userver::components::Postgres>("postgres-db")
          .Append<userver::components::Mongo>("mongo-blabla")
          .Append<userver::components::Secdist>()
          .Append<userver::components::DefaultSecdistProvider>()
          .Append<userver::components::Redis>("redis-blabla");

  blablacar_service::AppendStoragePg(component_list);
  blablacar_service::AppendStorageMongo(component_list);
  blablacar_service::AppendRedisCache(component_list);
  blablacar_service::AppendRateLimiter(component_list);
  blablacar_service::AppendApiHandlers(component_list);
  blablacar_service::AppendAuthMiddleware(component_list);

  return userver::utils::DaemonMain(argc, argv, component_list);
}

