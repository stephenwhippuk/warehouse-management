# Configuration Standard - Warehouse Management System

## Overview

This document defines the unified configuration standard for all C++ services in the warehouse management system. Following the 12-factor app methodology, we use environment variables as the primary configuration source.

## Configuration Philosophy

1. **DATABASE_URL First**: Use PostgreSQL connection string format as primary config
2. **Individual Variables as Fallback**: Support granular env vars for flexibility
3. **Smart Defaults**: Provide sensible defaults for development
4. **Cloud-Ready**: Compatible with Heroku, AWS RDS, Azure, and other PaaS providers
5. **Docker-Friendly**: Easy to configure in docker-compose.yml

## Database Configuration

### Primary Method: DATABASE_URL (RECOMMENDED)

**Format**: `postgresql://user:password@host:port/database`

**Examples**:
```bash
# Development
export DATABASE_URL="postgresql://warehouse:warehouse_dev@localhost:5432/warehouse_db"

# Production
export DATABASE_URL="postgresql://prod_user:secure_pass@db.example.com:5432/warehouse_prod"

# Cloud (Heroku, AWS RDS)
export DATABASE_URL="postgresql://amazon_user:pass@rds.amazonaws.com:5432/production_db"
```

**Benefits**:
- ✅ Single environment variable
- ✅ Industry standard format
- ✅ Works with psql, pg_dump, and other PostgreSQL tools
- ✅ Cloud provider native format
- ✅ Less error-prone (no mixing host/port/user)

### Fallback Method: Individual Variables

If `DATABASE_URL` is not set, services will use individual variables:

```bash
export DATABASE_HOST="localhost"      # Database host
export DATABASE_PORT="5432"          # Database port
export DATABASE_NAME="warehouse_db"  # Database name
export DATABASE_USER="warehouse"     # Database user
export DATABASE_PASSWORD="warehouse" # Database password
```

**Use Cases**:
- Legacy systems with existing config
- Environments where connection string is not available
- Debugging specific connection parameters
- Kubernetes ConfigMaps with separate fields

### Implementation Pattern (C++)

All services MUST implement this pattern:

```cpp
services.addService<utils::Database>([](http::IServiceProvider& /* provider */) {
    utils::Logger::info("Creating Database singleton");
    
    utils::Database::Config dbConfig;
    
    // PRIMARY: Try DATABASE_URL first
    const char* dbUrl = std::getenv("DATABASE_URL");
    
    if (dbUrl && std::strlen(dbUrl) > 0) {
        // Parse DATABASE_URL connection string
        std::string url(dbUrl);
        utils::Logger::info("Using DATABASE_URL for configuration");
        
        // Parse postgresql://user:pass@host:port/dbname
        size_t protocolPos = url.find("://");
        if (protocolPos != std::string::npos) {
            size_t authStart = protocolPos + 3;
            size_t atPos = url.find('@', authStart);
            
            if (atPos != std::string::npos) {
                // Extract user:pass
                std::string auth = url.substr(authStart, atPos - authStart);
                size_t colonPos = auth.find(':');
                if (colonPos != std::string::npos) {
                    dbConfig.user = auth.substr(0, colonPos);
                    dbConfig.password = auth.substr(colonPos + 1);
                }
                
                // Extract host:port/dbname
                size_t hostStart = atPos + 1;
                size_t slashPos = url.find('/', hostStart);
                if (slashPos != std::string::npos) {
                    std::string hostPort = url.substr(hostStart, slashPos - hostStart);
                    size_t portPos = hostPort.find(':');
                    if (portPos != std::string::npos) {
                        dbConfig.host = hostPort.substr(0, portPos);
                        dbConfig.port = std::stoi(hostPort.substr(portPos + 1));
                    } else {
                        dbConfig.host = hostPort;
                        dbConfig.port = 5432; // Default
                    }
                    
                    dbConfig.database = url.substr(slashPos + 1);
                }
            }
        }
    } else {
        // FALLBACK: Individual environment variables
        utils::Logger::info("DATABASE_URL not set, using individual variables");
        
        const char* dbHost = std::getenv("DATABASE_HOST");
        dbConfig.host = dbHost ? dbHost : "localhost";
        
        const char* dbPort = std::getenv("DATABASE_PORT");
        dbConfig.port = dbPort ? std::stoi(dbPort) : 5432;
        
        const char* dbName = std::getenv("DATABASE_NAME");
        dbConfig.database = dbName ? dbName : "warehouse_db";
        
        const char* dbUser = std::getenv("DATABASE_USER");
        dbConfig.user = dbUser ? dbUser : "warehouse";
        
        const char* dbPassword = std::getenv("DATABASE_PASSWORD");
        dbConfig.password = dbPassword ? dbPassword : "warehouse";
    }
    
    utils::Logger::info("Database config - host={}, port={}, db={}, user={}",
        dbConfig.host, dbConfig.port, dbConfig.database, dbConfig.user);
    
    auto db = std::make_shared<utils::Database>(dbConfig);
    if (!db->connect()) {
        utils::Logger::error("Failed to connect to database!");
    } else {
        utils::Logger::info("Database connection successful");
    }
    return db;
}, http::ServiceLifetime::Singleton);
```

## RabbitMQ Configuration

All C++ services using the `warehouse-messaging` library automatically read these variables:

```bash
export RABBITMQ_HOST="localhost"        # RabbitMQ host
export RABBITMQ_PORT="5672"            # RabbitMQ port
export RABBITMQ_USER="warehouse"       # RabbitMQ username
export RABBITMQ_PASSWORD="warehouse_dev" # RabbitMQ password
export RABBITMQ_VHOST="/"              # RabbitMQ virtual host
export RABBITMQ_EXCHANGE="warehouse.events" # Exchange name (optional)
```

**Note**: The `warehouse-messaging` library handles these internally - no code changes needed in services.

## Redis Configuration

```bash
export REDIS_HOST="localhost"   # Redis host
export REDIS_PORT="6379"       # Redis port
export REDIS_ENABLED="true"    # Enable/disable Redis caching
export REDIS_URL="redis://localhost:6379" # Alternative: connection string
```

## Server Configuration

```bash
export SERVER_PORT="8080"      # HTTP server port
export SERVER_HOST="0.0.0.0"   # HTTP server bind address
```

## Logging Configuration

```bash
export LOG_LEVEL="info"        # Logging level: debug, info, warn, error
```

## Service Authentication

```bash
export SERVICE_API_KEY="warehouse_test_key" # Service-to-service authentication
```

## Docker Compose Configuration Template

**Standard Pattern** for all C++ services:

```yaml
version: '3.8'

services:
  <service-name>:
    build:
      context: .
      dockerfile: Dockerfile
    container_name: <service-name>
    ports:
      - "<external-port>:<internal-port>"
    environment:
      # Server configuration
      - SERVER_PORT=8080
      - SERVER_HOST=0.0.0.0
      
      # Database configuration (PRIMARY)
      - DATABASE_URL=postgresql://<user>:<pass>@postgres:5432/<dbname>
      
      # Database configuration (FALLBACK - for flexibility)
      - DATABASE_HOST=postgres
      - DATABASE_PORT=5432
      - DATABASE_NAME=<dbname>
      - DATABASE_USER=<user>
      - DATABASE_PASSWORD=<pass>
      
      # Redis configuration
      - REDIS_HOST=redis
      - REDIS_PORT=6379
      - REDIS_ENABLED=false
      
      # RabbitMQ configuration (for event-driven services)
      - RABBITMQ_HOST=rabbitmq
      - RABBITMQ_PORT=5672
      - RABBITMQ_VHOST=/
      - RABBITMQ_USER=warehouse
      - RABBITMQ_PASSWORD=warehouse_dev
      
      # Logging
      - LOG_LEVEL=info
      
      # Service authentication
      - SERVICE_API_KEY=warehouse_test_key
    
    depends_on:
      - postgres
      - redis
      - rabbitmq
    
    networks:
      - warehouse-network
    
    volumes:
      - ./logs:/app/logs
      - ./config:/app/config
    
    restart: unless-stopped
```

## Configuration Priority

Services MUST follow this resolution order:

1. **Environment Variables** (highest priority)
2. **Config File** (e.g., `config/application.json`)
3. **Hard-coded Defaults** (lowest priority)

**Example**:
```cpp
std::string dbUrl = utils::Config::getEnv("DATABASE_URL",
    utils::Config::getString("database.connectionString", 
        "postgresql://warehouse:warehouse@localhost:5432/warehouse_db"));
```

## Service-Specific Configuration

### inventory-service
- Port: 8081
- Database: `inventory_db`
- User: `inventory_user` / `inventory_pass`

### warehouse-service
- Port: 8080
- Database: `warehouse_db`
- User: `warehouse` / `warehouse_password`

### order-service
- Port: 8083
- Database: `order_db`
- User: `order` / `order_dev`

### product-service (Rust)
- Port: 8082
- Database: `warehouse_db` (shared with warehouse-service)
- User: `warehouse` / `warehouse`

## Configuration Validation

Services MUST:
1. ✅ Log which configuration method is being used (DATABASE_URL vs individual vars)
2. ✅ Log sanitized connection details (hide passwords!)
3. ✅ Validate connection on startup
4. ✅ Fail gracefully with clear error messages
5. ✅ Include configuration source in health check responses

**Example Logging**:
```
[INFO] Using DATABASE_URL for configuration
[INFO] Database config - host=postgres, port=5432, db=warehouse_db, user=warehouse
[INFO] Database connection successful
```

## Migration Checklist

When updating a service to this standard:

- [ ] Update `Application.cpp` to parse `DATABASE_URL` first
- [ ] Add fallback to individual `DATABASE_*` variables
- [ ] Update `docker-compose.yml` to include `DATABASE_URL`
- [ ] Keep individual variables for backward compatibility
- [ ] Add RabbitMQ configuration if service publishes/consumes events
- [ ] Update service README.md with configuration examples
- [ ] Test with both `DATABASE_URL` and individual variables
- [ ] Verify logging shows correct configuration source

## Services Status

| Service | DATABASE_URL | Individual Vars | RabbitMQ | Status |
|---------|-------------|----------------|----------|---------|
| inventory-service | ✅ | ✅ | ✅ | ✅ Complete |
| warehouse-service | ✅ | ✅ | ✅ | ✅ Updated |
| order-service | ✅ | ✅ | ⚠️ Partial | 🔄 Needs RabbitMQ |
| product-service | ➖ | ✅ | ✅ | ➖ Rust (different) |

## Best Practices

1. **Never commit passwords** - Use `.env` files (gitignored) for local development
2. **Use secrets management** in production (AWS Secrets Manager, HashiCorp Vault)
3. **Rotate credentials regularly** - especially for production databases
4. **Different credentials per environment** - dev/staging/prod should never share
5. **Least privilege** - database users should only have necessary permissions
6. **SSL/TLS in production** - append `?sslmode=require` to DATABASE_URL
7. **Connection pooling** - configure via additional URL parameters

## Example .env File (Local Development)

```bash
# .env (DO NOT COMMIT)
DATABASE_URL=postgresql://warehouse:warehouse_dev@localhost:5432/warehouse_db
RABBITMQ_HOST=localhost
RABBITMQ_PORT=5672
RABBITMQ_USER=warehouse
RABBITMQ_PASSWORD=warehouse_dev
REDIS_HOST=localhost
REDIS_PORT=6379
LOG_LEVEL=debug
SERVICE_API_KEY=local_dev_key
```

## References

- [12-Factor App: Config](https://12factor.net/config)
- [PostgreSQL Connection Strings](https://www.postgresql.org/docs/current/libpq-connect.html#LIBPQ-CONNSTRING)
- [Docker Environment Variables](https://docs.docker.com/compose/environment-variables/)
- [Warehouse Messaging Library](/services/cpp/shared/warehouse-messaging/README.md)

## Support

For questions or issues with configuration:
- Check service logs for configuration source and connection details
- Verify environment variables are set: `docker-compose config`
- Test connection manually: `psql $DATABASE_URL`
- Review service-specific README.md for additional options
