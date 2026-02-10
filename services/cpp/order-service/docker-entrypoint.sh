#!/bin/bash
set -e

echo "==> Order Service - Startup"

# If migrations directory exists and database config is set, run migrations
if [ -d "/app/migrations" ] && [ -n "$DATABASE_HOST" ]; then
  # Wait for database to be ready
  echo "==> Waiting for database..."
  until PGPASSWORD=$DATABASE_PASSWORD psql -h "$DATABASE_HOST" -U "$DATABASE_USER" -d "$DATABASE_NAME" -c '\q' 2>/dev/null; do
    echo "    Database is unavailable - sleeping for 1s"
    sleep 1
  done

  echo "==> Database is up"

  # Run Sqitch migrations
  echo "==> Running database migrations..."
  cd /app

  # Build connection string from environment variables
  DB_URL="db:pg://${DATABASE_USER}:${DATABASE_PASSWORD}@${DATABASE_HOST}:${DATABASE_PORT:-5432}/${DATABASE_NAME}"

  # Check if sqitch files exist
  if [ -f "/app/sqitch.conf" ] && [ -f "/app/sqitch.plan" ]; then
    # Deploy migrations
    if sqitch status "$DB_URL" > /dev/null 2>&1; then
      echo "    Sqitch registry found, checking for pending migrations..."
      sqitch deploy --verify "$DB_URL"
      if [ $? -eq 0 ]; then
        echo "    ✓ Migrations applied successfully"
      else
        echo "    ✗ Migration failed!"
        exit 1
      fi
    else
      echo "    Initializing Sqitch and deploying migrations..."
      sqitch deploy --verify "$DB_URL"
      if [ $? -eq 0 ]; then
        echo "    ✓ Migrations initialized successfully"
      else
        echo "    ✗ Migration failed!"
        exit 1
      fi
    fi

    # Show current migration version
    CURRENT_VERSION=$(sqitch status "$DB_URL" | grep "Nothing to deploy" || echo "Migrations applied")
    echo "==> Database schema status: $CURRENT_VERSION"
  else
    echo "    ⚠ Sqitch files not found, skipping migrations"
  fi
else
  echo "==> Skipping database migrations (no migrations directory or database config)"
fi

echo "==> Startup complete"

# If we're running the test binary, optionally start the HTTP service in the same
# container so HTTP integration tests can target localhost without cross-container DNS.
if [ "$1" = "./order-service-tests" ] || [ "$1" = "order-service-tests" ]; then
  if [ "${ORDER_HTTP_INTEGRATION:-0}" = "1" ]; then
    echo "==> Starting Order Service in background for HTTP integration tests..."
    ./order-service &
    SERVICE_PID=$!
    # Give the service a brief moment to start; the tests also include retry logic
    sleep 2
  fi

  echo "==> Running tests: $*"
  exec "$@"
fi

echo "==> Starting Order Service..."

# Execute the main command
exec "$@"
