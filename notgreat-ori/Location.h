#ifndef LOCATION_H_
#define LOCATION_H_

/*
    struct for representing locations in the grid.
*/
struct Location
{
    bool operator==(const Location& Lb)
    {
        if( row == Lb.row && col == Lb.col)
            return true;
        return false;
    }
    int row, col;

    Location()
    {
        row = col = 0;
    };

    Location(int r, int c)
    {
        row = r;
        col = c;
    };
};
#endif //LOCATION_H_
