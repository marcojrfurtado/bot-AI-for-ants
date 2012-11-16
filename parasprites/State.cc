#include "State.h"

#include <algorithm>
#include "macros.h"
#include "symmetry.h"
#include "bfs.h"

#ifdef DEBUG
#include <iomanip>
#endif

//#define FOR_TCP_SERVER

using namespace std;

static int sqrti(int x){
    int v = 0;
    while ((v+1)*(v+1) < x) {++v;}
    return v;
}

const static unsigned int NOFOODDIST = 65536;

//constructor
State::State()
{
    //hillDefenceLevel = 0;
    gameover = 0;
    turn = 0;
    storedFood = 0;
    lastFilteredPossibleEnemiesSize = 100;
    bug.open("./debug.txt");
};

//deconstructor
State::~State()
{
    bug.close();
};

//sets the state up
void State::setup()
{
    ASSERT(max_turns < 65536);

    grid = Array2d<Square>(rows, cols);
    lastSeen = Array2d<unsigned int>(rows, cols, 65536u); //Unexplored areas have a large lastseen value, greater than maximum possible game length
    distToFood = Array2d<unsigned int>(rows, cols);
    distToHills = Array2d<unsigned int>(rows, cols);
    myAntInfluence = Array2d<unsigned int>(rows, cols);
    enemyInfluence = Array2d<unsigned int>(rows, cols);
    possibleEnemies = decltype(possibleEnemies)(rows, cols, true); //Everything is initially a possible enemy.

    symmetry = decltype(symmetry)(new Symmetry::SymmetryFinder(*this, rows, cols));
#ifdef TESTING
    symmetry->sanityTests();
#endif

//    symmetry->initialize(rows, cols, attackrad2);
    bug << "Setup transforms remaining " << symmetry->transforms.size() << " time " << time() << "\n";
};

//outputs move information to the engine
void State::makeMove(const Pos &loc, int direction)
{
    //Important to use endl so that stream is flushed properly
    cout << "o " << loc.row << " " << loc.col << " " << CDIRECTIONS[direction] << endl;
//    bug << "o " << loc.row << " " << loc.col << " " << CDIRECTIONS[direction] << endl;
};

int State::distMan(const Pos &loc1, const Pos &loc2) const
{
    int d1 = abs(loc1.row-loc2.row),
        d2 = abs(loc1.col-loc2.col),
        dr = min(d1, rows-d1),
        dc = min(d2, cols-d2);
    return dr+dc;
};

int State::distEuc2(const Pos &loc1, const Pos &loc2) const
{
    int d1 = abs(loc1.row-loc2.row),
        d2 = abs(loc1.col-loc2.col),
        dr = min(d1, rows-d1),
        dc = min(d2, cols-d2);
    return (dr*dr + dc*dc);
};

int State::distInf(const Pos &loc1, const Pos &loc2) const
{
    int d1 = abs(loc1.row-loc2.row),
        d2 = abs(loc1.col-loc2.col),
        dr = min(d1, rows-d1),
        dc = min(d2, cols-d2);
    return max(dr, dc);
};

Pos State::wrapPos(const Pos& loc) const{
    return Pos((loc.row + rows) % rows, (loc.col + cols) % cols);
}

//returns the new location from moving in a given direction with the edges wrapped
Pos State::getDest(Pos p, int direction) const
{
    if (direction >= 4) {return p;}
    return wrapPos(p.row + DIRECTIONS[direction][0], p.col + DIRECTIONS[direction][1]);
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Weight close distances more strongly
static int adjustFoodScore(int fdist){
//    ASSERT(fdist >= 2); //Due to ant influence, the min is 2, not 0, so readjust it
//    fdist -= 2;
    if (fdist < 7) {return 4 * fdist;}
    else if (fdist < 256) {return 2 * fdist + 13;}
    else {return 524;}
}

static int adjustHillScore(int fdist){
    if (fdist < 3) {return 3 * fdist;} else {return fdist + 5;}
}

static int getFoodPenalty(const State& state){
    int foodpenalty = state.myAnts.size() - state.enemyAnts.size()*3 - state.myHills.size()*3;
    int stored = state.storedFood - state.myHills.size();

    foodpenalty += state.turn/20;
    if (stored > 0) {foodpenalty += stored*stored*(1+state.myAnts.size()/16);}
    foodpenalty = (foodpenalty > 0) ? foodpenalty : 0;
    return foodpenalty;
}

int State::getNonCombatScore(Pos pos) const {
    int foodscore = adjustFoodScore(distToFood[pos]) + getFoodPenalty(*this) + 2*myAntInfluence[pos];
    int hillscore = adjustHillScore(distToHills[pos]);
    int mainscore = (hillscore < foodscore) ? hillscore : foodscore;
    return mainscore;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void State::updateNewlySpawnedFood(){
    if (turn <= 1) {return;}

    newlySpawnedFood.clear();
    for(auto it = seenInput.food.begin(); it != seenInput.food.end(); ++it){
        const Pos p = *it;
        bool isnew = !food.count(p);
        isnew = isnew && (lastSeen[p] == 0) && !myAnts.count(p) && !enemyAnts.count(p);
        if (isnew) {newlySpawnedFood.insert(p);}

//        if (lastSeen[p] == 0){
//            bug << p << " " << lastSeen[p] << " " << food.count(p) << "\n";
//        }
    }
}

void State::updateVisionInformation()
{
    //Increment last seen counts
    for(int y=0; y<rows; ++y){  for(int x=0; x<cols; ++x){  lastSeen(y,x)++;    }}

//    std::vector<Pos>& newseen = symmetry->newSeenSquares;
    for(auto it = seenInput.myAnts.begin(); it != seenInput.myAnts.end(); ++it){
        Pos a = *it;

        for(int y = -viewrad1; y <= viewrad1; ++y){
            for(int x = -viewrad1; x <= viewrad1; ++x){
                if ((x*x + y*y) > viewrad2) {continue;}
                Pos p = wrapPos(a.row + y, a.col + x);
                lastSeen[p] = 0;

                symmetry->newSquareObserved(p, grid[p].isWater);
            }
        }

//        for(int y = 0; y <= viewrad1; ++y){
//            for(int x = 0; (x*x + y*y) <= viewrad2; ++x){
//                lastSeen[wrapPos(a.row + y, a.col + x)] = 0;
//                lastSeen[wrapPos(a.row + y, a.col - x)] = 0;
//                lastSeen[wrapPos(a.row - y, a.col + x)] = 0;
//                lastSeen[wrapPos(a.row - y, a.col - x)] = 0;
//            }
//        }
    }
};

void State::updateSeenHills()
{
    //Find and remove hills that no longer exist
    for(auto it = myHills.begin(); it != myHills.end(); /**/){
        auto old_it = it++;
        auto& h = old_it->second;

        if (visible(h.pos) && !seenInput.myHills.count(h.pos)){
            bug << "My hill at " << h.pos << " razed!\n";
            myHills.erase(old_it);
        }
    }

    //Add new hills
    for(auto it = seenInput.myHills.begin(); it != seenInput.myHills.end(); ++it)
    {
        Pos pos = *it;
        if (!myHills.count(pos))
        {
            ASSERT(turn == 1);
            bug << "Initializing my hill " << pos << "\n";

            myHill h(pos);
            h.dists = decltype(h.dists)(rows, cols, ~0u);
            h.openset.push_back(h.pos);
            h.dists[h.pos] = 0;
            myHills.insert(std::make_pair(h.pos, std::move(h)));
        }
    }

    //Now check for spawning and update stored food. This is called after updateAgents, so we (think we) know survivor ants
    if (turn > 1){
        //If we have enough food to go around, any hill which didn't spawn is garuenteed to be razed
        const bool expectSpawning = (storedFood >= myHills.size());

        for(auto it = myHills.begin(); it != myHills.end(); /**/){
            auto old_it = it++;
            auto& h = old_it->second;

            if (!seenInput.myAnts.count(h.pos)) { //No ant on hill
                if (expectSpawning){
                    bug << "My hill at " << h.pos << " razed! (inferred from lack of spawning)\n";
                    ASSERT(!visible(h.pos)); //should have caught it earlier if visible
//                    myHills.erase(old_it);
                    if (!visible(h.pos)) {myHills.erase(old_it);}
                }
            }
            else { //We probably spawned an ant
                bool occupied = seenInput.myAnts.count(h.pos);
                bool diedOnHill = seenInput.myDeadAnts.count(h.pos);
//                if ((!occupied || diedOnHill) && storedFood){ storedFood--;}
//                bug << occupied << "\t" << diedOnHill << "\t";

                if (occupied && !diedOnHill && storedFood){
                    storedFood--;
                    bug << "spawning at " << h.pos << "\n";
                }
            }
        }

        bug << "Estimated food left: " << storedFood << "\n";
    }

    //Now do enemy hills //////////////////////////////////////////////////////////

    //Find and remove hills that no longer exist
    for(auto it = enemyHills.begin(); it != enemyHills.end(); /**/){
        auto old_it = it++;
        auto& h = old_it->second;

        if (visible(h.pos) && !seenInput.enemyHills.count(h.pos)){
            bug << "Enemy hill at " << h.pos << " razed!\n";
            enemyHills.erase(old_it);
        }
    }

    //Add new hills
    for(auto it = seenInput.enemyHills.begin(); it != seenInput.enemyHills.end(); ++it)
    {
        auto key = *it;
        Pos pos = get<0>(key);
        auto owner = get<1>(key);

        if (!enemyHills.count(pos))
        {
            bug << "Initializing enemy hill " << pos << "\n";

            ownedHill h(pos, owner);
            enemyHills.insert(std::make_pair(pos, std::move(h)));
        }
    }
}

void State::updateAgentStates()
{
    myAnts.clear();
//    std::set<Pos> seenAnts;
//    seenAnts.insert(seenInput.myAnts.begin(), seenInput.myAnts.end());
    const auto& seenAnts = seenInput.myAnts;

//    myAnts.reserve(seenAnts.size());
    //Create new myAnts
    for(auto it = seenAnts.begin(); it != seenAnts.end(); ++it){
//        myAnts.push_back(Ant(*it));
        auto pos = *it;
        myAnts.insert(std::make_pair(pos, Ant(pos)));
    }


    //Update enemy ants. This is simpler as they currently don't have any state
    enemyAnts.clear();

    for(auto it = seenInput.enemyAnts.begin(); it != seenInput.enemyAnts.end(); ++it){
        auto pos = get<0>(*it);
        enemyAnts.insert(std::make_pair(pos, EnemyAnt(pos)));
//        enemyAnts.push_back(EnemyAnt(get<0>(*it)));
    }
}

void State::updateAntInfluences()
{
    for(int y=0; y<rows; ++y){  for(int x=0; x<cols; ++x){  myAntInfluence(y,x) = 0; enemyInfluence(y,x) = 0;   }}

    for(auto it = myAnts.begin(); it != myAnts.end(); ++it){
        Pos a = it->first;

        for(int y = -3; y <= 3; ++y){
            for(int x = -3; x <= 3; ++x){
                if ((x*x + y*y) > 10) {continue;}

                auto mdist = abs(x) + abs(y);
                unsigned int influence = (mdist <= 2) ? (4-mdist) : 1;

                Pos p = wrapPos(a.row + y, a.col + x);
                myAntInfluence[p] += influence;
//                bug << p << ": " << myAntInfluence[p] << "\n";
            }
        }

//        for(int y = 0; y <= attackrad1; ++y){
//            for(int x = 0; (x*x + y*y) <= attackrad2; ++x){
//                myAntInfluence[wrapPos(p.row + y, p.col + x)]++;
//                myAntInfluence[wrapPos(p.row + y, p.col - x)]++;
//                myAntInfluence[wrapPos(p.row - y, p.col + x)]++;
//                myAntInfluence[wrapPos(p.row - y, p.col - x)]++;
//            }
//        }
    }

    //Halve influence: 0->0, 1->1, 2->1, 3->2, 4->2, etc.
    for(int y=0; y<rows; ++y){ for(int x=0; x<cols; ++x){  myAntInfluence(y,x) = (myAntInfluence(y,x)+1)/2; }}


    for(auto it = enemyAnts.begin(); it != enemyAnts.end(); ++it){
        Pos p = it->first;

        enemyInfluence[wrapPos(p.row + 0, p.col + 0)]++;
        enemyInfluence[wrapPos(p.row + 1, p.col + 0)]++;
        enemyInfluence[wrapPos(p.row - 1, p.col + 0)]++;
        enemyInfluence[wrapPos(p.row + 0, p.col + 1)]++;
        enemyInfluence[wrapPos(p.row + 0, p.col - 1)]++;
    }
}

#include "priorityqueue.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static unsigned int getAStarSearchKey(const State& state, const Array2d<unsigned int>& distMap, const unsigned int distIncrement,
                                     Pos target, Pos node){
    return distMap[node] + state.distMan(node, target)*distIncrement;
}

template<typename T>
static void astarSearchToTarget(State& state, T distCallback, Array2d<unsigned int>& distMap, PriorityQueue<Pos, unsigned int>& queue,
                               const unsigned int distIncrement, Pos target, unsigned int& nodecount)
{
    //Reassign keys to correspond to new target
    for(auto tup_it = queue.data.begin(); tup_it != queue.data.end(); ++tup_it){
        Pos node = get<0>(*tup_it);
        get<1>(*tup_it) = getAStarSearchKey(state, distMap, distIncrement, target, node);
    }
    queue.heapify();

    //Even if we found a path to the target, keep looking as long as there's a possibility of something better
    while(queue.size() && queue.minKey() < distMap[target])
    {
//        const auto node = get<0>( queue.pop() );
        const Pos node = queue.pop();
        auto nextDepth = distMap[node] + distIncrement;

        //Search specific callback to customize distance propogation
        nextDepth = distCallback(state, node, nextDepth);

        ++nodecount;
        for(int d=0; d<4; d++)
        {
            Pos next = state.getDest(node, d);
            if (state.symmetry->isWater(next)) {continue;}
            if (distMap[next] <= nextDepth) {continue;} //Already visited
            distMap[next] = nextDepth;

            queue.push(next, getAStarSearchKey(state, distMap, distIncrement, target, next));
        }
    }
}

template<typename T>
static void astarSearchMain(State& state, T distCallback, Array2d<unsigned int>& distMap, PriorityQueue<Pos, unsigned int>& queue,
                               const unsigned int distIncrement)
{
    state.bug << "Beginning A* Time: " << state.time() << "ms\n";
    unsigned int nodecount = 0;

    //Do a BFS until we have a score for every ant dest and hill
    for(auto it = state.myAnts.begin(); it != state.myAnts.end(); ++it){
        const Pos target = it->first;

        for(int d=0; d<5; d++){
            astarSearchToTarget(state, distCallback, distMap, queue, distIncrement, state.getDest(target, d), nodecount);
        }
    }

    for(auto it = state.myHills.begin(); it != state.myHills.end(); ++it){
        const Pos target = it->first;
        astarSearchToTarget(state, distCallback, distMap, queue, distIncrement, target, nodecount);
    }

    state.bug << "Nodes expanded: " << nodecount << " in " << state.time() << " ms\n";
}

static unsigned int foodDistAdjustCallback(const State& state, Pos node, unsigned int nextDepth){
    nextDepth += state.myAntInfluence[node] + state.enemyInfluence[node];

    //Reexpand from squares that haven't been explored
    if (state.distToFood[node] && !state.explored(node)) {nextDepth = 1;} //ant influence should be 0 here anyway
//        if (nextDepth > 3 && state.lastSeen[node] > 50) {nextDepth = 3;}
    return nextDepth;
}

void State::updateFood()
{
    std::set<Pos> newfood;

    //Make a set of ants that are not newly spawned
    const auto survivorAntLocs = setDifference<std::set<Pos>>(seenInput.myAnts, seenInput.myDeadAnts);

    //Process old food
    for(auto it = food.begin(); it != food.end(); ++it){
        if (!visible(*it)){ //If the oldfood is not visible, assume it still exists and insert it as a food
            newfood.insert(*it);
        }

        for(int i=0; i<4; ++i){
            Pos p = getDest(*it, i);
//            if (survivorAntLocs.count(p)) {storedFood++; break;}
            if (survivorAntLocs.count(p)) {
                storedFood++;
                bug << "Eating food at " << (*it) << " by " << p << "\n";
                break;
            }
        }
    }
    bug << "Estimated food left: " << storedFood << "\n";

    for(auto it = seenInput.food.begin(); it != seenInput.food.end(); ++it){
        auto p = *it;

        newfood.insert(p);
        if (newlySpawnedFood.count(p) && symmetry->group.size()){
            auto conjugates = symmetry->getConjugates(p);

            for(auto it2 = conjugates.begin(); it2 != conjugates.end(); ++it2){
                //Use possible enemies as a proxy for controlled territory. Don't bother adding it if an enemy will probably get it first
                if (!visible(*it2) && !possibleEnemies[*it2]) {newfood.insert(*it2);}

//                if (!visible(*it2) && !possibleEnemies[*it2]) {
//                if (!visible(*it2)) {
//                    newfood.insert(*it2);
//                    bug << "Inferred food at " << *it2 << " from " << p << "\n";
//                }
            }
        }
    }

    food.swap(newfood);

    // Update DistToFood ///////////////////////////////////////////////////////////////////////////////////
    for(int y=0; y<rows; ++y){  for(int x=0; x<cols; ++x){  distToFood(y,x) = NOFOODDIST;    }}
    if (enoughStoredFood()) {return;} //Don't bother searching for food if we have enough stored to spawn for rest of game

    PriorityQueue<Pos, unsigned int> queue;
    for(auto it = food.begin(); it != food.end(); ++it){
        distToFood[*it] = 0;
        queue.push(*it, 0); //Use 0 as temporary key
    }

    //Do the main searching using generic function
    astarSearchMain(*this, foodDistAdjustCallback, distToFood, queue, 1u);

//if (turn == 8){
//    bug << "Food map \n";
////    const auto& hdists = myHills.begin()->second.dists;
//    for(int y=0; y<rows; ++y){
//        for(int x=0; x<cols; ++x){
////            if (symmetry->isWater(Pos(y,x))) {bug << 'W';}
////            else if (symmetry->isUnknown(Pos(y,x))) {bug << '?';}
////            else if (grid(y,x).hillPlayer > -1) {bug << 'H';}
////            else if (grid(y,x).isMyAnt()) {bug << 'A';}
////            else if (hdists(y,x) < NOFOODDIST) {bug << '*';}
////            else {bug << ' ';}
//
//            bug << setw(3);
//            if (symmetry->isWater(Pos(y,x))){bug << 'W';}
//            else if (grid(y,x).hillPlayer > 0){bug << 'H';}
//            else if (grid(y,x).isMyAnt()){bug << 'A';}
//            else if (distToFood(y,x) >= NOFOODDIST){bug << ' ';}
//            else {
//                bug << distToFood(y,x);
//            }
//            if (x+1 < cols) {bug << ',';}
//        }
//        bug << "\n";
//    }
//}

}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const unsigned int HILLDISTINC = 2;

//True if there are at least 2 ants within attack range and at most 1 enemy in range+2
static bool isHillUnderControl(const State& state, Pos pos)
{
    auto rad1 = state.attackrad1;
    auto rad2 = state.attackrad2;
    unsigned int closeCount = 0;

    for(int y = -rad1; y <= rad1; ++y){
        for(int x = -rad1; x <= rad1; ++x){
            if ((x*x + y*y) > rad2) {continue;}
            Pos p = state.wrapPos(pos.row + y, pos.col + x);
            closeCount += state.seenInput.myAnts.count(p);
        }
    }

    if (closeCount < 2) {return false;}
    unsigned int ecnt = 0;

    //Increase search range by 2
    rad2 += rad1*4 + 4;
    rad1 += 2;

    for(int y = -rad1; y <= rad1; ++y){
        for(int x = -rad1; x <= rad1; ++x){
            if ((x*x + y*y) > rad2) {continue;}
            Pos p = state.wrapPos(pos.row + y, pos.col + x);
//            acnt += state.seenInput.myAnts.count(p);
            ecnt += state.seenInput.enemyAnts.count(p);
            if (ecnt > 1) {return false;}
        }
    }

    return true;
}

static unsigned int hillDistAdjustCallback(const State& state, Pos node, unsigned int nextDepth){
    auto eInf = state.enemyInfluence[node];
    auto aInf = state.myAntInfluence[node];
    eInf *= eInf;
    if (eInf > aInf) {nextDepth += (eInf - aInf);}
    else if (!eInf && nextDepth > state.attackrad1*HILLDISTINC*2) {nextDepth += aInf;}

    return nextDepth;
}

void State::updateHillDistances()
{
    // Update DistToHills ///////////////////////////////////////////////////////////////////////////////////
    for(int y=0; y<rows; ++y){  for(int x=0; x<cols; ++x){  distToHills(y,x) = NOFOODDIST;    }}

    std::vector<Pos> sources;
    for(auto it = enemyHills.begin(); it != enemyHills.end(); ++it) {
        Pos pos = it->first;

        //If hill is under control, propogate a small distance so guards can raze it, but no further so other ants don't waste their time
        if (isHillUnderControl(*this, pos)) {
            std::deque<Pos> q(1, pos);
            distToHills[pos] = 0;

            //Mini BFS copy originally taken from updateHill
            while(q.size())
            {
                const auto node = q.front();
                q.pop_front();

                const auto depth = distToHills[node];
                if (depth >= (unsigned)attackrad1) {break;}
                expandBFS(*this, distToHills, q, node);
            }
        }
        else {sources.push_back(it->first);}
    }

    if (turn > 3){
        auto possibleHills = symmetry->getPossibleHillLocs(100);
        size_t count = 0;

        for(auto it = possibleHills.begin(); it != possibleHills.end(); ++it)
        {
            if (!explored(*it)) {sources.push_back(*it); ++count;}
//            else {bug << "Bad loc at " << *it << "\n";}
        }

        bug << count << " possible hill locs found from inference\n";
    }

    PriorityQueue<Pos, unsigned int> queue;
    for(auto it = sources.begin(); it != sources.end(); ++it){
        distToHills[*it] = 0;
        queue.push(*it, 0); //Use 0 as temporary key
    }

    //Do the main searching using generic function
    astarSearchMain(*this, hillDistAdjustCallback, distToHills, queue, HILLDISTINC);

//if (turn  == 8) {
//    for(int y=0; y<rows; ++y){
//        for(int x=0; x<cols; ++x){
//            bug << setw(3);
//            if (symmetry->isWater(Pos(y,x))){bug << 'W';}
//            else if (grid(y,x).hillPlayer > 0){bug << 'H';}
//            else if (grid(y,x).isMyAnt()){bug << 'A';}
//            else if (distToHills(y,x) >= NOFOODDIST){bug << ' ';}
//            else {
//                bug << distToHills(y,x);
//            }
//            if (x+1 < cols) {bug << ',';}
//        }
//        bug << "\n";
//    }
//}

}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void State::updatePossibleEnemies()
{
    auto& oldVals = possibleEnemies;
    decltype(possibleEnemies) newVals(rows, cols, false);

    for (auto it = enemyAnts.begin(); it != enemyAnts.end(); ++it) {newVals[it->first] = true; ASSERT(visible(it->first));}

    for(int y=0; y<rows; ++y){  for(int x=0; x<cols; ++x){
        Pos p(y,x);

        if (!visible(p)){
            bool newval = false;

            if (!symmetry->isWater(p)){
                //There could be an enemy here if it is a mound or there was a possible neighbor last turn
                //If we found a cause, stop checking early
                if (oldVals[p]) {newval = true;}

                for(int d=0; !newval && d<4; d++)
                {
                    Pos next = getDest(p, d);
                    if (oldVals[next]) {newval = true;}
                }

                //We can get away with only checking seen hills, because the only way a square can lose its possible enemy status
                //Is if it was seen. This may need to be changed later if we used a more sophisticated algorithm
                if (enemyHills.count(p)) {newval = true;}
            }

            newVals[p] = newval;
        }
    }}

    //Do possible hill location filtering. This is useful for maps where we can quickly narrow down the possible hill locations
    if (turn > 1 && turn < 50){
        if (symmetry->transforms.size() < lastFilteredPossibleEnemiesSize){
            lastFilteredPossibleEnemiesSize = symmetry->transforms.size();

            Timer timer; timer.start();
            auto possibleHills = symmetry->getPossibleHillLocs(0);
            Array2d<unsigned int> dists = Array2d<unsigned int>(rows, cols, ~0u);
            std::deque<Pos> q;

            for(auto it = possibleHills.begin(); it != possibleHills.end(); ++it) {
                Pos p = *it;
                if (!explored(p)) {dists[p] = 0; q.push_back(p);}
            }
            for(auto it = enemyHills.begin(); it != enemyHills.end(); ++it) {
                Pos p = it->first;
                dists[p] = 0; q.push_back(p);
            }

            bug << "qsize " << q.size() << "\n";
            //Mini BFS copy originally taken from updateHill
            while(q.size())
            {
                const auto node = q.front();
                q.pop_front();

                const auto depth = dists[node];\
                ASSERT(depth < NOFOODDIST);
                if (depth >= (unsigned)turn-1) {break;}

                expandBFS(*this, dists, q, node);
            }

            unsigned int count = 0;
            for(int y=0; y<rows; ++y){  for(int x=0; x<cols; ++x){
                Pos p(y,x);

                if (dists[p] >= (unsigned)turn)
                {
                    if (newVals[p]) {ASSERT(!visible(p)); count++;}
                    newVals[p] = false;
                }
            }}

            bug << count << " possible enemy squares filtered in " << timer.getTime() << " ms\n";
        }
    }

    possibleEnemies.swap(newVals);
};

//static void updateHill(const State& state, myHill& h){
static void updateHill(State& state, myHill& h){

    if (state.myAnts.count(h.pos)) {h.lastTouched = 0;} else {++h.lastTouched;}
    state.bug << h.pos << " lastTouched " << h.lastTouched << "\n";

    ASSERT((int)h.lastTouched <= state.turn);

    ASSERT(h.dists.size()); //Make sure it's initialized
    unsigned int lastdepth=0;

    //Build up distances until we run into an unexplored tile
    //while(h.openset.size() && state.explored(h.openset.front())
    while(h.openset.size() && !state.symmetry->isUnknown(h.openset.front()))
    {
        const auto node = h.openset.front();
        h.openset.pop_front();

        if (state.symmetry->isWater(node)) {continue;} //It may have been unknown when we added it to queue, so we need to recheck it!
        const auto depth = h.dists[node];

        if (depth > lastdepth){
            state.bug << "Expanding " << node << " depth " << depth << " time: " << state.time() << "\n";
            lastdepth = depth;
        }

        ASSERT(depth < NOFOODDIST);
        expandBFS(state, h.dists, h.openset, node);

        if (!h.openset.size()) {state.bug << "Out of nodes!\n";}
    }

    if (h.openset.size()){
        state.bug << "Next node to expand: " << h.openset.front() << " depth " << h.dists[h.openset.front()] << "\n";
    }

    h.closestEnemyDist = ~0u;
    for(int y=0; y<state.rows; ++y){
        for(int x=0; x<state.cols; ++x){
            Pos p(y,x);

            if (state.symmetry->isWater(p)) {continue;}
            if (!h.knownDistance(p) && !h.openset.size()){continue;} //If the search terminated and the square wasn't reached, we can assume it is water, since the map must be connected

            if (state.possibleEnemies[p]){
                if (h.lbDist(p) < h.closestEnemyDist){
                    h.closestEnemyDist = h.lbDist(p);
                    h.closestEnemyPos = p;
                }
            } //Enemy hills could spawn an ant next turn, so count them as an enemy at dist+1 if they're empty
            //technically, we should check possible unexplored hills too, but it's unlikely to be a problem
            else if (state.enemyHills.count(p)){
                if (h.lbDist(p)+1 < h.closestEnemyDist){
                    h.closestEnemyDist = h.lbDist(p)+1;
                    h.closestEnemyPos = p;
                }
            }
        }
    }

    state.bug << "Closest enemy of " << h.pos << " is " << h.closestEnemyDist << " at " << h.closestEnemyPos << "\n";
}

static void updateHills(State& state){
    state.bug << "Beginning updateHills - time: " << state.time() << "/" << state.turntime << " ms\n";
    for(auto it = state.myHills.begin(); it != state.myHills.end(); ++it) {updateHill(state, it->second);}
}

struct spawnInfo{
    unsigned int lastTouched;
    int score;
    unsigned int blockable; //number of ants within dist 1
    int ocost; //Difference of best neighbor. Used to estimate chance of actually blocking the hill
    Pos pos;

    //key for sorting
    decltype(std::make_tuple(lastTouched, score, blockable)) tup() const {return std::make_tuple((~0u) - lastTouched, -score, blockable);}
};

bool operator < (const spawnInfo& a, const spawnInfo& b) {return a.tup() < b.tup();}

//Evaluate best score of all possible blocking assignments. No pruning is done
int spawnTreeSearch(std::deque<spawnInfo> info /*copied*/, unsigned int foodcnt){
    if (!foodcnt) {return 0;}
    if (!info.size()) {return 1999*foodcnt;} //Arbitrary penalty for failing to use all our spawns this turn

    int score = 0;
    while(foodcnt && info.size() && !info.front().blockable){
        score += info.front().score;
        --foodcnt;
        info.pop_front();
    }

    if (foodcnt && info.size()){
        auto node = info.front();
        info.pop_front();

        ASSERT(node.blockable);
        int noblockscore = node.score + spawnTreeSearch(info, foodcnt-1);
        int blockscore = spawnTreeSearch(info, foodcnt);
        score += min(blockscore, noblockscore);
    }
    return score;
}

void State::updateSpawnBlocking()
{
    spawnBlockIncentives.clear();
    if (myHills.size() <= 1 || enoughStoredFood()) {return;}
    auto foodcnt = storedFood;

    if (foodcnt){
        std::deque<spawnInfo> info;
        auto blockableHillCount = 0u; //Since our method is exponential in the number of blockable hills,
        //restrict ourselves to considering at most 6 blockable hills

        for(auto it = myHills.begin(); it != myHills.end(); ++it){
            const auto& h = it->second;

            auto bestn = getNonCombatScore(h.pos);
            auto blockable = 0u;

            for(int d=0; d<5; d++){
                auto dest = getDest(h.pos, d);
                if (symmetry->isWater(dest) || food.count(dest)) {continue;}

                blockable += myAnts.count(dest);
                bestn = min(bestn, getNonCombatScore(dest));
            }

            if (blockable){if (blockableHillCount++ >= 6) {continue;}}
            auto score = getNonCombatScore(h.pos) + 64*(min(h.closestEnemyDist, 7) + min(h.numGuards, 7));
            auto ocost = bestn - getNonCombatScore(h.pos);
            ASSERT(ocost <= 0);

            info.push_back(spawnInfo{h.lastTouched, score, blockable, ocost, h.pos});

            bug << "HScore " << h.pos << ": " << getNonCombatScore(h.pos) << " " << score << "\n";
        }

        sort(info);

        std::map<Pos, int> incentives;
        while(foodcnt && info.size()){
            if (info.front().blockable){
                //Decide how much we want to block this hill

                auto node = info.front();
                info.pop_front();
                int noblockscore = node.score + spawnTreeSearch(info, foodcnt-1);
                int blockscore = spawnTreeSearch(info, foodcnt);
                int incentive = blockscore - noblockscore;
                incentives[node.pos] = incentive;

                //Decide how likely we are to actually block the hill, so we can evaulate future choices
                if (incentive >= node.ocost){ --foodcnt;}
                bug << "Incentive to block " << node.pos << ": " << incentive << " ocost " << node.ocost << "\n";
            }
            else {info.pop_front(); --foodcnt;}
        }

        spawnBlockIncentives.swap(incentives);
        ASSERT(spawnBlockIncentives.size() <= myHills.size());
    }

    //Now assign blockers
    if (myAnts.size() <= myHills.size()*2) {return;}

    for(auto it = myHills.begin(); it != myHills.end(); ++it){
        const auto& h = it->second;

        for(int d=0; d<5; d++){
            auto dest = getDest(h.pos, 4-d); //Iterate in reverse order so we pick ants on the hill first

            if (myAnts.count(dest)){
                auto& ant = myAnts.at(dest);
                if (ant.bhill) {continue;}
                ant.bhill = &h;
                break;
            }
        }
    }
}

void State::update(){
    bug << "turn " << turn << ":" << endl;
    if (turn == 1) {symmetry->initializeHillLocs(seenInput.myHills);}

    updateNewlySpawnedFood(); //Must go first since it uses vision and ant information from last turn
    updateVisionInformation();
    updateAgentStates();
    updateSeenHills();  //If this is changed to go before updateagents, make sure not to break anything

    //Update ant influence map before food since it is used for goal occlusion
    updateAntInfluences();
    updateHillDistances();
    updateFood();
    updatePossibleEnemies(); //make sure this happens after hill spawn detection, and enemy(agent) updates
    updateHills(*this);
    updateSpawnBlocking();

    bug << "Food penalty " << getFoodPenalty(*this) << "\n";
}

void State::updateSymmetry(){
    bug << "Updating symmetry... " << endl;

    for(auto it = enemyHills.begin(); it != enemyHills.end(); ++it){
        //Call this even for a previously seen hill because we may have new information about it.
        symmetry->updateNewHill(it->second.pos, it->second.owner);

        while(symmetry->newGroupMembers.size() && timeCheckFrac(0.83)){
            auto trans = symmetry->newGroupMembers.back();
            symmetry->newGroupMembers.pop_back();
            symmetry->addToGroup(trans);

            if (symmetry->newGroupMembers.empty()) {symmetry->doOrderFiltering();} //Last iteration
        }
    }

    const bool isEarlyGame = turn <= 20; //Hills are supposed to be at least 20 apart, so no razing can happen in first 20 turns

    while(!symmetry->groupDone() && symmetry->newSeenSquares.size() && timeCheckFrac(0.9)){
        Pos sq = symmetry->newSeenSquares.back();
        symmetry->newSeenSquares.pop_back();

        //If it predicts a hill in a place where none was seen and can't have been razed, it's invalid
        bool noHillAllowed = isEarlyGame && !enemyHills.count(sq) && explored(sq); //If we haven't seen t
        symmetry->updateVisibleSquare(sq, noHillAllowed);

        bug << "Transforms remaining " << symmetry->transforms.size() <<
            ", squares remaining " << symmetry->newSeenSquares.size() << " time " << time() << "\n";
    }

    //In case we reduced fesible set down to minplayers and are done now
    while(symmetry->newGroupMembers.size() && timeCheckFrac(0.9)){
        auto trans = symmetry->newGroupMembers.back();
        symmetry->newGroupMembers.pop_back();
        symmetry->addToGroup(trans);
    }

    if (!symmetry->groupDone() && timeCheckFrac(0.83)){ //Make sure that group addition queue is empty when calling this
        auto stime = time();
        auto scnt = symmetry->transforms.size();
        bug << "Order filtering minplayers " << symmetry->minplayers << "\n";
        symmetry->doOrderFiltering();
        bug << "Transforms remaining " << symmetry->transforms.size() << " time taken " << time() - stime << "\n";
        if (symmetry->transforms.size() < scnt) {bug << "Filtering was effective!!!\n";}
    }

    if (symmetry->groupDone()) {bug << " ... we're done!\n";}
}

void State::readGameSettings(std::istream &is)
{
    State& state = *this;

    std::string inputType, junk;

//    std::vector<int> a = {0, 3, 4, 1};
//    std::vector<int> b = {0};
//    std::vector<int> c = {0, 3, 4, 1};
//    std::vector<int> d = {0, 3, 4, 1, 5};
//    ASSERT(a != b); ASSERT(a == c); ASSERT(a != d);
//    ASSERT(c != b); ASSERT(b != d); ASSERT(c != d);

    getline(is, junk);
//    ASSERT(junk == "turn 0");

        try{
            ASSERT(junk == "turn 0");
        }
        catch(const exception& e){
            state.bug << "An exception occured in readSettings\n" << e.what() << "\n";
        }


    //reads game parameters
    while(is >> inputType)
    {
        if(inputType == "loadtime")
            is >> state.loadtime;
        else if(inputType == "turntime")
            is >> state.turntime;
        else if(inputType == "rows")
            is >> state.rows;
        else if(inputType == "cols")
            is >> state.cols;
        else if(inputType == "turns")
            is >> state.max_turns;
        else if(inputType == "player_seed")
            is >> state.seed;
        else if(inputType == "viewradius2")
        {
            is >> state.viewrad2;
        }
        else if(inputType == "attackradius2")
        {
            is >> state.attackrad2;
        }
        else if(inputType == "spawnradius2")
        {
            is >> state.spawnrad2;
        }
        else if(inputType == "ready") //end of parameter input
        {
            state.timer.start();
            state.attackrad1 = sqrti(state.attackrad2);
            state.viewrad1 = sqrti(state.viewrad2);
            break;
        }
        else    //unknown line
            getline(is, junk);
    }

#ifdef FOR_TCP_SERVER
    if (turntime > 500) {turntime = 500;}
#endif
}

bool State::readGameInput(std::istream &is)
{
    //Temp
    for(int y=0; y<rows; ++y){  for(int x=0; x<cols; ++x){  grid(y,x).reset();    }}

    State& state = *this;
    seenInput.clear();

    std::string inputType, junk;
    int row, col;
    unsigned int player;
    unsigned int highestPlayer = 0; //Keep track of highest id seen this turn

    //finds out which turn it is
    while(is >> inputType)
    {
        if(inputType == "end")
        {
            state.gameover = 1;
            break;
        }
        else if(inputType == "turn")
        {
            is >> state.turn;
            break;
        }
        else //unknown line
            getline(is, junk);
    }

   //reads information about the current turn
    while(is >> inputType)
    {
        if(inputType == "w") //water square
        {
            is >> row >> col;
            state.grid(row,col).isWater = 1;
        }
        else if(inputType == "f") //food square
        {
            is >> row >> col;
            //state.grid(row,col).isFood = 1;
            seenInput.food.insert(Pos(row, col));
        }
        else if(inputType == "a") //live ant square
        {
            is >> row >> col >> player;
            highestPlayer = max(highestPlayer, player);
            state.grid(row,col).ant = player;

            if(player == 0){
                state.seenInput.myAnts.insert(Pos(row, col));
            }
            else
                {state.seenInput.enemyAnts[Pos(row, col)] = player;}
//                {state.enemyAnts.insert(Pos(row, col));}
        }
        else if(inputType == "d") //dead ant square
        {
            is >> row >> col >> player;
            highestPlayer = max(highestPlayer, player);
            if (player == 0) {state.seenInput.myDeadAnts.insert(Pos(row,col));}
            //state.grid(row,col).deadAnts.push_back(player);
        }
        else if(inputType == "h")
        {
            is >> row >> col >> player;
            highestPlayer = max(highestPlayer, player);
            state.grid(row,col).isHill = 1;
            state.grid(row,col).hillPlayer = player;
            if(player == 0)
                {state.seenInput.myHills.insert(Pos(row, col));}
            else
                {state.seenInput.enemyHills[Pos(row, col)] = player;}
//            if(player == 0)
//                state.myHills.push_back(Pos(row, col));
//            else
//                state.enemyHills.push_back(ownedHill(Pos(row, col), player));
        }
//        else if(inputType == "players") //player information
//            is >> state.noPlayers;
//        else if(inputType == "scores") //score information
//        {
//            state.scores = vector<double>(state.noPlayers, 0.0);
//            for(int p=0; p<state.noPlayers; p++)
//                is >> state.scores[p];
//        }
        else if(inputType == "go") //end of turn input
        {
            if(state.gameover)
                is.setstate(std::ios::failbit);
            else
                state.timer.start();
            break;
        }
        else //unknown line
            getline(is, junk);
    }

    symmetry->updateMinPlayers(highestPlayer + 1); //Add 1 to convert from id to num players
    return !state.gameover && is.good();
}
