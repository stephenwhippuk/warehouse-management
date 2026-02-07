#!/bin/bash
set -e

echo "==> Warehouse Service - Startup"

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

echo "==> Starting Warehouse Service..."

# Execute the main command
exec "$@"
