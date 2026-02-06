# Database Migrations for C++ Services

## Overview

This guide provides strategies for managing database schema migrations in C++ microservices, enabling code-first agile development similar to Entity Framework in C#.

## Challenge

Unlike C# with Entity Framework's automatic migration generation, C++ requires explicit migration tooling. We need:
- Version-controlled schema changes
- Automatic migration application
- Rollback capabilities
- Code-first approach where possible
- Integration with CI/CD pipelines

## Recommended Solutions

### Solution 1: Sqitch (Recommended for C++ Services)

**Overview:** Sqitch is a database-native change management system that works excellently with C++.

**Why Sqitch:**
- Language-agnostic (perfect for C++)
- Git-like workflow for database changes
- Supports PostgreSQL, MySQL, SQLite, Oracle
- Dependency tracking between changes
- Automatic rollback support
- VCS integration (Git)

**Installation:**
```bash
# Ubuntu/Debian
apt-get install sqitch libdbd-pg-perl

# MacOS
brew install sqitch

# Docker
docker pull sqitch/sqitch
```

**Project Structure:**
```
inventory-service/
├── src/
├── include/
├── migrations/
│   ├── deploy/
│   │   ├── 001_initial_schema.sql
│   │   ├── 002_add_location_index.sql
│   │   └── 003_add_stock_alerts.sql
│   ├── revert/
│   │   ├── 001_initial_schema.sql
│   │   ├── 002_add_location_index.sql
│   │   └── 003_add_stock_alerts.sql
│   ├── verify/
│   │   ├── 001_initial_schema.sql
│   │   ├── 002_add_location_index.sql
│   │   └── 003_add_stock_alerts.sql
│   └── sqitch.plan
├── CMakeLists.txt
└── README.md
```

**Initialize Sqitch:**
```bash
cd services/cpp/inventory-service

# Initialize Sqitch project
sqitch init inventory_service \
  --engine pg \
  --target db:pg://inventory_user@localhost/inventory_db

# Configure target
sqitch target add production \
  db:pg://inventory_user@prod-db/inventory_db
```

**Creating Migrations:**
```bash
# Add a new migration
sqitch add 001_initial_schema -n "Create initial inventory tables"

# This creates three files:
# - migrations/deploy/001_initial_schema.sql
# - migrations/revert/001_initial_schema.sql
# - migrations/verify/001_initial_schema.sql
```

**Example Migration Files:**

`migrations/deploy/001_initial_schema.sql`:
```sql
-- Deploy inventory_service:001_initial_schema to pg

BEGIN;

CREATE TABLE IF NOT EXISTS items (
    id BIGSERIAL PRIMARY KEY,
    sku VARCHAR(100) NOT NULL UNIQUE,
    name VARCHAR(255) NOT NULL,
    description TEXT,
    quantity INTEGER NOT NULL DEFAULT 0,
    location VARCHAR(50),
    min_quantity INTEGER DEFAULT 0,
    max_quantity INTEGER,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_items_sku ON items(sku);
CREATE INDEX idx_items_location ON items(location);

-- Trigger for updated_at
CREATE OR REPLACE FUNCTION update_updated_at_column()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = NOW();
    RETURN NEW;
END;
$$ language 'plpgsql';

CREATE TRIGGER update_items_updated_at 
    BEFORE UPDATE ON items 
    FOR EACH ROW 
    EXECUTE FUNCTION update_updated_at_column();

COMMIT;
```

`migrations/revert/001_initial_schema.sql`:
```sql
-- Revert inventory_service:001_initial_schema from pg

BEGIN;

DROP TRIGGER IF EXISTS update_items_updated_at ON items;
DROP FUNCTION IF EXISTS update_updated_at_column();
DROP INDEX IF EXISTS idx_items_location;
DROP INDEX IF EXISTS idx_items_sku;
DROP TABLE IF EXISTS items;

COMMIT;
```

`migrations/verify/001_initial_schema.sql`:
```sql
-- Verify inventory_service:001_initial_schema on pg

BEGIN;

SELECT id, sku, name, quantity, location, created_at, updated_at
FROM items
WHERE FALSE;

ROLLBACK;
```

**Applying Migrations:**
```bash
# Deploy all pending migrations
sqitch deploy

# Deploy to specific target
sqitch deploy production

# Deploy specific migration
sqitch deploy 001_initial_schema

# Check status
sqitch status

# Rollback last change
sqitch revert

# Rollback to specific version
sqitch revert 001_initial_schema
```

**Integration with C++ Service:**

Create a migration runner in your C++ service:

`src/database/migration_runner.hpp`:
```cpp
#ifndef MIGRATION_RUNNER_HPP
#define MIGRATION_RUNNER_HPP

#include <string>
#include <memory>
#include <pqxx/pqxx>

namespace warehouse {
namespace database {

class MigrationRunner {
public:
    explicit MigrationRunner(const std::string& connection_string);
    
    // Check if migrations are needed
    bool needsMigration();
    
    // Apply pending migrations
    bool applyMigrations();
    
    // Get current schema version
    std::string getCurrentVersion();
    
private:
    std::string connection_string_;
    std::unique_ptr<pqxx::connection> conn_;
    
    void ensureMigrationTable();
    int runSqitch(const std::vector<std::string>& args);
};

} // namespace database
} // namespace warehouse

#endif // MIGRATION_RUNNER_HPP
```

`src/database/migration_runner.cpp`:
```cpp
#include "migration_runner.hpp"
#include <spdlog/spdlog.h>
#include <cstdlib>
#include <array>

namespace warehouse {
namespace database {

MigrationRunner::MigrationRunner(const std::string& connection_string)
    : connection_string_(connection_string) {
    try {
        conn_ = std::make_unique<pqxx::connection>(connection_string_);
    } catch (const std::exception& e) {
        spdlog::error("Failed to connect to database: {}", e.what());
        throw;
    }
}

bool MigrationRunner::needsMigration() {
    try {
        pqxx::work txn(*conn_);
        
        // Check if sqitch table exists
        auto result = txn.exec(
            "SELECT EXISTS ("
            "  SELECT FROM information_schema.tables "
            "  WHERE table_schema = 'sqitch' "
            "  AND table_name = 'changes'"
            ")"
        );
        
        if (!result[0][0].as<bool>()) {
            return true; // No migrations applied yet
        }
        
        // Check for pending migrations by running sqitch status
        int status = runSqitch({"status", "--quiet"});
        return status != 0; // Non-zero means pending migrations
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to check migration status: {}", e.what());
        return false;
    }
}

bool MigrationRunner::applyMigrations() {
    spdlog::info("Applying database migrations...");
    
    int result = runSqitch({"deploy", "--verify"});
    
    if (result == 0) {
        spdlog::info("Migrations applied successfully");
        return true;
    } else {
        spdlog::error("Failed to apply migrations");
        return false;
    }
}

std::string MigrationRunner::getCurrentVersion() {
    try {
        pqxx::work txn(*conn_);
        
        auto result = txn.exec(
            "SELECT change_id, change "
            "FROM sqitch.changes "
            "ORDER BY committed_at DESC "
            "LIMIT 1"
        );
        
        if (result.size() > 0) {
            return result[0]["change"].as<std::string>();
        }
        
        return "none";
        
    } catch (const std::exception& e) {
        spdlog::warn("Failed to get current version: {}", e.what());
        return "unknown";
    }
}

int MigrationRunner::runSqitch(const std::vector<std::string>& args) {
    // Build command safely - args are internal, not user-provided
    std::string command = "sqitch";
    for (const auto& arg : args) {
        command += " " + arg;
    }
    
    spdlog::debug("Running: {}", command);
    
    // Note: In production, consider using fork()/exec() for better security
    // This is safe here as args are hardcoded within the application
    return std::system(command.c_str());
}

} // namespace database
} // namespace warehouse
```

**Usage in main.cpp:**
```cpp
#include "database/migration_runner.hpp"
#include <spdlog/spdlog.h>

int main(int argc, char* argv[]) {
    // Load configuration
    const char* db_url_env = std::getenv("DATABASE_URL");
    std::string db_url = db_url_env 
        ? db_url_env
        : "postgresql://inventory_user:password@localhost/inventory_db";
    
    // Run migrations on startup
    try {
        warehouse::database::MigrationRunner migrator(db_url);
        
        auto version = migrator.getCurrentVersion();
        spdlog::info("Current schema version: {}", version);
        
        if (migrator.needsMigration()) {
            spdlog::info("Pending migrations detected");
            
            if (!migrator.applyMigrations()) {
                spdlog::error("Failed to apply migrations, exiting");
                return 1;
            }
        } else {
            spdlog::info("Database schema is up to date");
        }
        
    } catch (const std::exception& e) {
        spdlog::error("Migration error: {}", e.what());
        return 1;
    }
    
    // Continue with service startup...
    return 0;
}
```

### Solution 2: Flyway (SQL-Based Migrations)

**Overview:** Flyway is a popular migration tool with good C++ integration.

**Installation:**
```bash
# Download Flyway CLI
wget https://repo1.maven.org/maven2/org/flywaydb/flyway-commandline/9.22.3/flyway-commandline-9.22.3-linux-x64.tar.gz
tar -xzf flyway-commandline-9.22.3-linux-x64.tar.gz
```

**Project Structure:**
```
inventory-service/
├── src/
├── include/
├── db/
│   └── migration/
│       ├── V1__Initial_schema.sql
│       ├── V2__Add_location_index.sql
│       └── V3__Add_stock_alerts.sql
└── flyway.conf
```

**Configuration (flyway.conf):**
```properties
flyway.url=jdbc:postgresql://localhost:5432/inventory_db
flyway.user=inventory_user
flyway.password=inventory_dev
flyway.locations=filesystem:db/migration
flyway.baselineOnMigrate=true
```

**Example Migration:**

`db/migration/V1__Initial_schema.sql`:
```sql
CREATE TABLE items (
    id BIGSERIAL PRIMARY KEY,
    sku VARCHAR(100) NOT NULL UNIQUE,
    name VARCHAR(255) NOT NULL,
    quantity INTEGER NOT NULL DEFAULT 0,
    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_items_sku ON items(sku);
```

**Applying Migrations:**
```bash
# Migrate
./flyway migrate

# Check status
./flyway info

# Repair (fix checksum mismatches)
./flyway repair

# Clean (drop all objects - use with caution)
./flyway clean
```

**C++ Integration:**
```cpp
#include <cstdlib>
#include <spdlog/spdlog.h>

bool runFlywayMigrations() {
    spdlog::info("Running Flyway migrations...");
    
    int result = std::system("flyway migrate");
    
    if (result == 0) {
        spdlog::info("Migrations completed successfully");
        return true;
    } else {
        spdlog::error("Migration failed with code: {}", result);
        return false;
    }
}
```

### Solution 3: Code-First with C++ ORM (sqlpp11 + Generator)

**Overview:** Generate SQL migrations from C++ model definitions.

**Installation:**
```bash
# sqlpp11
git clone https://github.com/rbock/sqlpp11.git
cd sqlpp11
mkdir build && cd build
cmake .. && make install
```

**Define Models in C++:**

`models/item.hpp`:
```cpp
#include <sqlpp11/table.h>
#include <sqlpp11/data_types.h>

namespace warehouse {
namespace models {

struct Items {
    struct Id {
        struct _alias_t {
            static constexpr const char _literal[] = "id";
            using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
            template<typename T>
            struct _member_t {
                T id;
                T& operator()() { return id; }
                const T& operator()() const { return id; }
            };
        };
        using _traits = sqlpp::make_traits<
            sqlpp::bigint, 
            sqlpp::tag::must_not_insert, 
            sqlpp::tag::must_not_update
        >;
    };
    
    struct Sku {
        struct _alias_t {
            static constexpr const char _literal[] = "sku";
            using _name_t = sqlpp::make_char_sequence<sizeof(_literal), _literal>;
            template<typename T>
            struct _member_t {
                T sku;
                T& operator()() { return sku; }
                const T& operator()() const { return sku; }
            };
        };
        using _traits = sqlpp::make_traits<sqlpp::varchar>;
    };
    
    // ... other fields
    
    using _alias_t = sqlpp::table_alias_t<Items>;
};

using items = sqlpp::table_t<Items>;

} // namespace models
} // namespace warehouse
```

**Generate Migration Script:**

Create a code generator tool that reads model definitions and generates SQL:

`tools/generate_migration.cpp`:
```cpp
#include "models/item.hpp"
#include <fstream>
#include <iostream>

int main() {
    std::ofstream migration("db/migration/V1__Generated_schema.sql");
    
    // Generate CREATE TABLE based on model definition
    migration << "CREATE TABLE items (\n";
    migration << "  id BIGSERIAL PRIMARY KEY,\n";
    migration << "  sku VARCHAR(100) NOT NULL UNIQUE,\n";
    migration << "  name VARCHAR(255) NOT NULL,\n";
    migration << "  quantity INTEGER NOT NULL DEFAULT 0\n";
    migration << ");\n";
    
    migration.close();
    
    std::cout << "Migration generated successfully\n";
    return 0;
}
```

### Solution 4: Custom Migration Framework

For full code-first control, create a custom migration framework:

`include/migration_framework.hpp`:
```cpp
#ifndef MIGRATION_FRAMEWORK_HPP
#define MIGRATION_FRAMEWORK_HPP

#include <string>
#include <vector>
#include <functional>
#include <pqxx/pqxx>

namespace warehouse {
namespace migrations {

class Migration {
public:
    virtual ~Migration() = default;
    virtual std::string name() const = 0;
    virtual std::string version() const = 0;
    virtual void up(pqxx::work& txn) = 0;
    virtual void down(pqxx::work& txn) = 0;
};

class MigrationRegistry {
public:
    static MigrationRegistry& instance();
    
    void registerMigration(std::unique_ptr<Migration> migration);
    void runMigrations(pqxx::connection& conn);
    void rollback(pqxx::connection& conn, const std::string& version);
    
private:
    std::vector<std::unique_ptr<Migration>> migrations_;
    
    void ensureVersionTable(pqxx::connection& conn);
    std::string getCurrentVersion(pqxx::connection& conn);
    void recordMigration(pqxx::work& txn, const std::string& version);
};

// Helper macro for defining migrations
#define DEFINE_MIGRATION(ClassName, Version, Name) \
    class ClassName : public warehouse::migrations::Migration { \
    public: \
        std::string name() const override { return Name; } \
        std::string version() const override { return Version; } \
        void up(pqxx::work& txn) override; \
        void down(pqxx::work& txn) override; \
    }; \
    static struct ClassName##Registrar { \
        ClassName##Registrar() { \
            warehouse::migrations::MigrationRegistry::instance() \
                .registerMigration(std::make_unique<ClassName>()); \
        } \
    } ClassName##_registrar;

} // namespace migrations
} // namespace warehouse

#endif
```

**Usage:**

`migrations/001_initial_schema.cpp`:
```cpp
#include "migration_framework.hpp"

DEFINE_MIGRATION(InitialSchema, "001", "Initial schema for inventory")

void InitialSchema::up(pqxx::work& txn) {
    txn.exec(R"(
        CREATE TABLE items (
            id BIGSERIAL PRIMARY KEY,
            sku VARCHAR(100) NOT NULL UNIQUE,
            name VARCHAR(255) NOT NULL,
            quantity INTEGER NOT NULL DEFAULT 0,
            created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
        )
    )");
    
    txn.exec("CREATE INDEX idx_items_sku ON items(sku)");
}

void InitialSchema::down(pqxx::work& txn) {
    txn.exec("DROP INDEX IF EXISTS idx_items_sku");
    txn.exec("DROP TABLE IF EXISTS items");
}
```

## CI/CD Integration

### Dockerfile with Migrations

```dockerfile
FROM ubuntu:22.04 AS builder

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libpqxx-dev \
    sqitch \
    libdbd-pg-perl

# Copy source
WORKDIR /app
COPY . .

# Build service
RUN mkdir build && cd build && \
    cmake .. && make -j$(nproc)

# Runtime stage
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libpqxx-6.4 \
    sqitch \
    libdbd-pg-perl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy binary and migrations
COPY --from=builder /app/build/inventory-service .
COPY --from=builder /app/migrations ./migrations
COPY --from=builder /app/sqitch.conf .

# Entry point script
COPY docker-entrypoint.sh .
RUN chmod +x docker-entrypoint.sh

ENTRYPOINT ["./docker-entrypoint.sh"]
CMD ["./inventory-service"]
```

`docker-entrypoint.sh`:
```bash
#!/bin/bash
set -e

echo "Waiting for database..."
until PGPASSWORD=$DB_PASSWORD psql -h "$DB_HOST" -U "$DB_USER" -d "$DB_NAME" -c '\q'; do
  echo "Database is unavailable - sleeping"
  sleep 1
done

echo "Database is up"

# Run migrations
echo "Running migrations..."
sqitch deploy db:pg://$DB_USER:$DB_PASSWORD@$DB_HOST/$DB_NAME

echo "Migrations complete"

# Execute the main command
exec "$@"
```

### GitHub Actions Workflow

`.github/workflows/migrate-and-deploy.yml`:
```yaml
name: Migrate and Deploy

on:
  push:
    branches: [main]

jobs:
  migrate:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Install Sqitch
        run: |
          sudo apt-get update
          sudo apt-get install -y sqitch libdbd-pg-perl
      
      - name: Run migrations
        env:
          DATABASE_URL: ${{ secrets.DATABASE_URL }}
        run: |
          cd services/cpp/inventory-service
          sqitch deploy $DATABASE_URL
      
      - name: Verify migrations
        env:
          DATABASE_URL: ${{ secrets.DATABASE_URL }}
        run: |
          cd services/cpp/inventory-service
          sqitch verify $DATABASE_URL
```

## Best Practices

### 1. Version Naming Convention
```
Format: VYYYYMMDDHHmmss__Description.sql
Example: V20260206143000__Add_stock_alerts.sql

Sqitch format:
001_description
002_another_change
```

### 2. Always Include Rollback
Every migration must have a corresponding revert/down script.

### 3. Idempotent Migrations
```sql
-- Good
CREATE TABLE IF NOT EXISTS items (...);
CREATE INDEX IF NOT EXISTS idx_items_sku ON items(sku);

-- Bad
CREATE TABLE items (...);  -- Fails if already exists
```

### 4. Test Migrations
```bash
# Test forward
sqitch deploy --verify

# Test rollback
sqitch revert

# Test forward again
sqitch deploy --verify
```

### 5. Separate Schema from Data
```
V001__schema_items.sql       # Structure only
V002__data_initial_items.sql # Initial data
```

### 6. Never Modify Applied Migrations
Create a new migration instead of modifying an existing one.

## Comparison Matrix

| Feature | Sqitch | Flyway | Custom | sqlpp11 |
|---------|--------|--------|--------|---------|
| SQL-based | ✅ | ✅ | ✅/❌ | ❌ |
| Code-first | ❌ | ❌ | ✅ | ✅ |
| Version control | ✅ | ✅ | ✅ | ❌ |
| Rollback | ✅ | ❌* | ✅ | ❌ |
| Dependencies | ✅ | ❌ | ✅ | ❌ |
| Learning curve | Medium | Low | High | High |
| C++ integration | Good | Good | Excellent | Excellent |

*Flyway undo requires paid version

## Recommendation for Warehouse Management System

**For C++ Services: Use Sqitch**
- Best balance of features and simplicity
- Excellent rollback support
- Dependency management
- Git-like workflow familiar to developers
- Free and open source

**Development Workflow:**
1. Developer creates C++ models/entities
2. Developer creates Sqitch migration (deploy/revert/verify)
3. Test locally: `sqitch deploy && sqitch verify`
4. Commit migration files with code
5. CI runs migrations on staging
6. Production deployment applies migrations automatically

This provides the agile, code-first development experience similar to Entity Framework while maintaining the performance benefits of C++.
