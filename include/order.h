#pragma once

#include "ordertype.h"
#include "alias.h"
#include "side.h"
#include "orderstatus.h"


#include <iostream>

class Order {

public:
    Order(OrderId orderId, OrderType orderType, Side side, Price price, Quantity quantity)
    : orderId_{ orderId }
    , orderType_{ orderType }
    , side_ { side }
    , price_{ price }
    , initialQuantity_{ quantity }
    , remainingQuantity_{ quantity }
    { }

    OrderId GetOrderId() const { return orderId_; }
    OrderType GetOrderType() const { return orderType_; }
    Price GetPrice() const { return price_; }
    Quantity GetInitialiQuantity() const { return initialQuantity_; }
    Quantity GetRemainingQuantity() const { return remainingQuantity_; }
    Quantity GetFilledQuantity() const { return GetInitialiQuantity() - GetRemainingQuantity(); }
    OrderStatus GetOrderStatus() const { return status_; }
    Timestamp GetTimestamp() const { return timestamp_; }
    bool IsBuy() const { return side_ == Side::Buy; }
    bool IsSell() const { return side_ == Side::Sell; }

    bool IsFilled() const { return GetRemainingQuantity() == 0; }
    bool IsPartiallyFilled() const { return GetFilledQuantity() && !IsFilled(); }
    bool IsCancelled() const { return status_ == OrderStatus::Cancelled; }
    bool IsActive() const { return !IsFilled() && !IsCancelled(); }

    void Fill(Quantity qty);
    void ToGoodTillCancel(Price price);
    void UpdatePrice(const OrderId orderid, const Price price);
    void UpdateQuantity(const OrderId orderid, const Quantity qty);


private:
    OrderId orderId_;
    OrderType orderType_;
    Price price_;
    Quantity initialQuantity_;
    Quantity remainingQuantity_;
    Side side_;
    OrderStatus status_;
    Timestamp timestamp_;

};
