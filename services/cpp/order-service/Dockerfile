# Build stage
FROM ubuntu:22.04 AS builder

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libboost-all-dev \
    libpoco-dev \
    libpq-dev \
    libssl-dev \
    nlohmann-json3-dev \
    libspdlog-dev \
    sqitch \
    libdbd-pg-perl \
    postgresql-client \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /build

# Copy source code
COPY . .

# Ensure directories exist for optional files
RUN mkdir -p migrations/deploy migrations/revert migrations/verify

# Build the application
RUN mkdir -p build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    cmake --build . --parallel $(nproc)

# Runtime stage
FROM ubuntu:22.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libboost-system1.74.0 \
    libboost-thread1.74.0 \
    libpoco-net1.11 \
    libpoco-netssl1.11 \
    libpoco-util1.11 \
    libpoco-foundation1.11 \
    libpq5 \
    libspdlog1 \
    sqitch \
    libdbd-pg-perl \
    postgresql-client \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Create app user
RUN useradd -m -u 1000 order && \
    mkdir -p /app/logs /app/config && \
    chown -R order:order /app

WORKDIR /app

# Copy binary and configuration from builder
COPY --from=builder /build/build/order-service /app/
COPY --from=builder /build/config /app/config/
COPY --from=builder /build/migrations /app/migrations/
COPY --from=builder /build/sqitch.conf /app/
COPY --from=builder /build/sqitch.plan /app/
COPY --from=builder /build/claims.json /app/

# Copy contracts for schema validation
COPY --from=builder /build/../../../contracts /app/contracts/

# Copy entrypoint script
COPY docker-entrypoint.sh /app/
RUN chmod +x /app/docker-entrypoint.sh

# Switch to non-root user
USER order

# Expose port
EXPOSE 8082

# Health check
HEALTHCHECK --interval=30s --timeout=3s --start-period=10s --retries=3 \
    CMD curl -f http://localhost:8082/health || exit 1

# Run application with entrypoint
ENTRYPOINT ["/app/docker-entrypoint.sh"]
CMD ["./order-service"]
