# Event Distribution Patterns: Fanout vs Competing Consumers

Quick reference for understanding how RabbitMQ delivers events in a multi-service architecture.

---

## Pattern 1: Fanout (Multiple Services)

**Goal**: Every service that cares about an event should receive it.

### Configuration

```cpp
// inventory-service
config.queue_name = "inventory-service-products";  // ← UNIQUE name

// order-service  
config.queue_name = "order-service-products";      // ← UNIQUE name

// Both bind to same routing keys
config.routing_keys = {"product.created", "product.updated", "product.deleted"};
```

### Flow Diagram

```
product-service publishes product.created
              ↓
    warehouse.events (exchange)
              ↓
       ┌──────┴──────┐
       ↓             ↓
   Queue A       Queue B
inventory-service- order-service-
   products        products
       ↓             ↓
  inventory-     order-
   service       service
```

### Result

✅ **Both** services receive the event  
✅ **Both** can process it independently  
✅ **If one service is down**, its queue accumulates messages  
✅ **No impact** between services

### Use Cases

- Product cache updates (inventory + order + shipment services all need product data)
- Warehouse updates (inventory + order services need warehouse data)  
- Order events (shipment + notification + analytics services all need order events)

---

## Pattern 2: Competing Consumers (Multiple Instances)

**Goal**: Load balance message processing across multiple instances of the SAME service.

### Configuration

```cpp
// All 3 inventory-service instances use:
config.queue_name = "inventory-service-products";  // ← SAME name
config.routing_keys = {"product.created", "product.updated", "product.deleted"};

// CRITICAL:
durable: 1,
exclusive: 0,     // ← Must be NON-exclusive
auto_delete: 0
```

### Flow Diagram

```
product-service publishes product.created
              ↓
    warehouse.events (exchange)
              ↓
    inventory-service-products
           (Queue)
              ↓
     Round-robin distribution
         ┌────┼────┐
         ↓    ↓    ↓
     instance instance instance
        1       2       3
```

### Result

✅ **Only ONE** instance processes each message  
✅ **Load balanced** automatically by RabbitMQ  
✅ **High availability** (if instance 1 dies, instances 2 & 3 continue)  
✅ **Horizontal scaling** (add more instances = more throughput)

### Use Cases

- Scale service for high throughput
- High availability (redundancy)
- Rolling deployments (zero downtime)

---

## Pattern 3: Combined (Multiple Services + Multiple Instances)

**Real-world scenario**: You have multiple services AND multiple instances of each service.

### Configuration

```cpp
// inventory-service (3 instances, all use same queue name)
config.queue_name = "inventory-service-products";

// order-service (2 instances, all use same queue name)  
config.queue_name = "order-service-products";

// shipment-service (1 instance)
config.queue_name = "shipment-service-orders";
config.routing_keys = {"order.created", "order.shipped"};
```

### Flow Diagram

```
product-service publishes product.created
              ↓
    warehouse.events (exchange)
              ↓
       ┌──────┴──────┐
       ↓             ↓
inventory-service- order-service-
   products        products
   (Queue)        (Queue)
       ↓             ↓
  Load balance   Load balance
   ┌───┼───┐       ┌─┴─┐
   ↓   ↓   ↓       ↓   ↓
 inv-1 2   3     ord-1  2
   
ONE of 3 processes  ONE of 2 processes
```

### Result

✅ **inventory-service** processes event ONCE (via one of 3 instances)  
✅ **order-service** processes event ONCE (via one of 2 instances)  
✅ **Total**: Event processed **2 times** (once per SERVICE)  
✅ **Each service scales independently**

---

## Common Mistakes

### ❌ Mistake 1: Shared Queue Across Services

```cpp
// inventory-service
config.queue_name = "product-events";  // ❌ Same name

// order-service  
config.queue_name = "product-events";  // ❌ Same name
```

**Result**: Competing consumers across SERVICES  
- inventory-service gets 50% of events ❌  
- order-service gets 50% of events ❌  
- Both services miss half the events ❌

### ❌ Mistake 2: Exclusive Queue with Multiple Instances

```cpp
amqp_queue_declare(
    ...,
    1,  // exclusive: true ❌
    ...
);

// Scale to 3 instances
docker-compose up --scale inventory-service=3
```

**Result**:  
- First instance connects ✅  
- Second instance FAILS (queue is exclusive) ❌  
- Third instance FAILS ❌  
- Cannot scale horizontally ❌

### ❌ Mistake 3: Dynamic Queue Names

```cpp
// DON'T generate random queue names
config.queue_name = "inventory-service-products-" + uuid();  // ❌
```

**Result**: Every instance creates unique queue  
- All instances receive ALL events (not load balanced) ❌  
- No competing consumers pattern ❌  
- Duplicate processing ❌

---

## Queue Naming Convention

✅ **Correct Pattern**:

```
{service-name}-{entity-type}

Examples:
- inventory-service-products
- inventory-service-warehouses  
- order-service-products
- order-service-orders
- shipment-service-orders
- notification-service-orders
```

**Why This Works**:
1. **Service-specific prefix** (inventory-service) → Different services use different queues → Fanout ✅
2. **Entity-type suffix** (products) → Same service consuming multiple event types uses multiple queues ✅  
3. **Hardcoded (not generated)** → Multiple instances share same queue → Competing consumers ✅

---

## Quick Decision Tree

```
Do multiple SERVICES need this event?
├─ YES: Use different queue names per service (fanout)
│  └─ inventory-service-products, order-service-products
│
└─ NO: Single service only
   └─ Will you scale horizontally (multiple instances)?
      ├─ YES: Use non-exclusive queue (competing consumers)
      │  └─ exclusive: 0, durable: 1, auto_delete: 0
      │
      └─ NO: Either config works (but use non-exclusive for future-proofing)
```

---

## Testing Your Configuration

### Test 1: Verify Fanout (Multiple Services)

```bash
# 1. Start services
docker-compose up -d inventory-service order-service

# 2. List queues (should see both)
docker-compose exec rabbitmq rabbitmqctl list_queues name
# Expected:
# inventory-service-products
# order-service-products

# 3. Publish ONE event
curl -X POST product-service/api/v1/products -d '{...}'

# 4. Check BOTH queues processed it (messages = 0 after ACK)
docker-compose exec rabbitmq rabbitmqctl list_queues name messages
# Expected:
# inventory-service-products  0
# order-service-products      0

# 5. Check BOTH databases updated
psql inventory_db -c "SELECT COUNT(*) FROM product_cache;"  # 1
psql order_db -c "SELECT COUNT(*) FROM product_cache;"      # 1

✅ SUCCESS: Both services received same event
```

### Test 2: Verify Competing Consumers (Multiple Instances)

```bash
# 1. Scale inventory-service to 3 instances
docker-compose up -d --scale inventory-service=3

# 2. Verify all connected to SAME queue
docker-compose exec rabbitmq rabbitmqctl list_consumers | grep inventory-service-products
# Expected: 3 consumers on same queue

# 3. Publish 10 events
for i in {1..10}; do
  curl -X POST product-service/api/v1/products -d "{\"sku\":\"TEST-$i\"}"
done

# 4. Check processing distribution
docker-compose logs inventory-service | grep "Processing message" | grep -oP "inventory-service-\d+" | sort | uniq -c
# Expected: Events distributed across instances (e.g., 3, 4, 3)

# 5. Verify total processing count
psql inventory_db -c "SELECT COUNT(*) FROM product_cache WHERE sku LIKE 'TEST-%';"
# Expected: 10 (each event processed ONCE, not 30)

✅ SUCCESS: Load balanced correctly
```

### Test 3: Verify Combined Pattern

```bash
# 1. Scale both services
docker-compose up -d --scale inventory-service=3 --scale order-service=2

# 2. Publish ONE event
curl -X POST product-service/api/v1/products -d '{...}'

# 3. Verify processing counts
docker-compose logs inventory-service | grep "Processing message" | wc -l  # 1
docker-compose logs order-service | grep "Processing message" | wc -l     # 1

# 4. Verify both databases updated
psql inventory_db -c "SELECT COUNT(*) FROM product_cache;"  # 1
psql order_db -c "SELECT COUNT(*) FROM product_cache;"      # 1

✅ SUCCESS: Event delivered to both SERVICES, processed once per service
```

---

## Summary Table

| Configuration | Use Case | Queue Names | Result |
|---------------|----------|-------------|--------|
| **Different queue names** | Multiple services | `inv-svc-products`, `order-svc-products` | Fanout: Both receive ✅ |
| **Same queue name, exclusive=0** | Multiple instances | `inv-svc-products` (all instances) | Competing: One receives ✅ |
| **Same queue name, exclusive=1** | Single instance only | `inv-svc-products` | Second instance fails ❌ |
| **Random queue names** | NEVER | `inv-svc-products-abc123` | All instances receive ❌ |

---

## Current Implementation Status

### inventory-service ✅

```cpp
config_.queue_name = "inventory-service-products";  // ✅ Service-specific
config_.routing_keys = {"product.created", "product.updated", "product.deleted"};
```

**Status**: 
- ✅ Correctly supports fanout (unique name)
- ❌ Currently exclusive (cannot scale horizontally) → Fix required

### order-service (To Be Implemented)

```cpp
// Recommended configuration:
config_.queue_name = "order-service-products";  // ✅ Different from inventory
config_.routing_keys = {"product.created", "product.updated", "product.deleted"};  // ✅ Same routing keys

// Queue settings:
durable: 1,
exclusive: 0,  // ✅ Allow scaling
auto_delete: 0
```

**Result**: Both services receive ALL product events independently ✅

---

## References

- [EVENT_CONSUMPTION_ARCHITECTURE.md](./EVENT_CONSUMPTION_ARCHITECTURE.md) - Complete implementation guide
- [CONSUMER_RESILIENCE_CHECKLIST.md](./CONSUMER_RESILIENCE_CHECKLIST.md) - Implementation checklist
- RabbitMQ Competing Consumers: https://www.rabbitmq.com/consumers.html
- RabbitMQ Exchanges and Routing: https://www.rabbitmq.com/tutorials/tutorial-four-python.html
