# Consumer Resilience Implementation Checklist

This checklist tracks the implementation of production-ready message consumption with error resilience.

## Status Legend
- ❌ Not started
- 🚧 In progress  
- ✅ Complete
- ⏸️ Blocked
- ⏭️ Deferred

---

## Phase 1: Critical Fixes (Required for Production)

### 1.1 Fix Queue Configuration
**Status**: ❌ Not started  
**Priority**: 🔴 CRITICAL  
**Estimated effort**: 30 minutes

**File**: `src/utils/RabbitMqMessageConsumer.cpp`

**Tasks**:
- [ ] Change `durable` from `0` to `1` (line 65)
  - Queue survives RabbitMQ restart
  - Messages not lost during maintenance
  
- [ ] Change `exclusive` from `1` to `0` (line 66)
  - Allow multiple consumer instances
  - Enable horizontal scaling
  - Enable high availability
  
- [ ] Change `auto_delete` from `1` to `0` (line 67)
  - Queue persists when no consumers connected
  - Messages accumulate safely during downtime

**Code change**:
```cpp
// OLD:
amqp_queue_declare(
    connection_, channel_,
    amqp_cstring_bytes(config_.queue_name.c_str()),
    0,  // passive
    0,  // durable: false ❌
    1,  // exclusive: true ❌
    1,  // auto_delete: true ❌
    amqp_empty_table
);

// NEW:
amqp_queue_declare(
    connection_, channel_,
    amqp_cstring_bytes(config_.queue_name.c_str()),
    0,  // passive
    1,  // durable: true ✅
    0,  // exclusive: false ✅
    0,  // auto_delete: false ✅
    amqp_empty_table
);
```

**Testing**:
```bash
# 1. Start service, create queue
docker-compose up -d inventory-service

# 2. Verify queue properties
docker-compose exec rabbitmq rabbitmqctl list_queues name durable exclusive auto_delete | grep inventory-service-products
# Expected: inventory-service-products	true	false	false

# 3. Stop service
docker-compose stop inventory-service

# 4. Verify queue still exists
docker-compose exec rabbitmq rabbitmqctl list_queues | grep inventory-service-products
# Expected: Queue still present

# 5. Restart RabbitMQ
docker-compose restart rabbitmq

# 6. Verify queue survives restart
docker-compose exec rabbitmq rabbitmqctl list_queues | grep inventory-service-products
# Expected: Queue still present with same settings
```

---

### 1.2 Implement Manual ACK
**Status**: ❌ Not started  
**Priority**: 🔴 CRITICAL  
**Estimated effort**: 1 hour

**File**: `src/utils/RabbitMqMessageConsumer.cpp`

**Tasks**:
- [ ] Change `no_ack` from `1` to `0` in `amqp_basic_consume()` (line 95)
- [ ] Add `amqp_basic_ack()` call after successful message processing
- [ ] Add `amqp_basic_nack()` call on processing failure
- [ ] Add retry count tracking
- [ ] Add retry limit check

**Code changes**:

```cpp
// In connect() method - line 95
// OLD:
amqp_basic_consume(
    connection_, channel_,
    amqp_cstring_bytes(config_.queue_name.c_str()),
    amqp_empty_bytes,
    0,  // no_local
    1,  // no_ack: true ❌
    1,  // exclusive
    amqp_empty_table
);

// NEW:
amqp_basic_consume(
    connection_, channel_,
    amqp_cstring_bytes(config_.queue_name.c_str()),
    amqp_empty_bytes,
    0,  // no_local
    0,  // no_ack: false ✅ Manual ACK required
    0,  // exclusive: false ✅
    amqp_empty_table
);
```

```cpp
// In consumeLoop() method - around line 182-195
// OLD:
try {
    json payload = json::parse(message_body);
    Logger::debug("Received message on routing key: {}", routing_key);
    handler(routing_key, payload);
} catch (const std::exception& e) {
    Logger::error("Error processing message: {}", e.what());
}
amqp_destroy_envelope(&envelope);

// NEW:
bool success = false;
int retry_count = 0;

try {
    json payload = json::parse(message_body);
    
    // Extract retry count from message headers (if present)
    retry_count = getRetryCount(envelope);
    
    Logger::debug("Processing message (attempt {}): {}", retry_count + 1, routing_key);
    
    // Call handler
    handler(routing_key, payload);
    success = true;
    
    // ACK on success
    amqp_basic_ack(connection_, channel_, envelope.delivery_tag, 0);
    Logger::debug("Message ACK'd successfully: {}", routing_key);
    
} catch (const std::exception& e) {
    Logger::error("Error processing message: {} (attempt {})", e.what(), retry_count + 1);
    
    if (retry_count < MAX_RETRIES) {
        // NACK with requeue=true (retry)
        Logger::warn("Message will be retried (attempt {}/{})", retry_count + 1, MAX_RETRIES);
        amqp_basic_nack(connection_, channel_, envelope.delivery_tag, 0, 1);
    } else {
        // NACK with requeue=false (send to DLQ if configured, or discard)
        Logger::error("Message failed after {} retries, discarding", MAX_RETRIES);
        amqp_basic_nack(connection_, channel_, envelope.delivery_tag, 0, 0);
    }
}

amqp_destroy_envelope(&envelope);
```

**Add helper method**:
```cpp
// In RabbitMqMessageConsumer.hpp
private:
    static constexpr int MAX_RETRIES = 3;
    int getRetryCount(const amqp_envelope_t& envelope);

// In RabbitMqMessageConsumer.cpp
int RabbitMqMessageConsumer::getRetryCount(const amqp_envelope_t& envelope) {
    // For now, return 0 (no retry tracking)
    // Phase 2 will implement proper x-death header parsing
    return 0;
}
```

**Testing**:
```bash
# 1. Start services
docker-compose up -d

# 2. Inject invalid event (will fail parsing)
docker-compose exec rabbitmq rabbitmqadmin publish exchange=warehouse.events routing_key=product.created payload='invalid json'

# Verify in logs:
# - Error logged
# - Message NACK'd with requeue
# - Message retried (up to MAX_RETRIES times)
# - After MAX_RETRIES, message discarded

# 3. Stop database during processing
docker-compose stop postgres
# Publish valid event
curl -X POST product-service/api/v1/products/...

# Verify:
# - Handler fails (DB connection error)
# - Message NACK'd with requeue
# - Message NOT lost

# 4. Restart database
docker-compose start postgres

# Verify:
# - Message redelivered automatically
# - Processing succeeds
# - Message ACK'd
```

---

### 1.3 Add QoS Prefetch
**Status**: ❌ Not started  
**Priority**: 🟡 MEDIUM  
**Estimated effort**: 15 minutes

**File**: `src/utils/RabbitMqMessageConsumer.cpp`

**Tasks**:
- [ ] Add `amqp_basic_qos()` call after `amqp_basic_consume()`
- [ ] Set `prefetch_count = 1`

**Purpose**: Process one message at a time. Don't fetch next message until current one is ACK'd or NACK'd.

**Code change**:
```cpp
// In connect() method, after amqp_basic_consume() - around line 100
amqp_basic_consume(...);
reply = amqp_get_rpc_reply(connection_);
if (!checkAmqpStatus(reply, "Basic consume")) {
    throw std::runtime_error("Failed to start consuming");
}

// ADD THIS:
amqp_basic_qos(
    connection_,
    channel_,
    0,  // prefetch_size (0 = no limit on message size)
    1,  // prefetch_count ✅ Process one at a time
    0   // global: false (per-consumer, not per-channel)
);
reply = amqp_get_rpc_reply(connection_);
if (!checkAmqpStatus(reply, "Basic QoS")) {
    Logger::warn("Failed to set QoS, continuing anyway");
}
```

**Impact**:
- Prevents consumer from being overwhelmed
- Ensures fair distribution across multiple consumers
- Better backpressure handling

---

## Phase 2: Production Hardening (High Priority)

### 2.1 Implement Dead Letter Queue
**Status**: ❌ Not started  
**Priority**: 🟡 MEDIUM  
**Estimated effort**: 1 hour

**Files**: 
- `src/utils/RabbitMqMessageConsumer.cpp`
- `migrations/deploy/003_setup_dlq.sql` (new)

**Tasks**:
- [ ] Create SQL migration to set up DLX and DLQ in RabbitMQ
- [ ] Modify queue declaration to include `x-dead-letter-exchange` argument
- [ ] Add `x-dead-letter-routing-key` argument
- [ ] Update docker-entrypoint.sh to declare DLX/DLQ via rabbitmqadmin

**Code change**:
```cpp
// In connect() method - replace amqp_empty_table with:
amqp_table_t queue_args;
amqp_table_entry_t entries[2];

// Dead Letter Exchange
entries[0].key = amqp_cstring_bytes("x-dead-letter-exchange");
entries[0].value.kind = AMQP_FIELD_KIND_UTF8;
entries[0].value.value.bytes = amqp_cstring_bytes("warehouse.dlx");

// Dead Letter Routing Key
entries[1].key = amqp_cstring_bytes("x-dead-letter-routing-key");
entries[1].value.kind = AMQP_FIELD_KIND_UTF8;
entries[1].value.value.bytes = amqp_cstring_bytes("inventory.product.failed");

queue_args.num_entries = 2;
queue_args.entries = entries;

amqp_queue_declare(
    connection_, channel_,
    amqp_cstring_bytes(config_.queue_name.c_str()),
    0, 1, 0, 0,
    queue_args  // ✅ Include DLX arguments
);
```

**docker-entrypoint.sh addition**:
```bash
# Before starting service
echo "Setting up Dead Letter Exchange and Queue..."
rabbitmqadmin -H rabbitmq -u warehouse -p warehouse_dev \
  declare exchange name=warehouse.dlx type=topic durable=true || true
rabbitmqadmin -H rabbitmq -u warehouse -p warehouse_dev \
  declare queue name=warehouse.dlq durable=true || true
rabbitmqadmin -H rabbitmq -u warehouse -p warehouse_dev \
  declare binding source=warehouse.dlx destination=warehouse.dlq routing_key="#" || true
```

**Testing**:
```bash
# 1. Publish message that will fail MAX_RETRIES times
# (e.g., malformed event that can't be parsed)

# 2. Verify message ends up in DLQ
docker-compose exec rabbitmq rabbitmqctl list_queues name messages | grep warehouse.dlq
# Expected: warehouse.dlq	1

# 3. Inspect dead letter
docker-compose exec rabbitmq rabbitmqadmin get queue=warehouse.dlq count=1
# Should show failed message with details
```

---

### 2.2 Implement Retry Count Tracking
**Status**: ❌ Not started  
**Priority**: 🟡 MEDIUM  
**Estimated effort**: 45 minutes

**File**: `src/utils/RabbitMqMessageConsumer.cpp`

**Tasks**:
- [ ] Implement `getRetryCount()` to parse `x-death` header
- [ ] Extract death count from AMQP table entries
- [ ] Use death count for retry logic

**Code**:
```cpp
int RabbitMqMessageConsumer::getRetryCount(const amqp_envelope_t& envelope) {
    if (envelope.message.properties._flags & AMQP_BASIC_HEADERS_FLAG) {
        amqp_table_t* headers = &envelope.message.properties.headers;
        
        for (int i = 0; i < headers->num_entries; i++) {
            std::string key((char*)headers->entries[i].key.bytes, headers->entries[i].key.len);
            
            if (key == "x-death") {
                // x-death is an array of tables
                if (headers->entries[i].value.kind == AMQP_FIELD_KIND_ARRAY) {
                    amqp_array_t death_array = headers->entries[i].value.value.array;
                    
                    if (death_array.num_entries > 0) {
                        // Get first death entry (most recent)
                        amqp_field_value_t* first_death = &death_array.entries[0];
                        
                        if (first_death->kind == AMQP_FIELD_KIND_TABLE) {
                            amqp_table_t* death_table = &first_death->value.table;
                            
                            // Find 'count' field
                            for (int j = 0; j < death_table->num_entries; j++) {
                                std::string death_key(
                                    (char*)death_table->entries[j].key.bytes,
                                    death_table->entries[j].key.len
                                );
                                
                                if (death_key == "count") {
                                    if (death_table->entries[j].value.kind == AMQP_FIELD_KIND_I64) {
                                        return static_cast<int>(death_table->entries[j].value.value.i64);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    return 0;  // No x-death header = first attempt
}
```

---

### 2.3 Add Consumer Reconnection Logic
**Status**: ❌ Not started  
**Priority**: 🟡 MEDIUM  
**Estimated effort**: 1 hour

**File**: `src/utils/RabbitMqMessageConsumer.cpp`

**Tasks**:
- [ ] Detect connection failures in consume loop
- [ ] Implement exponential backoff reconnection
- [ ] Resume consuming after reconnection

**Scenarios to handle**:
- Network partition
- RabbitMQ restart
- Connection timeout

---

## Phase 3: Observability (Medium Priority)

### 3.1 Add Metrics Collection
**Status**: ❌ Not started  
**Priority**: 🟢 LOW  
**Estimated effort**: 2 hours

**Tasks**:
- [ ] Add metrics structure to RabbitMqMessageConsumer
- [ ] Track: messages processed, failed, ACK'd, NACK'd
- [ ] Track: processing time (latency)
- [ ] Expose metrics via `/metrics` endpoint (Prometheus format)

---

### 3.2 Add Health Check
**Status**: ❌ Not started  
**Priority**: 🟢 LOW  
**Estimated effort**: 30 minutes

**Tasks**:
- [ ] Add `isHealthy()` method to MessageConsumer interface
- [ ] Check: connection alive, consumer active, no recent errors
- [ ] Expose via `/health` endpoint

---

### 3.3 Add Correlation ID Tracking
**Status**: ❌ Not started  
**Priority**: 🟢 LOW  
**Estimated effort**: 1 hour

**Tasks**:
- [ ] Extract `correlationId` from event payload
- [ ] Thread-local storage for correlation ID
- [ ] Include correlation ID in all log statements
- [ ] Pass correlation ID to handler

---

## Phase 4: Advanced Features (Future)

### 4.1 Exponential Backoff for Retries
**Status**: ⏭️ Deferred  
**Priority**: 🟢 LOW  
**Estimated effort**: 2 hours

**Approach**: Use delayed retry queue with increasing TTL

---

### 4.2 Circuit Breaker for Database
**Status**: ⏭️ Deferred  
**Priority**: 🟢 LOW  
**Estimated effort**: 3 hours

**Purpose**: Stop processing if database consistently fails

---

### 4.3 Batch Processing
**Status**: ⏭️ Deferred  
**Priority**: 🟢 LOW  
**Estimated effort**: 4 hours

**Purpose**: Process multiple events in single transaction for throughput

---

## Testing Checklist

### Resilience Tests
- [ ] **Test 1**: Multiple consumers (scale to 3 instances)
- [ ] **Test 2**: Consumer crash during processing (kill -9)
- [ ] **Test 3**: Database unavailable (stop postgres)
- [ ] **Test 4**: RabbitMQ restart (restart rabbitmq)
- [ ] **Test 5**: Network partition (iptables drop)
- [ ] **Test 6**: Malformed message (send invalid JSON)
- [ ] **Test 7**: Queue survives restart (stop service, verify queue exists)

### Idempotency Tests
- [ ] **Test 8**: Process same event twice (publish duplicate)
- [ ] **Test 9**: Consumer crash after processing, before ACK (verify no duplicate)

### Performance Tests
- [ ] **Test 10**: High throughput (1000 events/second)
- [ ] **Test 11**: Large messages (1MB payload)
- [ ] **Test 12**: Consumer lag recovery (backlog processing)

---

## Completion Criteria

**Phase 1 DONE when**:
- ✅ Queue is durable, non-exclusive, non-auto-delete
- ✅ Manual ACK implemented with retry logic
- ✅ QoS prefetch set to 1
- ✅ All Phase 1 tests pass
- ✅ Service can scale horizontally (multiple instances)
- ✅ Messages not lost on consumer crash

**Phase 2 DONE when**:
- ✅ DLQ configured and working
- ✅ Failed messages (after max retries) go to DLQ
- ✅ Retry count accurately tracked
- ✅ Consumer auto-reconnects on connection failure

**Phase 3 DONE when**:
- ✅ Metrics exposed via `/metrics` endpoint
- ✅ Health check available via `/health`
- ✅ All logs include correlation ID for tracing

---

## Notes

### Current Implementation Status (Before Starting)
- Queue: Not durable, exclusive, auto-delete ❌
- ACK: Auto-ack (before processing) ❌
- Retry: None (messages lost on error) ❌
- DLQ: Not configured ❌
- Scaling: Cannot scale (exclusive queue) ❌

### Target State (After Phase 1)
- Queue: Durable, non-exclusive, persistent ✅
- ACK: Manual (after successful processing) ✅
- Retry: Up to MAX_RETRIES with NACK requeue ✅
- DLQ: Not configured yet (Phase 2) ⏭️
- Scaling: Can scale horizontally ✅

### Dependencies
- Phase 2 depends on Phase 1 completion
- Phase 3 can run in parallel with Phase 2
- Phase 4 depends on Phase 1-3 completion

### Estimated Total Effort
- Phase 1: ~2 hours
- Phase 2: ~3 hours
- Phase 3: ~4 hours
- Phase 4: ~9 hours
- **Total**: ~18 hours
