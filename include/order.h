#pragma once

#include "ordertype.h"
#include "alias.h"
#include "side.h"



#include <iostream>

class Order {

public:
    Order(OrderId orderId, OrderType orderType, Price price, Quantity quantity)
    : orderId_{orderId}
    , orderType_{orderType}
    , price_{price}
    , initialQuantity_{quantity}
    , remainingQuantity_{quantity}
    { }


private:
    { }
    OrderId orderId_;
    OrderType orderType_;
    Price price_;
    Quantity initialQuantity_;
    Quantity remainingQuantity_;
    Side side_;    
};
