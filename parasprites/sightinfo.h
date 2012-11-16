#pragma once

//#include <vector>
//#include <tuple>
#include "Location.h"

struct InfoPosVecs{
    typedef std::tuple<Pos, int> PTup;

    //If these are changed to unordered, fix the set difference in updateFood
    std::set<Pos> myAnts, myHills;
    std::map<Pos, unsigned int> enemyAnts, enemyHills;
    std::set<Pos> food;

    //Under normal cicrumstances, it will never have duplicates, but we use a multiset so we can run it on replays from dumb bots that may do collisions
    std::multiset<Pos> myDeadAnts;

    void clear() {*this = InfoPosVecs();}
};
