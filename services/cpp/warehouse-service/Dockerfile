# Build stage
FROM ubuntu:22.04 AS builder

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libboost-all-dev \
    libpoco-dev \
    libpq-dev \
    libssl-dev \
    nlohmann-json3-dev \
    libspdlog-dev \
    librabbitmq-dev \
    catch2 \
    sqitch \
    libdbd-pg-perl \
    postgresql-client \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /build

# Copy shared libraries first
COPY shared ./shared

# Copy service code
COPY warehouse-service ./warehouse-service
WORKDIR /build/warehouse-service

# Build the application
RUN mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF && \
    cmake --build . --parallel $(nproc)

# Runtime stage
FROM ubuntu:22.04

# Install runtime dependencies 
# Using available package versions from Ubuntu 22.04
RUN apt-get update && apt-get install -y \
    libboost-system1.74.0 \
    libboost-thread1.74.0 \
    libpq5 \
    libspdlog1 \
    postgresql-client \
    && rm -rf /var/lib/apt/lists/*

# Create app user
RUN useradd -m -u 1000 warehouse && \
    mkdir -p /app/logs /app/config && \
    chown -R warehouse:warehouse /app

WORKDIR /app

# Copy binary and configuration from builder
COPY --from=builder /build/build/bin/warehouse-service /app/
COPY --from=builder /build/config /app/config/
COPY --from=builder /build/migrations /app/migrations/
COPY --from=builder /build/sqitch.conf /app/
COPY --from=builder /build/sqitch.plan /app/

# Copy contracts for schema validation
COPY --from=builder /build/../../../contracts /app/contracts/

# Switch to non-root user
USER warehouse

# Copy entrypoint script (must be after USER command for proper permissions)
COPY --chown=warehouse:warehouse warehouse-service/docker-entrypoint.sh /app/
RUN chmod +x /app/docker-entrypoint.sh

# Expose port
EXPOSE 8080

# Health check
HEALTHCHECK --interval=30s --timeout=3s --start-period=10s --retries=3 \
    CMD curl -f http://localhost:8080/health || exit 1

# Run application with entrypoint
ENTRYPOINT ["/app/docker-entrypoint.sh"]
CMD ["./warehouse-service", "config/application.json"]
