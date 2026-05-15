#include "orderbook.h"
#include "timeinforce.h"
#include <stdexcept>

void Orderbook::CancelOrderInternal(OrderId orderId)
{
    if(!orderMap.contains(orderId)) 
        return;

    auto mapIt = orderMap.find(orderId);

    OrderLocation orderLoc = mapIt->second;
    std::shared_ptr<Order> order = *orderLoc;

    order->Cancel();
    Price price = order->GetPrice();

    // Remove order from approprite side of the book
    if (order->IsBuy()) {
        auto bookIt = m_bids.find(price);
        if (bookIt != m_bids.end()) {
            bookIt->second.erase(orderLoc);
        }

        if (bookIt->second.empty()) {
            m_bids.erase(bookIt);
        }
    } else {
        auto bookIt = m_asks.find(price);
        if (bookIt != m_asks.end()) {
            bookIt->second.erase(orderLoc);
        }

        if (bookIt->second.empty()) {
            m_asks.erase(bookIt);
        }
    }

    orderMap.erase(orderId);
}

void Orderbook::CancelOrdersInternal(OrderIds orderIds) {
    for(auto orderId : orderIds)
        CancelOrderInternal(orderId);
}

void Orderbook::cancelOrder(OrderId orderId) {
    CancelOrderInternal(orderId);
}

void Orderbook::AddOrderInternal(std::shared_ptr<Order> order) {
    auto addToSide = [&](auto& side) {
        auto& orders = side[order->GetPrice()];
        orders.push_back(order);
        orderMap[order->GetOrderId()] = std:prev(orders.end());
    };

    if (order->IsBuy()) {
        addToSide(m_bids);
    } else {
        addToSide(m_asks);
    }
}

Trades Orderbook::addOrder(std::shared_ptr<Order> order) {
    if (orderMap.contains(order->GetOrderId()))
        throw std::logic_error("Duplicate OrderID");

    if (!order->IsActive())
        throw std::logic_error("Order is not Active");

    if (order->GetTimeInForce() == TimeInForce::FOK)
        if (!CanFullyFill(order))
            return {};                      // No trades, order rejected silently

    AddOrderInternal(order);

    auto trades = MatchOrders();

    if (order->GetTimeInForce() == TimeInForce::IOC) {
        if (order->IsActive())
            cancelOrder(order->GetOrderId());
    }

    return trades;
}

bool Orderbook::CanMatch(Side side, Price price) const {

}


bool Orderbook::CanFullyFill(const std::shared_ptr<Order>& order) const {
    Quantity available = 0;

    if (order->IsBuy()) {
        for (const auto& [price, orders] : m_asks) {
            if (price > order->GetPrice()) break;
            for (const auto& o : orders) available += o->GetRemainingQuantity();
            if (available >= order->GetRemainingQuantity()) return true;
        }
    } else {
        for (const auto& [price, orders] : m_bids) {
            if (price < order->GetPrice()) break;
            for (const auto& o : orders)
                available += o->GetRemainingQuantity();
            if (available >= order->GetRemainingQuantity()) return true;
        }
    }
    return false;
}
