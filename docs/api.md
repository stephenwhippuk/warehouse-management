# API Documentation

## Overview

This document provides API documentation for the Warehouse Management Solution microservices.

## Base URLs

- **Development**: `http://localhost:8080`
- **Staging**: `https://staging-api.warehouse.example.com`
- **Production**: `https://api.warehouse.example.com`

## Authentication

All API requests require authentication using JWT tokens.

### Obtaining a Token

```http
POST /api/auth/login
Content-Type: application/json

{
  "username": "user@example.com",
  "password": "password123"
}
```

**Response**:
```json
{
  "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
  "refreshToken": "refresh_token_here",
  "expiresIn": 3600
}
```

### Using the Token

Include the token in the Authorization header:

```http
GET /api/inventory/items
Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...
```

## API Services

### Inventory Service (Port 8081)

#### Get Inventory Items

```http
GET /api/inventory/items
```

**Query Parameters**:
- `page` (optional): Page number (default: 1)
- `limit` (optional): Items per page (default: 50)
- `search` (optional): Search term
- `location` (optional): Filter by location
- `status` (optional): Filter by status (in_stock, low_stock, out_of_stock)

**Response**:
```json
{
  "items": [
    {
      "id": 1,
      "sku": "ABC-123",
      "name": "Product Name",
      "quantity": 100,
      "location": "A-01-01",
      "status": "in_stock",
      "lastUpdated": "2026-02-06T12:00:00Z"
    }
  ],
  "pagination": {
    "page": 1,
    "limit": 50,
    "total": 500,
    "pages": 10
  }
}
```

#### Get Item Details

```http
GET /api/inventory/items/{id}
```

**Response**:
```json
{
  "id": 1,
  "sku": "ABC-123",
  "name": "Product Name",
  "description": "Product description",
  "quantity": 100,
  "location": "A-01-01",
  "zone": "A",
  "aisle": "01",
  "position": "01",
  "status": "in_stock",
  "minQuantity": 10,
  "maxQuantity": 500,
  "reorderPoint": 20,
  "price": 29.99,
  "lastUpdated": "2026-02-06T12:00:00Z",
  "createdAt": "2026-01-01T00:00:00Z"
}
```

#### Update Inventory

```http
PUT /api/inventory/items/{id}
Content-Type: application/json

{
  "quantity": 150,
  "location": "A-01-02"
}
```

#### Bulk Stock Adjustment

```http
POST /api/inventory/adjustments
Content-Type: application/json

{
  "items": [
    {
      "itemId": 1,
      "adjustment": 10,
      "reason": "recount"
    },
    {
      "itemId": 2,
      "adjustment": -5,
      "reason": "damaged"
    }
  ]
}
```

### Order Service (Port 8082)

#### Get Orders

```http
GET /api/orders
```

**Query Parameters**:
- `page` (optional): Page number
- `limit` (optional): Orders per page
- `status` (optional): Filter by status (pending, picking, packed, shipped, delivered)
- `priority` (optional): Filter by priority (low, medium, high, urgent)

**Response**:
```json
{
  "orders": [
    {
      "id": 1001,
      "orderNumber": "ORD-2026-001",
      "customer": "Customer Name",
      "status": "pending",
      "priority": "high",
      "itemCount": 5,
      "totalValue": 299.99,
      "createdAt": "2026-02-06T10:00:00Z",
      "dueDate": "2026-02-07T17:00:00Z"
    }
  ],
  "pagination": {
    "page": 1,
    "limit": 50,
    "total": 200,
    "pages": 4
  }
}
```

#### Create Order

```http
POST /api/orders
Content-Type: application/json

{
  "customer": {
    "name": "Customer Name",
    "email": "customer@example.com",
    "phone": "+1234567890",
    "address": {
      "street": "123 Main St",
      "city": "City",
      "state": "State",
      "zip": "12345",
      "country": "Country"
    }
  },
  "items": [
    {
      "sku": "ABC-123",
      "quantity": 2
    },
    {
      "sku": "DEF-456",
      "quantity": 1
    }
  ],
  "priority": "normal",
  "notes": "Handle with care"
}
```

#### Get Order Details

```http
GET /api/orders/{id}
```

**Response**:
```json
{
  "id": 1001,
  "orderNumber": "ORD-2026-001",
  "customer": {
    "name": "Customer Name",
    "email": "customer@example.com",
    "phone": "+1234567890",
    "address": { ... }
  },
  "status": "picking",
  "priority": "high",
  "items": [
    {
      "id": 1,
      "sku": "ABC-123",
      "name": "Product Name",
      "quantity": 2,
      "location": "A-01-01",
      "picked": 2,
      "status": "complete"
    }
  ],
  "totalValue": 299.99,
  "assignedTo": "picker@example.com",
  "createdAt": "2026-02-06T10:00:00Z",
  "updatedAt": "2026-02-06T11:00:00Z",
  "dueDate": "2026-02-07T17:00:00Z"
}
```

#### Update Order Status

```http
PATCH /api/orders/{id}/status
Content-Type: application/json

{
  "status": "packed",
  "notes": "Ready for shipment"
}
```

### Warehouse Service (Port 8083)

#### Get Locations

```http
GET /api/warehouse/locations
```

#### Get Location Details

```http
GET /api/warehouse/locations/{id}
```

#### Get Zones

```http
GET /api/warehouse/zones
```

### Reporting Service (Port 8091)

#### Generate Report

```http
POST /api/reports/generate
Content-Type: application/json

{
  "type": "inventory_summary",
  "dateFrom": "2026-01-01",
  "dateTo": "2026-02-06",
  "format": "pdf"
}
```

#### Get Report Types

```http
GET /api/reports/types
```

**Response**:
```json
{
  "types": [
    {
      "id": "inventory_summary",
      "name": "Inventory Summary",
      "description": "Overview of current inventory levels"
    },
    {
      "id": "order_fulfillment",
      "name": "Order Fulfillment Report",
      "description": "Orders processed and fulfillment metrics"
    },
    {
      "id": "picking_productivity",
      "name": "Picking Productivity",
      "description": "Picker performance and productivity metrics"
    }
  ]
}
```

## WebSocket API

Real-time updates are provided via WebSocket connections.

### Connect

```javascript
const ws = new WebSocket('ws://localhost:8080/ws');

ws.onopen = () => {
  // Send authentication
  ws.send(JSON.stringify({
    type: 'auth',
    token: 'your_jwt_token'
  }));
};

ws.onmessage = (event) => {
  const data = JSON.parse(event.data);
  // Handle real-time updates
};
```

### Message Types

#### Inventory Update
```json
{
  "type": "inventory.updated",
  "data": {
    "itemId": 1,
    "quantity": 95,
    "location": "A-01-01"
  }
}
```

#### Order Status Update
```json
{
  "type": "order.status_changed",
  "data": {
    "orderId": 1001,
    "status": "packed",
    "timestamp": "2026-02-06T12:00:00Z"
  }
}
```

## Error Responses

All errors follow a consistent format:

```json
{
  "error": {
    "code": "RESOURCE_NOT_FOUND",
    "message": "The requested resource was not found",
    "details": {
      "resourceType": "order",
      "resourceId": 9999
    }
  }
}
```

### HTTP Status Codes

- `200 OK`: Successful request
- `201 Created`: Resource created successfully
- `204 No Content`: Successful request with no content
- `400 Bad Request`: Invalid request parameters
- `401 Unauthorized`: Authentication required
- `403 Forbidden`: Insufficient permissions
- `404 Not Found`: Resource not found
- `409 Conflict`: Resource conflict
- `422 Unprocessable Entity`: Validation error
- `429 Too Many Requests`: Rate limit exceeded
- `500 Internal Server Error`: Server error
- `503 Service Unavailable`: Service temporarily unavailable

## Rate Limiting

- **Rate Limit**: 1000 requests per hour per user
- **Headers**:
  - `X-RateLimit-Limit`: Total requests allowed
  - `X-RateLimit-Remaining`: Requests remaining
  - `X-RateLimit-Reset`: Time when limit resets (Unix timestamp)

## Pagination

Paginated endpoints support these parameters:
- `page`: Page number (default: 1)
- `limit`: Items per page (default: 50, max: 100)

Response includes pagination metadata:
```json
{
  "data": [...],
  "pagination": {
    "page": 1,
    "limit": 50,
    "total": 500,
    "pages": 10,
    "hasNext": true,
    "hasPrev": false
  }
}
```

## Versioning

API versions are specified in the URL:
- Current version: `/api/v1/`
- Legacy version: `/api/v0/` (deprecated)

## Support

For API support, please contact the development team or open an issue on GitHub.
