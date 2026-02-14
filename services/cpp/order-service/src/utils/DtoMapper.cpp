#include "order/utils/DtoMapper.hpp"
#include <algorithm>

namespace order {
namespace utils {

namespace {
    /**
     * @brief Convert OrderStatus enum to lowercase string
     */
    std::string orderStatusToLowerString(models::OrderStatus status) {
        std::string str = models::orderStatusToString(status);
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        return str;
    }
    
    /**
     * @brief Convert OrderPriority enum to lowercase string
     */
    std::string orderPriorityToLowerString(models::OrderPriority priority) {
        std::string str = models::orderPriorityToString(priority);
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        return str;
    }
}

dtos::OrderDto DtoMapper::toOrderDto(
    const models::Order& order,
    const std::string& warehouseCode,
    const std::optional<std::string>& warehouseName) {
    
    // Convert enums to strings
    std::string statusStr = orderStatusToLowerString(order.getStatus());
    std::string priorityStr = orderPriorityToLowerString(order.getPriority());
    
    // Calculate totals from line items
    const auto& lineItems = order.getLineItems();
    int totalItems = static_cast<int>(lineItems.size());
    int totalQuantity = 0;
    for (const auto& item : lineItems) {
        totalQuantity += item.quantity;
    }
    
    // Get customer info (TODO: fetch from customer service)
    // For now, using customerId as placeholder
    std::optional<std::string> customerName = std::nullopt;
    std::optional<std::string> customerEmail = std::nullopt;
    
    // Convert addresses to JSON if present
    std::optional<json> shippingAddressJson = std::nullopt;
    if (order.getShippingAddress()) {
        const auto& addr = *order.getShippingAddress();
        shippingAddressJson = json{
            {"name", addr.name},
            {"line1", addr.line1},
            {"city", addr.city},
            {"state", addr.state},
            {"postalCode", addr.postalCode},
            {"country", addr.country}
        };
        if (addr.line2) (*shippingAddressJson)["line2"] = *addr.line2;
        if (addr.phone) (*shippingAddressJson)["phone"] = *addr.phone;
    }
    
    std::optional<json> billingAddressJson = std::nullopt;
    if (order.getBillingAddress()) {
        const auto& addr = *order.getBillingAddress();
        billingAddressJson = json{
            {"name", addr.name},
            {"line1", addr.line1},
            {"city", addr.city},
            {"state", addr.state},
            {"postalCode", addr.postalCode},
            {"country", addr.country}
        };
        if (addr.line2) (*billingAddressJson)["line2"] = *addr.line2;
        if (addr.phone) (*billingAddressJson)["phone"] = *addr.phone;
    }
    
    // Determine order type based on priority and status
    std::string orderType = "standard"; // TODO: Implement proper type logic
    if (order.getPriority() == models::OrderPriority::URGENT) {
        orderType = "express";
    }
    
    // Use shipByDate as requestedShipDate if available
    std::optional<std::string> requestedShipDate = order.getShipByDate();
    
    // Create DTO with all required and optional fields
    return dtos::OrderDto(
        order.getId(),
        order.getOrderNumber(),
        order.getCustomerId(),
        order.getWarehouseId(),
        warehouseCode,
        order.getOrderDate(),
        priorityStr,
        orderType,
        statusStr,
        totalItems,
        totalQuantity,
        order.getOrderDate(), // use as createdAt
        order.getOrderDate(), // use as updatedAt (TODO: add proper timestamps to Order model)
        customerName,
        customerEmail,
        warehouseName,
        requestedShipDate,
        std::nullopt, // requestedDeliveryDate
        shippingAddressJson,
        billingAddressJson,
        order.getNotes(),
        std::nullopt, // tags
        std::nullopt  // metadata
    );
}

} // namespace utils
} // namespace order
