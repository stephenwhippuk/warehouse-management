#include "order/models/Order.hpp"
#include <stdexcept>
#include <numeric>

namespace order {
namespace models {

std::string orderStatusToString(OrderStatus status) {
    switch (status) {
        case OrderStatus::PENDING: return "pending";
        case OrderStatus::CONFIRMED: return "confirmed";
        case OrderStatus::PROCESSING: return "processing";
        case OrderStatus::PICKING: return "picking";
        case OrderStatus::PACKING: return "packing";
        case OrderStatus::READY_TO_SHIP: return "ready_to_ship";
        case OrderStatus::SHIPPED: return "shipped";
        case OrderStatus::IN_TRANSIT: return "in_transit";
        case OrderStatus::DELIVERED: return "delivered";
        case OrderStatus::CANCELLED: return "cancelled";
        case OrderStatus::RETURNED: return "returned";
        default: throw std::invalid_argument("Unknown OrderStatus");
    }
}

OrderStatus orderStatusFromString(const std::string& str) {
    if (str == "pending") return OrderStatus::PENDING;
    if (str == "confirmed") return OrderStatus::CONFIRMED;
    if (str == "processing") return OrderStatus::PROCESSING;
    if (str == "picking") return OrderStatus::PICKING;
    if (str == "packing") return OrderStatus::PACKING;
    if (str == "ready_to_ship") return OrderStatus::READY_TO_SHIP;
    if (str == "shipped") return OrderStatus::SHIPPED;
    if (str == "in_transit") return OrderStatus::IN_TRANSIT;
    if (str == "delivered") return OrderStatus::DELIVERED;
    if (str == "cancelled") return OrderStatus::CANCELLED;
    if (str == "returned") return OrderStatus::RETURNED;
    throw std::invalid_argument("Invalid order status: " + str);
}

std::string orderPriorityToString(OrderPriority priority) {
    switch (priority) {
        case OrderPriority::LOW: return "low";
        case OrderPriority::NORMAL: return "normal";
        case OrderPriority::HIGH: return "high";
        case OrderPriority::URGENT: return "urgent";
        default: throw std::invalid_argument("Unknown OrderPriority");
    }
}

OrderPriority orderPriorityFromString(const std::string& str) {
    if (str == "low") return OrderPriority::LOW;
    if (str == "normal") return OrderPriority::NORMAL;
    if (str == "high") return OrderPriority::HIGH;
    if (str == "urgent") return OrderPriority::URGENT;
    throw std::invalid_argument("Invalid order priority: " + str);
}

// Address implementation
json Address::toJson() const {
    json j = {
        {"name", name},
        {"line1", line1},
        {"city", city},
        {"state", state},
        {"postalCode", postalCode},
        {"country", country}
    };
    
    if (line2) j["line2"] = *line2;
    if (phone) j["phone"] = *phone;
    
    return j;
}

Address Address::fromJson(const json& j) {
    Address addr;
    addr.name = j.at("name").get<std::string>();
    addr.line1 = j.at("line1").get<std::string>();
    addr.city = j.at("city").get<std::string>();
    addr.state = j.at("state").get<std::string>();
    addr.postalCode = j.at("postalCode").get<std::string>();
    addr.country = j.at("country").get<std::string>();
    
    if (j.contains("line2") && !j["line2"].is_null()) {
        addr.line2 = j["line2"].get<std::string>();
    }
    if (j.contains("phone") && !j["phone"].is_null()) {
        addr.phone = j["phone"].get<std::string>();
    }
    
    return addr;
}

// OrderLineItem implementation
OrderLineItem::OrderLineItem(const std::string& id, const std::string& productId,
                             const std::string& productSku, const std::string& productName,
                             int quantity, double unitPrice)
    : id(id), productId(productId), productSku(productSku), productName(productName),
      quantity(quantity), unitPrice(unitPrice) {
    lineTotal = quantity * unitPrice;
}

json OrderLineItem::toJson() const {
    json j = {
        {"id", id},
        {"productId", productId},
        {"productSku", productSku},
        {"productName", productName},
        {"quantity", quantity},
        {"unitPrice", unitPrice},
        {"lineTotal", lineTotal}
    };
    
    if (notes) j["notes"] = *notes;
    
    return j;
}

OrderLineItem OrderLineItem::fromJson(const json& j) {
    OrderLineItem item;
    item.id = j.at("id").get<std::string>();
    item.productId = j.at("productId").get<std::string>();
    item.productSku = j.at("productSku").get<std::string>();
    item.productName = j.at("productName").get<std::string>();
    item.quantity = j.at("quantity").get<int>();
    item.unitPrice = j.at("unitPrice").get<double>();
    item.lineTotal = j.at("lineTotal").get<double>();
    
    if (j.contains("notes") && !j["notes"].is_null()) {
        item.notes = j["notes"].get<std::string>();
    }
    
    return item;
}

// Order implementation
Order::Order(const std::string& id, const std::string& orderNumber,
             const std::string& customerId, const std::string& warehouseId,
             OrderStatus status, const std::string& orderDate)
    : id_(id), orderNumber_(orderNumber), customerId_(customerId),
      warehouseId_(warehouseId), status_(status), orderDate_(orderDate) {
}

void Order::calculateTotal() {
    total_ = std::accumulate(lineItems_.begin(), lineItems_.end(), 0.0,
        [](double sum, const OrderLineItem& item) {
            return sum + item.lineTotal;
        });
}

bool Order::canBeCancelled() const {
    return status_ == OrderStatus::PENDING ||
           status_ == OrderStatus::CONFIRMED ||
           status_ == OrderStatus::PROCESSING;
}

void Order::cancel(const std::string& reason) {
    if (!canBeCancelled()) {
        throw std::runtime_error("Order cannot be cancelled in current status");
    }
    status_ = OrderStatus::CANCELLED;
    cancellationReason_ = reason;
}

json Order::toJson() const {
    json j = {
        {"id", id_},
        {"orderNumber", orderNumber_},
        {"customerId", customerId_},
        {"warehouseId", warehouseId_},
        {"status", orderStatusToString(status_)},
        {"orderDate", orderDate_},
        {"total", total_},
        {"priority", orderPriorityToString(priority_)}
    };
    
    if (warehouseCode_) j["WarehouseCode"] = *warehouseCode_;
    if (warehouseName_) j["WarehouseName"] = *warehouseName_;
    if (shipByDate_) j["shipByDate"] = *shipByDate_;
    if (notes_) j["notes"] = *notes_;
    if (cancellationReason_) j["cancellationReason"] = *cancellationReason_;
    
    if (shippingAddress_) {
        j["shippingAddress"] = shippingAddress_->toJson();
    }
    if (billingAddress_) {
        j["billingAddress"] = billingAddress_->toJson();
    }
    
    json lineItemsJson = json::array();
    for (const auto& item : lineItems_) {
        lineItemsJson.push_back(item.toJson());
    }
    j["lineItems"] = lineItemsJson;
    
    return j;
}

Order Order::fromJson(const json& j) {
    Order order;
    order.id_ = j.at("id").get<std::string>();
    order.orderNumber_ = j.at("orderNumber").get<std::string>();
    order.customerId_ = j.at("customerId").get<std::string>();
    order.warehouseId_ = j.at("warehouseId").get<std::string>();
    order.status_ = orderStatusFromString(j.at("status").get<std::string>());
    order.orderDate_ = j.at("orderDate").get<std::string>();
    order.total_ = j.at("total").get<double>();
    
    if (j.contains("priority")) {
        order.priority_ = orderPriorityFromString(j.at("priority").get<std::string>());
    }
    
    if (j.contains("WarehouseCode") && !j["WarehouseCode"].is_null()) {
        order.warehouseCode_ = j["WarehouseCode"].get<std::string>();
    }
    if (j.contains("WarehouseName") && !j["WarehouseName"].is_null()) {
        order.warehouseName_ = j["WarehouseName"].get<std::string>();
    }
    if (j.contains("shipByDate") && !j["shipByDate"].is_null()) {
        order.shipByDate_ = j["shipByDate"].get<std::string>();
    }
    if (j.contains("notes") && !j["notes"].is_null()) {
        order.notes_ = j["notes"].get<std::string>();
    }
    if (j.contains("cancellationReason") && !j["cancellationReason"].is_null()) {
        order.cancellationReason_ = j["cancellationReason"].get<std::string>();
    }
    
    if (j.contains("shippingAddress") && !j["shippingAddress"].is_null()) {
        order.shippingAddress_ = Address::fromJson(j["shippingAddress"]);
    }
    if (j.contains("billingAddress") && !j["billingAddress"].is_null()) {
        order.billingAddress_ = Address::fromJson(j["billingAddress"]);
    }
    
    if (j.contains("lineItems") && j["lineItems"].is_array()) {
        for (const auto& itemJson : j["lineItems"]) {
            order.lineItems_.push_back(OrderLineItem::fromJson(itemJson));
        }
    }
    
    return order;
}

} // namespace models
} // namespace order
