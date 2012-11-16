#include "strategery.h"

#include <bitset>
#include <vector>
#include <map>
#include <algorithm>
#include "setops.h"
#include "priorityqueue.h"
#include "maxandmin.h"

bool shouldQuit(const State& s) {return !s.timeCheckCappedOk(35);}

typedef std::bitset<5> Moveset;

inline bool canMoveTo(const State& state, Pos p, bool doHillCheck=true){
    //Should probably be changed to use inferred water, not that it really matters
    if (state.grid[p].isWater || state.food.count(p)) {return false;}

    //Decide whether to force movement off a hill
    if(doHillCheck
        && state.myHills.count(p)
        && state.storedFood >= state.myHills.size()
        && state.myHills.at(p).closestEnemyDist > 1) {return false;}
    return true;
}

inline void makeMove(State& state, Pos p, int move) {
    if (move < 4) {state.makeMove(p, move);}
//    state.bug << "Move " << p << " -> " << state.getDest(, p, move) <<
//        "\t\t" << CDIRECTIONS[move] << "\n";
}

//Move scoring
static int getHillGuardingScore(const myHill& hill, Pos pos){
    if (hill.dists[pos] < hill.closestEnemyDist) {return -1;}
    auto diff = (hill.dists[pos] - hill.closestEnemyDist);
    return (diff < 64) ? (int)diff : 64;
}

static int getTotalScore(const State& state, const Ant& a, Pos pos){
    int score = state.getNonCombatScore(pos);
    if (state.spawnBlockIncentives.count(pos)) {score += state.spawnBlockIncentives.at(pos);}

    if (a.ghill){score += 64 * getHillGuardingScore(*a.ghill, pos);}
    if (a.bhill){score += 11 * (a.bhill->dists[pos] > 1);}

    return score;
}

namespace Bounty{
    typedef uint bounty_t;
    static const bounty_t DEATHAMOUNT = 1<<16;

    inline bounty_t frac(bounty_t a, bounty_t b){
        ASSERT(a < (1<<16) && b > 0);
        return (a * DEATHAMOUNT)/b;
    }
}

static std::set<Pos> generateScenario(const State& state, std::set<Pos> enemyLocs, int move){
    ASSERT(move >= 0 && move < 4);
    std::set<Pos> newLocs;

    while(enemyLocs.size()){
        bool progress = false;

        for(auto it = enemyLocs.begin(); it != enemyLocs.end(); /**/){
            auto dest = state.getDest(*it, move);
            bool remove = false;

            if (!canMoveTo(state, dest, false) || newLocs.count(dest)){
                newLocs.insert(*it); remove=true; //Can't move, so stay in place
            }
            else if (canMoveTo(state, dest, false) && !enemyLocs.count(dest)) {
                newLocs.insert(dest); remove=true; //Can move
            }
            else {} //May be able to move, but it is currently blocked, so check later

            if (remove){
                auto it2 = it++;
                enemyLocs.erase(it2);
                progress = true;
            }
            else {++it;}
        }

        //The only way we can get stuck is if they have a chain of ants going all the way across the map, thus giving a circular dependency
        if (!progress) {ASSERT(enemyLocs.size() >= (unsigned)state.rows || enemyLocs.size() >= (unsigned)state.cols);}
    }

    //Just in case we somehow got stuck
    for(auto it = enemyLocs.begin(); it != enemyLocs.end(); ++it) {newLocs.insert(*it);}
    return newLocs;
}

static void getHillDominationBounties(const State& state, std::map<Pos, Bounty::bounty_t>& bounties, Pos hpos){
    uint acount = 0, ecount = 0;

    auto rad1 = state.attackrad1;
    auto rad2 = state.attackrad2;
    for(int y = -rad1; y <= rad1; ++y){
        for(int x = -rad1; x <= rad1; ++x){
            if ((x*x + y*y) > rad2) {continue;}
            Pos p = state.wrapPos(hpos.row + y, hpos.col + x);
            acount += state.myAnts.count(p);
        }
    }

    if (acount <= 1) {return;}

    rad1 *= 2; rad2 *= 4;
    for(int y = -rad1; y <= rad1; ++y){
        for(int x = -rad1; x <= rad1; ++x){
            if ((x*x + y*y) > rad2) {continue;}
            Pos p = state.wrapPos(hpos.row + y, hpos.col + x);

            ecount += !!state.possibleEnemies[p];
        }
    }

    if (ecount >= acount) {return;}
    if (!ecount) {return;}
    ASSERT(ecount && acount>1);

    rad1 = state.attackrad1;
    rad2 = state.attackrad2;
    //Set all bounties to a minimum of (acount-1)/ecount
    for(int y = -rad1; y <= rad1; ++y){
        for(int x = -rad1; x <= rad1; ++x){
            if ((x*x + y*y) > rad2) {continue;}
            Pos p = state.wrapPos(hpos.row + y, hpos.col + x);

            bounties[p] = max(bounties[p], Bounty::frac(acount-1, ecount));
        }
    }
}

#include "constraintsolver.h"
//void testCSP(State& state);
void solveEverything(State& state){
//    if (state.turn == 1) {testCSP(state); testCSP(state); return;}
//    if (state.turn == 1) {testCSP(state); return;}
    Timer timer;

//    Array2d<unsigned int> contested(state.rows, state.cols, 0u);
    ConstraintGraph rootGraph(state);

    //Initialze ant keys and movesets
    for(auto it = state.myAnts.begin(); it != state.myAnts.end(); ++it){
        const Pos pos = it->first;

        //Make sure reference isn't invalidated by other insertions to the rootGraph!!
        auto& newNode = rootGraph.ants[rootGraph.getAntKey(pos)];
        newNode.pos = pos;

        for(int i=0; i<5; ++i){
            Pos dest = state.getDest( pos, i);
            if (!canMoveTo(state, dest)) {continue;}

//            contested[dest]++;
            newNode.allowed.set(i, true);
            newNode.movedata[i].fscore = getTotalScore(state, it->second, dest);
//            newNode.movedata[i].sqrkey = dest;

//            state.bug << "Score " << pos << " -> " << dest <<
//                "\t\t" << CDIRECTIONS[i] << ":\t" << newNode.movedata[i].fscore << "\n";
        }
    }

    //Calculate extra bounties from dominated enemy hills
    std::map<Pos, Bounty::bounty_t> dominationBounties;
    for(auto it = state.enemyHills.begin(); it != state.enemyHills.end(); ++it){getHillDominationBounties(state, dominationBounties, it->first);}

    std::set<Pos> enemyLocs, forbiddenSquares;
    for(auto it = state.enemyAnts.begin(); it != state.enemyAnts.end(); ++it){
        const Pos pos = it->first;
        enemyLocs.insert(pos);
        for(int i=0; i<5; ++i) {
            auto dest = state.getDest( pos, i);
            if (canMoveTo(state, dest, false)) {forbiddenSquares.insert(dest);}
        }
    }

    std::vector<std::set<Pos>> scenarios;
    scenarios.push_back(enemyLocs); //No move scenario
    for(int i=0; i<4; ++i) {scenarios.push_back(generateScenario(state, enemyLocs, i));}

    //At this point, the union of the scenarios should be equal to forbiddenSquares, so we just use that instead
    for(auto it = forbiddenSquares.begin(); it != forbiddenSquares.end(); ++it){
        const Pos pos = *it;
        auto& newNode = rootGraph.enemies[rootGraph.getEnemyKey(pos)];
        newNode.pos = pos;

        ASSERT(!state.grid[pos].isWater);

        //Calculate bounty
        auto bounty = Bounty::frac(1,2); //Default to value of 1/2 an ant
        for(auto it2 = state.myHills.begin(); it2 != state.myHills.end(); ++it2) {
            const auto& hill = it2->second;
            if (hill.dists[pos] < 256 && hill.dists[pos] <= hill.closestEnemyDist) {bounty += Bounty::frac(2, 1+hill.dists[pos]);}
        }

        if (dominationBounties.count(pos)) {bounty = max(bounty, dominationBounties[pos]);}

//        state.bug << "Bounty of " << pos << " = " << ((double)bounty)/Bounty::DEATHAMOUNT << "\n";
        newNode.bounty = bounty;
    }

    //Set up scenario masks in graph
    for(auto it = scenarios.begin(); it != scenarios.end(); ++it){
        const auto& sc = *it;
        VectorSet<ekey_t> keys;

        for(auto it2 = sc.begin(); it2 != sc.end(); ++it2){
            ASSERT(rootGraph.hasEnemyKey(*it2));
            keys.push_back(rootGraph.getEnemyKey(*it2));
        }

        keys.sort();
        rootGraph.scenarios.push_back(keys);
    }

    //Prevent ants from moving into a space where an emeny can move, unless it is one of our hills
    //also, find all enemy threats
    for(auto it = rootGraph.ants.begin(); it != rootGraph.ants.end(); ++it){
        auto& ant = *it;//it->second;
        for(int i=0; i<5; ++i){
            if (ant.allowed.test(i)
                && forbiddenSquares.count(state.getDest( ant.pos, i))
                && !state.myHills.count(state.getDest( ant.pos, i)))
                    {ant.allowed.set(i, false);}
        }

        for(int i=0; i<5; ++i){
            if (!ant.allowed.test(i)) {continue;}
            Pos pos = state.getDest( ant.pos, i);

            //Set up threats, inefficiently as usual
            std::vector<ekey_t> allThreats;

            const auto rad1 = state.attackrad1;
            const auto rad2 = state.attackrad2;
            for(int y = -rad1; y <= rad1; ++y){
            for(int x = -rad1; x <= rad1; ++x){
                if ((x*x + y*y) > rad2) {continue;}

                Pos sqr = state.wrapPos(pos.row + y, pos.col + x);
                if (rootGraph.hasEnemyKey(sqr)){
                    ekey_t key = rootGraph.getEnemyKey(sqr);
                    rootGraph.enemies[key].possibleAnts.insert(ant.key);
                    allThreats.push_back(key);
                }
            }}

            sort(allThreats);
            ant.movedata[i].threats.swap(allThreats);
        }
    }

    //Initialize neighbor constraints
    for(auto it = rootGraph.ants.begin(); it != rootGraph.ants.end(); ++it){
        auto& ant = *it;//it->second;
        const Pos pos = ant.pos;

        std::set<akey_t> neighbors;
        for(int i=0; i<5; ++i){
            if (!ant.allowed.test(i)) {continue;}
            const Pos dest = state.getDest( pos, i);

            for(int j=0; j<5; ++j){
                Pos other = state.getDest( dest, j);
                if (other == pos) {continue;} //Can't be a neighbor of yourself

                if (rootGraph.hasAntKey(other)) {
                    auto akey = rootGraph.getAntKey(other);
                    //Make sure neighbor can also move to the square we think is contested
                    //Except it doesn't matter at this point since it should always be true unless we have already started pruning moves
                    //if (!rootGraph.ants[akey].allowed.test(reverseMove(j))) {continue;}
                    neighbors.insert(akey);
                }
            }
        }

        ASSERT(neighbors.size() <= 12);
        for(auto it = neighbors.begin(); it != neighbors.end(); ++it){
            AntNode::Constraint c = {(*it), getConstraint(state, pos, rootGraph.ants[*it].pos)};
            ant.neighbors.push_back(c);
        }
    }

    state.bug << rootGraph.ants.size() << " ants, " << rootGraph.enemies.size() << " enemies\n";

    //Split up the rootGraph and initialize search queues
    rootGraph.pruneDominatedMoves();
    std::vector<ConstraintGraphSearch> searches;

    {
        auto graphs = CreateConstraintGraphs({state, rootGraph});
        state.bug << graphs.size() << " blocks found\n";

        auto prunecount = 0u;
        for(auto it = graphs.begin(); it != graphs.end(); ++it)
        {
            if (it->ants.size() == 1){
                const auto& ant = it->ants.front();
                const Moveset moves = ant.allowed;

                if (moves.count() == 1){
                    makeMove(state, ant.pos, getFirstMove(moves));
                    ++prunecount;
//                    state.bug << ant.pos << ": " << movees << " pruned\n";
                    continue;
                }
            }

            searches.push_back(ConstraintGraphSearch(state));
            searches.back().graph = *it;
    //        searches.back().graph = std::move(*it);
            searches.back().initalizeSearch(state);
    //        ASSERT(it->ants.empty());//should have been moved
//            auto& search = searches.back();
//            if (search.graph.ants.size() != 4) {searches.pop_back();}
        }

        state.bug << prunecount << " ants pruned\n";
    }

    //Sort graphs by appoximate difficulty
    sort(searches, graphSizeComparisonFunc);

    state.bug << "Block sizes: ";
    for(auto it = searches.begin(); it != searches.end(); ++it){
        state.bug << "(" << it->graph.ants.size() << "," << it->graph.enemies.size() << ")";
        if (it+1 == searches.end()) {state.bug << "\n";} else state.bug << ", ";
    }

    size_t unsolvedCount = searches.size();
    unsigned int numNodesToExpand = 64;

    while (unsolvedCount && !shouldQuit(state)){
        for(auto it = searches.begin(); it != searches.end(); ++it){
            auto& search = *it;
            if (search.solved()) {continue;}

            timer.start();
            state.bug << "Begin solving graph " <<
                search.graph.ants.size() << " ants, " << search.graph.enemies.size() <<" enemies\n";


            search.solve(state, numNodesToExpand);
            state.bug << "Solved - time taken: " << timer.getTime() << "\n";

            ASSERT(unsolvedCount);
            if (search.solved()) {--unsolvedCount;}
        }

        numNodesToExpand *= 2; //Double the amount of work we do each round to avoid looping too many times
    }

    //Make moves from best solutions we've found so far
    timer.start();
    for(auto it = searches.begin(); it != searches.end(); ++it){
        std::map<Pos,int> solution = it->getSolution();

        if (solution.empty()){
            state.bug << "Graph with " << it->graph.ants.size() << " ants was unsolved!\n";

//            for(auto it2 = it->graph.antkeys.begin(); it2 != it->graph.antkeys.end(); ++it2){
//                makeMove(state, it2->first, 5);
//            }
        }
        else {
            for(auto it2 = solution.begin(); it2 != solution.end(); ++it2){
                makeMove(state, it2->first, it2->second);
            }
        }
    }

    state.bug << "Finished registering moves - time taken: " << timer.getTime() << "\n";
}
