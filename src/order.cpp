#include "order.h"
#include <format>
#include <stdexcept>

void Order::Fill(Quantity qty)
{
    if (qty > GetRemainingQuantity())
        throw std::logic_error(std::format("Order ({}) cannot be filled for more than its remaining quantity.", GetOrderId()));
    
    remainingQuantity_ -= qty;    
}

void Order::UpdatePrice(const Price price)
{
    price_ = price;
}

void Order::UpdateQuantity(const Quantity qty)
{
    remainingQuantity_ = qty; 
}

void Order::Cancel()
{
    remainingQuantity_ = 0;
}

void Order::ToGoodTillCancel(const Price price)
{
    if (GetOrderType() != OrderType::Market)
        throw std::logic_error(std::format("Order ({}) cannot have it price adjusted, only market orders can.", GetOrderId()));

    UpdatePrice(price);
}




