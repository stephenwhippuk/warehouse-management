# Build stage
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libboost-all-dev \
    libpoco-dev \
    libpq-dev \
    libpqxx-dev \
    librabbitmq-dev \
    nlohmann-json3-dev \
    libspdlog-dev \
    sqitch \
    libdbd-pg-perl \
    postgresql-client \
    && rm -rf /var/lib/apt/lists/*

# Copy entire repository
COPY . /warehouse-management/

# Set working directory
WORKDIR /warehouse-management/services/cpp/inventory-service

# Build application (clean build directory inside image)
RUN rm -rf build && mkdir build && cd build && \
    cmake -DWAREHOUSE_MESSAGING_DIR=/warehouse-management/services/cpp/shared/warehouse-messaging -DCMAKE_BUILD_TYPE=Release .. && \
    make -j"$(nproc)"

# Runtime stage
FROM ubuntu:22.04

# Install runtime dependencies
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    libboost-system1.74.0 \
    libboost-thread1.74.0 \
    libpoco-dev \
    libpq5 \
    libpqxx-dev \
    librabbitmq4 \
    sqitch \
    libdbd-pg-perl \
    postgresql-client \
    libspdlog1 \
    tzdata \
    && rm -rf /var/lib/apt/lists/*

# Create app user
RUN useradd -m -u 1000 appuser

# Set working directory
WORKDIR /app

COPY --from=builder /warehouse-management/services/cpp/inventory-service/migrations /app/migrations
COPY --from=builder /warehouse-management/services/cpp/inventory-service/sqitch.conf /app/
COPY --from=builder /warehouse-management/services/cpp/inventory-service/sqitch.plan /app/
COPY --from=builder /warehouse-management/services/cpp/inventory-service/seed /app/seed

# Copy entrypoint script
COPY services/cpp/inventory-service/docker-entrypoint.sh /app/
RUN chmod +x /app/docker-entrypoint.sh

# Copy binary, config, and contracts from builder
COPY --from=builder /warehouse-management/services/cpp/inventory-service/build/bin/inventory-service /app/
COPY --from=builder /warehouse-management/services/cpp/inventory-service/config /app/config
COPY --from=builder /warehouse-management/services/cpp/inventory-service/contracts /app/contracts

# Change ownership
RUN chown -R appuser:appuser /app

# Switch to app user
USER appuser

# Expose port
EXPOSE 8080

# Health check
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:8080/health || exit 1

# Run application with entrypoint
ENTRYPOINT ["/app/docker-entrypoint.sh"]
CMD ["./inventory-service", "config/application.json"]

