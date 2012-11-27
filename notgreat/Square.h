#ifndef SQUARE_H_
#define SQUARE_H_

#include <vector>

/*
    struct for representing a square in the grid.
*/
struct Square
{
    bool sinceSeen,hasSeen, isWater, isHill, isFood;
    int ant, hillPlayer;
    double foodDif;


    double guardDif;

    bool AntHereCheck;//for use in small-scale fight battles
    //Food diffusion is also exploration diffusion
    //std::vector<int> deadAnts;

    /*Square()
    {
        hasSeen = isVisible = isWater = isHill = isFood = 0;
        ant = hillPlayer = -1;
        foodDif=0.;
    };*/
    //remove this and I think it's a POD structure

    void init()//initialize everything since we can't use a constructor
    {
        hasSeen = sinceSeen = isWater = isHill = isFood = AntHereCheck = 0;
        ant = hillPlayer = -1;
        foodDif=0.;
        guardDif=0.;
    };
    void reset()//resets the information for the square except water information
    {
        isFood = AntHereCheck = 0;
        ant = hillPlayer = -1;
        sinceSeen++;
        //foodDif = 0;
        //deadAnts.clear();
    };
    bool isOpen()
    {
        return ((ant == -1) && (isWater == false) && (isFood == false) && (hasSeen == true));
    };//if an ant can move there
    bool difCheck()
    {
        return ((ant == -1) && (isWater == false) && (hasSeen == true) && (isFood == false) && hillPlayer <= 0);
    };//if there is nothing that diffuses (or is water)
    bool land()
    {
        return ((isWater == false)&& (hasSeen == true));
    };//if its land
    bool movable()
    {
        return ((isWater == false)&& (isFood == false));
    };//if its movable or potentially movable in the future on for battle checks
};

#endif //SQUARE_H_
