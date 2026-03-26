# Fast Limit Orderbook

An orderbook should have three primary functions:

    - add : places order at the end of a list of orders to be executed at a limit price.

    - cancel : remove an arbitrary order

    - execute : removes an order from the inside of the book

## Goal
The goal is to implement these operations in O(1) time while making it possible for the trading model to
efficiently ask questions like:
    - What are the best bid and ask?
    - How much volume is there between prices a and b?
    - What is the order x's current position in the book?

The vast majority of the activity in a book is usually made up of add and cancel operations as market makers
jockey for position, with executions a distant third (in fact I would argue that the bulk of the useful information on many
stocks, particularly in the morning, is in the patterns of adds and cancels, not executions, but that is another topic.)

Each of these operations is keyed off an id number (Order.idNumber) in the pseudo-code below, making a hash table a suitable
algorithm for tracking them.

## The two sides & semantics

**Bids**(Buy side): sorted descending. The highest bid is the most aggresive buyer.
**Asks(Sell side)**: sorted ascending. The lowest ask is the most aggresive seller.

**Spread**: Best ask - Best Bid. This the cost of immediacy. A tight spread means a liquid, competitive market. A wide spread signals
illequidity or uncertainty.
**mid price**: (Best Bid + Best Ask)/2, used as the fair value estimate by many algorithms.

## Order types that is supported

* Limit: Rests in the book at a specific price. Only executes at that price or better.
* Market: Executes immediately at whatever price is available. Walks the book greedly.
* Cancel: Removes a resting limit order by idNumber
* Modify/Amend: Tricky - typically implemented as cancel + re-insert. Loses queue priority.
* IOC (Immediate or Cancel): FIll as much as possible now; cancel the remainder.
* FOK (Fill or Kill): Fill entirely or not at all. All or nothing.
* Stop: Dormant until a trigger price is hit, then becomes a market/limit order.















