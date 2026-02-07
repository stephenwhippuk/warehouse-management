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
    nlohmann-json3-dev \
    libspdlog-dev \
    sqitch \
    libdbd-pg-perl \
    postgresql-client \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Build application
RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    make -j$(nproc)

# Runtime stage
FROM ubuntu:22.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libboost-system1.74.0 \
    libboost-thread1.74.0 \
    libpoco-net80 \
    libpoco-util80 \
    libpoco-foundation80 \
    libpq5 \
    libpqxx-6.4 \
    sqitch \
    libdbd-pg-perl \
    postgresql-client \
    && rm -rf /var/lib/apt/lists/*

# Create app user
RUN useradd -m -u 1000 appuser

# Set working directory
WORKDIR /app

COPY --from=builder /app/migrations /app/migrations
COPY --from=builder /app/sqitch.conf /app/
COPY --from=builder /app/sqitch.plan /app/

# Copy entrypoint script
COPY docker-entrypoint.sh /app/
RUN chmod +x /app/docker-entrypoint.sh
# Copy binary and config from builder
COPY --from=builder /app/build/bin/inventory-service /app/
COPY --from=builder /app/config /app/config

# Change ownership
RUN chown -R appuser:appuser /app

# Switch to app user10s --retries=3 \
    CMD curl -f http://localhost:8080/health || exit 1

# Run application with entrypoint
ENTRYPOINT ["/app/docker-entrypoint.sh"]
EXPOSE 8080

# Health check
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:8080/health || exit 1

# Run application
CMD ["./inventory-service", "config/application.json"]
