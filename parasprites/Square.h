#ifndef SQUARE_H_
#define SQUARE_H_

#include <vector>

/*
    struct for representing a square in the grid.
*/
struct Square
{
    bool isExplored, isWater;
    bool isVisible, isHill;//isFood;
    int ant, hillPlayer;

    Square()
    {
        isExplored = isVisible = isWater = isHill = false;
//        isExplored = isVisible = isWater = isHill = isFood = false;
        ant = hillPlayer = -1;
    };

    //resets the information for the square except water information
    void reset()
    {
        isVisible = 0;
        isHill = 0;
//        isFood = 0;
        ant = hillPlayer = -1;
    };

    bool isMyAnt() const {return ant == 0;}
    bool isMyHill() const {return hillPlayer == 0;}
};

#endif //SQUARE_H_
