#include <iostream>
#include <cassert>
#include "orderbook.h"

static OrderId nextId = 1;

std::shared_ptr<Order> makeLimitBuy(Price price, Quantity qty) {
    return std::make_shared<Order>(nextId++, OrderType::Limit, price, qty, Side::Buy);
}

std::shared_ptr<Order> makeLimitSell(Price price, Quantity qty) {
    return std::make_shared<Order>(nextId++, OrderType::Limit, price, qty, Side::Sell);
}

void printTrades(const Trades& trades) {
    if (trades.empty()) {
        std::cout << "  [no trades]\n";
        return;
    }
    for (const auto& t : trades) {
        std::cout << "  TRADE => "
                  << "BidOrder#" << t.bidInfo.orderId
                  << " x AskOrder#" << t.askInfo.orderId
                  << " | qty: "    << t.bidInfo.qty
                  << " @ price: "  << t.bidInfo.price
                  << "\n";
    }
}

void section(const std::string& title) {
    std::cout << "\n══════════════════════════════════════\n";
    std::cout << "  " << title << "\n";
    std::cout << "══════════════════════════════════════\n";
}

// ── Tests ──────────────────────────────────────────────────────────────────

// 1. No match — orders rest in the book
void testNoMatch() {
    section("TEST 1: No match — orders rest");
    Orderbook book;

    auto buy  = makeLimitBuy (100, 10);
    auto sell = makeLimitSell(101, 10);   // ask > bid, no cross

    auto t1 = book.addOrder(buy);
    auto t2 = book.addOrder(sell);

    printTrades(t1);
    printTrades(t2);

    assert(t1.empty() && t2.empty());
    assert(book.size() == 2);
    std::cout << "  PASSED\n";
}

// 2. Full match — both orders completely filled
void testFullMatch() {
    section("TEST 2: Full match");
    Orderbook book;

    auto buy  = makeLimitBuy (100, 10);
    auto sell = makeLimitSell(100, 10);

    book.addOrder(buy);
    auto trades = book.addOrder(sell);

    printTrades(trades);

    assert(trades.size() == 1);
    assert(trades[0].bidInfo.qty == 10);
    assert(trades[0].askInfo.qty == 10);
    assert(book.size() == 0);             // both orders gone
    std::cout << "  PASSED\n";
}

// 3. Partial match — larger order partially filled, rests in book
void testPartialMatch() {
    section("TEST 3: Partial match");
    Orderbook book;

    auto buy  = makeLimitBuy (100, 20);
    auto sell = makeLimitSell(100, 10);   // sell is smaller

    book.addOrder(buy);
    auto trades = book.addOrder(sell);

    printTrades(trades);

    assert(trades.size() == 1);
    assert(trades[0].bidInfo.qty == 10);  // only 10 filled
    assert(book.size() == 1);             // buy order still resting
    assert(buy->GetRemainingQuantity() == 10);
    std::cout << "  PASSED\n";
}

// 4. Cancel order — order removed, no longer matches
void testCancel() {
    section("TEST 4: Cancel order");
    Orderbook book;

    auto buy  = makeLimitBuy(100, 10);
    book.addOrder(buy);
    assert(book.size() == 1);

    book.cancelOrder(buy->GetOrderId());
    assert(book.size() == 0);

    // Now add a matching sell — should NOT trade
    auto sell = makeLimitSell(100, 10);
    auto trades = book.addOrder(sell);
    printTrades(trades);

    assert(trades.empty());
    assert(book.size() == 1);             // sell rests unmatched
    std::cout << "  PASSED\n";
}

// 5. Multiple price levels — best price matched first
void testPricePriority() {
    section("TEST 5: Price priority");
    Orderbook book;

    auto buy1 = makeLimitBuy(99,  10);
    auto buy2 = makeLimitBuy(100, 10);   // better bid — matched first
    book.addOrder(buy1);
    book.addOrder(buy2);

    auto sell   = makeLimitSell(99, 10);
    auto trades = book.addOrder(sell);

    printTrades(trades);

    assert(trades.size() == 1);
    assert(trades[0].bidInfo.orderId == buy2->GetOrderId()); // best bid matched
    assert(book.size() == 1);            // buy1 still resting
    std::cout << "  PASSED\n";
}

// 6. FIFO at same price level
void testTimePriority() {
    section("TEST 6: FIFO / time priority at same price");
    Orderbook book;

    auto buy1 = makeLimitBuy(100, 10);   // arrives first
    auto buy2 = makeLimitBuy(100, 10);
    book.addOrder(buy1);
    book.addOrder(buy2);

    auto sell   = makeLimitSell(100, 10);
    auto trades = book.addOrder(sell);

    printTrades(trades);

    assert(trades.size() == 1);
    assert(trades[0].bidInfo.orderId == buy1->GetOrderId()); // first in, first matched
    assert(book.size() == 1);
    std::cout << "  PASSED\n";
}

// 7. Duplicate OrderId — must throw
void testDuplicateOrderId() {
    section("TEST 7: Duplicate OrderId throws");
    Orderbook book;

    auto buy = makeLimitBuy(100, 10);
    book.addOrder(buy);

    // Re-use the same order pointer (same ID)
    try {
        book.addOrder(buy);
        assert(false && "Should have thrown");
    } catch (const std::logic_error& e) {
        std::cout << "  Caught expected exception: " << e.what() << "\n";
    }
    std::cout << "  PASSED\n";
}

// 8. Multi-level sweep — one large order eats multiple price levels
void testMultiLevelSweep() {
    section("TEST 8: Multi-level sweep");
    Orderbook book;

    auto s1 = makeLimitSell(100, 5);
    auto s2 = makeLimitSell(101, 5);
    auto s3 = makeLimitSell(102, 5);
    book.addOrder(s1);
    book.addOrder(s2);
    book.addOrder(s3);

    auto buy    = makeLimitBuy(102, 15);  // sweeps all three levels
    auto trades = book.addOrder(buy);

    printTrades(trades);

    assert(trades.size() == 3);
    assert(book.size() == 0);
    std::cout << "  PASSED\n";
}

int main() {
    testNoMatch();
    testFullMatch();
    testPartialMatch();
    testCancel();
    testPricePriority();
    testTimePriority();
    testDuplicateOrderId();
    testMultiLevelSweep();

    std::cout << "\n✅ All tests passed.\n";
    return 0;
}
