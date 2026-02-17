# Database Scaling and Consistency Strategies

## Overview

This document details how the Warehouse Management Solution handles database scaling and eventual consistency challenges in a microservices architecture where each service owns its database.

## Database Per Service Pattern

### Why Database Per Service?

Each microservice has its own database instance to ensure:
- **Loose Coupling**: Services don't share database schemas
- **Independent Scaling**: Each database scales according to service needs
- **Technology Freedom**: Different services can use different database types
- **Fault Isolation**: Database issues don't cascade across services
- **Team Autonomy**: Teams can evolve schemas independently

### Database Allocation

```
┌─────────────────────┬──────────────────┬─────────────────┐
│ Service             │ Database         │ Primary Use     │
├─────────────────────┼──────────────────┼─────────────────┤
│ Inventory Service   │ inventory_db     │ Stock levels    │
│ Order Service       │ order_db         │ Orders, picks   │
│ Warehouse Service   │ warehouse_db     │ Locations       │
│ Reporting Service   │ reporting_db     │ Analytics       │
│ Notification Service│ notification_db  │ Templates, logs │
│ Integration Service │ integration_db   │ Sync state      │
└─────────────────────┴──────────────────┴─────────────────┘
```

## Preventing Database Single Point of Failure

### Strategy 1: Primary-Replica Replication

**Configuration:**
```
Primary (Write) ──┬──→ Replica 1 (Read)
                  ├──→ Replica 2 (Read)
                  └──→ Replica 3 (Read)
```

**Implementation:**
- PostgreSQL streaming replication
- Asynchronous replication for performance
- Synchronous replication for critical data

**Failover Process:**
1. Health check detects primary failure
2. Replica promoted to new primary (30-60 seconds)
3. Other replicas reconfigured to follow new primary
4. DNS/connection string updated
5. Services reconnect automatically

**Tools:**
- **Patroni**: Automates PostgreSQL HA with etcd/Consul
- **Stolon**: Kubernetes-native PostgreSQL HA
- **Cloud-managed**: AWS RDS Multi-AZ, Azure Database HA

**Example docker-compose with replication:**
```yaml
inventory-db-primary:
  image: postgres:15-alpine
  environment:
    POSTGRES_REPLICATION_MODE: master
    
inventory-db-replica1:
  image: postgres:15-alpine
  environment:
    POSTGRES_REPLICATION_MODE: slave
    POSTGRES_MASTER_HOST: inventory-db-primary
```

### Strategy 2: Database Clustering

**Distributed SQL Databases:**
- **CockroachDB**: Multi-region, strongly consistent
- **YugabyteDB**: PostgreSQL-compatible, globally distributed
- **Citus**: PostgreSQL extension for horizontal scaling

**Benefits:**
- No single point of failure
- Automatic rebalancing
- Transparent failover
- Horizontal write scaling

**Trade-offs:**
- Increased complexity
- Network latency for distributed transactions
- Higher cost

### Strategy 3: Connection Pooling

**Purpose:** Prevent connection exhaustion and improve reliability

**Implementation:**
- **PgBouncer**: Lightweight connection pooler
- **pgpool-II**: Advanced pooling with load balancing
- Application-level pooling (HikariCP, npgsql)

**Configuration:**
```yaml
pgbouncer:
  image: edoburu/pgbouncer
  environment:
    DB_HOST: inventory-db-primary
    POOL_MODE: transaction
    MAX_CLIENT_CONN: 1000
    DEFAULT_POOL_SIZE: 25
```

### Strategy 4: Automated Backup and Recovery

**Backup Strategy:**
```
Continuous WAL Archiving ──→ S3/Cloud Storage
     │
     ├─ Full Backup: Daily at 02:00 UTC
     ├─ Incremental: Every 6 hours
     └─ Point-in-Time Recovery: Any time within 30 days
```

**Tools:**
- **WAL-E / WAL-G**: PostgreSQL continuous archiving
- **pgBackRest**: Enterprise-grade backup solution
- **Cloud-native**: AWS RDS automated backups, Azure backup vault

**Recovery:**
```bash
# Point-in-time recovery example
wal-g backup-fetch /var/lib/postgresql/data LATEST
wal-g wal-fetch --until '2026-02-06 14:30:00'
```

### Strategy 5: Multi-Region Deployment

**Architecture:**
```
Region A (Primary)          Region B (Standby)
├─ inventory-db-primary  ──→ inventory-db-replica
├─ order-db-primary      ──→ order-db-replica
└─ warehouse-db-primary  ──→ warehouse-db-replica
```

**Disaster Recovery:**
- Cross-region replication (async)
- Automated failover to standby region
- RPO: 1-5 minutes, RTO: 5-15 minutes

## Handling Eventual Consistency

### Challenge

With distributed databases, immediate consistency is difficult:
- Network delays
- Service independence
- Performance requirements
- Availability priorities (CAP theorem)

### Solution: Event-Driven Architecture

**Core Principle:** Services communicate via events, not direct database access

**Event Flow:**
```
Service A
  └─ Update local DB
     └─ Publish Event ──→ Message Queue ──→ Service B
                                         └──→ Service C
                                         └──→ Service D
```

### Pattern 1: Event Sourcing

**Concept:** Store all state changes as immutable events

**Implementation:**
```
events table:
├─ event_id: uuid
├─ aggregate_id: uuid
├─ event_type: string
├─ event_data: jsonb
├─ timestamp: timestamptz
└─ version: integer

Current state = replay all events for aggregate
```

**Benefits:**
- Complete audit trail
- Temporal queries ("state at time X")
- Event replay for debugging
- Easy to add new read models

**Example:**
```sql
-- Event log
INSERT INTO events (aggregate_id, event_type, event_data) VALUES
  ('order-123', 'OrderCreated', '{"items": [...], "total": 299.99}'),
  ('order-123', 'PaymentReceived', '{"amount": 299.99}'),
  ('order-123', 'OrderShipped', '{"carrier": "FedEx"}');

-- Reconstruct current state
SELECT * FROM events WHERE aggregate_id = 'order-123' ORDER BY version;
```

### Pattern 2: Saga Pattern

**Purpose:** Coordinate distributed transactions across services

**Choreography-Based Saga:**
```
OrderService: CreateOrder
  └─ Publish: OrderCreated ──→ InventoryService: ReserveInventory
                                  └─ Publish: InventoryReserved ──→ PaymentService: ProcessPayment
                                                                        └─ Success ──→ ShippingService
                                                                        └─ Failure ──→ CompensatingTx
```

**Orchestration-Based Saga:**
```
SagaOrchestrator (centralized):
  1. Command → OrderService.CreateOrder
  2. Command → InventoryService.ReserveInventory
  3. Command → PaymentService.ProcessPayment
  4. If any fails → trigger compensating transactions
```

**Compensating Transactions:**
```javascript
// Saga definition
const orderFulfillmentSaga = {
  steps: [
    {
      action: createOrder,
      compensation: cancelOrder
    },
    {
      action: reserveInventory,
      compensation: releaseInventory
    },
    {
      action: processPayment,
      compensation: refundPayment
    }
  ]
};

// On failure, execute compensations in reverse order
```

### Pattern 3: CQRS (Command Query Responsibility Segregation)

**Concept:** Separate write model from read model

**Architecture:**
```
Write Side (Commands):
  OrderService ──→ order_db (normalized, transactional)
      │
      └─ Publish Events
            │
            ↓
Read Side (Queries):
  ReportingService ──→ reporting_db (denormalized, optimized for reads)
```

**Benefits:**
- Optimize each side independently
- Scale reads and writes separately
- Different database technologies (SQL vs NoSQL)
- Complex queries don't impact write performance

**Example:**
```javascript
// Write model (normalized)
orders table: { id, customer_id, status, created_at }
order_items table: { id, order_id, product_id, quantity }

// Read model (denormalized)
order_summary view: {
  order_id,
  customer_name,
  total_items,
  total_value,
  status,
  estimated_delivery
}
```

### Pattern 4: Distributed Cache

**Purpose:** Reduce database load and improve consistency window

**Implementation:**
```
Service Request
  └─ Check Cache (Redis)
     ├─ Hit: Return cached data (low latency)
     └─ Miss: Query database → Update cache
```

**Cache Invalidation:**
```javascript
// On database update
1. Update database
2. Publish event: "InventoryUpdated"
3. Other services: invalidate relevant cache entries
4. Next read: cache miss → fetch fresh data

// Cache-aside pattern with TTL
cache.set(key, value, ttl=60); // expires after 60 seconds
```

### Pattern 5: Idempotency and Deduplication

**Problem:** Events may be delivered multiple times

**Solution:** Make all operations idempotent

**Implementation:**
```javascript
// Idempotency key pattern
async function processOrder(orderId, idempotencyKey) {
  // Check if already processed
  const existing = await db.query(
    'SELECT * FROM processed_requests WHERE idempotency_key = $1',
    [idempotencyKey]
  );
  
  if (existing.rows.length > 0) {
    return existing.rows[0].result; // Return cached result
  }
  
  // Process the order
  const result = await createOrder(orderId);
  
  // Store idempotency record
  await db.query(
    'INSERT INTO processed_requests (idempotency_key, result) VALUES ($1, $2)',
    [idempotencyKey, result]
  );
  
  return result;
}
```

## Consistency Guarantees and Trade-offs

### Consistency Levels

**1. Strong Consistency** (rarely achievable in distributed systems)
- Read reflects all previous writes
- Highest latency, lowest availability
- Use for: Financial transactions, inventory reservations

**2. Eventual Consistency** (our primary approach)
- All replicas converge to same state eventually
- High availability, low latency
- Use for: Most operations, analytics, reporting

**3. Bounded Staleness**
- Guarantees maximum staleness (e.g., <100ms)
- Balance between strong and eventual
- Use for: User-facing dashboards, critical metrics

### Monitoring Consistency

**Key Metrics:**
```
- Event Processing Latency: Time from publish to consume
  Target: p99 < 100ms

- Database Replication Lag: Primary to replica delay
  Target: < 1 second

- Saga Completion Time: End-to-end distributed transaction
  Target: p95 < 5 seconds

- Failed Event Rate: Events that couldn't be processed
  Target: < 0.01%
```

**Alerting:**
```yaml
alerts:
  - name: HighReplicationLag
    condition: replication_lag > 5s
    severity: critical
    
  - name: EventProcessingDelay
    condition: event_latency_p99 > 500ms
    severity: warning
    
  - name: SagaFailureRate
    condition: saga_failure_rate > 1%
    severity: critical
```

## Real-World Example: Order Fulfillment

### Scenario
Customer places an order for 2 items. System must:
1. Create order
2. Reserve inventory
3. Process payment
4. Update dashboard
5. Send notification

### Implementation with Eventual Consistency

```
Timeline:
T+0ms:    Client → API Gateway → Order Service
T+10ms:   Order Service: Insert to order_db (order_id=123, status=created)
T+15ms:   Order Service: Publish OrderCreated(order_id=123)
T+20ms:   Inventory Service: Receive OrderCreated event
T+25ms:   Inventory Service: Reserve stock in inventory_db
T+30ms:   Inventory Service: Publish InventoryReserved(order_id=123)
T+35ms:   Payment Service: Receive InventoryReserved event
T+40ms:   Payment Service: Process payment
T+45ms:   Payment Service: Publish PaymentProcessed(order_id=123)
T+50ms:   Order Service: Receive PaymentProcessed → Update status=paid
T+55ms:   Reporting Service: Update dashboard read model
T+60ms:   Notification Service: Send confirmation email
T+100ms:  All services consistent

Consistency Window: 100ms
User Experience: "Order placed! Processing..."
```

### Handling Failures

**Scenario: Payment fails**
```
T+40ms:   Payment Service: Payment declined
T+45ms:   Payment Service: Publish PaymentFailed(order_id=123)
T+50ms:   Inventory Service: Release reserved inventory
T+55ms:   Order Service: Update status=payment_failed
T+60ms:   Notification Service: Send "payment failed" email

Result: System self-heals via compensating transactions
```

## Best Practices

### 1. Design for Failure
- Assume events can be lost, duplicated, or reordered
- Implement retry logic with exponential backoff
- Use dead-letter queues for failed events

### 2. Monitor and Alert
- Track end-to-end consistency metrics
- Alert on anomalies (high lag, failed events)
- Dashboard showing cross-service consistency

### 3. Test Eventual Consistency
- Chaos engineering: randomly delay/drop events
- Test compensating transactions
- Verify system converges to consistent state

### 4. Document Consistency Guarantees
- Clear SLAs for each operation
- User-facing messaging about async operations
- Team training on distributed systems concepts

### 5. Use Proven Tools
- Message queue: RabbitMQ, Kafka
- Event store: EventStore, PostgreSQL with JSONB
- Orchestration: Temporal, Apache Camel
- Monitoring: Prometheus, Grafana, Jaeger

## Summary

**Database Scaling:** Each service has dedicated database(s) with replication, clustering, and automated failover to eliminate single points of failure.

**Eventual Consistency:** Managed through event-driven architecture, saga pattern, CQRS, idempotency, and careful monitoring.

**Trade-off:** Accept slight delays in consistency (typically <100ms) in exchange for better availability, performance, and scalability.

**Result:** Highly available, scalable system that gracefully handles failures and maintains data consistency across distributed services.
