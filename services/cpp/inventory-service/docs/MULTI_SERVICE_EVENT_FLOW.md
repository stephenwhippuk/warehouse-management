# Multi-Service Event Flow - Visual Guide

## Your Exact Scenario

**Product-service publishes an event. Both inventory-service AND order-service need to consume it.**

---

## Current Architecture (Correct! ✅)

```
┌─────────────────┐
│ product-service │
│                 │
│ POST /products  │
└────────┬────────┘
         │
         │ 1. Save to database
         │ 2. Publish event
         │
         v
    ┌────────────────────────────┐
    │ RabbitMQ                   │
    │                            │
    │ Exchange: warehouse.events │
    │ Type: topic                │
    │                            │
    │ Event Details:             │
    │ - Key: product.created     │
    │ - Payload: {id, sku, ...}  │
    └──────┬────────────┬────────┘
           │            │
           │            │ Topic exchange routes to ALL matching queues
           │            │
     ┌─────v────┐   ┌──v──────┐
     │ Queue A  │   │ Queue B │
     │inventory-│   │ order-  │
     │ service- │   │ service-│
     │ products │   │ products│
     └─────┬────┘   └──┬──────┘
           │            │
           │            │ Each queue has a COPY of the event
           │            │
     ┌─────v───────────────────┐   ┌────v───────────────────┐
     │ inventory-service       │   │ order-service          │
     │                         │   │                        │
     │ Consumes from Queue A   │   │ Consumes from Queue B  │
     │                         │   │                        │
     │ ProductEventHandler:    │   │ ProductEventHandler:   │
     │ - Parses event          │   │ - Parses event         │
     │ - Validates data        │   │ - Validates data       │
     │ - Updates product_cache │   │ - Updates product_cache│
     │                         │   │                        │
     │ Database:               │   │ Database:              │
     │ inventory_db            │   │ order_db               │
     │   product_cache table   │   │   product_cache table  │
     └─────────────────────────┘   └────────────────────────┘
```

**Result**: Event processed by BOTH services independently ✅

---

## With Horizontal Scaling (Also Correct! ✅)

```
┌─────────────────┐
│ product-service │
└────────┬────────┘
         │
         v
    ┌────────────────┐
    │ warehouse.     │
    │ events         │
    └───┬────────┬───┘
        │        │
        │        └──────────────────────────────┐
        │                                       │
  ┌─────v────────┐                       ┌─────v────────┐
  │ Queue A      │                       │ Queue B      │
  │ inventory-   │                       │ order-       │
  │ service-     │                       │ service-     │
  │ products     │                       │ products     │
  └─────┬────────┘                       └─────┬────────┘
        │                                       │
        │ Competing Consumers                   │ Competing Consumers
        │ (Load Balanced)                       │ (Load Balanced)
        │                                       │
    ┌───┼────┬────┐                         ┌──┴───┐
    │   │    │    │                         │      │
    v   v    v    v                         v      v
┌─────┐ ┌─────┐ ┌─────┐                 ┌─────┐ ┌─────┐
│inv-1│ │inv-2│ │inv-3│                 │ord-1│ │ord-2│
└─────┘ └─────┘ └─────┘                 └─────┘ └─────┘
   |       |       |                       |       |
   └───────┴───────┴────> inventory_db    └───────┴────> order_db
                           product_cache               product_cache

ONE of 3 processes it                    ONE of 2 processes it
(RabbitMQ round-robin)                   (RabbitMQ round-robin)
```

**Result**: 
- Event delivered to 2 queues (fanout)
- Within each service, ONE instance processes it (competing consumers)
- Total: Event processed exactly TWICE (once per service)

---

## Message Flow Timeline

```
Time    Product-Service    RabbitMQ           Inventory-Service    Order-Service
────────────────────────────────────────────────────────────────────────────────
T+0ms   POST /products
        │
T+10ms  Save to DB ✓
        │
T+20ms  Publish event ────> Receive event
        │                   │
        │                   Copy to Queue A
        │                   Copy to Queue B
        │                   │
T+30ms  Return 201          │
                            ├──> Deliver ──────> Receive event
                            │                    Parse JSON
                            │                    Update cache
                            │                    
                            └──> Deliver ───────────────────────> Receive event
                                                                   Parse JSON
                                                                   Update cache
                                                                   
T+40ms                                     Send ACK ✓
                                                                   Send ACK ✓
                                           
T+50ms                      Remove from ──> Queue A empty
                            both queues   
                                          └─> Queue B empty
```

---

## Why Different Queue Names Matter

### ❌ WRONG: Shared Queue

```
If both services used the SAME queue name:

    warehouse.events
          │
          v
    ┌────────────┐
    │ product-   │  ← SINGLE QUEUE
    │ events     │
    └─────┬──────┘
          │
      ┌───┴────┐
      v        v
  inventory  order
  -service   -service

Messages distributed (competing consumers):
- inventory gets 50% ❌
- order gets 50% ❌
- Both services MISS half the events ❌
```

### ✅ CORRECT: Separate Queues

```
If each service uses UNIQUE queue name:

    warehouse.events
          │
      ┌───┴────┐
      v        v
  ┌────────┐ ┌────────┐
  │Queue A │ │Queue B │  ← SEPARATE QUEUES
  └───┬────┘ └───┬────┘
      v          v
  inventory    order
  -service     -service

Messages duplicated (fanout):
- inventory gets 100% ✅
- order gets 100% ✅
- Both services get ALL events ✅
```

---

## Configuration Comparison

### inventory-service (Current)

```cpp
// Current configuration
config_.queue_name = "inventory-service-products";  // ✅ Unique name
config_.routing_keys = {
    "product.created",
    "product.updated", 
    "product.deleted"
};

// Queue flags (NEED FIXING):
durable: 0,      // ❌ Change to 1
exclusive: 1,    // ❌ Change to 0
auto_delete: 1,  // ❌ Change to 0
no_ack: 1        // ❌ Change to 0
```

### order-service (When Implemented)

```cpp
// Recommended configuration
config_.queue_name = "order-service-products";  // ✅ Different name
config_.routing_keys = {
    "product.created",
    "product.updated", 
    "product.deleted"  // ✅ Same routing keys
};

// Queue flags (CORRECT):
durable: 1,      // ✅ Survives restart
exclusive: 0,    // ✅ Allows scaling
auto_delete: 0,  // ✅ Queue persists
no_ack: 0        // ✅ Manual ACK
```

**Key Points**:
- Different queue names ✅ (enables fanout)
- Same routing keys ✅ (both listen to same events)
- Same exchange name ✅ (warehouse.events)
- Non-exclusive queues ✅ (enables horizontal scaling)

---

## Real-World Example

### Scenario: Create Product "Widget Pro Max"

```
Step 1: API Call
───────────────
curl -X POST http://product-service/api/v1/products \
  -d '{
    "sku": "WPM-001",
    "name": "Widget Pro Max",
    "price": 99.99
  }'

Response: 201 Created
{
  "id": "a1b2c3d4-...",
  "sku": "WPM-001",
  "name": "Widget Pro Max"
}


Step 2: Event Published
────────────────────────
Event: product.created
Routing Key: product.created
Payload:
{
  "eventId": "e5f6g7h8-...",
  "eventType": "ProductCreated",
  "timestamp": "2026-02-14T23:45:00Z",
  "source": "product-service",
  "data": {
    "id": "a1b2c3d4-...",
    "sku": "WPM-001",
    "name": "Widget Pro Max",
    "price": 99.99
  }
}


Step 3: RabbitMQ Routing
─────────────────────────
warehouse.events exchange routes to:
✓ inventory-service-products queue
✓ order-service-products queue


Step 4: inventory-service Processes
────────────────────────────────────
1. Receives event from Queue A
2. Parses JSON payload
3. Executes:
   INSERT INTO product_cache (product_id, sku, name, cached_at)
   VALUES ('a1b2c3d4-...', 'WPM-001', 'Widget Pro Max', NOW())
   ON CONFLICT (product_id) DO UPDATE
   SET sku = EXCLUDED.sku, name = EXCLUDED.name, updated_at = NOW();
4. Sends ACK to RabbitMQ
5. Log: "Product cache updated: WPM-001"


Step 5: order-service Processes (Simultaneously)
─────────────────────────────────────────────────
1. Receives event from Queue B
2. Parses JSON payload
3. Executes:
   INSERT INTO product_cache (product_id, sku, name, cached_at)
   VALUES ('a1b2c3d4-...', 'WPM-001', 'Widget Pro Max', NOW())
   ON CONFLICT (product_id) DO UPDATE
   SET sku = EXCLUDED.sku, name = EXCLUDED.name, updated_at = NOW();
4. Sends ACK to RabbitMQ
5. Log: "Product cache updated: WPM-001"


Step 6: Final State
───────────────────
inventory_db.product_cache:
┌─────────────┬──────────┬─────────────────┬─────────────┐
│ product_id  │   sku    │      name       │  cached_at  │
├─────────────┼──────────┼─────────────────┼─────────────┤
│ a1b2c3d4-...│ WPM-001  │ Widget Pro Max  │ 2026-02-14..│
└─────────────┴──────────┴─────────────────┴─────────────┘

order_db.product_cache:
┌─────────────┬──────────┬─────────────────┬─────────────┐
│ product_id  │   sku    │      name       │  cached_at  │
├─────────────┼──────────┼─────────────────┼─────────────┤
│ a1b2c3d4-...│ WPM-001  │ Widget Pro Max  │ 2026-02-14..│
└─────────────┴──────────┴─────────────────┴─────────────┘

Both caches synchronized! ✅
```

---

## Summary

✅ **Your architecture is correct for multi-service consumption**

✅ **Each service gets its own queue with unique name**

✅ **RabbitMQ topic exchange routes copies to all matching queues**

✅ **Supports both fanout AND competing consumers simultaneously**

❌ **Only needs production hardening (durable, non-exclusive, manual ACK)**

---

## Next Steps

1. Fix crash loop (separate issue)
2. Implement Phase 1 resilience fixes:
   - durable: 1
   - exclusive: 0  
   - auto_delete: 0
   - no_ack: 0
3. Test multi-service event delivery
4. Implement order-service consumer (same pattern)
5. Scale horizontally (verify competing consumers)

The foundation is solid! 🎯
