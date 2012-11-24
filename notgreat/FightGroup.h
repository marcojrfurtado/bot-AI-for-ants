#ifndef FIGHT_GROUP_H_
#define FIGHT_GROUP_H_

#include "Location.h"

struct FightGroup
{
    std::vector<Location> enemyAnts,myAnts;
    std::vector<Location> N_en,N_my;
    std::vector<int> en_enemies,my_enemies;
};

#endif // FIGHT_GROUP_H_
