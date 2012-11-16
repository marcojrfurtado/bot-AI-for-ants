#pragma once
#include <unordered_set>
#include <array>
#include "Location.h"
#include "array.h"
#include "setops.h"
#include "hash.h"
#include "maxandmin.h"

namespace Symmetry{

inline unsigned int gcd(unsigned int a, unsigned int b){
    ASSERT(a > 0 && b > 0);
    //Possibly swap
    {auto c = b; if (b < a) {b = a; a = c;}}
    ASSERT(a <= b);

    while(b % a){
        auto r = b/a;
        b -= r*a;
        {auto c = b; if (b < a) {b = a; a = c;}}
    }

    return a;
}

inline unsigned int lcm(unsigned int a, unsigned int b) {return (a*b)/gcd(a,b);}

//Is there an integer multiple of x between min and max inclusive?
inline bool multBetween(unsigned int x, unsigned int min, unsigned int max) {return ((min-1)/x)+1 <= (max/x);}

enum AimType : unsigned int {I, R, nI, nR, F, RF, nF, nRF};
typedef unsigned int aim_t;

struct Offset { int y, x; };
struct Transform { Offset t; aim_t m; };
}

BEGIN_HASH(Symmetry::Offset) {return (arg.y << 8) ^ arg.x;} END_HASH
BEGIN_HASH(Symmetry::Transform) {return (arg.m << 16) ^ (arg.t.y << 8) ^ arg.t.x;} END_HASH
//BEGIN_HASH(Symmetry::Transform) {return 0;} END_HASH

namespace Symmetry{

const Transform IDENTITY = {{0,0},0};

inline Offset operator + (const Offset& a, const Offset& b) {return Offset{a.y + b.y, a.x + b.x};}
inline Offset operator - (const Offset& a) {return Offset{-a.y, -a.x};}
inline Offset operator - (const Offset& a, const Offset& b) {return a + -b;}

inline bool operator == (const Offset& a, const Offset& b) {return a.y == b.y && a.x == b.x;}
inline bool operator != (const Offset& a, const Offset& b) {return !(a == b);}
inline bool operator < (const Offset& a, const Offset& b) {return a.y < b.y || (a.y == b.y && a.x < b.x);}
inline bool operator == (const Transform& a, const Transform& b) {return a.t == b.t && a.m == b.m;}
inline bool operator != (const Transform& a, const Transform& b) {return !(a == b);}
inline bool operator < (const Transform& a, const Transform& b) {return a.t < b.t || (a.t == b.t && a.m < b.m);}

inline std::ostream& operator << (std::ostream& stream, const Offset& a) {return stream << "(" << a.x << "," << a.y << ")";}
inline std::ostream& operator << (std::ostream& stream, const Transform& a) {return stream << "(" << a.m << "," << a.t << ")";}

//Wraps from 0 to n-1 Note that the mod can't be declared as unsigned or we get a nasty implicit conversion bug
inline int wrapCoordU(/*unsigned*/int n, int x) {ASSERT(n>0); return (((x%n)+n)%n);}
//Wraps x from -n/2 to (n-1)/2.
inline int wrapCoordS(/*unsigned*/int n, int x) {int temp = wrapCoordU(n,x); return (temp*2 >= n) ? temp-n : temp; }

inline Offset doAim(Offset o, aim_t aim){
    ASSERT(aim < 8);

    switch (aim){
        case AimType::I : return o;
        case AimType::R : return Offset{-o.x,o.y};
        case AimType::nI : return Offset{-o.y,-o.x};
        case AimType::nR : return Offset{o.x,-o.y};
        case AimType::F : return Offset{-o.y,o.x};
        case AimType::RF : return Offset{-o.x,-o.y};
        case AimType::nF : return Offset{o.y,-o.x};
        case AimType::nRF : return Offset{o.x,o.y};
    }
    ASSERT(false); return o;
}

inline aim_t invAim(aim_t M) {return (M < 4 && (M&1)) ? (M ^ 2u) : M;}

//return a*b by composition
//inline aim_t mult(aim_t a, aim_t b) {
//    bool Fa = (a & 4); unsigned int Ra = (a & 3);
//    bool Fb = (b & 4); unsigned int Rb = (b & 3);
//
//    if (Fa){
//        if (Fb) {return (Ra + Rb + 2) % 4;}
//        return (Ra + Rb + (Rb & 1)) % 4 + 4;
//    }
//    if (Fb) {return (Ra + Rb) % 4 + 4;}
//    return (Ra + Rb) % 4;
//}

struct SymmetryFinder{
    //Must be member func due to name lookup rules
    aim_t mult(aim_t a, aim_t b) const{
        bool Fa = (a & 4); unsigned int Ra = (a & 3);
        bool Fb = (b & 4); unsigned int Rb = (b & 3);

        if (Fa) {Ra = 4-Ra;}
        unsigned int val = (Ra + Rb) % 4;

        if (Fa != Fb) {val += 4;}
        if (Fa && (val&1)) {val ^= 2;}
        return val;
    }

//    aim_t mult(aim_t a, aim_t b) const{
//        state.bug << "Mult(" << a << ',' << b << ") = " << mult2(a,b) << "\n";
//        return mult2(a,b);
//    }

    //////////////////////////////////////
    struct PlayerInfo{
        bool done;
        Transform transform;
        std::set<Transform> candidates;
    };

    State& state;
    unsigned int rows, cols;
    Array2d<unsigned int> grid;
    std::unordered_set<Transform> transforms;
    std::unordered_set<Transform> group;
    std::vector<Pos> hills;
    Pos basePos;

    std::array<PlayerInfo, 10> pdata;
    unsigned int minplayers, maxplayers;

    bool isConsistent(Transform t, Pos pos, bool water) const {
        ASSERT(hills.size()); //Make sure our base pos was initialized
//        Transform itran = inv(t);
//        Pos tranpos = doTransform(itran, pos);
        auto off = Offset{pos.row - basePos.row, pos.col - basePos.col} - t.t;
        auto off2 = doAim(off, invAim(t.m));
        Pos tranpos(wrapCoordU(rows, basePos.row + off2.y), wrapCoordU(cols, basePos.col + off2.x));

        ASSERT(doTransform(t, tranpos) == pos);
        return isUnknown(tranpos) || (water == isWater(tranpos));
    }

    Transform mult(Transform t1, Transform t2) const {return Transform{wrapOffset(doAim(t2.t, t1.m) + t1.t), mult(t1.m, t2.m)};}
    Transform inv(Transform t) const {return Transform{wrapOffset(-doAim(t.t, invAim(t.m))), invAim(t.m)};}

    Pos doTransform(Transform t, Pos pos) const{
        auto off = wrapOffset( {pos.row - basePos.row, pos.col - basePos.col} );
        auto off2 = t.t + doAim(off, t.m);
        return Pos(wrapCoordU(rows, basePos.row + off2.y), wrapCoordU(cols, basePos.col + off2.x));
    }

    Offset wrapOffset(Offset o) const {
        o.y = wrapCoordS(rows, o.y);
        o.x = wrapCoordS(cols, o.x);
        return o;
    }

    Offset getOffset(Pos start, Pos dest) const {return wrapOffset(Offset{dest.row-start.row, dest.col-start.col});}

    void setGridState(Pos pos, bool water) {
        unsigned int flag = water;
        ASSERT( grid[pos] & (~1u) || grid[pos] == flag);
        grid[pos] = flag;
    }

    unsigned int getWrapOrder(unsigned int mod, int x) const {
        if (x==0) {return 1;}
        unsigned int a = (x < 0) ? -x : x;
        return mod / gcd(mod, a);
    }

    //Does transform predict a hill at given loc?
    bool predictsHillAtLoc(Transform t, Pos pos) const {
        for(auto it2 = hills.begin(); it2 != hills.end(); ++it2){
            Pos hpos = doTransform(t, *it2);
            if (hpos == pos) {return true;}
        }
        return false;
    }

    unsigned int order(Transform t) const{
        const auto x = t.t.x; const auto y = t.t.y;
        switch(t.m){
            case AimType::I: return lcm(getWrapOrder(cols, x), getWrapOrder(rows, y));
            case AimType::R: return 4;
            case AimType::nI: return 2;
            case AimType::nR: return 4;
            case AimType::F: return lcm(getWrapOrder(cols, x), 2);
            case AimType::RF: return 2*lcm(getWrapOrder(cols, x-y), getWrapOrder(rows, y-x));
            case AimType::nF: return lcm(2, getWrapOrder(rows, y));
            case AimType::nRF: return 2*lcm(getWrapOrder(cols, x+y), getWrapOrder(rows, y+x));
        }
        ASSERT(false); return 1;
    }

    void checkTransformCount(){
        const auto tsize = (transforms.size()+1); const auto gsize = (group.size()+1);
        //Have we narrowed down the transforms enough to find all players?
        bool done = tsize <= minplayers;
        //Do an additional check for the case where we know that the current group is insufficient, and there is only one possible extension of the group
        done = done || (groupClosed() && (gsize < minplayers) && (tsize == 2*gsize));

        if (done){
            if (group.size() < transforms.size()) {state.bug << "Sucessfully inferred transforms!\n";}
            newGroupMembers.insert(newGroupMembers.end(), transforms.begin(), transforms.end());
            newSeenSquares.clear(); //No need for this anymore, we're done!
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////
    public:

    bool groupClosed() const {return newGroupMembers.empty();}
    bool groupDone() const {return group.size() == transforms.size();}

    bool isWater(Pos p) const {return grid[p] & 1u;}
    bool isUnknown(Pos p) const {return grid[p] & ~1u;}

    //initialize
    SymmetryFinder(State& s, unsigned int nrows, unsigned int ncols): state(s)
    {
        ASSERT(nrows > 4 && ncols > 4); ASSERT(nrows*ncols <= 25000 && nrows*ncols >= 1800);
        rows = nrows; cols = ncols;
        minplayers = max(2, (rows*cols)/5000); //According to specs, area/players is 900-5000
        maxplayers = min(10, (rows*cols)/900);


        grid = Array2d<unsigned int>(rows, cols, 2u);
        for(int i=0; i<10; ++i) {pdata[i].done = false;}

        const bool isSquare = (rows == cols);
        const auto maxOrder = maxplayers;

        //Create initial transform list
        for (int y = -((int)rows/2); y <= ((int)rows-1)/2; ++y){
            for (int x = -((int)cols/2); x <= ((int)cols-1)/2; ++x){
                if (x*x + y*y <= 25) {continue;} //Enemy hills can't start within range^2 of 25
                Offset off = {y,x};
                ASSERT(off == wrapOffset(off));

                //Only insert transforms with an order <= 10
                transforms.insert( Transform{off, AimType::nI} );
                if (isSquare && 4<=maxOrder) {transforms.insert( Transform{off, AimType::R} );}
                if (isSquare && 4<=maxOrder) {transforms.insert( Transform{off, AimType::nR} );}

                {
                    const auto yord = getWrapOrder(rows, y);
                    const auto xord = getWrapOrder(cols, x);
                    ASSERT(lcm(xord, yord) > 1);

//                    state.bug << Offset{x,y} << "\t" << Offset{xord,yord} << "\n";
                    if (lcm(xord, yord) <= maxOrder) {transforms.insert( Transform{off, AimType::I} );}
                    if (lcm(xord, 2) <= maxOrder) {transforms.insert( Transform{off, AimType::F} );}
                    if (lcm(yord, 2) <= maxOrder) {transforms.insert( Transform{off, AimType::nF} );}
                }
                if (isSquare)
                {
                    const auto yord = getWrapOrder(rows, y-x);
                    const auto xord = getWrapOrder(cols, x-y);
                    if (lcm(xord, yord)*2 <= maxOrder) {transforms.insert( Transform{off, AimType::RF} );}
                }
                if (isSquare)
                {
                    const auto yord = getWrapOrder(rows, y+x);
                    const auto xord = getWrapOrder(cols, x+y);
                    if (lcm(xord, yord)*2 <= maxOrder) {transforms.insert( Transform{off, AimType::nRF} );}
                }
            }
        }
    }

    //initialize with hill info
    void initializeHillLocs(const std::set<Pos>& newHills)
        {hills.insert(hills.end(), newHills.begin(), newHills.end()); basePos = hills.front();}

    //newly visible square
    void updateVisibleSquare(Pos pos, bool noHillAllowed){
        if (groupDone()) {return;}

        std::vector<Transform> removed;
        const bool water = isWater(pos);

        for(auto it = transforms.begin(); it != transforms.end(); ++it){
            auto t = *it;
//            if (removed.count(t)) {continue;}

            bool remove = !isConsistent(t, pos, water);

            if (noHillAllowed){ //Razing can't have happened yet, so if it predicts a hill in current spot, it's invalid
                 remove = remove || predictsHillAtLoc(t, pos);
            }

            if (remove){
//                state.bug << "Removing " << t << " due to square " << pos << ", which is " << water << "\n";

                ASSERT(!group.count(t));
                const Transform invT = inv(t);

                for(auto it2 = group.begin(); it2 != group.end(); ++it2){
//                    removed.insert(  mult(*it2, t)   );
//                    removed.insert(  mult(*it2, invT)   );
                    removed.push_back(  mult(*it2, t)   );
                    removed.push_back(  mult(*it2, invT)   );
                }

                removed.push_back(t);
                removed.push_back(invT);
//                removed.insert(t);
//                removed.insert(invT);
            }
        }

//        if (removed.size()) {differenceEq(transforms, removed);}
        for(auto it = removed.begin(); it != removed.end(); ++it) {transforms.erase(*it);}
        checkTransformCount();
    }

    //newly visible hill
    void updateNewHill(Pos pos, int owner){
        ASSERT(owner > 0 && owner < 10);
        PlayerInfo& data = pdata[owner];
        if (data.done){return;}

        state.bug << "Filtering hill at " << pos << " for player " << owner << "\n";

        bool firstTime = data.candidates.empty(); //Assume if we have no candidates that this is our first time, so we don't filter baed on previosu info
        std::set<Transform> newCandidates;

        if (firstTime){
            for(auto it = transforms.begin(); it != transforms.end(); ++it){
                if (predictsHillAtLoc(*it, pos)) {newCandidates.insert(*it);}
            }
        } else {
            for(auto it = data.candidates.begin(); it != data.candidates.end(); ++it){
                if (!transforms.count(*it)) {continue;} //Make sure it hasn't been invalidated since we first found it as candidate
                if (predictsHillAtLoc(*it, pos)) {newCandidates.insert(*it);}
            }
        }

//        state.bug << "Candidates: ";
//        for(auto it = newCandidates.begin(); it != newCandidates.end(); ++it){
//            state.bug << *it << "\t";
//        }
//        state.bug << "\n";

        state.bug << newCandidates.size() << " candidates remaining\n";
        ASSERT(newCandidates.size() && newCandidates.size() <= hills.size()*8);
        if (newCandidates.size() == 1){
            data.transform = *(newCandidates.begin());
            data.done = true;

            state.bug << "Fixed player " << owner << " to " << data.transform << "\n";
            newGroupMembers.push_back(data.transform);
        }
        else {data.candidates.swap(newCandidates);} //Store filtered list of candidates
    }

    //new minplayers
    void updateMinPlayers(unsigned int newval) {if (newval > minplayers) {minplayers = newval;}}

    void doOrderFiltering(){
        ASSERT(groupClosed()); //These tests assume that the group is currently closed
        std::vector<Transform> removed;

        if (group.empty()){
            if (minplayers <= (maxplayers/2 + 1)) {return;} //Nothing to remove by order test

            for(auto it = transforms.begin(); it != transforms.end(); ++it){
                auto t = *it;
                auto o = order(t); ASSERT(o <= 10 && o > 1);
                bool remove = !multBetween(o, minplayers, maxplayers); //Remove if there is no multiple of o between minplayers and maxplayers
                if (remove){removed.push_back(t);} //There is no need to remove conjugates too since they'll be gotten anyway
            }
        }
        else { //If we've reached this point there probably aren't many valid transforms left, so don't bother trying to optimize
            for(auto it = transforms.begin(); it != transforms.end(); ++it){
                auto t = *it;
                bool remove = false;
//                state.bug << "OTesting " << t << " time " << state.time() << "\n";

                //Make copy of group, add t to it, and make sure the new group isn't too big
                auto tempGroup = group;
                auto tempQueue = newGroupMembers;
                tempQueue.push_back(t);

                while(tempQueue.size()){
                    auto temp = tempQueue.back();
                    tempQueue.pop_back();

                    if (tempGroup.count(temp)) {continue;}
                    if (!transforms.count(temp)) {remove = true; break;}
                    tempGroup.insert(temp);
                    ASSERT(tempGroup.size() < 90);

                    for(auto it2 = tempGroup.begin(); it2 != tempGroup.end(); ++it2){
                        auto newT = mult(*it2, temp);
                        if (newT != IDENTITY) {tempQueue.push_back( newT );}
                    }
                }

                const auto g = tempGroup.size() + 1; //+1 since it doesn't include identity
                remove = remove || g>maxplayers;
                remove = remove || !multBetween(g, minplayers, maxplayers);
//                state.bug << "Remove " << remove << " gsize " << g << "\n";
                if (remove){removed.push_back(t);} //There is no need to remove conjugates too since they'll be gotten anyway
            }
        }

        if (removed.size()){
            differenceEq(transforms, removed);
            checkTransformCount();
        }
    }

    //Should only be called with data from queue
    void addToGroup(Transform t){
        if (group.count(t)) {return;}
        group.insert(t);
        ASSERT(group.size() < maxplayers);
        state.bug << "Adding " << t << "\tto group (new size " << group.size() << ")\n";

        //Update grid
        for(int y=0; y<(int)rows; ++y){
            for(int x=0; x<(int)cols; ++x)
            {
                Pos p(y,x); if(isUnknown(p)) {continue;}
                Pos tp = doTransform(t, p);
//                if (isUnknown(tp)) {newSquareObserved(tp, isWater(p));}
                if (isUnknown(tp)) {
                    newSquareObserved(tp, isWater(p));
//                    state.bug << "Inferring " << tp << " from " << p << "\t" << isWater(p) << "\n";
                }
            }
        }

        ASSERT(!group.count(IDENTITY));

        //Recursively insert group products until it is once again closed under *
        for(auto it2 = group.begin(); it2 != group.end(); ++it2){
            auto newT = mult(*it2, t);
            if (newT != IDENTITY && !group.count(newT)) {newGroupMembers.push_back( newT );}
        }
    }

    //Queue of inputs waiting to be processed
    std::vector<Pos> newSeenSquares;
    std::vector<Transform> newGroupMembers;

    void newSquareObserved(Pos pos, bool water) {
        if (isUnknown(pos)){
            setGridState(pos, water);
            //And set conjuagtes too
            for(auto it2 = group.begin(); it2 != group.end(); ++it2){
                setGridState(doTransform(*it2, pos), water);
            }

            newSeenSquares.push_back(pos); //Only need to search one, since conjuagtes are deleted anyway
        }
        else {
            ASSERT(isWater(pos) == water);
            setGridState(pos, water); //In case the map doens't meet specifications and we inferred incorrect info, override it with what we actually saw
        }
    }

    std::set<Pos> getPossibleHillLocs(size_t maxnum) const {
        std::set<Pos> locs;

        for(auto it = group.begin(); it != group.end(); ++it){
            for(auto it2 = hills.begin(); it2 != hills.end(); ++it2){
                locs.insert(doTransform(*it, *it2));
                if (maxnum && locs.size() >= maxnum) {return locs;}
            }
        }

        if (!groupDone()){
            for(auto it = transforms.begin(); it != transforms.end(); ++it){
                if (it->m != AimType::I) {continue;} //Prioritize pure translations over everything else

                for(auto it2 = hills.begin(); it2 != hills.end(); ++it2){
                    locs.insert(doTransform(*it, *it2));
                    if (maxnum && locs.size() >= maxnum) {return locs;}
                }
            }

            for(auto it = transforms.begin(); it != transforms.end(); ++it){
                if (it->m == AimType::I) {continue;} //Prioritize pure translations over everything else

                for(auto it2 = hills.begin(); it2 != hills.end(); ++it2){
                    locs.insert(doTransform(*it, *it2));
                    if (maxnum && locs.size() >= maxnum) {return locs;}
                }
            }
        }

        return locs;
    }

    std::vector<Pos> getConjugates(Pos pos) const{
        std::vector<Pos> locs;

        for(auto it = group.begin(); it != group.end(); ++it){
            const auto& t = *it;
            //Calc transformed position of pos by t
            auto off = Offset{pos.row - basePos.row, pos.col - basePos.col} - t.t;
            auto off2 = doAim(off, invAim(t.m));
            Pos tranpos(wrapCoordU(rows, basePos.row + off2.y), wrapCoordU(cols, basePos.col + off2.x));
            locs.push_back(tranpos);
        }
        return locs;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void sanityTests() const {
        //Test basic math funcs
        ASSERT(multBetween(6,6,10)); ASSERT(multBetween(3,8,10)); ASSERT(multBetween(13,61,65)); ASSERT(!multBetween(13,61,64));
        ASSERT(lcm(34523,12938) == 446658574); ASSERT(gcd(34523,12938) == 1);

        ASSERT(rows > 4 && cols > 4 && minplayers >= 2 && maxplayers <= 10);

        //Make sure all aims commute with identity, -identity, and have order at most 4
        for(aim_t a = 0; a<8; ++a) {
            ASSERT(mult(a,0) == mult(0,a));
            ASSERT(mult(a,2) == mult(2,a));
            ASSERT(mult(mult(a,a),mult(a,a)) == 0);
            if (a != 1 && a != 3) {ASSERT(mult(a,a) == 0);}

            ASSERT(mult(a, invAim(a)) == 0);
            ASSERT(mult(invAim(a), a) == 0);
        }

        //Every transform should have order <= 10
        const Transform IDENTITY = {{0,0},0};

        for(auto it = transforms.begin(); it != transforms.end(); ++it){
            ASSERT(mult(inv(*it), *it) == IDENTITY);
            ASSERT(mult(*it, inv(*it)) == IDENTITY);

            Transform exp = IDENTITY;
            unsigned int i;
            for(i=1; i<=11; ++i){
                exp = mult(exp, *it);
                if(exp == IDENTITY) {break;}
            }

//            state.bug << *it << "\t" << i << "\n";
            ASSERT(order(*it) == order(inv(*it)));
            ASSERT(i == order(*it));
            ASSERT(i <= 10 && i > 1);
        }
    }
};
}
