#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <set>
#include <map>
//#include <stdint.h>

#include "sightinfo.h"

#include "Timer.h"
#include "Bug.h"
#include "Square.h"
#include "Location.h"
#include "ant.h"
#include "hill.h"
#include "array.h"
namespace Symmetry {struct SymmetryFinder;}

const char CDIRECTIONS[5] = {'N', 'E', 'S', 'W', 'X'};
const int DIRECTIONS[4][2] = { {-1, 0}, {0, 1}, {1, 0}, {0, -1} };      //{N, E, S, W}

inline int reverseMove(int d) {return (d < 4) ? (d ^ 2) : d;}


struct State
{
    int rows, cols, max_turns;
    int attackrad2, spawnrad2, viewrad2;
    int attackrad1, viewrad1;

    double loadtime, turntime;
    int64_t seed;

    bool gameover;
    int turn;

    std::unique_ptr<Symmetry::SymmetryFinder> symmetry;
    Array2d<Square> grid;
    Array2d<unsigned int> lastSeen;
    Array2d<unsigned int> distToFood;
    Array2d<unsigned int> distToHills;
    Array2d<unsigned int> myAntInfluence;
    Array2d<unsigned int> enemyInfluence;

    //Can't use bool because vector<bool> is broken
    Array2d<int> possibleEnemies;
    std::size_t lastFilteredPossibleEnemiesSize;

    InfoPosVecs seenInput; //All data here is preserved until end of turn except food set

//    std::vector<Pos> myAnts, enemyAnts, myHills, enemyHills, food;
//    std::vector<Pos> enemyAnts;
    unsigned int storedFood;
    std::set<Pos> food, newlySpawnedFood;
    std::map<Pos, myHill> myHills;
    std::map<Pos, ownedHill> enemyHills;
    std::map<Pos, Ant> myAnts;
    std::map<Pos, EnemyAnt> enemyAnts;

    std::map<Pos, int> spawnBlockIncentives;

    Timer timer;
    Bug bug;

    /*
        Functions
    */
    State();
    void setup(); //does initialization after params are read

    ~State();

    double time() const {return timer.getTime();}
    bool timeCheckFrac(double frac) const {return time() < frac * turntime;}
    bool timeCheckCappedOk(double amount) const {return time() < turntime - amount;}

    unsigned int turnsLeft() const {return max_turns - turn;}
    bool enoughStoredFood() const {return storedFood >= turnsLeft() * myHills.size();}

    void makeMove(const Pos &loc, int direction);

    int distMan(const Pos &loc1, const Pos &loc2) const;
    int distEuc2(const Pos &loc1, const Pos &loc2) const;
    int distInf(const Pos &loc1, const Pos &loc2) const;

    Pos getDest(Pos p, int direction) const;

    Pos wrapPos(const Pos& loc) const;
    Pos wrapPos(int r, int c) const {return wrapPos(Pos(r,c));}

    bool explored(const Pos& loc) const {return lastSeen[loc] < (unsigned)max_turns;}
    bool visible(const Pos& loc) const {return lastSeen[loc] == 0;}

    int getNonCombatScore(Pos pos) const;


    void update(); //Stuff that happens at start of turn
    void updateSymmetry(); //Runs at end of turn while time is left

    //IO Functions
    void readGameSettings(std::istream &is);
    bool readGameInput(std::istream &is);       //Return true if game is still going

    private:
    void updateNewlySpawnedFood();
    void updateVisionInformation();
    void updateSeenHills();
    void updateAgentStates();
    void updateAntInfluences();
    void updateFood();
    void updateHillDistances();
    void updatePossibleEnemies();
    void updateSpawnBlocking();
};
