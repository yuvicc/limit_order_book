#pragma once

#include <iostream>
#include "alias.h"
#include "trade.h"
#include "order.h"
#include "orderbook_alias.h"

class Orderbook {
private:
    Bids m_bids;
    Asks m_asks;
    OrderMap orderMap;

    Trades MatchOrders();
    bool CanMatch();
    void AddOrderInternal(std::shared_ptr<Order> order);
    void CancelOrderInternal(OrderId orderId);
    void CancelOrdersInternal(OrderIds orderIds);
    bool CanMatch(Side side, Price price) const;
    bool CanFullyFill(const std::shared_ptr<Order>& order) const;

public:
    Trades addOrder(std::shared_ptr<Order> order);
    void cancelOrder(OrderId orderId);
    Trades modifyOrder(OrderId orderid, Price newPrice, Quantity newQty);

    std::size_t size() const;
    bool hasBids() const;
    bool hasAsks() const;
};
