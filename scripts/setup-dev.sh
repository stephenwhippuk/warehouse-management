#!/bin/bash
# Development setup script for Warehouse Management Solution

set -e

echo "🏗️  Warehouse Management Solution - Development Setup"
echo "======================================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to print status
print_status() {
    echo -e "${GREEN}✓${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}⚠${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

echo ""
echo "Checking prerequisites..."
echo ""

# Check Docker
if command_exists docker; then
    print_status "Docker is installed: $(docker --version)"
else
    print_error "Docker is not installed. Please install Docker first."
    exit 1
fi

# Check Docker Compose
if command_exists docker-compose; then
    print_status "Docker Compose is installed: $(docker-compose --version)"
else
    print_error "Docker Compose is not installed. Please install Docker Compose first."
    exit 1
fi

# Check Node.js
if command_exists node; then
    print_status "Node.js is installed: $(node --version)"
else
    print_warning "Node.js is not installed. Required for frontend development."
fi

# Check npm
if command_exists npm; then
    print_status "npm is installed: $(npm --version)"
else
    print_warning "npm is not installed. Required for frontend development."
fi

# Check C++ compiler
if command_exists g++; then
    print_status "g++ is installed: $(g++ --version | head -n 1)"
elif command_exists clang++; then
    print_status "clang++ is installed: $(clang++ --version | head -n 1)"
else
    print_warning "C++ compiler not found. Required for C++ services development."
fi

# Check CMake
if command_exists cmake; then
    print_status "CMake is installed: $(cmake --version | head -n 1)"
else
    print_warning "CMake is not installed. Required for C++ services development."
fi

# Check .NET
if command_exists dotnet; then
    print_status ".NET SDK is installed: $(dotnet --version)"
else
    print_warning ".NET SDK is not installed. Required for C# services development."
fi

echo ""
echo "Starting infrastructure services..."
echo ""

# Start infrastructure with Docker Compose
docker-compose up -d postgres redis rabbitmq

# Wait for services to be ready
echo "Waiting for services to start..."
sleep 5

# Check PostgreSQL
if docker-compose ps | grep -q "postgres.*Up"; then
    print_status "PostgreSQL is running"
else
    print_error "PostgreSQL failed to start"
fi

# Check Redis
if docker-compose ps | grep -q "redis.*Up"; then
    print_status "Redis is running"
else
    print_error "Redis failed to start"
fi

# Check RabbitMQ
if docker-compose ps | grep -q "rabbitmq.*Up"; then
    print_status "RabbitMQ is running"
else
    print_error "RabbitMQ failed to start"
fi

echo ""
echo "Development environment is ready! 🚀"
echo ""
echo "Next steps:"
echo "  1. Navigate to a service or app directory"
echo "  2. Follow the setup instructions in the README.md"
echo ""
echo "Infrastructure services:"
echo "  PostgreSQL: localhost:5432"
echo "  Redis: localhost:6379"
echo "  RabbitMQ Management: http://localhost:15672 (warehouse/warehouse_dev)"
echo ""
echo "To stop infrastructure services: docker-compose down"
echo ""
