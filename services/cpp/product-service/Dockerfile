FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    libboost-system-dev \
    libboost-thread-dev \
    libpoco-dev \
    libpq-dev \
    libpqxx-dev \
    nlohmann-json3-dev \
    libspdlog-dev \
    uuid-dev \
    librabbitmq-dev \
    git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN rm -rf build && mkdir -p build && cd build && \
    cmake .. 2>&1 && \
    make -j$(nproc) product-service 2>&1 | tail -100

FROM ubuntu:22.04

RUN apt-get update && apt-get install -y --no-install-recommends \
    libboost-system1.74.0 \
    libboost-thread1.74.0 \
    libpoco-dev \
    libpq5 \
    libpqxx-dev \
    libspdlog1 \
    uuid-runtime \
    librabbitmq4 \
    postgresql-client \
    sqitch \
    libdbd-pg-perl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/build/product-service /app/
COPY --from=builder /app/migrations /app/migrations
COPY sqitch.conf sqitch.plan /app/
COPY config/ /app/config/
COPY docker-entrypoint.sh /app/

RUN chmod +x /app/docker-entrypoint.sh

EXPOSE 8082

ENTRYPOINT ["/app/docker-entrypoint.sh"]
CMD ["./product-service"]
