#pragma once

struct myHill;

struct Ant{
    Pos pos;

    //If ants are stored from one tick to the next, make sure to reset these pointers so they don't become invalid
    const myHill* ghill; //Guarded hill if any
    const myHill* bhill; //Blocked hill if any

    Ant(Pos p): pos(p), ghill(NULL), bhill(NULL) {}
};

struct EnemyAnt{
    Pos pos;
    EnemyAnt(Pos p): pos(p) {}
};
