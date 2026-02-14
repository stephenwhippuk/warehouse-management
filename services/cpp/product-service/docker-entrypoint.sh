#!/bin/bash

echo "Product Service starting..."

# Wait for database
if [ -n "$DATABASE_URL" ] || [ -n "$DATABASE_HOST" ]; then
    DB_HOST=${DATABASE_HOST:-localhost}
    DB_PORT=${DATABASE_PORT:-5432}
    DB_NAME=${DATABASE_NAME:-warehouse_db}
    
    echo "Waiting for database at $DB_HOST:$DB_PORT..."
    while ! pg_isready -h "$DB_HOST" -p "$DB_PORT" -U warehouse; do
        echo "Database not ready, retrying..."
        sleep 2
    done
    echo "Database is ready!"
    
    # Run migrations (optional - continue even if it fails)
    if [ -d "migrations" ]; then
        echo "Running database migrations..."
        if sqitch deploy; then
            echo "Migrations completed successfully"
        else
            echo "Warning: Migrations failed, but continuing..."
        fi
    fi
fi

# Run tests if flag is set
if [ "$PRODUCT_HTTP_INTEGRATION" = "1" ]; then
    echo "Running HTTP integration tests..."
    ./product-service-tests
    exit $?
fi

# Start the service
echo "Starting HTTP server on 0.0.0.0:8082..."
exec "$@"
