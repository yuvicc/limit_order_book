#include "order.h"
#include <format>
#include <stdexcept>

void Order::Fill(Quantity qty)
{
    if (qty > GetRemainingQuantity())
        throw std::logic_error(std::format("Order ({}) cannot be filled for more than its remaining quantity.", GetOrderId()));
    
    remainingQuantity_ -= qty;    
}




