#ifndef BOARDEVAL_H_
#define BOARDEVAL_H_

#include "Location.h"
#include "FightGroup.h"
struct Orders//stores all orders for all player ants, as well as a "fitness" for use in board evaluation
{
    std::vector<int> dir;
    std::vector<Location> ants;
    float fitness;

    int md,ed;//debug
    float dist;
    void add(Location L, int d)
    {
        dir.push_back(d);
        ants.push_back(L);
    }
    Orders(float F)
    {
        fitness = F;
    };
    Orders()
    {
        fitness = -9999999;
    };
};

#endif // BOARDEVAL_H_
