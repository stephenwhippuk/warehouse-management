# C# Microservices

This directory contains C# microservices for business logic, reporting, and integrations.

## Overview

C# services handle:
- Business reporting and analytics
- Email and push notifications
- Third-party system integrations
- Non-performance-critical business logic

## Prerequisites

- .NET 8 SDK or later
- Docker (for containerization)
- SQL Server or PostgreSQL client tools

## Technology Stack

- **.NET 8**: Runtime and framework
- **ASP.NET Core**: Web API framework
- **Entity Framework Core**: ORM
- **MassTransit**: Message bus abstraction
- **Serilog**: Logging
- **xUnit**: Testing framework
- **FluentValidation**: Input validation
- **AutoMapper**: Object mapping

## Project Structure

```
csharp/
├── reporting-service/     # Analytics and BI
├── notification-service/  # Email and push notifications
├── integration-service/   # Third-party integrations
├── common/                # Shared libraries
│   ├── Common.Models/
│   ├── Common.Data/
│   └── Common.Messaging/
└── WarehouseManagement.sln
```

## Building Services

### Using .NET CLI

Build all services:
```bash
cd services/csharp
dotnet restore
dotnet build
```

Build specific service:
```bash
cd services/csharp/{service-name}
dotnet build
```

### Using Docker

Build with Docker Compose from project root:
```bash
docker-compose build reporting-service notification-service
```

## Development Setup

1. **Install .NET 8 SDK**:
```bash
# Linux
wget https://dot.net/v1/dotnet-install.sh
sudo bash dotnet-install.sh --channel 8.0

# Windows
# Download from https://dotnet.microsoft.com/download
```

2. **Restore packages**:
```bash
cd services/csharp
dotnet restore
```

3. **Run service locally**:
```bash
cd services/csharp/{service-name}
dotnet run
```

4. **Run with hot reload**:
```bash
dotnet watch run
```

## Testing

Run all tests:
```bash
cd services/csharp
dotnet test
```

Run tests with coverage:
```bash
dotnet test /p:CollectCoverage=true /p:CoverageReportFormat=opencover
```

Run specific test project:
```bash
cd services/csharp/{service-name}.Tests
dotnet test
```

## Coding Standards

- Follow Microsoft's C# Coding Conventions
- Use async/await for I/O operations
- Implement dependency injection
- Use nullable reference types
- Follow SOLID principles
- Use EditorConfig for consistent formatting

## Service Structure

Each service follows Clean Architecture:

```
ServiceName/
├── ServiceName.Api/          # Web API layer
│   ├── Controllers/
│   ├── Middleware/
│   └── Program.cs
├── ServiceName.Application/  # Business logic
│   ├── Services/
│   ├── DTOs/
│   └── Interfaces/
├── ServiceName.Domain/       # Domain models
│   ├── Entities/
│   └── ValueObjects/
├── ServiceName.Infrastructure/ # Data access
│   ├── Data/
│   ├── Repositories/
│   └── Messaging/
└── ServiceName.Tests/        # Unit and integration tests
    ├── Unit/
    └── Integration/
```

## Configuration

Services use appsettings.json and environment variables:

```json
{
  "ConnectionStrings": {
    "DefaultConnection": "Host=postgres;Database=warehouse_db;Username=warehouse;Password=warehouse_dev"
  },
  "MessageBus": {
    "Host": "rabbitmq",
    "Username": "warehouse",
    "Password": "warehouse_dev"
  },
  "Logging": {
    "LogLevel": {
      "Default": "Information",
      "Microsoft.AspNetCore": "Warning"
    }
  }
}
```

## API Documentation

Services use Swagger/OpenAPI:
- Development: `http://localhost:{port}/swagger`
- API spec: `http://localhost:{port}/swagger/v1/swagger.json`

## Logging

All services use structured logging with Serilog:
- Console sink for development
- File sink with rolling files
- Seq or Elasticsearch for production

## Health Checks

Services implement health checks:
- Liveness: `/health/live`
- Readiness: `/health/ready`
- Database: `/health/db`

## Creating a New Service

1. Create solution and projects:
```bash
dotnet new webapi -n ServiceName.Api
dotnet new classlib -n ServiceName.Application
dotnet new classlib -n ServiceName.Domain
dotnet new classlib -n ServiceName.Infrastructure
dotnet new xunit -n ServiceName.Tests
dotnet new sln -n ServiceName
dotnet sln add **/*.csproj
```

2. Install common packages:
```bash
dotnet add package Microsoft.EntityFrameworkCore
dotnet add package MassTransit.RabbitMQ
dotnet add package Serilog.AspNetCore
dotnet add package FluentValidation
dotnet add package AutoMapper
```

3. Implement service logic
4. Add tests
5. Create Dockerfile
6. Update docker-compose.yml

## Database Migrations

Create migration:
```bash
dotnet ef migrations add InitialCreate --project ServiceName.Infrastructure
```

Apply migration:
```bash
dotnet ef database update --project ServiceName.Api
```

## Resources

- [.NET Documentation](https://docs.microsoft.com/dotnet/)
- [ASP.NET Core Documentation](https://docs.microsoft.com/aspnet/core/)
- [Clean Architecture](https://blog.cleancoder.com/uncle-bob/2012/08/13/the-clean-architecture.html)
- [Microsoft REST API Guidelines](https://github.com/microsoft/api-guidelines)
