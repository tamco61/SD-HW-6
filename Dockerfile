ARG TARGETPLATFORM=linux/amd64
FROM --platform=$TARGETPLATFORM ghcr.io/userver-framework/ubuntu-24.04-userver:latest

WORKDIR /app
COPY . .

# Build the project in release mode (with Redis and RabbitMQ support)
RUN cmake --preset release -DUSERVER_FEATURE_REDIS=ON -DUSERVER_FEATURE_RABBITMQ=ON && \
    cmake --build build-release -j $(nproc)

EXPOSE 8080

CMD ["/app/build-release/blablacar_service", "--config", "/app/configs/static_config.yaml", "--config_vars", "/app/configs/config_vars.docker.yaml"]
