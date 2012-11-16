#pragma once
#include "array.h"
#include <deque>

struct Hill{
    Pos pos;

    Hill(Pos p): pos(p) {}
};

struct myHill: Hill{
    typedef unsigned int dist_t;
    const static int owner = 0;

    Array2d<dist_t> dists;
    std::deque<Pos> openset;

    Pos closestEnemyPos;
    dist_t closestEnemyDist;
    unsigned int numGuards;
    unsigned int lastTouched;

    myHill(Pos p): Hill(p), closestEnemyPos(p), closestEnemyDist(~0u), numGuards(0), lastTouched(0) {}

    bool knownDistance(Pos p) const {return dists[p] < (~0u);}
    dist_t minUnknownDistance() const {ASSERT(openset.size()); return dists[openset.front()] + 1;}
    //A lower bound on the distance to p. Is exact if the distance is known
    dist_t lbDist(Pos p) const {return knownDistance(p) ? dists[p] : minUnknownDistance();}
};

struct ownedHill: Hill{
    int owner;

    ownedHill(Pos p, int player): Hill(p), owner(player) {}
};
