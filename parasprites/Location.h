#pragma once
#include "hash.h"

/*
    struct for representing locations in the grid.
*/
struct Pos
{
    int row, col;

    Pos()
    {
        row = col = 0;
    };

    Pos(int r, int c)
    {
        row = r;
        col = c;
    };
};

BEGIN_HASH(Pos) {return (arg.row << 8) ^ arg.col;} END_HASH

inline bool operator == (const Pos& a, const Pos& b) {return a.row == b.row && a.col == b.col;}
inline bool operator != (const Pos& a, const Pos& b) {return !(a == b);}
inline bool operator < (const Pos& a, const Pos& b) {return a.row < b.row || (a.row == b.row && a.col < b.col);}

inline std::ostream& operator << (std::ostream& stream, const Pos& a) {return stream << "(" << a.col << "," << a.row << ")";}

