#pragma once

#include "ordertype.h"
#include "alias.h"
#include "side.h"
#include "orderstatus.h"
#include "timeinforce.h"

class Order {

public:
    Order(OrderId orderId, OrderType orderType, Price price, Quantity qty, Side side)
    : orderId_{ orderId }
    , orderType_{ orderType }
    , price_{ price }
    , initialQuantity_{ qty }
    , remainingQuantity_{ qty }
    , side_ { side }
    { }

    [[nodiscard]] OrderId GetOrderId() const { return orderId_; }
    [[nodiscard]] OrderType GetOrderType() const { return orderType_; }
    [[nodiscard]] Price GetPrice() const { return price_; }
    [[nodiscard]] Quantity GetInitialiQuantity() const { return initialQuantity_; }
    [[nodiscard]] Quantity GetRemainingQuantity() const { return remainingQuantity_; }
    [[nodiscard]] Quantity GetFilledQuantity() const { return GetInitialiQuantity() - GetRemainingQuantity(); }
    [[nodiscard]] OrderStatus GetOrderStatus() const { return status_; }
    [[nodiscard]] Timestamp GetTimestamp() const { return timestamp_; }
    [[nodiscard]] TimeInForce GetTimeInForce() const { return tif_; }
    [[nodiscard]] bool IsBuy() const { return side_ == Side::Buy; }
    [[nodiscard]] bool IsSell() const { return side_ == Side::Sell; }

    [[nodiscard]] bool IsFilled() const { return GetRemainingQuantity() == 0; }
    [[nodiscard]] bool IsPartiallyFilled() const { return GetFilledQuantity() && !IsFilled(); }
    [[nodiscard]] bool IsCancelled() const { return status_ == OrderStatus::Cancelled; }
    [[nodiscard]] bool IsActive() const { return !IsFilled() && !IsCancelled(); }

    void Fill(Quantity qty);
    void UpdatePrice(const Price price);
    void UpdateQuantity(const Quantity qty);
    void ToGoodTillCancel(const Price price);
    void Cancel();

private:
    OrderId orderId_;
    OrderType orderType_;
    TimeInForce tif_;
    Price price_;
    Quantity initialQuantity_;
    Quantity remainingQuantity_;
    Side side_;
    OrderStatus status_;
    Timestamp timestamp_;
};
