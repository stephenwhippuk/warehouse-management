# Shared Messaging Library - Design Proposal

## Problem Statement

Currently, each C++ service implements RabbitMQ integration from scratch:
- 250+ lines of AMQP boilerplate code per service
- Manual ACK, retry, DLQ logic duplicated across services
- Inconsistent error handling and logging
- Risk of configuration mistakes (exclusive queues, auto-ack, etc.)
- Difficult to maintain and update (changes needed in multiple services)

## Proposed Solution: `warehouse-messaging` Shared Library

A production-ready C++ library that abstracts RabbitMQ complexity behind a clean, simple API.

---

## Architecture

### Directory Structure

```
services/cpp/shared/warehouse-messaging/
├── include/
│   └── warehouse/
│       └── messaging/
│           ├── EventPublisher.hpp        # Simple publisher interface
│           ├── EventConsumer.hpp         # Simple consumer interface
│           ├── Event.hpp                 # Event data structure
│           ├── EventHandler.hpp          # Handler callback types
│           ├── MessagingConfig.hpp       # Configuration structures
│           └── MessagingException.hpp    # Custom exceptions
├── src/
│   ├── RabbitMqPublisher.cpp            # RabbitMQ publisher implementation
│   ├── RabbitMqConsumer.cpp             # RabbitMQ consumer implementation (with all resilience)
│   ├── Event.cpp                        # Event serialization
│   └── MessagingConfig.cpp              # Config builders
├── tests/
│   ├── PublisherTests.cpp
│   ├── ConsumerTests.cpp
│   └── IntegrationTests.cpp
├── docs/
│   ├── README.md                        # Getting started guide
│   ├── PUBLISHING.md                    # Publishing events
│   ├── CONSUMING.md                     # Consuming events
│   └── CONFIGURATION.md                 # Configuration options
├── examples/
│   ├── simple_publisher.cpp
│   ├── simple_consumer.cpp
│   └── multi_queue_consumer.cpp
└── CMakeLists.txt
```

---

## Simple API Design

### Publishing Events

**Before (Current - 50+ lines):**
```cpp
// In every service that publishes events...
amqp_connection_state_t conn = amqp_new_connection();
amqp_socket_t* socket = amqp_tcp_socket_new(conn);
amqp_socket_open(socket, host.c_str(), port);
amqp_login(conn, vhost.c_str(), 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, user.c_str(), pass.c_str());
amqp_channel_open(conn, 1);
// ... declare exchange, set properties, publish, handle errors, close connection
// Plus: JSON serialization, correlation ID generation, timestamp formatting
```

**After (Shared Library - 5 lines):**
```cpp
#include <warehouse/messaging/EventPublisher.hpp>
using namespace warehouse::messaging;

auto publisher = EventPublisher::create("product-service");
Event event("product.created", productData);
publisher->publish(event);  // That's it! ✅
```

### Consuming Events

**Before (Current - 200+ lines):**
```cpp
// RabbitMqMessageConsumer class with:
// - Connection management
// - Queue declaration (need to remember: durable=1, exclusive=0, etc.)
// - Manual ACK logic
// - Retry counting
// - Error handling
// - Reconnection logic
// - Thread management
```

**After (Shared Library - 10 lines):**
```cpp
#include <warehouse/messaging/EventConsumer.hpp>
using namespace warehouse::messaging;

auto consumer = EventConsumer::create("inventory-service", {"product.*"});

consumer->onEvent("product.created", [](const Event& event) {
    // Your business logic here
    updateProductCache(event.data());
});

consumer->start();  // Handles everything: ACK, retry, DLQ, reconnection ✅
```

---

## Core Classes

### 1. Event Class

```cpp
namespace warehouse::messaging {

class Event {
public:
    // Constructors
    Event(const std::string& type, const nlohmann::json& data);
    
    // Factory from JSON
    static Event fromJson(const nlohmann::json& j);
    
    // Accessors
    std::string getId() const;          // Auto-generated UUID
    std::string getType() const;        // e.g., "product.created"
    std::string getTimestamp() const;   // ISO 8601
    std::string getSource() const;      // Service name
    std::string getCorrelationId() const;
    nlohmann::json getData() const;
    
    // Serialization
    nlohmann::json toJson() const;
    std::string toString() const;
    
    // Fluent setters
    Event& withCorrelationId(const std::string& id);
    Event& withMetadata(const std::string& key, const std::string& value);
    
private:
    std::string id_;
    std::string type_;
    std::string timestamp_;
    std::string source_;
    std::string correlationId_;
    nlohmann::json data_;
    std::map<std::string, std::string> metadata_;
};

} // namespace
```

### 2. EventPublisher Interface

```cpp
namespace warehouse::messaging {

class EventPublisher {
public:
    virtual ~EventPublisher() = default;
    
    // Factory method (auto-connects to RabbitMQ from config)
    static std::unique_ptr<EventPublisher> create(const std::string& serviceName);
    static std::unique_ptr<EventPublisher> create(const MessagingConfig& config);
    
    // Simple publish (fire and forget)
    virtual void publish(const Event& event) = 0;
    
    // Publish with confirmation (waits for RabbitMQ ACK)
    virtual void publishWithConfirmation(const Event& event) = 0;
    
    // Batch publish (more efficient)
    virtual void publishBatch(const std::vector<Event>& events) = 0;
    
    // Health check
    virtual bool isHealthy() const = 0;
    
    // Metrics
    virtual uint64_t getPublishedCount() const = 0;
    virtual uint64_t getFailedCount() const = 0;
};

} // namespace
```

### 3. EventConsumer Interface

```cpp
namespace warehouse::messaging {

// Handler callback type
using EventHandler = std::function<void(const Event& event)>;

class EventConsumer {
public:
    virtual ~EventConsumer() = default;
    
    // Factory method
    // serviceName: for queue naming (e.g., "inventory-service")
    // routingKeys: patterns to bind (e.g., {"product.*", "warehouse.*"})
    static std::unique_ptr<EventConsumer> create(
        const std::string& serviceName,
        const std::vector<std::string>& routingKeys
    );
    
    static std::unique_ptr<EventConsumer> create(const ConsumerConfig& config);
    
    // Register event handlers
    virtual void onEvent(const std::string& eventType, EventHandler handler) = 0;
    virtual void onAnyEvent(EventHandler handler) = 0;  // Catch-all
    
    // Lifecycle
    virtual void start() = 0;           // Start consuming (blocking or threaded)
    virtual void stop() = 0;            // Graceful shutdown
    virtual bool isRunning() const = 0;
    
    // Health check
    virtual bool isHealthy() const = 0;
    
    // Metrics
    virtual uint64_t getProcessedCount() const = 0;
    virtual uint64_t getFailedCount() const = 0;
    virtual uint64_t getRetriedCount() const = 0;
};

} // namespace
```

### 4. Configuration Structures

```cpp
namespace warehouse::messaging {

struct MessagingConfig {
    // Connection
    std::string host = "localhost";
    int port = 5672;
    std::string virtualHost = "/";
    std::string username = "guest";
    std::string password = "guest";
    
    // Service identity
    std::string serviceName;  // e.g., "inventory-service"
    
    // Exchange
    std::string exchange = "warehouse.events";
    std::string exchangeType = "topic";
    
    // Load from environment variables
    static MessagingConfig fromEnvironment();
    
    // Load from config file
    static MessagingConfig fromFile(const std::string& path);
};

struct ConsumerConfig : public MessagingConfig {
    // Queue configuration
    std::string queuePrefix;               // e.g., "inventory-service"
    std::vector<std::string> routingKeys;  // e.g., {"product.*"}
    
    // Resilience settings
    int maxRetries = 3;
    int prefetchCount = 1;
    bool autoReconnect = true;
    int reconnectDelayMs = 5000;
    
    // Dead letter queue
    bool enableDLQ = true;
    std::string dlxExchange = "warehouse.dlx";
    
    // Performance
    bool useThreadPool = false;
    int threadPoolSize = 4;
};

struct PublisherConfig : public MessagingConfig {
    // Publisher settings
    bool enableConfirmations = false;  // Slower but guaranteed delivery
    int connectionPoolSize = 1;        // For high throughput
};

} // namespace
```

---

## Implementation Details

### RabbitMqConsumer Class (Internal)

This implements the `EventConsumer` interface with all production-ready features:

```cpp
class RabbitMqConsumer : public EventConsumer {
public:
    explicit RabbitMqConsumer(const ConsumerConfig& config);
    ~RabbitMqConsumer() override;
    
    void onEvent(const std::string& eventType, EventHandler handler) override;
    void onAnyEvent(EventHandler handler) override;
    void start() override;
    void stop() override;
    bool isRunning() const override;
    bool isHealthy() const override;
    uint64_t getProcessedCount() const override;
    uint64_t getFailedCount() const override;
    uint64_t getRetriedCount() const override;

private:
    // Connection management
    void connect();
    void reconnect();
    void close();
    
    // Queue setup (with production-ready defaults)
    void declareQueue();
    void configureDLQ();
    
    // Consume loop
    void consumeLoop();
    void processMessage(const amqp_envelope_t& envelope);
    
    // ACK/NACK logic
    void acknowledge(uint64_t deliveryTag);
    void rejectWithRequeue(uint64_t deliveryTag);
    void rejectToDeadLetter(uint64_t deliveryTag);
    
    // Retry logic
    int getRetryCount(const amqp_envelope_t& envelope);
    bool shouldRetry(int retryCount);
    
    // Handler dispatch
    void dispatchEvent(const Event& event);
    
    // Metrics
    std::atomic<uint64_t> processed_{0};
    std::atomic<uint64_t> failed_{0};
    std::atomic<uint64_t> retried_{0};
    
    // Configuration
    ConsumerConfig config_;
    
    // AMQP state
    amqp_connection_state_t connection_{nullptr};
    amqp_socket_t* socket_{nullptr};
    uint16_t channel_{1};
    
    // Threading
    std::atomic<bool> running_{false};
    std::unique_ptr<std::thread> consumerThread_;
    
    // Handler registry
    std::map<std::string, std::vector<EventHandler>> handlers_;
    std::vector<EventHandler> anyHandlers_;  // Catch-all
    std::mutex handlersMutex_;
};
```

**Key Features Built-In:**
- ✅ Durable, non-exclusive queues (correct by default)
- ✅ Manual ACK with retry (up to max retries)
- ✅ Dead letter queue configuration
- ✅ Automatic reconnection on connection loss
- ✅ Prefetch QoS (process one at a time)
- ✅ Thread-safe handler registration
- ✅ Comprehensive error handling
- ✅ Metrics collection

---

## Usage Examples

### Example 1: Simple Product Service Publisher

```cpp
#include <warehouse/messaging/EventPublisher.hpp>
#include "product/models/Product.hpp"

class ProductService {
public:
    ProductService() {
        publisher_ = EventPublisher::create("product-service");
    }
    
    void createProduct(const Product& product) {
        // 1. Save to database
        repository_->save(product);
        
        // 2. Publish event (one line!)
        Event event("product.created", product.toJson());
        publisher_->publish(event);
        
        // That's it! 🎉
    }

private:
    std::unique_ptr<EventPublisher> publisher_;
};
```

### Example 2: Simple Inventory Service Consumer

```cpp
#include <warehouse/messaging/EventConsumer.hpp>
#include "inventory/handlers/ProductEventHandler.hpp"

class Application {
public:
    void initialize() {
        // Create consumer for product events
        consumer_ = EventConsumer::create(
            "inventory-service",           // Queue: inventory-service-products
            {"product.created", "product.updated", "product.deleted"}
        );
        
        // Register handlers (clean and simple!)
        consumer_->onEvent("product.created", [this](const Event& event) {
            productHandler_->handleCreated(event.data());
        });
        
        consumer_->onEvent("product.updated", [this](const Event& event) {
            productHandler_->handleUpdated(event.data());
        });
        
        consumer_->onEvent("product.deleted", [this](const Event& event) {
            productHandler_->handleDeleted(event.data());
        });
        
        // Start consuming (in background thread)
        consumer_->start();
    }
    
    void shutdown() {
        consumer_->stop();
    }

private:
    std::unique_ptr<EventConsumer> consumer_;
    std::shared_ptr<ProductEventHandler> productHandler_;
};
```

### Example 3: Order Service (Multiple Event Types)

```cpp
class Application {
public:
    void initialize() {
        // Consumer for product events
        productConsumer_ = EventConsumer::create(
            "order-service",
            {"product.*"}  // All product events
        );
        
        productConsumer_->onEvent("product.created", [this](const Event& e) {
            productCache_->upsert(e.data());
        });
        
        productConsumer_->onEvent("product.updated", [this](const Event& e) {
            productCache_->upsert(e.data());
        });
        
        productConsumer_->onEvent("product.deleted", [this](const Event& e) {
            productCache_->remove(e.data()["id"]);
        });
        
        // Consumer for warehouse events (separate queue)
        warehouseConsumer_ = EventConsumer::create(
            "order-service",
            {"warehouse.*"}  // All warehouse events
        );
        
        warehouseConsumer_->onEvent("warehouse.created", [this](const Event& e) {
            warehouseCache_->upsert(e.data());
        });
        
        // Start both consumers
        productConsumer_->start();
        warehouseConsumer_->start();
    }

private:
    std::unique_ptr<EventConsumer> productConsumer_;
    std::unique_ptr<EventConsumer> warehouseConsumer_;
};
```

### Example 4: Advanced Configuration

```cpp
// Custom configuration for high-throughput service
ConsumerConfig config;
config.serviceName = "analytics-service";
config.routingKeys = {"#"};  // All events
config.maxRetries = 5;
config.prefetchCount = 10;  // Process 10 at a time
config.useThreadPool = true;
config.threadPoolSize = 8;  // 8 worker threads
config.enableDLQ = true;

auto consumer = EventConsumer::create(config);

consumer->onAnyEvent([](const Event& event) {
    // Process all events for analytics
    metricsCollector.record(event);
});

consumer->start();
```

---

## Benefits

### For Service Developers

| Before (Manual) | After (Library) |
|-----------------|-----------------|
| 250+ lines of AMQP code | 10-20 lines of clean API |
| Need to understand AMQP protocol | Just call `publish()` and `onEvent()` |
| Need to implement retry logic | Built-in with sensible defaults |
| Need to configure queue params | Correct by default (durable, non-exclusive) |
| Need to handle reconnection | Automatic |
| Need to track metrics | Built-in metrics |
| Copy-paste from other services | Import shared library |
| Risk of configuration mistakes | Safe defaults, validated config |

### For the Team

- **Consistency**: All services use same patterns
- **Maintainability**: Update library once, all services benefit
- **Testability**: Library thoroughly tested independently
- **Onboarding**: New developers learn one API
- **Best Practices**: Production patterns baked in
- **Evolution**: Add features (tracing, metrics) in one place

---

## Implementation Plan

### Phase 1: Core Library (1 week)

- [ ] Create shared library structure
- [ ] Implement Event class with serialization
- [ ] Implement RabbitMqPublisher (basic)
- [ ] Implement RabbitMqConsumer (with all resilience features)
- [ ] Configuration structures and builders
- [ ] Unit tests
- [ ] CMake build system
- [ ] Documentation

### Phase 2: Integration (1 week)

- [ ] Migrate inventory-service to use library
- [ ] Migrate product-service to use library
- [ ] Migrate order-service to use library
- [ ] Integration tests
- [ ] Performance testing
- [ ] Documentation updates

### Phase 3: Advanced Features (Future)

- [ ] Distributed tracing support (OpenTelemetry)
- [ ] Prometheus metrics export
- [ ] Circuit breaker for cascading failures
- [ ] Event schema validation
- [ ] Request-reply pattern support
- [ ] Saga orchestration support

---

## Testing Strategy

### Library Tests

```cpp
// Example: Consumer resilience test
TEST_CASE("Consumer retries on handler failure", "[consumer][retry]") {
    MockRabbitMQ mockMQ;
    ConsumerConfig config;
    config.maxRetries = 3;
    
    auto consumer = EventConsumer::create(config);
    
    int attemptCount = 0;
    consumer->onEvent("test.event", [&](const Event& e) {
        attemptCount++;
        if (attemptCount < 3) {
            throw std::runtime_error("Simulated failure");
        }
        // Success on 3rd attempt
    });
    
    consumer->start();
    mockMQ.publishEvent("test.event", {});
    
    REQUIRE(attemptCount == 3);
    REQUIRE(mockMQ.getAckCount() == 1);  // ACK'd after success
}
```

### Service Integration Tests

Services test against real RabbitMQ but with simple library API:

```cpp
TEST_CASE("Inventory receives product events", "[integration]") {
    auto publisher = EventPublisher::create("product-service");
    auto consumer = EventConsumer::create("inventory-service", {"product.*"});
    
    bool received = false;
    consumer->onEvent("product.created", [&](const Event& e) {
        received = true;
    });
    consumer->start();
    
    Event event("product.created", {{"sku", "TEST-001"}});
    publisher->publish(event);
    
    // Wait for delivery
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    REQUIRE(received == true);
}
```

---

## Documentation

### README.md (Quick Start)

```markdown
# Warehouse Messaging Library

Production-ready RabbitMQ messaging for C++ microservices.

## Quick Start

### Publishing Events

```cpp
#include <warehouse/messaging/EventPublisher.hpp>

auto publisher = EventPublisher::create("my-service");
Event event("entity.action", data);
publisher->publish(event);
```

### Consuming Events

```cpp
#include <warehouse/messaging/EventConsumer.hpp>

auto consumer = EventConsumer::create("my-service", {"entity.*"});

consumer->onEvent("entity.created", [](const Event& event) {
    // Handle event
});

consumer->start();
```

## Features

✅ Simple, clean API
✅ Production-ready resilience (retry, DLQ, reconnection)
✅ Horizontal scaling support
✅ Multi-service fanout support
✅ Automatic metrics collection
✅ Comprehensive error handling

See [docs/](docs/) for complete guides.
```

---

## Next Steps

1. **Create library structure** under `services/cpp/shared/warehouse-messaging/`
2. **Implement core classes** (Event, Publisher, Consumer)
3. **Write tests** (unit + integration)
4. **Migrate inventory-service** as proof-of-concept
5. **Document usage** with examples
6. **Roll out to other services**

This shared library will dramatically simplify event-driven architecture across all C++ services! 🚀
