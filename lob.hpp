#include <iostream>

namespace lob {

class LimitOrderBook {

public:

    void add_order();
    
    void modify_order();

    void cancel_order();

private:
    struct Order {
        uint_fast64_t order_id;
        std::string client_order_id;
        std::string symbol;
        
        enum side {
            BUY, SELL
        };

        enum order_type {
            LIMIT, MARKET, IOC, FOK 
        };

        double price;
        double quantity;
        double filled_quantity;

        enum status {
            OPEN, PARTIAL, FILLED, CANCELLED 
        };

        std::time_t timestamp;
        std::uint64_t expiry_time;
        std::string account_id;
    };


};












} // namespace lob
