#include "order/services/OrderService.hpp"
#include "order/repositories/OrderRepository.hpp"
#include "order/utils/Logger.hpp"
#include "order/utils/DtoMapper.hpp"
#include <stdexcept>

namespace order::services {

OrderService::OrderService(std::shared_ptr<repositories::OrderRepository> repository)
    : repository_(repository) {}

std::optional<dtos::OrderDto> OrderService::getById(const std::string& id) {
    utils::Logger::debug("OrderService::getById({})", id);
    
    auto order = repository_->findById(id);
    if (!order) {
        return std::nullopt;
    }
    
    // TODO: Fetch warehouse code from warehouse service API
    std::string warehouseCode = "WH-" + order->getWarehouseId().substr(0, 8);
    
    return utils::DtoMapper::toOrderDto(*order, warehouseCode, std::nullopt);
}

std::vector<dtos::OrderDto> OrderService::getAll() {
    utils::Logger::debug("OrderService::getAll()");
    
    auto orders = repository_->findAll();
    std::vector<dtos::OrderDto> dtos;
    dtos.reserve(orders.size());
    
    for (const auto& order : orders) {
        // TODO: Batch fetch warehouse codes from warehouse service API
        std::string warehouseCode = "WH-" + order.getWarehouseId().substr(0, 8);
        dtos.push_back(utils::DtoMapper::toOrderDto(order, warehouseCode, std::nullopt));
    }
    
    return dtos;
}

dtos::OrderDto OrderService::create(const models::Order& order) {
    utils::Logger::debug("OrderService::create({})", order.getOrderNumber());
    
    // TODO: Implement validation
    auto created = repository_->create(order);
    
    // TODO: Fetch warehouse code from warehouse service API
    std::string warehouseCode = "WH-" + created.getWarehouseId().substr(0, 8);
    
    return utils::DtoMapper::toOrderDto(created, warehouseCode, std::nullopt);
}

dtos::OrderDto OrderService::update(const models::Order& order) {
    utils::Logger::debug("OrderService::update({})", order.getId());
    
    // TODO: Implement validation
    auto updated = repository_->update(order);
    
    // TODO: Fetch warehouse code from warehouse service API
    std::string warehouseCode = "WH-" + updated.getWarehouseId().substr(0, 8);
    
    return utils::DtoMapper::toOrderDto(updated, warehouseCode, std::nullopt);
}

bool OrderService::deleteById(const std::string& id) {
    utils::Logger::debug("OrderService::deleteById({})", id);
    // TODO: Implement
    return false;
}

dtos::OrderDto OrderService::cancelOrder(const std::string& id, const std::string& reason) {
    utils::Logger::debug("OrderService::cancelOrder({})", id);
    
    // 1. Get order by ID
    auto order = repository_->findById(id);
    if (!order) {
        throw std::runtime_error("Order not found: " + id);
    }
    
    // 2. Check if order can be cancelled (TODO: Implement business logic)
    // 3. Call order.cancel(reason)
    order->cancel(reason);
    
    // 4. Update in repository
    auto updated = repository_->update(*order);
    
    // TODO: Fetch warehouse code from warehouse service API
    std::string warehouseCode = "WH-" + updated.getWarehouseId().substr(0, 8);
    
    return utils::DtoMapper::toOrderDto(updated, warehouseCode, std::nullopt);
}

} // namespace order::services
