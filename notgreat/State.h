#ifndef STATE_H_
#define STATE_H_


#include <iostream>
#include <stdio.h>
#include <cstdlib>
#include <cmath>
#include <string>
#include <vector>
#include <list>
#include <queue>
#include <stack>
#include <stdint.h>
#include <algorithm>

#include "Timer.h"
#include "Bug.h"
#include "Square.h"
#include "Location.h"
#include "orders.h"
#include "FightGroup.h"

const int TDIRECTIONS = 5;
const char CDIRECTIONS[5] = {'N', 'E', 'S', 'W', 'X'};
const int DIRECTIONS[5][2] = { {-1, 0}, {0, 1}, {1, 0}, {0, -1}, {0,0} };      //{N, E, S, W, X}
#define rowRep for(int row=0; row<rows; row++)
#define colRep for(int col=0; col<cols; col++)

#define GRD_RAD 6

extern int rows,cols,turn;//these are global variables
/*
    struct to store current state information
*/
struct State
{
    /*
        Variables
    */
    int turns,noPlayers;
    double attackradius, spawnradius, viewradius;
    double loadtime, turntime;
    std::vector<double> scores;
    bool gameover;
    int64_t seed;

    std::vector<std::vector<Square> > grid;
    std::vector<std::vector<double> > invisigrid,invisigrid2;//this is used to speed up diffusions

    std::vector<Location> myHills, enemyHills, food,hillsNowSeen, enemyNonFightingAnts, enemyFightingAnts;
    std::list<Location> myAnts, enemyAnts;
    std::vector<FightGroup> fightingGroups;


    std::vector<Location> guardLocation;

    //myAnts = foodAnts actually
    Timer timer;
    Bug bug;

    //Functions

    State();
    ~State();

    void setup();
    void reset();
    void defineGuardians();
    void print_status();

    void makeMove(const Location &loc, int direction);

    int distance(const Location &loc1, const Location &loc2);
    Location getLoc(const Location &startLoc, int direction);

    void updateVisionInformation();
    void diffuse(int repetitions);
    void preDiffuse();


    int xw(int x);
    int yw(int y);
};

std::ostream& operator<<(std::ostream &os, const State &state);
std::istream& operator>>(std::istream &is, State &state);

#endif //STATE_H_
