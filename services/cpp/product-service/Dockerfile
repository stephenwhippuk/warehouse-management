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

# Copy everything including shared library
COPY . /warehouse-management/

WORKDIR /warehouse-management/services/cpp/product-service

RUN rm -rf build && mkdir -p build && cd build && \
    cmake -DWAREHOUSE_MESSAGING_DIR=/warehouse-management/services/cpp/shared/warehouse-messaging .. 2>&1 && \
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

COPY --from=builder /warehouse-management/services/cpp/product-service/build/product-service /app/
COPY --from=builder /warehouse-management/services/cpp/product-service/migrations /app/migrations
COPY services/cpp/product-service/sqitch.conf services/cpp/product-service/sqitch.plan /app/
COPY services/cpp/product-service/config/ /app/config/
COPY services/cpp/product-service/docker-entrypoint.sh /app/

RUN chmod +x /app/docker-entrypoint.sh

EXPOSE 8082

ENTRYPOINT ["/app/docker-entrypoint.sh"]
CMD ["./product-service"]
