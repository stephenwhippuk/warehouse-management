# Event Consumption Architecture

## Overview

This document describes the event consumption patterns, error handling strategies, and resilience mechanisms for the inventory-service message consumer.

## Current Implementation Analysis

### Queue Configuration

```cpp
// Current settings in RabbitMqMessageConsumer::connect()
amqp_queue_declare(
    connection_, channel_,
    amqp_cstring_bytes(config_.queue_name.c_str()),  // "inventory-service-products"
    0,  // passive: false
    0,  // durable: false ⚠️ PROBLEM: Queue lost on restart
    1,  // exclusive: true ⚠️ PROBLEM: Only one consumer allowed
    1,  // auto_delete: true ⚠️ PROBLEM: Queue deleted when consumer disconnects
    amqp_empty_table
);

amqp_basic_consume(
    connection_, channel_,
    amqp_cstring_bytes(config_.queue_name.c_str()),
    amqp_empty_bytes,
    0,  // no_local: false
    1,  // no_ack: true ⚠️ PROBLEM: Auto-ack (message lost if crash during processing)
    1,  // exclusive: true
    amqp_empty_table
);
```

### Current Behavior

| Scenario | Current Behavior | Risk Level |
|----------|-----------------|------------|
| **Multiple server instances** | ❌ Second instance FAILS to connect (exclusive queue) | 🔴 HIGH - Cannot scale horizontally |
| **Server crashes during processing** | ❌ Message LOST (auto-ack before handler completes) | 🔴 HIGH - Data loss |
| **Server unavailable** | ⚠️ Messages accumulate in queue, BUT queue is NOT durable | 🟡 MEDIUM - Messages lost on RabbitMQ restart |
| **Handler throws exception** | ❌ Exception logged, message LOST (already ACK'd) | 🔴 HIGH - Silent failures |
| **Database temporarily down** | ❌ Update fails, message LOST | 🔴 HIGH - Cache inconsistency |
| **Malformed event payload** | ❌ Parse error, message LOST | 🟡 MEDIUM - Need dead letter queue |

## Distributed Systems Concerns

### 1. Multiple **Services** Consuming Same Events (Fanout Pattern)

**Question**: What happens when product-service publishes an event that BOTH inventory-service AND order-service need to consume?

#### The Requirement

```
product-service publishes: product.created
├─> inventory-service needs it (update product_cache)
└─> order-service needs it (update product_cache)

Both services MUST receive EVERY event
```

This is **NOT** competing consumers (load balancing).  
This is **publish-subscribe** (fanout to multiple services).

#### Solution: Separate Queue Per Service

Each service declares its own queue:

```
RabbitMQ Exchange: warehouse.events
         |
         ├─> Queue: inventory-service-products (inventory-service consumers)
         |     Bindings: product.created, product.updated, product.deleted
         |
         └─> Queue: order-service-products (order-service consumers)
               Bindings: product.created, product.updated, product.deleted
```

**When product.created published:**
```
product-service
    │
    └─> Publish to exchange: warehouse.events
         routing_key: product.created
              │
              ├─> Copy to: inventory-service-products ✅
              │    └─> inventory-service consumes it
              │
              └─> Copy to: order-service-products ✅
                   └─> order-service consumes it
```

**Key Points:**
- ✅ Each queue gets a COPY of every matching event
- ✅ inventory-service processes independently from order-service
- ✅ If inventory-service is down, its queue accumulates messages
- ✅ If order-service is down, its queue accumulates messages
- ✅ Services don't affect each other (decoupled)

#### Current Implementation Status

**inventory-service**: ✅ Already correct!
```cpp
config_.queue_name = "inventory-service-products";  // Service-specific queue ✅
config_.routing_keys = {"product.created", "product.updated", "product.deleted"};
```

**order-service**: ✅ Would do the same:
```cpp
config_.queue_name = "order-service-products";  // Different queue name ✅
config_.routing_keys = {"product.created", "product.updated", "product.deleted"};
```

**Result**: Both services receive ALL events independently ✅

#### What NOT to Do (Common Mistake)

❌ **WRONG: Shared queue between services**
```
// Both services declare:
config_.queue_name = "product-events";  // Same queue name ❌

Result: Messages distributed (competing consumers)
- inventory-service gets 50% of events
- order-service gets 50% of events
- Both miss half the events ❌
```

### 2. Multiple **Instances** of Same Service (Competing Consumers)

**Question**: What happens if multiple inventory-service instances want to consume product events?

#### Current Implementation (EXCLUSIVE Queue)

```
RabbitMQ                 Instance 1              Instance 2
   |                         |                        |
   |<---- connect ----------|                        |
   |                         |                        |
   |<---- bind queue --------|                        |
   |===== messages =========>|                        |
   |                         |                        |
   |                         |    <---- connect -----|
   |                         |                        |
   |------ ERROR: Queue is exclusive --------------->| ❌ FAILS
```

**Result**: Second instance CANNOT connect. Only ONE consumer allowed.

**Impact**:
- ❌ Cannot scale horizontally for load
- ❌ No high availability (if instance dies, no failover until restart)
- ❌ Single point of failure

#### Recommended: COMPETING CONSUMERS Pattern

```
RabbitMQ                 Instance 1              Instance 2
   |                         |                        |
   |<---- connect ----------|                        |
   |<---- bind queue --------|                        |
   |                         |                        |
   |                         |    <---- connect -----|
   |                         |    <---- bind queue ---|
   |                         |                        |
   |=== msg 1 ==============>|                        |
   |=== msg 2 ====================================>|
   |=== msg 3 ==============>|                        |
   |=== msg 4 ====================================>|
```

**RabbitMQ distributes messages using Round-robin**:
- Each message delivered to ONE consumer only
- Load balanced automatically
- If one consumer dies, others continue processing

**Configuration Change**:
```cpp
amqp_queue_declare(
    connection_, channel_,
    amqp_cstring_bytes(config_.queue_name.c_str()),
    0,  // passive
    1,  // durable: true ✅ Queue survives RabbitMQ restart
    0,  // exclusive: false ✅ Multiple consumers allowed
    0,  // auto_delete: false ✅ Queue persists when no consumers
    amqp_empty_table
);
```

#### Visual Summary: Services vs Instances

```
Single Event Published: product.created
         │
         v
   warehouse.events exchange
         │
         ├─────────────────────────────────┬────────────────────────────────┐
         │                                 │                                │
         v                                 v                                v
inventory-service-products      order-service-products      shipment-service-products
     (Queue)                         (Queue)                      (Queue)
         │                                 │                                │
         │ Competing Consumers             │ Competing Consumers            │
         ├──────┬──────┐                  └─> order-service               └─> shipment-service
         v      v      v                       (single instance)                (single instance)
    inv-svc inv-svc inv-svc
   instance1 instance2 instance3
   
Result:
- inventory-service processes it ONCE (load balanced across 3 instances)
- order-service processes it ONCE (single instance)
- shipment-service processes it ONCE (single instance)
- Total: Event processed 3 times (once per SERVICE)
```

**Key Principle**: 
- **Different queue names** = Different services = Each gets copy (fanout)
- **Same queue name** = Multiple instances = Load balanced (competing consumers)

### 3. Server Error During Message Processing

**Question**: What happens if handler throws exception?

#### Current Implementation (AUTO-ACK)

```
Timeline of message processing:

1. RabbitMQ sends message to consumer
2. Consumer IMMEDIATELY acknowledges (auto-ack) ✅ Message removed from queue
3. Consumer calls handler(routingKey, payload)
4. Handler updates database...
5. ❌ EXCEPTION thrown (database timeout, constraint violation, etc.)
6. Exception caught and logged
7. Message is GONE (already ACK'd in step 2)

Result: MESSAGE LOST, cache not updated, no retry
```

**Impact**:
- ❌ Data loss on transient errors (network blip, DB connection pool exhausted)
- ❌ No retry mechanism
- ❌ Silent failures (logged but not recovered)

#### Recommended: MANUAL ACK with Retry

```cpp
Timeline with manual ACK:

1. RabbitMQ sends message to consumer
2. Consumer does NOT acknowledge yet
3. Consumer calls handler(routingKey, payload)
4. IF handler succeeds:
     ✅ Consumer sends ACK → message removed from queue
   ELSE IF handler throws exception:
     ❌ Consumer sends NACK with requeue=true → message returned to queue
     ⏱️ Message will be redelivered (possibly to different instance)
5. After MAX_RETRIES, send NACK with requeue=false → message goes to DLQ
```

**Configuration Change**:
```cpp
amqp_basic_consume(
    connection_, channel_,
    amqp_cstring_bytes(config_.queue_name.c_str()),
    amqp_empty_bytes,
    0,  // no_local
    0,  // no_ack: false ✅ Manual acknowledgment required
    0,  // exclusive: false
    amqp_empty_table
);

// After successful processing:
amqp_basic_ack(connection_, channel_, envelope.delivery_tag, 0);

// On error (with retry):
amqp_basic_nack(connection_, channel_, envelope.delivery_tag, 0, 1); // requeue=true

// On permanent failure (after max retries):
amqp_basic_nack(connection_, channel_, envelope.delivery_tag, 0, 0); // requeue=false
```

### 4. Server Unavailable

**Question**: What happens if inventory-service is down?

#### Current Implementation (Non-durable Queue)

```
Timeline:

1. inventory-service starts, declares queue "inventory-service-products"
2. Queue binds to "warehouse.events" exchange with routing keys
3. product-service publishes product.created event
4. Event routed to queue ✅
5. inventory-service CRASHES 💥
6. Queue still exists (but no consumers)
7. product-service publishes product.updated event
8. Event routed to queue ✅ (accumulating messages)
9. RabbitMQ RESTARTS 🔄
10. ❌ Queue DELETED (not durable)
11. ❌ All accumulated messages LOST
12. inventory-service restarts, creates new empty queue
```

**Impact**:
- ❌ Messages lost during RabbitMQ maintenance
- ❌ Messages lost during network partitions
- ❌ Cache becomes stale (missing updates)

#### Recommended: DURABLE Queue with Persistent Messages

```
Timeline with durable queue:

1. inventory-service starts, declares DURABLE queue
2. Queue binds to exchange
3. product-service publishes product.created event (with delivery_mode=2 persistent)
4. Event stored to disk ✅
5. inventory-service CRASHES 💥
6. Queue persists (no consumers, but messages safe)
7. product-service publishes product.updated event
8. Event stored to disk ✅
9. RabbitMQ RESTARTS 🔄
10. ✅ Queue SURVIVES with all messages intact
11. inventory-service restarts, reconnects to existing queue
12. ✅ Processes all accumulated messages in order
```

**Publisher Configuration** (product-service):
```cpp
amqp_basic_properties_t props;
props._flags = AMQP_BASIC_DELIVERY_MODE_FLAG;
props.delivery_mode = 2; // persistent ✅ Message survives RabbitMQ restart
```

## Recommended Production Architecture

### Complete Multi-Service Event Flow Example

```
Scenario: Product Updated (3 services need this event)

product-service
    │ (HTTP PUT /api/v1/products/123)
    │
    └─> Updates database
    └─> Publishes event to RabbitMQ
         exchange: warehouse.events
         routing_key: product.updated
         payload: {eventId, timestamp, data: {id, sku, name, ...}}
              │
              │ Topic Exchange Routes to ALL bound queues
              │
              ├────────────────────────────────────┐
              │                                    │
              v                                    v
      inventory-service-products          order-service-products
      (Durable, Non-exclusive)            (Durable, Non-exclusive)
              │                                    │
              │                                    │
      ┌───────┼────────┐                          │
      v       v        v                           v
   inv-1   inv-2   inv-3                       order-1
   (scale=3)                                    (scale=1)
              │                                    │
              └─> ONE instance processes it        └─> Processes event
                  (load balanced via RabbitMQ)         
                  Updates: product_cache               Updates: product_cache
```

**Result**: 
- inventory-service processes it ONCE (via one of 3 instances)
- order-service processes it ONCE (via its single instance)
- Each service's product_cache is now consistent ✅

### Queue Naming Convention (Critical)

Use this pattern for queue names:

```
{service-name}-{entity-type}

Examples:
- inventory-service-products
- inventory-service-warehouses
- order-service-products
- order-service-warehouses
- shipment-service-orders
- notification-service-orders
```

**Why?**
- Clear ownership (which service declares this queue)
- No collisions (each service has unique queue names)
- Enables fanout (different queues = each gets copy)
- Enables competing consumers (same queue = load balanced)

### Complete RabbitMQ Setup for Multi-Service Architecture

#### Exchange Declaration (One Time, in docker-compose setup)

```bash
# Main event exchange (topic for routing flexibility)
rabbitmqadmin declare exchange \
  name=warehouse.events \
  type=topic \
  durable=true

# Dead letter exchange (for failed messages)
rabbitmqadmin declare exchange \
  name=warehouse.dlx \
  type=topic \
  durable=true

# Dead letter queue
rabbitmqadmin declare queue \
  name=warehouse.dlq \
  durable=true

rabbitmqadmin declare binding \
  source=warehouse.dlx \
  destination=warehouse.dlq \
  routing_key="#"
```

#### Queue Declaration Per Service (Automatic via each service)

**inventory-service declares:**
```cpp
// Product events
queue: inventory-service-products
bindings: product.created, product.updated, product.deleted
exchange: warehouse.events

// Warehouse events
queue: inventory-service-warehouses
bindings: warehouse.created, warehouse.updated, warehouse.deleted
exchange: warehouse.events
```

**order-service declares:**
```cpp
// Product events (same routing keys, different queue)
queue: order-service-products
bindings: product.created, product.updated, product.deleted
exchange: warehouse.events

// Warehouse events
queue: order-service-warehouses
bindings: warehouse.created, warehouse.updated, warehouse.deleted
exchange: warehouse.events

// Order events (consumes its own events for saga patterns, etc.)
queue: order-service-orders-internal
bindings: order.created, order.updated
exchange: warehouse.events
```

**shipment-service declares:**
```cpp
// Order events only
queue: shipment-service-orders
bindings: order.created, order.updated, order.cancelled
exchange: warehouse.events
```

#### Event Flow Examples

**Example 1: product.created published**
```
product-service publishes
    ↓
warehouse.events exchange
    ├─> inventory-service-products ✅ (inventory-service receives)
    └─> order-service-products ✅ (order-service receives)

shipment-service-orders: No match (not bound to product.*) ❌
```

**Example 2: order.created published**
```
order-service publishes
    ↓
warehouse.events exchange
    ├─> shipment-service-orders ✅ (shipment-service receives)
    └─> order-service-orders-internal ✅ (order-service receives its own event)

inventory-service queues: No match ❌
```

**Example 3: warehouse.updated published**
```
warehouse-service publishes
    ↓
warehouse.events exchange
    ├─> inventory-service-warehouses ✅ (inventory-service receives)
    └─> order-service-warehouses ✅ (order-service receives)

No other services bound to warehouse.* ❌
```

### Configuration Per Service Type

#### Service with Horizontal Scaling (Multiple Instances)

```cpp
// inventory-service configuration
MessageConsumer::Config config;
config.queue_name = "inventory-service-products";  // Shared across instances ✅
config.exchange = "warehouse.events";
config.routing_keys = {"product.created", "product.updated", "product.deleted"};

// Queue settings for competing consumers:
durable: 1,      // Survives restart
exclusive: 0,    // ✅ CRITICAL: Allow multiple instances
auto_delete: 0,  // Queue persists
no_ack: 0        // Manual ACK for reliability

// Result when scaled:
// docker-compose up --scale inventory-service=3
// All 3 instances connect to SAME queue
// RabbitMQ load balances messages (round-robin)
// Each message processed by ONE instance only
```

#### Service with Single Instance (No Scaling)

```cpp
// notification-service configuration (intentionally single instance)
MessageConsumer::Config config;
config.queue_name = "notification-service-orders";
config.exchange = "warehouse.events";
config.routing_keys = {"order.created", "order.shipped"};

// Queue settings (same as multi-instance):
durable: 1,
exclusive: 0,    // Still use 0 (future-proof for scaling)
auto_delete: 0,
no_ack: 0
```

#### Service Consuming from Multiple Event Types

```cpp
// order-service consumes both product and warehouse events
// Solution: Create multiple consumers

class Application {
    std::shared_ptr<MessageConsumer> productConsumer_;
    std::shared_ptr<MessageConsumer> warehouseConsumer_;
    
    void initializeServices() {
        // Product events consumer
        MessageConsumer::Config productConfig;
        productConfig.queue_name = "order-service-products";
        productConfig.routing_keys = {"product.created", "product.updated", "product.deleted"};
        productConsumer_ = std::make_shared<RabbitMqMessageConsumer>(productConfig);
        productConsumer_->startConsuming([this](const std::string& key, const json& payload) {
            productEventHandler_->handle(key, payload);
        });
        
        // Warehouse events consumer
        MessageConsumer::Config warehouseConfig;
        warehouseConfig.queue_name = "order-service-warehouses";
        warehouseConfig.routing_keys = {"warehouse.created", "warehouse.updated", "warehouse.deleted"};
        warehouseConsumer_ = std::make_shared<RabbitMqMessageConsumer>(warehouseConfig);
        warehouseConsumer_->startConsuming([this](const std::string& key, const json& payload) {
            warehouseEventHandler_->handle(key, payload);
        });
    }
};
```

### Queue Configuration

```cpp
// Production-ready queue settings
amqp_table_t queue_args;
amqp_table_entry_t entries[3];

// Dead Letter Exchange for failed messages
entries[0].key = amqp_cstring_bytes("x-dead-letter-exchange");
entries[0].value.kind = AMQP_FIELD_KIND_UTF8;
entries[0].value.value.bytes = amqp_cstring_bytes("warehouse.dlx");

// Dead Letter Routing Key
entries[1].key = amqp_cstring_bytes("x-dead-letter-routing-key");
entries[1].value.kind = AMQP_FIELD_KIND_UTF8;
entries[1].value.value.bytes = amqp_cstring_bytes("inventory.product.failed");

// Message TTL (optional, for retry delay)
entries[2].key = amqp_cstring_bytes("x-message-ttl");
entries[2].value.kind = AMQP_FIELD_KIND_I32;
entries[2].value.value.i32 = 300000; // 5 minutes

queue_args.num_entries = 3;
queue_args.entries = entries;

amqp_queue_declare(
    connection_, channel_,
    amqp_cstring_bytes("inventory-service-products"),
    0,  // passive: false
    1,  // durable: true ✅
    0,  // exclusive: false ✅
    0,  // auto_delete: false ✅
    queue_args  // arguments ✅
);
```

### Consumer Configuration

```cpp
amqp_basic_consume(
    connection_, channel_,
    amqp_cstring_bytes("inventory-service-products"),
    amqp_empty_bytes,  // consumer_tag (auto-generated)
    0,  // no_local: false
    0,  // no_ack: false ✅ Manual ACK
    0,  // exclusive: false ✅ Allow multiple consumers
    amqp_empty_table
);

// Set prefetch count (QoS)
amqp_basic_qos(
    connection_, channel_,
    0,      // prefetch_size (0 = no limit)
    1,      // prefetch_count ✅ Process one message at a time
    0       // global: false (per-consumer)
);
```

### Message Processing with Retry Logic

```cpp
void RabbitMqMessageConsumer::consumeLoop(MessageHandler handler) {
    while (running_) {
        amqp_envelope_t envelope;
        struct timeval timeout = {1, 0};
        
        amqp_rpc_reply_t reply = amqp_consume_message(connection_, &envelope, &timeout, 0);
        
        if (reply.reply_type == AMQP_RESPONSE_NORMAL) {
            std::string routing_key(
                static_cast<const char*>(envelope.routing_key.bytes),
                envelope.routing_key.len
            );
            
            std::string message_body(
                static_cast<const char*>(envelope.message.body.bytes),
                envelope.message.body.len
            );
            
            bool success = false;
            try {
                json payload = json::parse(message_body);
                
                // Check retry count from headers
                int retry_count = getRetryCount(envelope);
                
                Logger::debug("Processing message (attempt {}): {}", retry_count + 1, routing_key);
                
                // Call handler
                handler(routing_key, payload);
                success = true;
                
                // ACK on success
                amqp_basic_ack(connection_, channel_, envelope.delivery_tag, 0);
                Logger::debug("Message processed successfully: {}", routing_key);
                
            } catch (const std::exception& e) {
                Logger::error("Error processing message: {}", e.what());
                
                int retry_count = getRetryCount(envelope);
                
                if (retry_count < MAX_RETRIES) {
                    // NACK with requeue (retry)
                    Logger::warn("Message will be retried (attempt {}/{})", 
                                 retry_count + 1, MAX_RETRIES);
                    amqp_basic_nack(connection_, channel_, envelope.delivery_tag, 0, 1);
                } else {
                    // NACK without requeue (dead letter)
                    Logger::error("Message failed after {} retries, sending to DLQ", MAX_RETRIES);
                    amqp_basic_nack(connection_, channel_, envelope.delivery_tag, 0, 0);
                }
            }
            
            amqp_destroy_envelope(&envelope);
        }
        // ... handle timeouts and errors
    }
}

int RabbitMqMessageConsumer::getRetryCount(const amqp_envelope_t& envelope) {
    // Check x-death header for retry count
    amqp_table_t* headers = &envelope.message.properties.headers;
    for (int i = 0; i < headers->num_entries; i++) {
        if (std::string((char*)headers->entries[i].key.bytes, headers->entries[i].key.len) == "x-death") {
            // Parse death count
            return extractDeathCount(headers->entries[i].value);
        }
    }
    return 0;
}
```

## Idempotency Considerations

### Problem: Same Message Processed Multiple Times

Even with manual ACK, a message can be delivered multiple times:
- Consumer processes message, updates database
- Consumer crashes BEFORE sending ACK
- RabbitMQ redelivers message (thinks it wasn't processed)
- Consumer processes AGAIN → duplicate update

### Solution 1: Idempotent Operations (Recommended for Cache Updates)

Product cache updates are naturally idempotent:

```cpp
// UPSERT is idempotent - safe to run multiple times
void ProductEventHandler::upsertProductCache(
    const std::string& product_id,
    const std::string& sku,
    const std::string& name
) {
    pqxx::work txn(*db_);
    txn.exec_params(
        "INSERT INTO product_cache (product_id, sku, name, cached_at, updated_at) "
        "VALUES ($1, $2, $3, NOW(), NOW()) "
        "ON CONFLICT (product_id) DO UPDATE SET "
        "  sku = EXCLUDED.sku, "
        "  name = EXCLUDED.name, "
        "  updated_at = NOW()",
        product_id, sku, name
    );
    txn.commit();
}

// DELETE is idempotent - safe to run if already deleted
void ProductEventHandler::deleteProductCache(const std::string& product_id) {
    pqxx::work txn(*db_);
    txn.exec_params("DELETE FROM product_cache WHERE product_id = $1", product_id);
    txn.commit();
}
```

Result: Processing same event twice produces same final state ✅

### Solution 2: Deduplication Table (For Non-Idempotent Operations)

If operations are NOT idempotent (e.g., incrementing counters), track processed events:

```sql
CREATE TABLE processed_events (
    event_id UUID PRIMARY KEY,
    routing_key VARCHAR(255) NOT NULL,
    processed_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_processed_at (processed_at)
);

-- Cleanup old entries periodically
DELETE FROM processed_events WHERE processed_at < NOW() - INTERVAL '7 days';
```

```cpp
bool ProductEventHandler::isEventProcessed(const std::string& event_id) {
    pqxx::work txn(*db_);
    auto result = txn.exec_params(
        "SELECT 1 FROM processed_events WHERE event_id = $1",
        event_id
    );
    return !result.empty();
}

void ProductEventHandler::markEventProcessed(const std::string& event_id, const std::string& routing_key) {
    pqxx::work txn(*db_);
    txn.exec_params(
        "INSERT INTO processed_events (event_id, routing_key) VALUES ($1, $2) ON CONFLICT DO NOTHING",
        event_id, routing_key
    );
    txn.commit();
}

void ProductEventHandler::handleProductCreated(const json& event) {
    std::string event_id = event["eventId"].get<std::string>();
    
    // Check if already processed
    if (isEventProcessed(event_id)) {
        Logger::info("Event {} already processed, skipping", event_id);
        return;
    }
    
    // Process event
    auto data = event["data"];
    upsertProductCache(
        data["id"].get<std::string>(),
        data["sku"].get<std::string>(),
        data["name"].get<std::string>()
    );
    
    // Mark as processed
    markEventProcessed(event_id, "product.created");
}
```

## Dead Letter Queue (DLQ) Setup

### 1. Declare Dead Letter Exchange and Queue

```bash
# Create DLX and DLQ in RabbitMQ
rabbitmqadmin declare exchange name=warehouse.dlx type=topic durable=true
rabbitmqadmin declare queue name=warehouse.dlq durable=true
rabbitmqadmin declare binding source=warehouse.dlx destination=warehouse.dlq routing_key="#"
```

### 2. Configure Main Queue with DLX

```cpp
// Set x-dead-letter-exchange on main queue (shown in Production Architecture section above)
```

### 3. Monitor and Handle Dead Letters

Failed messages (after max retries) go to DLQ for:
- Manual inspection
- Alerting/monitoring
- Manual retry after fix
- Permanent deletion if truly invalid

Example monitoring query:
```bash
# Check DLQ depth
rabbitmqctl list_queues name messages_ready messages_unacknowledged | grep warehouse.dlq
```

## Monitoring and Observability

### Key Metrics to Track

1. **Queue Depth**: `rabbitmqctl list_queues`
   - Alert if > 1000 messages (consumer lag)

2. **Consumer Count**: `rabbitmqctl list_consumers`
   - Alert if = 0 (no consumers connected)

3. **Processing Rate**: Log events/second
   ```cpp
   // In application:
   std::atomic<int> events_processed = 0;
   // Increment on successful processing
   // Export metric every 60 seconds
   ```

4. **Error Rate**: Track handler exceptions
   ```cpp
   std::atomic<int> events_failed = 0;
   // Alert if error_rate > 5%
   ```

5. **DLQ Depth**: Messages in dead letter queue
   - Alert if > 0 (requires investigation)

6. **Consumer Lag**: Time between event publish and consumption
   ```cpp
   // Compare event.timestamp to processing time
   auto lag = std::chrono::system_clock::now() - event_timestamp;
   // Alert if lag > 5 minutes
   ```

## Recommended Implementation Phases

### Phase 1: Immediate Fixes (Critical)
1. ✅ Change queue to durable, non-exclusive, non-auto-delete
2. ✅ Implement manual ACK
3. ✅ Add basic retry logic (NACK with requeue)
4. ⚠️ Current operations are idempotent (no deduplication needed yet)

### Phase 2: Production Hardening (High Priority)
1. Add Dead Letter Exchange/Queue configuration
2. Implement max retry limit with DLQ routing
3. Add prefetch QoS (process one message at a time)
4. Implement retry count tracking from x-death header
5. Add consumer reconnection logic (if connection drops)

### Phase 3: Observability (Medium Priority)
1. Add metrics collection (events processed, failed, lag)
2. Implement health check endpoint (consumer connected?)
3. Add structured logging with correlation IDs
4. Set up monitoring/alerting

### Phase 4: Advanced Features (Future)
1. Implement exponential backoff for retries
2. Add circuit breaker for database failures
3. Implement batch processing for high throughput
4. Add consumer auto-scaling based on queue depth

## Testing Strategies

### 0. Multi-Service Event Fanout Testing

```bash
# Test: Both inventory-service AND order-service receive same event

# 1. Start both services
docker-compose up -d inventory-service order-service product-service rabbitmq

# 2. Verify both queues exist
docker-compose exec rabbitmq rabbitmqctl list_queues name | grep -E "(inventory|order)-service-products"
# Expected:
# inventory-service-products	0
# order-service-products	0

# 3. Verify both queues bound to same routing keys
docker-compose exec rabbitmq rabbitmqctl list_bindings | grep "product.created"
# Expected:
# warehouse.events	exchange	inventory-service-products	queue	product.created	[]
# warehouse.events	exchange	order-service-products	queue	product.created	[]

# 4. Create a product (publishes product.created event)
curl -X POST http://localhost:8081/api/v1/products \
  -H "Content-Type: application/json" \
  -d '{
    "sku": "TEST-FANOUT-001",
    "name": "Test Fanout Product",
    "description": "Testing multi-service event delivery"
  }'

# 5. Check inventory-service logs
docker-compose logs inventory-service | grep "product.created"
# Expected: "Processing message: product.created"
# Expected: "Product cache updated: TEST-FANOUT-001"

# 6. Check order-service logs
docker-compose logs order-service | grep "product.created"
# Expected: "Processing message: product.created"
# Expected: "Product cache updated: TEST-FANOUT-001"

# 7. Verify BOTH caches updated
docker-compose exec inventory-service psql -U warehouse -d inventory_db \
  -c "SELECT * FROM product_cache WHERE sku='TEST-FANOUT-001';"
# Expected: 1 row

docker-compose exec order-service psql -U warehouse -d order_db \
  -c "SELECT * FROM product_cache WHERE sku='TEST-FANOUT-001';"
# Expected: 1 row

# 8. CRITICAL: Verify message counts (both processed same event)
docker-compose exec rabbitmq rabbitmqctl list_queues name messages_ready messages_unacknowledged
# Expected:
# inventory-service-products	0	0  (processed, ACK'd)
# order-service-products	0	0  (processed, ACK'd)

# ✅ SUCCESS: BOTH services received and processed the SAME event independently
```

### 1. Resilience Testing

```bash
# Test 1: Multiple consumers (same service, competing consumers pattern)
docker-compose up -d --scale inventory-service=3

# Send events
# Verify:
# - All 3 instances connected to SAME queue (inventory-service-products)
# - Messages distributed across instances (round-robin load balancing)
# - NO duplicate processing (each message to ONE instance only)
# - Check consumers: docker-compose exec rabbitmq rabbitmqctl list_consumers

# Test 1b: Multiple SERVICES (different services, fanout pattern)
docker-compose up -d inventory-service order-service

# Send single product.created event via product-service
# Verify:
# - inventory-service processes it (check logs + inventory_db.product_cache)
# - order-service ALSO processes it (check logs + order_db.product_cache)
# - BOTH services processed SAME event independently ✅
# - Each service's queue now empty (both ACK'd)

# Test 2: Consumer crash during processing
# Add sleep to handler, kill process mid-processing
# Verify: Message redelivered and processed by different instance

# Test 3: Database unavailable
docker-compose stop postgres
# Send events
# Verify: Messages NACK'd and requeued, not lost

# Test 4: RabbitMQ restart
docker-compose restart rabbitmq
# Verify: Queue persists, accumulated messages safe

# Test 5: Network partition
# Use iptables to drop packets
# Verify: Consumer reconnects, no messages lost
```

### 2. Idempotency Testing

```bash
# Publish same event twice
curl -X POST product-service/api/v1/products/[id]/publish-created

# Verify: Cache updated once, final state correct
psql -c "SELECT * FROM product_cache WHERE product_id = '[id]';"
```

## Summary: Answering Your Questions

| Question | Current Implementation | Recommended Solution |
|----------|----------------------|---------------------|
| **Multiple SERVICES consuming same events?** | ✅ Already correct! (service-specific queue names) | ✅ Each service uses unique queue name → all get copies (fanout) |
| **Multiple INSTANCES of same service?** | ❌ Second instance FAILS (exclusive queue) | ✅ Remove exclusive flag → competing consumers (load balanced, HA) |
| **Server error during processing?** | ❌ Message LOST (auto-ack) | ✅ Manual ACK + NACK on error + retry with DLQ |
| **Server unavailable?** | ⚠️ Messages accumulate but queue not durable | ✅ Durable queue + persistent messages → survive restarts |
| **Message processed twice?** | ⚠️ Possible, no deduplication | ✅ Operations are idempotent (UPSERT/DELETE safe) |
| **Permanent failure (bad data)?** | ❌ Message lost, logged | ✅ Dead letter queue after max retries |

### Critical Distinction

**Fanout (Multiple Services):**
```
Different queue names = Each service gets copy
✅ inventory-service-products
✅ order-service-products
Result: BOTH get product.created event
```

**Competing Consumers (Multiple Instances):**
```
Same queue name = Load balanced
✅ inventory-service-products (shared by 3 instances)
Result: ONE instance processes each message
```

**Combined Example:**
```
Scale inventory-service: 3 instances
Scale order-service: 2 instances

product.created published
├─> inventory-service-products queue
│   ├─> ONE of 3 instances processes it
├─> order-service-products queue
│   └─> ONE of 2 instances processes it

Total: Event processed 2 times (once per SERVICE, by one instance per service)
```

## Next Steps

1. **Immediate**: Fix crash loop (separate issue)
2. **Then**: Implement Phase 1 critical fixes in RabbitMqMessageConsumer
3. **Test**: Resilience scenarios
4. **Roll out**: Phase 2 and 3 incrementally
