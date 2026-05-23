#pragma once

#include <iostream>
#include <memory>
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
    void AddOrderInternal(std::shared_ptr<Order> order);
    void CancelOrderInternal(OrderId orderId);
    void CancelOrdersInternal(const OrderIds orderIds);
    bool CanMatch(Side side, Price price) const;
    bool CanFullyFill(const std::shared_ptr<Order>& order) const;
    Trades matchPureProrata(std::shared_ptr<Order> taker_order);
public:
    Trades addOrder(std::shared_ptr<Order> order);
    void cancelOrder(OrderId orderId);
    Trades modifyOrder(OrderId orderid, Price newPrice, Quantity newQty);
    PriceLevel getPriceLevel(Price price);

    std::size_t size() const;
    bool hasBids() const;
    bool hasAsks() const;
    Price getBestBidPrice() const;
    Price getBestAskPrice() const;
    void cleanFilledOrders(Price price, Side side);
};
