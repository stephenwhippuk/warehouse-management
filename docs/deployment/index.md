# Deployment Guide

This guide covers deployment strategies for the Warehouse Management Solution.

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Local Development](#local-development)
3. [Docker Deployment](#docker-deployment)
4. [Kubernetes Deployment](#kubernetes-deployment)
5. [Cloud Deployments](#cloud-deployments)
6. [CI/CD Pipeline](#cicd-pipeline)
7. [Monitoring and Logging](#monitoring-and-logging)

## Prerequisites

### Required Software

- Docker 20.10+
- Docker Compose 2.0+
- Kubernetes 1.24+ (for production)
- kubectl CLI
- Helm 3.0+ (for Kubernetes deployments)

### Infrastructure Requirements

- **Minimum**:
  - 4 CPU cores
  - 8 GB RAM
  - 50 GB storage

- **Recommended Production**:
  - 16 CPU cores
  - 32 GB RAM
  - 500 GB SSD storage
  - Load balancer
  - Database cluster

## Local Development

### Quick Start

```bash
# Clone the repository
git clone https://github.com/stephenwhippuk/warehouse-management.git
cd warehouse-management

# Run setup script
./scripts/setup-dev.sh

# Start infrastructure
docker-compose up -d postgres redis rabbitmq
```

### Building Services

#### C++ Services
```bash
cd services/cpp/{service-name}
mkdir build && cd build
cmake ..
make -j$(nproc)
```

#### C# Services
```bash
cd services/csharp/{service-name}
dotnet build
dotnet run
```

#### Vue Applications
```bash
cd apps/{app-name}
npm install
npm run dev
```

## Docker Deployment

### Building Images

Build all images:
```bash
docker-compose build
```

Build specific service:
```bash
docker-compose build inventory-service
```

### Running Services

Start all services:
```bash
docker-compose up -d
```

Start specific services:
```bash
docker-compose up -d postgres redis rabbitmq inventory-service
```

### Environment Configuration

Create `.env` file in project root:
```env
# Database
POSTGRES_HOST=postgres
POSTGRES_PORT=5432
POSTGRES_DB=warehouse_db
POSTGRES_USER=warehouse
POSTGRES_PASSWORD=secure_password_here

# Redis
REDIS_HOST=redis
REDIS_PORT=6379

# RabbitMQ
RABBITMQ_HOST=rabbitmq
RABBITMQ_PORT=5672
RABBITMQ_USER=warehouse
RABBITMQ_PASSWORD=secure_password_here

# Services
API_GATEWAY_PORT=8080
INVENTORY_SERVICE_PORT=8081
ORDER_SERVICE_PORT=8082
WAREHOUSE_SERVICE_PORT=8083
REPORTING_SERVICE_PORT=8091

# Frontend
TABLET_PWA_PORT=3000
OFFICE_WEB_PORT=3001

# Security
JWT_SECRET=your_jwt_secret_here
JWT_EXPIRATION=3600

# Logging
LOG_LEVEL=info
```

### Health Checks

Check service health:
```bash
# API Gateway
curl http://localhost:8080/health

# Individual services
curl http://localhost:8081/health  # Inventory
curl http://localhost:8082/health  # Order
curl http://localhost:8091/health  # Reporting
```

## Kubernetes Deployment

### Cluster Setup

1. **Create namespace**:
```bash
kubectl create namespace warehouse
```

2. **Set context**:
```bash
kubectl config set-context --current --namespace=warehouse
```

### Using Helm Charts

1. **Add Helm repository** (once charts are published):
```bash
helm repo add warehouse https://charts.warehouse.example.com
helm repo update
```

2. **Install with Helm**:
```bash
helm install warehouse-mgmt warehouse/warehouse-management \
  --namespace warehouse \
  --values values-production.yaml
```

### Manual Kubernetes Deployment

Create Kubernetes manifests for each service:

**Example: Inventory Service Deployment**
```yaml
# k8s/inventory-service-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: inventory-service
  namespace: warehouse
spec:
  replicas: 3
  selector:
    matchLabels:
      app: inventory-service
  template:
    metadata:
      labels:
        app: inventory-service
    spec:
      containers:
      - name: inventory-service
        image: warehouse/inventory-service:latest
        ports:
        - containerPort: 8081
        env:
        - name: POSTGRES_HOST
          valueFrom:
            secretKeyRef:
              name: warehouse-secrets
              key: postgres-host
        livenessProbe:
          httpGet:
            path: /health
            port: 8081
          initialDelaySeconds: 30
          periodSeconds: 10
        readinessProbe:
          httpGet:
            path: /ready
            port: 8081
          initialDelaySeconds: 5
          periodSeconds: 5
```

**Example: Service**
```yaml
# k8s/inventory-service-service.yaml
apiVersion: v1
kind: Service
metadata:
  name: inventory-service
  namespace: warehouse
spec:
  selector:
    app: inventory-service
  ports:
  - protocol: TCP
    port: 8081
    targetPort: 8081
  type: ClusterIP
```

Apply manifests:
```bash
kubectl apply -f k8s/
```

### Ingress Configuration

```yaml
# k8s/ingress.yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: warehouse-ingress
  namespace: warehouse
  annotations:
    cert-manager.io/cluster-issuer: letsencrypt-prod
spec:
  tls:
  - hosts:
    - api.warehouse.example.com
    secretName: warehouse-tls
  rules:
  - host: api.warehouse.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: api-gateway
            port:
              number: 8080
```

## Cloud Deployments

### AWS Deployment

#### Using ECS (Elastic Container Service)

1. **Create ECR repositories**:
```bash
aws ecr create-repository --repository-name warehouse/inventory-service
aws ecr create-repository --repository-name warehouse/order-service
# ... repeat for each service
```

2. **Push images**:
```bash
aws ecr get-login-password --region us-east-1 | docker login --username AWS --password-stdin ACCOUNT_ID.dkr.ecr.us-east-1.amazonaws.com
docker tag inventory-service:latest ACCOUNT_ID.dkr.ecr.us-east-1.amazonaws.com/warehouse/inventory-service:latest
docker push ACCOUNT_ID.dkr.ecr.us-east-1.amazonaws.com/warehouse/inventory-service:latest
```

3. **Create ECS cluster and services** using AWS Console or Terraform

#### Using EKS (Elastic Kubernetes Service)

```bash
# Create cluster
eksctl create cluster --name warehouse-cluster --region us-east-1

# Deploy services
kubectl apply -f k8s/
```

### Azure Deployment

#### Using AKS (Azure Kubernetes Service)

```bash
# Create resource group
az group create --name warehouse-rg --location eastus

# Create AKS cluster
az aks create --resource-group warehouse-rg --name warehouse-cluster --node-count 3

# Get credentials
az aks get-credentials --resource-group warehouse-rg --name warehouse-cluster

# Deploy
kubectl apply -f k8s/
```

### Google Cloud Deployment

#### Using GKE (Google Kubernetes Engine)

```bash
# Create cluster
gcloud container clusters create warehouse-cluster --num-nodes=3

# Get credentials
gcloud container clusters get-credentials warehouse-cluster

# Deploy
kubectl apply -f k8s/
```

## CI/CD Pipeline

### GitHub Actions Example

```yaml
# .github/workflows/deploy.yml
name: Build and Deploy

on:
  push:
    branches: [main]

jobs:
  build-and-test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Build C++ Services
        run: |
          cd services/cpp
          # Build commands
      
      - name: Build C# Services
        run: |
          cd services/csharp
          dotnet build
          dotnet test
      
      - name: Build Frontend Apps
        run: |
          cd apps/tablet-pwa
          npm ci
          npm run build
          npm test

  deploy-staging:
    needs: build-and-test
    runs-on: ubuntu-latest
    steps:
      - name: Deploy to staging
        run: |
          # Deployment commands
```

### GitLab CI Example

```yaml
# .gitlab-ci.yml
stages:
  - build
  - test
  - deploy

build:
  stage: build
  script:
    - docker-compose build

test:
  stage: test
  script:
    - docker-compose run --rm test

deploy-production:
  stage: deploy
  script:
    - kubectl apply -f k8s/
  only:
    - main
```

## Database Migrations

### C++ Services - Sqitch Migrations

C++ services use Sqitch for code-first database migrations. See the comprehensive guide:

📖 **[C++ Database Migrations Guide](./cpp-database-migrations.md)**

**Quick Example:**
```bash
cd services/cpp/inventory-service

# Initialize
sqitch init inventory_service --engine pg

# Create migration
sqitch add 001_initial_schema -n "Create initial tables"

# Deploy
sqitch deploy
```

### PostgreSQL Migrations (Alternative Tools)

```bash
# Using Flyway
flyway -url=jdbc:postgresql://localhost:5432/warehouse_db \
       -user=warehouse \
       -password=password \
       migrate

# Using Liquibase
liquibase --changeLogFile=db/changelog.xml update
```

### C# Entity Framework Migrations

```bash
cd services/csharp/ServiceName.Infrastructure
dotnet ef migrations add MigrationName
dotnet ef database update
```

## Monitoring and Logging

### Prometheus & Grafana

Deploy monitoring stack:
```bash
helm install prometheus prometheus-community/kube-prometheus-stack \
  --namespace monitoring \
  --create-namespace
```

### ELK Stack (Elasticsearch, Logstash, Kibana)

```bash
helm install elasticsearch elastic/elasticsearch --namespace logging
helm install kibana elastic/kibana --namespace logging
```

### Application Logs

Access logs:
```bash
# Docker Compose
docker-compose logs -f inventory-service

# Kubernetes
kubectl logs -f deployment/inventory-service
```

## Backup and Recovery

### Database Backup

```bash
# Backup PostgreSQL
docker exec postgres pg_dump -U warehouse warehouse_db > backup.sql

# Restore PostgreSQL
docker exec -i postgres psql -U warehouse warehouse_db < backup.sql
```

### Automated Backups

Schedule regular backups using cron or Kubernetes CronJobs:

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: postgres-backup
spec:
  schedule: "0 2 * * *"  # Daily at 2 AM
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: backup
            image: postgres:15
            command: ["/bin/sh"]
            args:
              - -c
              - pg_dump -U warehouse warehouse_db | gzip > /backup/$(date +%Y%m%d).sql.gz
          restartPolicy: OnFailure
```

## Scaling

### Horizontal Scaling

```bash
# Docker Compose
docker-compose up -d --scale inventory-service=3

# Kubernetes
kubectl scale deployment inventory-service --replicas=5
```

### Auto-scaling in Kubernetes

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: inventory-service-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: inventory-service
  minReplicas: 2
  maxReplicas: 10
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
```

## Security Considerations

1. **Use secrets management** (Kubernetes Secrets, AWS Secrets Manager, Azure Key Vault)
2. **Enable TLS** for all communications
3. **Implement network policies** to restrict inter-service communication
4. **Regular security updates** for base images and dependencies
5. **Scan images** for vulnerabilities before deployment
6. **Use non-root containers**
7. **Implement RBAC** for Kubernetes access

## Troubleshooting

### Common Issues

1. **Service won't start**:
   ```bash
   docker-compose logs service-name
   kubectl describe pod pod-name
   ```

2. **Database connection issues**:
   - Check credentials
   - Verify network connectivity
   - Check database is running

3. **Out of memory**:
   - Increase container memory limits
   - Scale horizontally
   - Optimize application code

## Support

For deployment support:
- Check documentation: [docs/](../docs/)
- Open an issue: [GitHub Issues](https://github.com/stephenwhippuk/warehouse-management/issues)
- Contact DevOps team

## Resources

- [Docker Documentation](https://docs.docker.com/)
- [Kubernetes Documentation](https://kubernetes.io/docs/)
- [Helm Documentation](https://helm.sh/docs/)
