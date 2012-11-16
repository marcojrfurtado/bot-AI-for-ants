#include <array>
#include "vectorset.h"
typedef unsigned int uint;

static int getFirstMove(const Moveset& m){
    ASSERT(m.any());
    for(int i=0; i<5; ++i) {if (m[i]) { return i; } }
    return 6;
}

//Don't use this with base class pointers, since STL containers aren't designed for inheritance
struct MovesetPair: std::bitset<5*5>
{
    typedef std::bitset<5*5>::reference reference;

    MovesetPair ( unsigned long val ): std::bitset<5*5>(val) {}

    reference allowed(int myMove, int otherMove) {return (*this)[ myMove*5 + otherMove ];}
    bool allowed(int myMove, int otherMove) const {return (*this)[ myMove*5 + otherMove ];}

    Moveset otherMask(int move) const {return (to_ulong() >> (5*move));}

    Moveset getConsistentOtherMoves(Moveset myMoves) const
    {
        Moveset okset;
        for(int i=0; i<5; ++i) {if (myMoves[i]) { okset |= otherMask(i); } }
        return okset;
    }

    Moveset getGuaranteedMyMoves(Moveset otherMoves) const
    {
        Moveset okset = ~0ul;
        for(int x=0; x<5; ++x) {for(int y=0; y<5; ++y) {
            if (otherMoves[y] && !allowed(x,y)) {okset[x] = false;}
        }}
        return okset;
    }

    bool isEffective(Moveset myMoves, Moveset otherMoves) const{
        for(int i=0; i<5; ++i) {
            if (myMoves[i] && ((otherMask(i) & otherMoves) != otherMoves)) {return true;}
        }
        return false;
    }
};

static int reverseMove(const State& state, Pos to, Pos from){
    for(int i=0; i<5; ++i) {if (to == state.getDest(from, i)) {return i;}}
    return -1;
}

static MovesetPair getConstraint(const State& state, Pos pos, Pos other){
    ASSERT(pos != other);
    MovesetPair mask(~0u); //Initially all 0

    for(int i=0; i<5; ++i) {
        const Pos dest = state.getDest(pos, i);
        int rmove = reverseMove(state, dest, other);

        if (rmove >= 0){
            mask.allowed(i, rmove) = false;
        }
    }

    //Prevent position exchange
    if (reverseMove(state, other, pos) >= 0){
        ASSERT(reverseMove(state, pos, other) >= 0);

        mask.allowed(reverseMove(state, other, pos), reverseMove(state, pos, other)) = false;
    }

    return mask;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef std::size_t akey_t;
typedef std::size_t ekey_t;
//typedef Pos sqkey_t;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct MoveData{
    int fscore;
//    std::vector<ekey_t> threats; //One threat set per scenario
    VectorSet<ekey_t> threats; //One threat set per scenario

//    std::vector<ekey_t> threats;
//    sqkey_t sqrkey;
//    Pos sqrkey; //For debugging purposes, mainly
};

struct AntNode{
    typedef Moveset State;

    struct Constraint{
        akey_t key;
        MovesetPair mask;
    };

    akey_t key;
    Moveset allowed;
    std::array<MoveData, 5> movedata;
    std::vector<Constraint> neighbors;
    Pos pos; //For debugging purposes, mainly

    std::vector<ekey_t> getThreatenedEnemies(Moveset moves) const {
        std::vector<ekey_t> any;

        for(int i=0; i<5; ++i){
            if (!moves.test(i)) {continue;}
            unionEq(any, movedata[i].threats);
        }

        return any;
    }

    std::vector<ekey_t> getEnemiesUnthreatenedByMove(Moveset oldm, Moveset newm) const {
        auto oldThreats = getThreatenedEnemies(oldm);
        auto newThreats = getThreatenedEnemies(newm);

        ASSERT(oldThreats.size() >= newThreats.size());
        differenceEq(oldThreats, newThreats);
        return oldThreats;
    }
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct EnemyNode{
    struct State{
        std::size_t max;
    };

    Pos pos;
    std::set<akey_t> possibleAnts;
    uint bounty; //Penalty for letting an ant live at this location
};

struct GraphState{
    std::vector<AntNode::State> ants;
    std::vector<EnemyNode::State> enemies;

    bool solveable() const {
        for(auto i = ants.begin(); i != ants.end(); ++i)    {if (i->none()) {return false;}}
        return true;
    }

    bool solved() const {
        for(auto i = ants.begin(); i != ants.end(); ++i)    {if (i->count() != 1) {return false;}}
        return true;
    }

    akey_t getMostConstrained() const
    {
        using namespace std;
        typedef tuple<size_t, unsigned long> cmp_t;

        ASSERT(solveable());
        akey_t best;
//        size_t bestval = 6;
        cmp_t bestval = make_tuple(6u,0ul);

        for(auto i = ants.begin(); i != ants.end(); ++i){
            const Moveset& m = *i;
            const akey_t key = i - ants.begin();

            if (m.count() <= 1) {continue;}

            cmp_t newVal = make_tuple(m.count(), m.to_ulong());

            if (newVal < bestval){
                best = key;
                bestval = newVal;
            }
        }

        return best;
    }

    operator bool() const {return ants.size();}
};

bool operator < (const std::bitset<5>& a, const std::bitset<5>& b) {return a.to_ulong() < b.to_ulong();}
//bool operator < (const AntNode::State& a, const AntNode::State& b) {return a.to_ulong() < b.to_ulong();}
//template<typename T> struct TupleConvertable { auto tup() const ->

struct ScoreTup{
    uint deaths;
    int foodscore;
//        uint movesRemaining;

//    ScoreTup(): deaths(0), foodscore(0), movesRemaining(0) {}
//        operator std::tuple<uint, int, uint>() const {return std::make_tuple(deaths, -foodscore, movesRemaining);}
    std::tuple<uint, int> tup() const {return std::make_tuple(deaths, foodscore);}

    void operator+=(const ScoreTup& other){deaths += other.deaths; foodscore += other.foodscore;}

    static ScoreTup MAXSCORE() {return {~0u, 0};}
};

//ScoreTup operator * (ScoreTup t, int mult){t.deaths *= mult; t.foodscore *= mult; return t;}
std::ostream& operator<<(std::ostream &os, const ScoreTup& a) {return os << "(" << a.deaths << "," << a.foodscore << ")";}

bool operator > (const ScoreTup& a, const ScoreTup& b) {return a.tup() < /*reversed*/ b.tup();}
bool operator < (const ScoreTup& a, const ScoreTup& b) {return a.tup() > /*reversed*/ b.tup();}
bool operator <= (const ScoreTup& a, const ScoreTup& b) {return a.tup() >= /*reversed*/ b.tup();}
//#include <unordered_set>

//An adapter used for applying arc consistency algorithm to a graph state
struct GraphStateModifier{
    GraphState& gstate;
    GraphStateModifier(GraphState& gs): gstate(gs) {}

    Moveset& moves(akey_t k) {return gstate.ants[k];}

    void removeEnemyThreat(akey_t a, ekey_t e) {
        ASSERT(gstate.enemies[e].max);
        gstate.enemies[e].max--;
    }
};

struct ConstraintGraph{
    std::vector<AntNode> ants;
    std::map<Pos, akey_t> antkeys;

    std::vector<EnemyNode> enemies;
    std::map<Pos, ekey_t> enemykeys;

    std::vector<VectorSet<ekey_t>> scenarios;
    Bug* debugLogPtr;

    ConstraintGraph(State& state): debugLogPtr(&state.bug) {}

    akey_t getAntKey(Pos p) {
        if (antkeys.count(p)) {return antkeys[p];}

        ants.push_back( AntNode() );
        antkeys[p] = ants.size() - 1;
        ants[ants.size() - 1].key = ants.size() - 1; //Until we get a real vector map implementation, store the keys
        return ants.size() - 1;
    }

    ekey_t getEnemyKey(Pos p) {
        if (enemykeys.count(p)) {return enemykeys[p];}

        enemies.push_back( EnemyNode() );
        enemykeys[p] = enemies.size() - 1;
        return enemies.size() - 1;
    }

    Pos getSqrKey(Pos p) {return p;}
    bool hasAntKey(Pos p) {return antkeys.count(p);}
    bool hasEnemyKey(Pos p) {return enemykeys.count(p);}

    template<typename T> //True if still valid (every node has a move). Note, though it is const, it modifies the given graphstate
    bool applyMoveMaskSingle(T stateAcessor, std::vector<akey_t>& stack, akey_t okey, Moveset mask) const {
        Moveset oldMoves = stateAcessor.moves(okey);
        Moveset newMoves = oldMoves & mask;
        if (!newMoves.any()) {return false;}

        if (newMoves != oldMoves){
            //Adjust enemy threats
            auto unthreats = ants.at(okey).getEnemiesUnthreatenedByMove(oldMoves, newMoves);

            for(auto it2 = unthreats.begin(); it2 != unthreats.end(); ++it2){
                ASSERT(enemies[*it2].possibleAnts.count(okey));
                stateAcessor.removeEnemyThreat(okey, *it2);
            }

            stateAcessor.moves(okey) = newMoves;
            stack.push_back(okey);
        }

        return true;
    }

    template<typename T> //True if still valid (every node has a move). Note, though it is const, it modifies the given graphstate
    bool applyMoveMaskRecursive(T stateAcessor, akey_t startkey, Moveset newmask) const {
//    bool applyMoveMaskRecursive(GraphState& gstate, akey_t startkey, Moveset newmask) const {
        std::vector<akey_t> stack;

        if (!applyMoveMaskSingle(stateAcessor, stack, startkey, newmask)) {return false;}
        ASSERT(stack.size()); //false if no moves were removed

        while(!stack.empty()){
            auto nkey = stack.back();
            stack.pop_back();
            const auto& node = ants.at(nkey);

            for(auto it = node.neighbors.begin(); it != node.neighbors.end(); ++it){
                auto okey = it->key;

                //Now make sure the moves are consistent
                Moveset mask = it->mask.getConsistentOtherMoves( stateAcessor.moves(nkey) );
                if (!applyMoveMaskSingle(stateAcessor, stack, okey, mask)) {return false;}
            }
        }

        return true;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool isMoveDeadly(const GraphState& gstate, const AntNode& ant, int move, std::size_t scenario_id) const {
        ASSERT(ant.allowed[move]);
        ASSERT(scenario_id < scenarios.size());

        const auto threats = setIntersection<std::vector<ekey_t>>(ant.movedata[move].threats, scenarios[scenario_id]);

        for(auto it2 = threats.begin(); it2 != threats.end(); ++it2){
            ASSERT(gstate.enemies[*it2].max); //It should always have at least one threat since we're threatening it
            if (gstate.enemies[*it2].max <= threats.size()) {return true;}
        }
        return false;
    }

    //Is move deadly in all scenarios?
    bool isMoveDeadlyAll(const GraphState& gstate, const AntNode& ant, int move) const {
        for(uint i=0; i<scenarios.size(); ++i){
            if (!isMoveDeadly(gstate, ant, move, i)) {return false;}
        }
        return true;
    }

    ScoreTup getScoreOfMove(const GraphState& gstate, const AntNode& ant, int move) const {
        unsigned int death = isMoveDeadlyAll(gstate, ant, move);
        int fscore = ant.movedata[move].fscore;
        return {death*Bounty::DEATHAMOUNT, fscore};
    }

    ScoreTup getBestScore(const GraphState& gstate, const AntNode& ant, const AntNode::State& moves) const {
        ScoreTup best = {2*Bounty::DEATHAMOUNT,0};

        for(int i=0; i<5; ++i) {
            if (!moves[i]) { continue; }
            auto score = getScoreOfMove(gstate, ant, i);
            if (score > best) {best = score;}
        }
        return best;
    }

    int getBestScoreMove(const GraphState& gstate, const AntNode& ant, const AntNode::State& moves) const {
        ScoreTup best;
        int move = -1;

//        for(int i=5-1; i>=0; --i) {
        for(int i=0; i<5; ++i) {
            if (!moves[i]) { continue; }
            auto score = getScoreOfMove(gstate, ant, i);
            if (move < 0 || score > best) {best = score; move = i;}
        }
        return move;
    }

    ScoreTup getUpperBoundScore(const GraphState& gstate) const {
        ScoreTup total = {0,0};

        for(auto it = ants.begin(); it != ants.end(); ++it){
            const Moveset& moves = gstate.ants.at( it->key );
            total += getBestScore(gstate, *it, moves);
        }

//        //Add bounty penalty for any enemy that isn't threatened, since they are sure to live
//        for(auto e_id = 0u; e_id != enemies.size(); ++e_id){
//            if (!gstate.enemies[e_id].max) {total.deaths += enemies[e_id].bounty;}
//        }

        return total;
    }

    //Applies greedy algorithm to state and returns score, which is a lower bound on best possible score
    ScoreTup getLowerBoundScore(GraphState gstate) const {
        for(auto it = ants.begin(); it != ants.end(); ++it){
            const Moveset& moves = gstate.ants.at( it->key );
            if (moves.none()) {return {0xFFFFFFFF,0};} //Oops, our extension violated a constraint

            int move = getBestScoreMove(gstate, *it, moves);
            ASSERT(move >= 0);

            if (moves.count() > 1){
                applyMoveMaskRecursive(GraphStateModifier(gstate), it->key, Moveset(1<<move));
            }
        }
        return evaluateSolutionScore(gstate);
    }

    //Gets the minimum score over all scenarios
    ScoreTup evaluateSolutionScore(const GraphState& gstate) const {
        ASSERT(gstate.solved());
        std::array<uint, 5> deaths = {};
        ScoreTup score = {0u, 0};

        ASSERT(deaths.size() == scenarios.size());
//        (*debugLogPtr) << "Evaluating state score...\n";
//        printState(gstate);

        for(auto it = ants.begin(); it != ants.end(); ++it){
            const auto& ant = *it;
            const auto move = getFirstMove(gstate.ants.at( it->key ));
            score.foodscore += ant.movedata[move].fscore;

            for(uint i=0; i<scenarios.size(); ++i){
                if (isMoveDeadly(gstate, ant, move, i)) {deaths[i]+= Bounty::DEATHAMOUNT;}
            }
        }

        //adjust score for living enemy ants
//        for(auto it = enemies.begin(); it != enemies.end(); ++it){
        for(ekey_t e_id = 0u; e_id != enemies.size(); ++e_id){
            const auto& enemy = enemies[e_id];
//            if (!enemy.bounty) {continue;}
            const auto acount = gstate.enemies[e_id].max;

            //For each scenario, check if the enemy lived
            for(uint i=0; i<scenarios.size(); ++i){
                if (!scenarios[i].count(e_id)) {continue;}
                bool lived = true;

//                (*debugLogPtr) << " checking bounty for sc. " << i << " acount = " << acount << "\t";

                for(auto it2 = enemy.possibleAnts.begin(); it2 != enemy.possibleAnts.end(); ++it2){
                    const auto& ant = ants[*it2];
                    const auto move = getFirstMove(gstate.ants.at( *it2 ));
                    const auto athreats = setIntersection<VectorSet<ekey_t>>( ant.movedata[move].threats, scenarios[i] );

                    //Make sure ant is actually stil threatening us
                    if (!athreats.count(e_id)) {continue;}
                    if (athreats.size() <= acount) {lived = false; break;}
                }

//                (*debugLogPtr) << lived << "\n";
                if (lived) {deaths[i] += enemy.bounty;}
            }
        }

        //Now take the score from the worst case scenario
        for(uint i=0; i<deaths.size(); ++i)  { score.deaths = max(score.deaths, deaths[i]);        }
        return score;
    }

    int selectMove(const GraphState& gstate, akey_t nodekey) const {
//        return getBestScoreMove(gstate, ants.at(nodekey), gstate.ants.at(nodekey));
        return getFirstMove(gstate.ants.at(nodekey));
    }

    /////////////////////////////////////////////////////////////////////
    struct GraphBaseStateModifier{
        ConstraintGraph& gstate;
        GraphBaseStateModifier(ConstraintGraph& gs): gstate(gs) {}

        Moveset& moves(akey_t k) {return gstate.ants[k].allowed;}

        void removeEnemyThreat(akey_t a, ekey_t e) {
            ASSERT(gstate.enemies[e].possibleAnts.count(a));
            gstate.enemies[e].possibleAnts.erase(a);
        }
    };

    void pruneDominatedMoves(){
        while(ants.size()){
            bool done = true; //Keep going until an enitre iteration with no updates

            for(auto it = ants.begin(); it != ants.end(); ++it){
                auto& ant = *it;

                //Find dominated moves
                Moveset moves = ant.allowed;
                Moveset guaranteed = moves;
                ASSERT(moves.any());

                for(auto it2 = ant.neighbors.begin(); it2 != ant.neighbors.end(); ++it2){
                    Moveset mask = it2->mask.getGuaranteedMyMoves( ants[it2->key].allowed );
                    guaranteed = guaranteed & mask;
                }

                Moveset combatMoves = moves;
                for(int i=0; i<5; ++i) {combatMoves[i] = combatMoves[i] && ant.movedata[i].threats.size();}

                //Find best guarenteed noncombat move, if any
                guaranteed = guaranteed & ~combatMoves;
                int bgmove = -1; int bfscore = 7777777;

                for(int i=0; i<5; ++i) {
                    if (guaranteed[i] && (bgmove < 0 || ant.movedata[i].fscore < bfscore)){
                        bgmove = i;
                        bfscore = ant.movedata[i].fscore;
                    }
                }

                ASSERT((guaranteed.none() && bgmove == -1) ||
                       (guaranteed.any() && bgmove >= getFirstMove(guaranteed)));

//                if (ant.pos == Pos(18,77)){
//                    (*debugLogPtr) << moves << " " << guaranteed << " " << combatMoves << " "
//                        << bgmove << " " << bfscore << "\n";
//                }

                if (bgmove > -1){ //prune noncombat moves dominated by move we found
                    for(int i=0; i<5; ++i) {
                        if (!moves[i] || combatMoves[i]) {continue;}
                        if (i != bgmove && ant.movedata[i].fscore >= bfscore) {moves[i] = false;}
                    }

                    if (moves != ant.allowed){
                        applyMoveMaskRecursive(GraphBaseStateModifier(*this), ant.key, moves);
                        done = false;
                    }
                }

                //remove ineffective constraints
                std::vector<AntNode::Constraint> newConstraints;
                newConstraints.reserve(ant.neighbors.size());
                for(auto it2 = ant.neighbors.begin(); it2 != ant.neighbors.end(); ++it2){
                    if(it2->mask.isEffective(ant.allowed, ants[it2->key].allowed)) {newConstraints.push_back(*it2);}
                }
                ant.neighbors.swap(newConstraints);
            }

            if (done) {break;}
        }
    }

    //////////////////////////////////////////////////////////////////////
    void printState(const GraphState& gstate) const {
        for(auto it = ants.begin(); it != ants.end(); ++it){
            (*debugLogPtr) << it->pos << ": " << gstate.ants.at(it->key);
            (*debugLogPtr) << ((it+1 == ants.end()) ? "\n" : ", ");
        }
        for(auto it = enemies.begin(); it != enemies.end(); ++it){
            (*debugLogPtr) << it->pos << ": " << gstate.enemies.at(it - enemies.begin()).max;
            (*debugLogPtr) << ((it+1 == enemies.end()) ? "\n" : ", ");
        }
    }
};


struct ConstraintGraphSearch{
    ConstraintGraph graph;

    //Search Data
    typedef std::tuple<ScoreTup, ScoreTup> searchkey_t;
    PriorityQueue<GraphState, searchkey_t, false> q;
    GraphState solutionState;
    ScoreTup bestScore;// = ScoreTup::MAXSCORE(); //Prevent used unitialized warning



//    ConstraintGraphSearch(): bestScore(ScoreTup::MAXSCORE()) {} //Make sure best score is initialized!
    ConstraintGraphSearch(State& state): graph(state) {}


    std::tuple<ScoreTup, ScoreTup> createKey(const GraphState& gstate) const
        { return std::make_tuple(graph.getLowerBoundScore(gstate), graph.getUpperBoundScore(gstate)); }

    void initalizeSearch(State& state){
        bestScore = ScoreTup::MAXSCORE();

        GraphState baseState;
        baseState.ants.reserve(graph.ants.size()); baseState.enemies.reserve(graph.enemies.size());
        for(auto it = graph.ants.begin(); it != graph.ants.end(); ++it)
            { baseState.ants.push_back(it->allowed); }
        for(auto it = graph.enemies.begin(); it != graph.enemies.end(); ++it)
            { baseState.enemies.push_back( EnemyNode::State{it->possibleAnts.size()} ); }
//            {
//                baseState.enemies.push_back( EnemyNode::State{it->possibleAnts.size()} );
//
//                state.bug << "possible ants of " << it->pos << "\t" << it->possibleAnts.size();// << "\n";
//                for(auto it2 = it->possibleAnts.begin(); it2 != it->possibleAnts.end(); ++it2)
//                    {state.bug << "\t" << graph.ants[*it2].pos;}
//                state.bug << "\n";
//            }

        q.push(baseState, createKey(baseState));
        ASSERT(q.size() && !solutionState);
    }

    void solve(State& state, uint maxnodes)
    {
        uint nodecount = 0;

        while(q.size() && nodecount < maxnodes && !shouldQuit(state)){
//            GraphState cur = q.pop();
            GraphState cur;
            ScoreTup lowerb, upperb;
            searchkey_t temp;
            std::tie(cur, temp) = q.pop_val();
            std::tie(lowerb, upperb) = temp;

            const auto& newUpperBoundScore = upperb;
//            state.bug << "Expanding node of score at least " << newUpperBoundScore << "\n";
//            auto newUpperBoundScore = graph.getUpperBoundScore(cur);
//            graph.printState(cur);
//            state.bug << "UB " << upperb << ", LB " << lowerb << "\n";

            if (cur.solved()) {
                const auto& newScore = graph.evaluateSolutionScore(cur); //If it's solved, the score must be equal to the upper bound on the score
                ASSERT(newScore <= newUpperBoundScore);

                if (!solutionState || newScore > bestScore){
                    bestScore = newScore;
                    solutionState.ants.swap(cur.ants);
//                    state.bug << "New score: " << bestScore << " " << nodecount << " nodes expanded, q size " << q.size() << "\n";
                    state.bug << "New score: " << bestScore << "\tncount " << nodecount << " time " << state.time() << "\n";
                }

//                break;
                continue;
            }
            else if (newUpperBoundScore <= bestScore){ continue; }

            ASSERT(cur.solveable());

            ++nodecount;
            auto nodekey = cur.getMostConstrained();

            int move = graph.selectMove(cur, nodekey);
            Moveset moveMask(1u << move);
            GraphState copy = cur;

            if (graph.applyMoveMaskRecursive(GraphStateModifier(copy), nodekey, moveMask)){
                if (graph.getUpperBoundScore(copy) > bestScore){
                    q.push(copy, createKey(copy));
                }
            }

            if (graph.applyMoveMaskRecursive(GraphStateModifier(cur), nodekey, ~moveMask)){
                if (graph.getUpperBoundScore(cur) > bestScore){
                    q.push(cur, createKey(cur));
                }
            }
        }

//        state.bug << nodecount << " nodes expanded, q size " << q.size() << ", visited size " << visited.size() << "\n";
        state.bug << nodecount << " nodes expanded, q size " << q.size() << "\n";
    }

    std::map<Pos,int> getSolution() const{
        std::map<Pos,int> solution;

        if (solutionState) {
            ASSERT(solutionState.solved());

            for(auto it = graph.antkeys.begin(); it != graph.antkeys.end(); ++it){
                auto key = it->second;

                ASSERT((solutionState.ants[key].count() == 1));
                solution[ it->first ] = getFirstMove( solutionState.ants[key] );
            }
        }

        return solution;
    }

    bool solved() const {return !q;}
    size_t scale() const {return graph.ants.size() * (1 + graph.enemies.size());}
};

static bool graphSizeComparisonFunc(const ConstraintGraphSearch& g1, const ConstraintGraphSearch& g2) {return g1.scale() < g2.scale();}


// Graph creation /////////////////////////////////////////////////////////////////////////////////////////////
struct GraphCreationReferences{
    State& state;
    const ConstraintGraph& parent;
//    std::map<Pos, TempAntNode>& freenodes;
//    std::map<Pos, TempEnemyNode>& enemies;
//    const std::map<Pos, std::set<Pos>>& threats;
};

#include "graphtest.h"
typedef std::set<Pos> usedNodeCounter_t;
static void insertAntNodeRecursive(GraphCreationReferences args, ConstraintGraph& graph,
                                   usedNodeCounter_t& usedAnts, Pos pos);

static void insertEnemyNodeRecursive(GraphCreationReferences args, ConstraintGraph& graph,
                                     usedNodeCounter_t& usedAnts, Pos pos)
{
    if (!graph.hasEnemyKey(pos)){
        const auto tempNode = args.parent.enemies.at(  args.parent.enemykeys.at(pos) );

        auto key = graph.getEnemyKey(pos);
        graph.enemies[key].pos = pos;
        graph.enemies[key].bounty = tempNode.bounty;

        for(auto it = tempNode.possibleAnts.begin(); it != tempNode.possibleAnts.end(); ++it){
            Pos npos = args.parent.ants.at(*it).pos;
            insertAntNodeRecursive(args, graph, usedAnts, npos);
            ASSERT(graph.hasAntKey(npos));

            graph.enemies[key].possibleAnts.insert(  graph.getAntKey(npos) );
        }
    }
}

static void insertAntNodeRecursive(GraphCreationReferences args, ConstraintGraph& graph,
                                   usedNodeCounter_t& usedAnts, Pos pos)
{
    if (!graph.hasAntKey(pos)){
        const auto tempNode = args.parent.ants.at(  args.parent.antkeys.at(pos) );
        ASSERT(!usedAnts.count(pos));
        usedAnts.insert(pos);

        auto key = graph.getAntKey(pos);
        //DO NOT TAKE A REFERENCE SINCE WE'RE USING A VECTOR NOW AND IT WILL BE INVALIDATED
        //AntNode& graph.ants[key] = graph.ants[key]; //This reference will not be invalidated by map insertions
        graph.ants[key].key = key;
        graph.ants[key].pos = pos;
        graph.ants[key].allowed = tempNode.allowed;

        //Update move data and insert enemies recursively
        for(int i=0; i<5; ++i){
            if (!tempNode.allowed.test(i)) {continue;}
            graph.ants[key].movedata[i].fscore = tempNode.movedata[i].fscore;

            //We can take a reference to nodes in the old map since it won't be modified
            const auto& oldThreats = tempNode.movedata[i].threats;
            std::vector<ekey_t> newKeys; newKeys.reserve(oldThreats.size());

            //Always propogate connectivity over enemy links bcause the allowable cases of pruning are subtle and easy to get wrong
            for(auto it = oldThreats.begin(); it != oldThreats.end(); ++it){

                Pos npos = args.parent.enemies.at(*it).pos;
                insertEnemyNodeRecursive(args, graph, usedAnts, npos);

                ASSERT(graph.hasEnemyKey(npos));
                newKeys.push_back(graph.getEnemyKey(npos));
            }

            sort(newKeys); //so set ops work
            graph.ants[key].movedata[i].threats.swap(newKeys);
        }

        const auto& neighbors = tempNode.neighbors;
        ASSERT(neighbors.size() < args.parent.ants.size()); //can't be neighbor of self, so must be strictly less

        for(auto it = neighbors.begin(); it != neighbors.end(); ++it){
            Pos npos = args.parent.ants.at(it->key).pos;
            insertAntNodeRecursive(args, graph, usedAnts, npos);
            ASSERT(graph.hasAntKey(npos));

            AntNode::Constraint c = {graph.getAntKey(npos), getConstraint(args.state, pos, npos)};
            graph.ants[key].neighbors.push_back(c);
        }
    }
}

//typedef ConstraintGraphSearch myreturn_t;
typedef ConstraintGraph myreturn_t;
static std::vector<myreturn_t> CreateConstraintGraphs(GraphCreationReferences args)
{
    std::vector<myreturn_t> graphs;
    usedNodeCounter_t usedAnts;
    uint asum = 0, esum = 0; //Total ant counts to make sure we didn't accidently duplicate

    ASSERT(args.parent.antkeys.size() == args.parent.ants.size());
    for(auto it = args.parent.antkeys.begin(); it != args.parent.antkeys.end(); ++it){
        if (usedAnts.count(it->first)) {continue;}

        graphs.push_back( myreturn_t(args.state) );
//        auto& graph = graphs.back().graph;
        auto& graph = graphs.back();

        insertAntNodeRecursive(args, graph, usedAnts, it->first);
        ASSERT(usedAnts.count(it->first));

        //Now copy over scenario masks
        const auto& oldScenarios = args.parent.scenarios;
        for(auto it = oldScenarios.begin(); it != oldScenarios.end(); ++it){
            const auto& scenario = *it;

            VectorSet<ekey_t> newKeys;
            for(auto it2 = scenario.begin(); it2 != scenario.end(); ++it2){
                Pos npos = args.parent.enemies.at(*it2).pos;

                if (graph.hasEnemyKey(npos)) {newKeys.push_back(graph.getEnemyKey(npos));}
            }

            newKeys.sort();
            graph.scenarios.push_back(newKeys);
        }
        ASSERT(graph.scenarios.size() == oldScenarios.size());

        ASSERT(graph.ants.size());
        asum += graph.ants.size();
        esum += graph.enemies.size();
    }

    ASSERT(asum == usedAnts.size());
    ASSERT(asum == args.parent.ants.size());
    ASSERT(esum <= args.parent.enemies.size());
    return graphs;
}



