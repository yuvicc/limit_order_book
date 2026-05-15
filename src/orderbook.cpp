#include "orderbook.h"
#include "timeinforce.h"
#include <stdexcept>

std::size_t Orderbook::size() const {
    std::size_t count = 0;
    for (const auto& [price, orders] : m_bids)
        count += orders.size();
    for (const auto& [price, orders] : m_asks)
        count += orders.size();
    return count;
}

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
            if (bookIt->second.empty()) {
                m_bids.erase(bookIt);
            }
        }

    } else {
        auto bookIt = m_asks.find(price);
        if (bookIt != m_asks.end()) {
            bookIt->second.erase(orderLoc);
            if (bookIt->second.empty()) {
                m_asks.erase(bookIt);
            }
        }
    }

    orderMap.erase(orderId);
}

void Orderbook::CancelOrdersInternal(const OrderIds orderIds) {
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
        orderMap[order->GetOrderId()] = std::prev(orders.end());
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
    if (side == Side::Buy) {
        if (m_asks.empty()) return false;
        const auto& [best_ask, _] = *m_asks.begin();
        return price >= best_ask;
    } else {
        if (m_bids.empty()) return false;
        const auto& [best_bid, _] = *m_bids.begin();
        return price <= best_bid;
    }
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

Trades Orderbook::MatchOrders() {
    Trades trades;

    while (true) {
        if (m_bids.empty() || m_asks.empty()) break;

        auto& [best_bid, bid_orders] = *m_bids.begin();
        auto& [best_ask, ask_orders] = *m_asks.begin();

        if (best_bid < best_ask) break;

        auto bid_order = bid_orders.front();
        auto ask_order = ask_orders.front();

        Quantity fillQty = std::min(
            bid_order->GetRemainingQuantity(),
            ask_order->GetRemainingQuantity()
        );
        Price fillPrice = ask_order->GetPrice();

        bid_order->Fill(fillQty);
        ask_order->Fill(fillQty);

        trades.push_back(Trade{
            TradeInfo{ bid_order->GetOrderId(), fillQty, fillPrice },
            TradeInfo{ ask_order->GetOrderId(), fillQty, fillPrice }
        });

        // Clean up
        if (bid_order->IsFilled()) {
            bid_orders.pop_front();
            orderMap.erase(bid_order->GetOrderId());
        }
        if (ask_order->IsFilled()) {
            ask_orders.pop_front();
            orderMap.erase(ask_order->GetOrderId());
        }
        if (bid_orders.empty()) m_bids.erase(best_bid);
        if (ask_orders.empty()) m_asks.erase(best_ask);
    }
    return trades;
}





