# Architecture Documentation

## System Overview

The Warehouse Management Solution is a microservices-based system designed for scalability, maintainability, and high performance.

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                         Client Layer                             │
├─────────────────────────────────┬───────────────────────────────┤
│     Tablet PWA (Vue 3)         │   Office Web App (Vue 3)      │
│   - Touch optimized            │   - Desktop optimized         │
│   - Offline support            │   - Advanced features         │
│   - PWA features               │   - Reporting                 │
└─────────────────────────────────┴───────────────────────────────┘
                               │
                               │ HTTPS/WSS
                               │
┌──────────────────────────────▼────────────────────────────────┐
│                      API Gateway (C++)                        │
│   - Authentication/Authorization                              │
│   - Request routing                                          │
│   - Rate limiting                                            │
│   - Load balancing                                           │
└──────────────────────────────┬────────────────────────────────┘
                               │
                               │ Internal HTTP/gRPC
                               │
┌──────────────────────────────┴────────────────────────────────┐
│                    Microservices Layer                         │
├────────────────────────────────────────────────────────────────┤
│  C++ Services (High Performance)                               │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐          │
│  │  Inventory   │ │    Order     │ │  Warehouse   │          │
│  │   Service    │ │   Service    │ │   Service    │          │
│  └──────────────┘ └──────────────┘ └──────────────┘          │
│                                                                 │
│  C# Services (Business Logic)                                  │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐          │
│  │  Reporting   │ │ Notification │ │ Integration  │          │
│  │   Service    │ │   Service    │ │   Service    │          │
│  └──────────────┘ └──────────────┘ └──────────────┘          │
└────────────────────────────────────────────────────────────────┘
                               │
                               │
┌──────────────────────────────┴────────────────────────────────┐
│                     Infrastructure Layer                       │
├────────────────────────────────────────────────────────────────┤
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐     │
│  │PostgreSQL│  │  Redis   │  │ RabbitMQ │  │  Backup  │     │
│  │ Database │  │  Cache   │  │  Queue   │  │ Storage  │     │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘     │
└────────────────────────────────────────────────────────────────┘
```

## Design Principles

### 1. Contract-First Design
- **Business Entity Contracts**: Centralized JSON Schema definitions serve as the single source of truth
- **Cross-Service Consistency**: All services validate against the same entity schemas
- **Static Validation**: Type-safe validation in C++, C#, and TypeScript
- **Versioned Evolution**: Schema versioning supports backward compatibility
- **Boundary Clarity**: Contracts define external inputs/outputs; internal DB/model naming can differ if requests/DTOs map correctly
- See [contracts/README.md](../contracts/README.md) for details

### 2. Microservices Architecture
- **Single Responsibility**: Each service has a specific business domain
- **Independence**: Services can be developed, deployed, and scaled independently
- **Technology Diversity**: Use the best tool for each job (C++ for performance, C# for business logic)

### 3. API Gateway Pattern
- Centralized entry point for all client requests
- Handles cross-cutting concerns (auth, logging, rate limiting)
- Simplifies client-side logic

### 4. Event-Driven Communication
- Services communicate via message queue (RabbitMQ)
- Asynchronous processing for non-critical operations
- Event sourcing for audit trails

### 5. CQRS (Command Query Responsibility Segregation)
- Separate read and write operations
- Optimized read models for reporting
- Better performance and scalability

### 6. Database Per Service
- Each microservice owns its data
- Prevents tight coupling
- Allows independent scaling

## Business Entity Contracts

### Overview

The system uses **centralized JSON Schema contracts** as the single source of truth for all business entities. These contracts ensure consistency across all services and applications, regardless of the programming language used.

**Location**: `contracts/schemas/v1/`

**Core Entities**:
- **Product**: Product/SKU master data
- **Inventory**: Stock levels and tracking
- **Order**: Customer orders and line items
- **Warehouse**: Warehouse facility information
- **Location**: Storage locations within warehouses
- **User**: System users and permissions
- **Shipment**: Outbound shipments and deliveries
- **Common**: Shared types (UUID, Money, Address, etc.)

### Benefits

1. **Cross-Language Consistency**
   - Same entity definitions across C++, C#, and TypeScript
   - Validation libraries available for all languages
   - Reduces integration bugs

2. **Static Validation**
   - C++ services validate JSON against schemas at runtime
   - C# services can generate strongly-typed classes
   - TypeScript types generated from schemas

3. **Documentation**
   - Self-documenting API contracts
   - Examples provided for each entity
   - Clear field descriptions and constraints

4. **Versioning**
   - Schema versions tracked (v1, v2, etc.)
   - Breaking changes require new version
   - Services can support multiple versions during migration

### Usage in Services

**C++ Services (Inventory, Order, Warehouse)**:
```cpp
// Use nlohmann/json-schema-validator
#include <nlohmann/json-schema.hpp>

json_validator validator;
validator.set_root_schema(product_schema);
validator.validate(product_data); // Throws if invalid
```

**C# Services (Reporting, Notification, Integration)**:
```csharp
// Use NJsonSchema for validation or code generation
var schema = await JsonSchema.FromFileAsync("product.schema.json");
var errors = schema.Validate(productData);

// Or generate C# classes
var generator = new CSharpGenerator(schema);
var code = generator.GenerateFile("Product");
```

**Vue 3 Applications (Tablet PWA, Office Web)**:
```typescript
// Use ajv for validation
import Ajv from 'ajv';
import productSchema from '@/contracts/schemas/v1/product.schema.json';

const ajv = new Ajv();
const validate = ajv.compile(productSchema);

// Generate TypeScript types
// npx json-schema-to-typescript contracts/schemas/v1/*.schema.json
```

### Service-Specific Extensions

Services may extend contracts with additional fields but **must not** modify or remove core fields:

```json
{
  "$schema": "https://json-schema.org/draft/2020-12/schema",
  "title": "Extended Product (Inventory Service)",
  "allOf": [
    { "$ref": "../../contracts/schemas/v1/product.schema.json" },
    {
      "type": "object",
      "properties": {
        "binLocations": {
          "type": "array",
          "items": { "type": "string" }
        }
      }
    }
  ]
}
```

### Contract Evolution

- **Minor changes** (adding optional fields): Update existing version
- **Breaking changes** (removing fields, changing types): Create new version
- Deprecate old versions gradually with migration period
- Document all changes in contract README

For detailed usage and examples, see [contracts/README.md](../contracts/README.md).

#### Database Scaling and High Availability

**Per-Service Database Instances**

Each microservice has its own dedicated database instance or schema:

```
Inventory Service → inventory_db (PostgreSQL)
Order Service     → order_db (PostgreSQL)
Warehouse Service → warehouse_db (PostgreSQL)
Reporting Service → reporting_db (PostgreSQL + read replicas)
```

**Handling Database as Single Point of Failure**

1. **Primary-Replica Replication (Read Scaling)**
   - Each service database has a primary (write) and multiple replicas (read)
   - Write operations go to primary
   - Read operations distributed across replicas
   - Automatic failover if primary fails
   - Example: PostgreSQL streaming replication with Patroni

2. **Database Clustering**
   - Multi-master setups for write scalability
   - Examples: PostgreSQL with Citus, CockroachDB, YugabyteDB
   - Provides both high availability and horizontal write scaling
   - No single point of failure

3. **Database Connection Pooling**
   - PgBouncer or built-in connection pooling
   - Prevents connection exhaustion
   - Improves performance and reliability

4. **Automated Failover**
   - Health checks detect primary database failure
   - Automatic promotion of replica to primary
   - Kubernetes StatefulSets with operators (e.g., Zalando Postgres Operator)
   - Cloud-managed databases (AWS RDS Multi-AZ, Azure Database for PostgreSQL)

**Production Database Architecture Per Service:**

```
Service → Connection Pool → Load Balancer → Primary DB (writes)
                                         └→ Replica 1 (reads)
                                         └→ Replica 2 (reads)
```

#### Handling Eventual Consistency

**Challenge**: When data is split across service databases, maintaining consistency is complex.

**Strategies Employed:**

1. **Event-Driven Architecture (Primary Strategy)**
   - Services publish domain events to message queue (RabbitMQ)
   - Other services subscribe and update their local data
   - Each service maintains denormalized data it needs
   - Example:
     ```
     Order Service creates order → publishes OrderCreated event
     Inventory Service subscribes → reserves stock in its DB
     Reporting Service subscribes → updates its read model
     ```

2. **Saga Pattern for Distributed Transactions**
   - Choreography-based sagas: Services react to events
   - Orchestration-based sagas: Central coordinator manages flow
   - Compensating transactions for rollback
   - Example: Order fulfillment saga
     ```
     1. Create Order → Success
     2. Reserve Inventory → Success
     3. Process Payment → Failure
     4. Compensate: Release Inventory → Success
     5. Compensate: Cancel Order → Success
     ```

3. **CQRS with Event Sourcing**
   - All state changes stored as events
   - Current state derived by replaying events
   - Reporting Service builds read models from event stream
   - Provides complete audit trail
   - Enables time-travel debugging

4. **Optimistic Locking**
   - Version numbers on entities
   - Concurrent updates detected and handled
   - Retry logic with exponential backoff

5. **Idempotent Operations**
   - All operations designed to be safely retried
   - Idempotency keys for duplicate detection
   - Prevents double-processing of events

**Eventual Consistency Trade-offs:**

✅ **Benefits:**
- Better availability (services don't block on each other)
- Better performance (no distributed transactions)
- Easier to scale horizontally
- Services remain independent

⚠️ **Challenges:**
- Stale reads possible (data not immediately consistent)
- More complex error handling
- Requires careful business process design

**Mitigating Eventual Consistency Issues:**

1. **Bounded Staleness**
   - Define acceptable staleness windows (e.g., <100ms)
   - Monitor replication lag
   - Alert if consistency SLA violated

2. **Read-Your-Writes Consistency**
   - After write, reads go to primary briefly
   - Session affinity to same replica
   - Client-side caching with cache invalidation

3. **Conflict Resolution Strategies**
   - Last-Write-Wins (with timestamps)
   - Application-specific merge logic
   - Manual conflict resolution for critical data

4. **Business Process Adaptation**
   - Design UI to show "processing" states
   - Use optimistic UI updates
   - Clear communication about async operations
   - Example: "Your order is being processed..."

**Monitoring Consistency:**

- Track event processing latency
- Monitor replication lag across databases
- Alert on failed event deliveries
- Dashboard showing cross-service consistency metrics

**Example: Inventory Update Flow**

```
1. Tablet PWA: Picker confirms item picked
   ├─→ Order Service: Update pick status (local DB)
   ├─→ Publish: ItemPicked event
   
2. Inventory Service: Subscribes to ItemPicked
   ├─→ Update stock level (local DB)
   ├─→ Publish: InventoryUpdated event
   
3. Reporting Service: Subscribes to both events
   ├─→ Update dashboard read model (local DB)
   
4. Notification Service: Subscribes to OrderCompleted
   ├─→ Send customer notification

Timeline: 0-50ms for event propagation
Consistency: Eventually consistent within 100ms
```

## Technology Choices

### Backend: C++ for Performance-Critical Services
**Rationale**:
- Low latency requirements for real-time inventory
- High throughput for order processing
- Efficient memory usage
- Direct hardware access

**Trade-offs**:
- Longer development time
- Steeper learning curve
- More complex debugging

### Backend: C# for Business Logic Services
**Rationale**:
- Rapid development for business logic
- Rich ecosystem (.NET libraries)
- Good for integrations and reporting
- Easier to maintain

**Trade-offs**:
- Higher memory footprint
- Slightly lower performance
- Platform considerations

### Frontend: Vue 3
**Rationale**:
- Progressive framework (scales from simple to complex)
- Excellent performance
- Composition API for better code organization
- Great PWA support
- Smaller bundle size than alternatives

**Trade-offs**:
- Smaller ecosystem than React
- Less corporate backing than Angular

## Service Descriptions

### API Gateway (C++)
- **Purpose**: Single entry point for all API requests
- **Technology**: C++ 20 with Poco or Crow framework
- **Responsibilities**:
  - Authentication and authorization
  - Request routing to microservices
  - Rate limiting and throttling
  - Request/response transformation
  - Logging and monitoring

### Inventory Service (C++)
- **Purpose**: Real-time inventory management
- **Technology**: C++ 20
- **Responsibilities**:
  - Track stock levels in real-time
  - Handle stock movements
  - Manage warehouse locations
  - Low stock alerts
  - Inventory reservations

### Order Service (C++)
- **Purpose**: Order processing and fulfillment
- **Technology**: C++ 20
- **Responsibilities**:
  - Order creation and validation
  - Order status tracking
  - Pick list generation
  - Order prioritization
  - Fulfillment orchestration

### Warehouse Service (C++)
- **Purpose**: Warehouse layout and operations
- **Technology**: C++ 20
- **Responsibilities**:
  - Warehouse zone management
  - Location management
  - Route optimization
  - Space utilization
  - Equipment tracking

### Reporting Service (C#)
- **Purpose**: Business intelligence and analytics
- **Technology**: .NET 8, Entity Framework Core
- **Responsibilities**:
  - Generate reports
  - Data aggregation
  - Historical analysis
  - KPI calculations
  - Export to various formats

### Notification Service (C#)
- **Purpose**: Multi-channel notifications
- **Technology**: .NET 8, MassTransit
- **Responsibilities**:
  - Email notifications
  - Push notifications
  - SMS alerts
  - In-app notifications
  - Notification templates

### Integration Service (C#)
- **Purpose**: Third-party system integrations
- **Technology**: .NET 8
- **Responsibilities**:
  - ERP integration
  - Shipping carrier APIs
  - E-commerce platforms
  - EDI processing
  - Data synchronization

## Data Flow Examples

### Order Creation Flow
1. User creates order in Office Web App
2. Request sent to API Gateway
3. Gateway authenticates and routes to Order Service
4. Order Service validates and creates order
5. Order Service publishes OrderCreated event to message queue
6. Inventory Service subscribes and reserves stock
7. Notification Service sends confirmation email
8. Real-time update sent to clients via WebSocket

### Picking Flow
1. Picker logs into Tablet PWA
2. PWA fetches pick lists from Order Service
3. Picker scans items using camera
4. Inventory Service updates stock in real-time
5. Changes propagated via WebSocket
6. Office dashboard shows live updates
7. On completion, Order Service updates order status
8. Notification sent to customer

## Security Architecture

### Authentication & Authorization
- **JWT Tokens**: Stateless authentication
- **OAuth 2.0**: Third-party authentication
- **Role-Based Access Control (RBAC)**: Fine-grained permissions
- **API Keys**: Service-to-service authentication

### Data Security
- **Encryption at Rest**: Database encryption
- **Encryption in Transit**: TLS 1.3 for all communications
- **Secret Management**: HashiCorp Vault or Azure Key Vault
- **Audit Logging**: All actions logged for compliance

### Network Security
- **Private Network**: Services in isolated network
- **API Gateway**: Only public-facing component
- **Firewall Rules**: Restrict inter-service communication
- **DDoS Protection**: Rate limiting and throttling

## Scalability

### Horizontal Scaling
- Services deployed in containers (Docker)
- Orchestrated with Kubernetes
- Auto-scaling based on metrics
- Load balancing across instances

### Database Scaling
- Read replicas for reporting
- Sharding for large datasets
- Connection pooling
- Caching layer (Redis)

### Caching Strategy
- **Application Cache**: Redis for frequently accessed data
- **CDN**: Static assets cached at edge
- **Browser Cache**: Client-side caching
- **Database Query Cache**: Materialized views

## Monitoring & Observability

### Metrics
- **Prometheus**: Metrics collection
- **Grafana**: Visualization
- **Custom Metrics**: Business KPIs

### Logging
- **Structured Logging**: JSON format
- **Centralized Logs**: ELK Stack or Loki
- **Log Levels**: DEBUG, INFO, WARN, ERROR, CRITICAL

### Tracing
- **Distributed Tracing**: Jaeger or Zipkin
- **Request Correlation**: Trace IDs across services
- **Performance Analysis**: Identify bottlenecks

### Alerting
- **PrometheusAlert Manager**: Alert routing
- **PagerDuty**: On-call management
- **Slack Integration**: Team notifications

## Deployment Architecture

### Container Strategy
- Each service in its own container
- Multi-stage builds for optimization
- Minimal base images (Alpine Linux)
- Security scanning (Trivy, Snyk)

### Orchestration
- **Kubernetes**: Production orchestration
- **Helm Charts**: Package management
- **GitOps**: ArgoCD for deployments
- **Service Mesh**: Istio for advanced networking

### CI/CD Pipeline
1. Code commit triggers pipeline
2. Run unit tests
3. Build containers
4. Run integration tests
5. Security scanning
6. Deploy to staging
7. Automated testing
8. Deploy to production (with approval)

## Disaster Recovery

### Backup Strategy
- **Database**: Daily full backups, hourly incrementals
- **Configuration**: Version controlled
- **Disaster Recovery**: Multi-region deployment
- **RTO**: 4 hours
- **RPO**: 1 hour

### High Availability
- **Database**: Primary-replica setup
- **Services**: Multiple replicas
- **Load Balancer**: Health checks and failover
- **Geographic Distribution**: Multi-region for critical services

## Future Considerations

### Potential Enhancements
- Machine learning for demand forecasting
- Computer vision for automated counting
- Robotics integration
- Blockchain for supply chain tracking
- Voice picking with speech recognition
- Augmented reality for warehouse navigation

### Technology Evolution
- Migrate to service mesh (Istio)
- Implement GraphQL API
- Add real-time analytics (Apache Flink)
- Enhanced AI/ML capabilities
- Edge computing for offline operations
