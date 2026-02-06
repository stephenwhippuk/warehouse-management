# Warehouse Management Solution

A semi-realistic, microservice-based warehouse management system designed for modern warehouse operations.

## Overview

This warehouse management solution provides comprehensive tools for warehouse operations including inventory management, order processing, picking, and administrative functions. The system is built with a microservices architecture to ensure scalability, maintainability, and performance.

## Architecture

### Technology Stack

- **Backend Services**: C++ 20 (primary) and C# (selected microservices)
- **Frontend Applications**: Vue 3 (Progressive Web Apps)
- **Message Queue**: RabbitMQ
- **Cache**: Redis
- **Database**: PostgreSQL
- **Containerization**: Docker & Docker Compose

### System Components

```
warehouse-management/
├── services/               # Backend microservices
│   ├── cpp/               # C++ 20 microservices (performance-critical)
│   └── csharp/            # C# microservices (business logic, reporting)
├── apps/                  # Frontend applications
│   ├── tablet-pwa/        # Tablet PWA for warehouse floor operations
│   └── office-web/        # Web/desktop app for office management
├── docs/                  # Documentation
└── scripts/               # Build and deployment scripts
```

## Services

### Backend Microservices (C++)

C++ 20 microservices handle performance-critical operations:
- **Inventory Service**: Real-time inventory tracking and management
- **Order Service**: Order processing and fulfillment
- **Warehouse Service**: Warehouse layout and location management
- **API Gateway**: Request routing and authentication

### Backend Microservices (C#)

C# microservices handle business logic and reporting:
- **Reporting Service**: Analytics and business intelligence
- **Notification Service**: Email and push notifications
- **Integration Service**: Third-party system integrations

## Frontend Applications

### Tablet PWA (Vue 3)
Progressive Web App designed for warehouse floor operations:
- **Picking Interface**: Guided picking workflows
- **Receiving**: Inventory receiving and put-away
- **Sign-off**: Digital signature capture
- **Stock Counting**: Cycle counting and inventory audits
- **Offline Support**: Works without constant connectivity

### Office Web Application (Vue 3)
Desktop/web application for administrative tasks:
- **Dashboard**: Real-time warehouse metrics
- **Order Management**: Order creation and monitoring
- **Inventory Management**: Stock levels and movements
- **Reporting**: Analytics and historical data
- **User Management**: User accounts and permissions

## Getting Started

### Prerequisites

- Docker and Docker Compose
- C++ 20 compiler (GCC 11+ or Clang 13+)
- .NET 8 SDK
- Node.js 18+ and npm

### Quick Start with Docker

1. Clone the repository:
```bash
git clone https://github.com/stephenwhippuk/warehouse-management.git
cd warehouse-management
```

2. Start the infrastructure services:
```bash
docker-compose up -d postgres redis rabbitmq
```

3. Build and start all services:
```bash
docker-compose up --build
```

### Development Setup

Detailed setup instructions for each component:

- [C++ Services Setup](./services/cpp/README.md)
- [C# Services Setup](./services/csharp/README.md)
- [Tablet PWA Setup](./apps/tablet-pwa/README.md)
- [Office Web App Setup](./apps/office-web/README.md)

## Project Structure

```
.
├── apps/
│   ├── tablet-pwa/          # Vue 3 PWA for tablets
│   └── office-web/          # Vue 3 web application
├── services/
│   ├── cpp/                 # C++ microservices
│   │   ├── api-gateway/
│   │   ├── inventory-service/
│   │   ├── order-service/
│   │   └── warehouse-service/
│   └── csharp/              # C# microservices
│       ├── reporting-service/
│       ├── notification-service/
│       └── integration-service/
├── docs/                    # Documentation
├── scripts/                 # Build and deployment scripts
├── docker-compose.yml       # Docker orchestration
└── README.md               # This file
```

## Development Workflow

1. **Create a feature branch**: `git checkout -b feature/my-feature`
2. **Develop your feature**: Make changes in the appropriate service/app directory
3. **Test locally**: Use Docker Compose to test integration
4. **Commit changes**: Follow conventional commit messages
5. **Create PR**: Submit for code review

## Testing

Each service and application has its own test suite:

```bash
# Test C++ services
cd services/cpp/{service-name}
mkdir build && cd build
cmake .. && make test

# Test C# services
cd services/csharp/{service-name}
dotnet test

# Test Vue applications
cd apps/{app-name}
npm run test
```

## Documentation

- [Architecture Documentation](./docs/architecture.md)
- [API Documentation](./docs/api.md)
- [Deployment Guide](./docs/deployment.md)
- [Contributing Guidelines](./docs/contributing.md)

## Contributing

Please read [CONTRIBUTING.md](./docs/contributing.md) for details on our code of conduct and the process for submitting pull requests.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Support

For support and questions, please open an issue in the GitHub repository.
