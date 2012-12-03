#ifndef BOT_H_
#define BOT_H_

#include "State.h"
#include "orders.h"
#include "BoardEval.h"
#include "Bug.h"


#define ENG_RAD 18 //engagement radius (atk+2)
#define ATK_RAD 5 //attack radius
#define MID_RAD 11 //middle radius (atk+1)


#define MAX_PLANS 6000000
/*
    This struct represents your bot in the game of Ants
*/
extern int turn;

struct Bot
{
    State state;
    //int enemycount;
    Bot();
    ~Bot();
    int moveTo(Location loc1, Location loc2);//best direction to go from loc1 to loc2, ignoring everything
    int moveAway(Location loc1, Location loc2);//best direction to go from loc1 to loc2, ignoring everything

    void playGame();    //plays a single game of Ants
    void battleCheck(); //updates list of all ants that should use battle AI
    void makeMoves();   //run all gathering AI
    void tactics();    //run tactical AI
    void endTurn();     //indicates to the engine that it has made its moves
    Orders bestof(int i,FightGroup B);
    Orders alphabeta(int i,FightGroup B,float alpha, float beta);
    Orders kindofbestof(int i,FightGroup B);
    void smallbatt(FightGroup B);
    void mediumbatt(FightGroup B);
    void largebatt(FightGroup B);
    Orders getfit(FightGroup B);
    long long int get_move_plans(FightGroup f);
    Bug bug;

};

#endif //BOT_H_
