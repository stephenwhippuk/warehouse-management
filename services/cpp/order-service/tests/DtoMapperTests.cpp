#include <catch2/catch_all.hpp>
#include "order/utils/DtoMapper.hpp"
#include "order/models/Order.hpp"
#include "order/dtos/OrderDto.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>

using namespace order;

// Helper to create valid ISO 8601 timestamp
std::string createIso8601Timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

// Helper to create valid ISO 8601 date
std::string createIso8601Date() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%d");
    return ss.str();
}

// Helper to create a valid order model
models::Order createValidOrder() {
    models::Order order(
        "550e8400-e29b-41d4-a716-446655440000",  // Valid UUID
        "ORD-2024-001",  // orderNumber
        "CUST-123"  // customerId
    );
    
    order.setWarehouseId("650e8400-e29b-41d4-a716-446655440001");
    order.setOrderDate(createIso8601Date());
    order.setStatus(models::OrderStatus::Pending);
    order.setPriority(models::OrderPriority::Normal);
    order.setType(models::OrderType::Standard);
    order.setTotalItems(5);
    order.setTotalQuantity(10);
    order.setCreatedAt(createIso8601Timestamp());
    order.setUpdatedAt(createIso8601Timestamp());
    
    return order;
}

TEST_CASE("DtoMapper converts valid order to DTO", "[dto][mapper][order]") {
    auto order = createValidOrder();
    
    SECTION("Successful conversion with required fields") {
        auto dto = utils::DtoMapper::toOrderDto(
            order,
            "WH-MAIN"  // warehouseCode
        );
        
        REQUIRE(dto.getId() == order.getId());
        REQUIRE(dto.getOrderNumber() == order.getOrderNumber());
        REQUIRE(dto.getCustomerId() == order.getCustomerId());
        REQUIRE(dto.getWarehouseId() == order.getWarehouseId());
        REQUIRE(dto.getWarehouseCode() == "WH-MAIN");
        REQUIRE(dto.getStatus() == "pending");
        REQUIRE(dto.getPriority() == "normal");
        REQUIRE(dto.getType() == "standard");
        REQUIRE(dto.getTotalItems() == 5);
        REQUIRE(dto.getTotalQuantity() == 10);
    }
    
    SECTION("Successful conversion with optional fields") {
        order.setCustomerName("John Doe");
        order.setCustomerEmail("john@example.com");
        order.setNotes("Handle with care");
        
        auto dto = utils::DtoMapper::toOrderDto(
            order,
            "WH-MAIN",
            "Main Warehouse"  // warehouseName (optional)
        );
        
        REQUIRE(dto.getCustomerName().has_value());
        REQUIRE(dto.getCustomerName().value() == "John Doe");
        REQUIRE(dto.getCustomerEmail().has_value());
        REQUIRE(dto.getCustomerEmail().value() == "john@example.com");
        REQUIRE(dto.getNotes().has_value());
        REQUIRE(dto.getNotes().value() == "Handle with care");
        REQUIRE(dto.getWarehouseName().has_value());
        REQUIRE(dto.getWarehouseName().value() == "Main Warehouse");
    }
}

TEST_CASE("DtoMapper handles different order statuses", "[dto][mapper][order][status]") {
    SECTION("Pending status") {
        auto order = createValidOrder();
        order.setStatus(models::OrderStatus::Pending);
        
        auto dto = utils::DtoMapper::toOrderDto(order, "WH-1");
        REQUIRE(dto.getStatus() == "pending");
    }
    
    SECTION("Processing status") {
        auto order = createValidOrder();
        order.setStatus(models::OrderStatus::Processing);
        
        auto dto = utils::DtoMapper::toOrderDto(order, "WH-1");
        REQUIRE(dto.getStatus() == "processing");
    }
    
    SECTION("Picking status") {
        auto order = createValidOrder();
        order.setStatus(models::OrderStatus::Picking);
        
        auto dto = utils::DtoMapper::toOrderDto(order, "WH-1");
        REQUIRE(dto.getStatus() == "picking");
    }
    
    SECTION("Packing status") {
        auto order = createValidOrder();
        order.setStatus(models::OrderStatus::Packing);
        
        auto dto = utils::DtoMapper::toOrderDto(order, "WH-1");
        REQUIRE(dto.getStatus() == "packing");
    }
    
    SECTION("Shipped status") {
        auto order = createValidOrder();
        order.setStatus(models::OrderStatus::Shipped);
        
        auto dto = utils::DtoMapper::toOrderDto(order, "WH-1");
        REQUIRE(dto.getStatus() == "shipped");
    }
    
    SECTION("Delivered status") {
        auto order = createValidOrder();
        order.setStatus(models::OrderStatus::Delivered);
        
        auto dto = utils::DtoMapper::toOrderDto(order, "WH-1");
        REQUIRE(dto.getStatus() == "delivered");
    }
    
    SECTION("Cancelled status") {
        auto order = createValidOrder();
        order.setStatus(models::OrderStatus::Cancelled);
        
        auto dto = utils::DtoMapper::toOrderDto(order, "WH-1");
        REQUIRE(dto.getStatus() == "cancelled");
    }
}

TEST_CASE("DtoMapper handles different order priorities", "[dto][mapper][order][priority]") {
    auto order = createValidOrder();
    
    SECTION("Low priority") {
        order.setPriority(models::OrderPriority::Low);
        auto dto = utils::DtoMapper::toOrderDto(order, "WH-1");
        REQUIRE(dto.getPriority() == "low");
    }
    
    SECTION("Normal priority") {
        order.setPriority(models::OrderPriority::Normal);
        auto dto = utils::DtoMapper::toOrderDto(order, "WH-1");
        REQUIRE(dto.getPriority() == "normal");
    }
    
    SECTION("High priority") {
        order.setPriority(models::OrderPriority::High);
        auto dto = utils::DtoMapper::toOrderDto(order, "WH-1");
        REQUIRE(dto.getPriority() == "high");
    }
    
    SECTION("Urgent priority") {
        order.setPriority(models::OrderPriority::Urgent);
        auto dto = utils::DtoMapper::toOrderDto(order, "WH-1");
        REQUIRE(dto.getPriority() == "urgent");
    }
}

TEST_CASE("DtoMapper handles different order types", "[dto][mapper][order][type]") {
    auto order = createValidOrder();
    
    SECTION("Standard type") {
        order.setType(models::OrderType::Standard);
        auto dto = utils::DtoMapper::toOrderDto(order, "WH-1");
        REQUIRE(dto.getType() == "standard");
    }
    
    SECTION("Express type") {
        order.setType(models::OrderType::Express);
        auto dto = utils::DtoMapper::toOrderDto(order, "WH-1");
        REQUIRE(dto.getType() == "express");
    }
    
    SECTION("Return type") {
        order.setType(models::OrderType::Return);
        auto dto = utils::DtoMapper::toOrderDto(order, "WH-1");
        REQUIRE(dto.getType() == "return");
    }
    
    SECTION("Transfer type") {
        order.setType(models::OrderType::Transfer);
        auto dto = utils::DtoMapper::toOrderDto(order, "WH-1");
        REQUIRE(dto.getType() == "transfer");
    }
}

TEST_CASE("DtoMapper validates identity fields", "[dto][mapper][validation]") {
    auto order = createValidOrder();
    
    SECTION("Empty warehouseCode throws") {
        REQUIRE_THROWS_WITH(
            utils::DtoMapper::toOrderDto(order, "", std::nullopt),
            Catch::Matchers::ContainsSubstring("cannot be empty")
        );
    }
}

TEST_CASE("DtoMapper handles quantity edge cases", "[dto][mapper][quantities]") {
    auto order = createValidOrder();
    
    SECTION("Zero quantities are valid") {
        order.setTotalItems(0);
        order.setTotalQuantity(0);
        
        auto dto = utils::DtoMapper::toOrderDto(order, "WH-1");
        REQUIRE(dto.getTotalItems() == 0);
        REQUIRE(dto.getTotalQuantity() == 0);
    }
    
    SECTION("Large quantities are valid") {
        order.setTotalItems(1000);
        order.setTotalQuantity(10000);
        
        auto dto = utils::DtoMapper::toOrderDto(order, "WH-1");
        REQUIRE(dto.getTotalItems() == 1000);
        REQUIRE(dto.getTotalQuantity() == 10000);
    }
    
    SECTION("Negative quantities throw") {
        order.setTotalItems(-1);
        
        REQUIRE_THROWS(
            utils::DtoMapper::toOrderDto(order, "WH-1")
        );
    }
}

TEST_CASE("OrderDto validates on construction", "[dto][validation]") {
    SECTION("Valid order DTO construction succeeds") {
        REQUIRE_NOTHROW(
            dtos::OrderDto(
                "550e8400-e29b-41d4-a716-446655440000",  // id
                "ORD-2024-001",                           // orderNumber
                "CUST-123",                               // customerId
                "650e8400-e29b-41d4-a716-446655440001",  // warehouseId
                "WH-MAIN",                                // warehouseCode
                createIso8601Date(),                      // orderDate
                "normal",                                 // priority
                "standard",                               // type
                "pending",                                // status
                5,                                        // totalItems
                10,                                       // totalQuantity
                createIso8601Timestamp(),                // createdAt
                createIso8601Timestamp()                 // updatedAt
            )
        );
    }
    
    SECTION("Invalid UUID throws") {
        REQUIRE_THROWS_WITH(
            dtos::OrderDto(
                "not-a-uuid",  // Invalid ID
                "ORD-2024-001",
                "CUST-123",
                "650e8400-e29b-41d4-a716-446655440001",
                "WH-MAIN",
                createIso8601Date(),
                "normal",
                "standard",
                "pending",
                5, 10,
                createIso8601Timestamp(),
                createIso8601Timestamp()
            ),
            Catch::Matchers::ContainsSubstring("valid UUID")
        );
    }
    
    SECTION("Empty orderNumber throws") {
        REQUIRE_THROWS_WITH(
            dtos::OrderDto(
                "550e8400-e29b-41d4-a716-446655440000",
                "",  // Empty
                "CUST-123",
                "650e8400-e29b-41d4-a716-446655440001",
                "WH-MAIN",
                createIso8601Date(),
                "normal",
                "standard",
                "pending",
                5, 10,
                createIso8601Timestamp(),
                createIso8601Timestamp()
            ),
            Catch::Matchers::ContainsSubstring("cannot be empty")
        );
    }
    
    SECTION("Empty warehouseCode throws") {
        REQUIRE_THROWS_WITH(
            dtos::OrderDto(
                "550e8400-e29b-41d4-a716-446655440000",
                "ORD-2024-001",
                "CUST-123",
                "650e8400-e29b-41d4-a716-446655440001",
                "",  // Empty
                createIso8601Date(),
                "normal",
                "standard",
                "pending",
                5, 10,
                createIso8601Timestamp(),
                createIso8601Timestamp()
            ),
            Catch::Matchers::ContainsSubstring("cannot be empty")
        );
    }
    
    SECTION("Invalid status throws") {
        REQUIRE_THROWS_WITH(
            dtos::OrderDto(
                "550e8400-e29b-41d4-a716-446655440000",
                "ORD-2024-001",
                "CUST-123",
                "650e8400-e29b-41d4-a716-446655440001",
                "WH-MAIN",
                createIso8601Date(),
                "normal",
                "standard",
                "invalid-status",  // Invalid
                5, 10,
                createIso8601Timestamp(),
                createIso8601Timestamp()
            ),
            Catch::Matchers::ContainsSubstring("status")
        );
    }
    
    SECTION("Invalid priority throws") {
        REQUIRE_THROWS_WITH(
            dtos::OrderDto(
                "550e8400-e29b-41d4-a716-446655440000",
                "ORD-2024-001",
                "CUST-123",
                "650e8400-e29b-41d4-a716-446655440001",
                "WH-MAIN",
                createIso8601Date(),
                "invalid-priority",  // Invalid
                "standard",
                "pending",
                5, 10,
                createIso8601Timestamp(),
                createIso8601Timestamp()
            ),
            Catch::Matchers::ContainsSubstring("priority")
        );
    }
    
    SECTION("Invalid type throws") {
        REQUIRE_THROWS_WITH(
            dtos::OrderDto(
                "550e8400-e29b-41d4-a716-446655440000",
                "ORD-2024-001",
                "CUST-123",
                "650e8400-e29b-41d4-a716-446655440001",
                "WH-MAIN",
                createIso8601Date(),
                "normal",
                "invalid-type",  // Invalid
                "pending",
                5, 10,
                createIso8601Timestamp(),
                createIso8601Timestamp()
            ),
            Catch::Matchers::ContainsSubstring("type")
        );
    }
    
    SECTION("Negative totalItems throws") {
        REQUIRE_THROWS_WITH(
            dtos::OrderDto(
                "550e8400-e29b-41d4-a716-446655440000",
                "ORD-2024-001",
                "CUST-123",
                "650e8400-e29b-41d4-a716-446655440001",
                "WH-MAIN",
                createIso8601Date(),
                "normal",
                "standard",
                "pending",
                -1,  // Negative
                10,
                createIso8601Timestamp(),
                createIso8601Timestamp()
            ),
            Catch::Matchers::ContainsSubstring("non-negative")
        );
    }
    
    SECTION("Negative totalQuantity throws") {
        REQUIRE_THROWS_WITH(
            dtos::OrderDto(
                "550e8400-e29b-41d4-a716-446655440000",
                "ORD-2024-001",
                "CUST-123",
                "650e8400-e29b-41d4-a716-446655440001",
                "WH-MAIN",
                createIso8601Date(),
                "normal",
                "standard",
                "pending",
                5,
                -10,  // Negative
                createIso8601Timestamp(),
                createIso8601Timestamp()
            ),
            Catch::Matchers::ContainsSubstring("non-negative")
        );
    }
    
    SECTION("Invalid timestamp throws") {
        REQUIRE_THROWS_WITH(
            dtos::OrderDto(
                "550e8400-e29b-41d4-a716-446655440000",
                "ORD-2024-001",
                "CUST-123",
                "650e8400-e29b-41d4-a716-446655440001",
                "WH-MAIN",
                createIso8601Date(),
                "normal",
                "standard",
                "pending",
                5, 10,
                "not-a-timestamp",  // Invalid
                createIso8601Timestamp()
            ),
            Catch::Matchers::ContainsSubstring("ISO 8601")
        );
    }
}
