#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <optional>

using json = nlohmann::json;

namespace order {
namespace models {

enum class OrderStatus {
    PENDING,
    CONFIRMED,
    PROCESSING,
    PICKING,
    PACKING,
    READY_TO_SHIP,
    SHIPPED,
    IN_TRANSIT,
    DELIVERED,
    CANCELLED,
    RETURNED
};

enum class OrderPriority {
    LOW,
    NORMAL,
    HIGH,
    URGENT
};

std::string orderStatusToString(OrderStatus status);
OrderStatus orderStatusFromString(const std::string& str);

std::string orderPriorityToString(OrderPriority priority);
OrderPriority orderPriorityFromString(const std::string& str);

struct Address {
    std::string name;
    std::string line1;
    std::optional<std::string> line2;
    std::string city;
    std::string state;
    std::string postalCode;
    std::string country;
    std::optional<std::string> phone;

    json toJson() const;
    static Address fromJson(const json& j);
};

struct OrderLineItem {
    std::string id;
    std::string productId;
    std::string productSku;
    std::string productName;
    int quantity;
    double unitPrice;
    double lineTotal;
    std::optional<std::string> notes;

    OrderLineItem() = default;
    OrderLineItem(const std::string& id, const std::string& productId, 
                  const std::string& productSku, const std::string& productName,
                  int quantity, double unitPrice);

    json toJson() const;
    static OrderLineItem fromJson(const json& j);
};

class Order {
public:
    Order() = default;
    Order(const std::string& id, const std::string& orderNumber,
          const std::string& customerId, const std::string& warehouseId,
          OrderStatus status, const std::string& orderDate);

    // Getters
    std::string getId() const { return id_; }
    std::string getOrderNumber() const { return orderNumber_; }
    std::string getCustomerId() const { return customerId_; }
    std::string getWarehouseId() const { return warehouseId_; }
    OrderStatus getStatus() const { return status_; }
    std::string getOrderDate() const { return orderDate_; }
    double getTotal() const { return total_; }
    OrderPriority getPriority() const { return priority_; }
    
    std::optional<std::string> getWarehouseCode() const { return warehouseCode_; }
    std::optional<std::string> getWarehouseName() const { return warehouseName_; }
    std::optional<std::string> getShipByDate() const { return shipByDate_; }
    std::optional<std::string> getNotes() const { return notes_; }
    std::optional<std::string> getCancellationReason() const { return cancellationReason_; }
    std::optional<Address> getShippingAddress() const { return shippingAddress_; }
    std::optional<Address> getBillingAddress() const { return billingAddress_; }
    
    const std::vector<OrderLineItem>& getLineItems() const { return lineItems_; }

    // Setters
    void setId(const std::string& id) { id_ = id; }
    void setOrderNumber(const std::string& orderNumber) { orderNumber_ = orderNumber; }
    void setCustomerId(const std::string& customerId) { customerId_ = customerId; }
    void setWarehouseId(const std::string& warehouseId) { warehouseId_ = warehouseId; }
    void setStatus(OrderStatus status) { status_ = status; }
    void setOrderDate(const std::string& orderDate) { orderDate_ = orderDate; }
    void setTotal(double total) { total_ = total; }
    void setPriority(OrderPriority priority) { priority_ = priority; }
    
    void setWarehouseCode(const std::optional<std::string>& code) { warehouseCode_ = code; }
    void setWarehouseName(const std::optional<std::string>& name) { warehouseName_ = name; }
    void setShipByDate(const std::optional<std::string>& date) { shipByDate_ = date; }
    void setNotes(const std::optional<std::string>& notes) { notes_ = notes; }
    void setCancellationReason(const std::optional<std::string>& reason) { cancellationReason_ = reason; }
    void setShippingAddress(const std::optional<Address>& address) { shippingAddress_ = address; }
    void setBillingAddress(const std::optional<Address>& address) { billingAddress_ = address; }
    
    void setLineItems(const std::vector<OrderLineItem>& lineItems) { lineItems_ = lineItems; }
    void addLineItem(const OrderLineItem& item) { lineItems_.push_back(item); }

    // Business methods
    void calculateTotal();
    bool canBeCancelled() const;
    void cancel(const std::string& reason);

    // Serialization
    json toJson() const;
    static Order fromJson(const json& j);

private:
    std::string id_;
    std::string orderNumber_;
    std::string customerId_;
    std::string warehouseId_;
    OrderStatus status_ = OrderStatus::PENDING;
    std::string orderDate_;
    double total_ = 0.0;
    OrderPriority priority_ = OrderPriority::NORMAL;
    
    std::optional<std::string> warehouseCode_;
    std::optional<std::string> warehouseName_;
    std::optional<std::string> shipByDate_;
    std::optional<std::string> notes_;
    std::optional<std::string> cancellationReason_;
    std::optional<Address> shippingAddress_;
    std::optional<Address> billingAddress_;
    
    std::vector<OrderLineItem> lineItems_;
};

} // namespace models
} // namespace order
