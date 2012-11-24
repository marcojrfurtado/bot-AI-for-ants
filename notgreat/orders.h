#ifndef ORDERS_H_
#define ORDERS_H_
#include "Location.h"
struct Order//store an ant location and direction for checking if it works later
{
    int dir;
    Location loc;

    Order(Location L, int d)
    {
        loc = L;
        dir = d;
    };
};


#endif // ORDERS_H_
