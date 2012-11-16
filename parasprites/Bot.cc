#include "Bot.h"
#include "strategery.h"

using namespace std;

//constructor
Bot::Bot()
{

};

//void handleException(State& s

//plays a single game of Ants.
void Bot::playGame(std::istream& inputStream)
{
    //reads the game parameters and sets up
    try{
        state.readGameSettings( inputStream );
        state.setup();
    }
    catch(const exception& e){ //Should catch by reference to prevent type info loss
        state.bug << "An exception occured in setup\n" << e.what() << "\n";
    }
    endTurn();


    //continues making moves while the game is not over
    //while(state.readGameInput( std::cin ))
    while(true)
    {
        try{
            if (!state.readGameInput( inputStream )) {break;}
            state.update();
        }
        catch(const exception& e){ //Should catch by reference to prevent type info loss
//#ifdef TESTING
            state.bug << "An exception occured in read/update\n" << e.what() << "\n";
//            std::cerr << "An exception occured in read/update!\n";
//#endif
            break;
        }

        state.bug << "Beginning makeMoves - time: " << state.time() << "/" << state.turntime << " ms\n\n";

        try{
            //state.bug << "Calling makeMoves\n";
            makeMoves();
            //state.bug << "Returned from makeMoves\n";
        }
        catch(const exception& e){
//#ifdef TESTING
            state.bug << "An exception occured in makeMoves\n" << e.what() << "\n";
//            std::cerr << "An exception occured in makeMoves!\n";
//#endif
        }

        //state.bug << "about to call endturn time: " << state.time() << "/" << state.turntime << " ms\n\n";
        endTurn();
        state.bug << "endturn time: " << state.time() << "/" << state.turntime << " ms\n\n";
    }
};


#include "priorityqueue.h"
//makes the bots moves for the turn
void Bot::makeMoves()
{
    auto& myAnts = state.myAnts;
    auto& hills = state.myHills;

    //Reset all guard pointers (they may be invalid anyway)
//    for(auto it = myAnts.begin(); it != myAnts.end(); ++it) { it->ghill = NULL; }

    if (!state.enoughStoredFood())
    {
        unsigned int numGuardedHills = 0;
        for(auto it = hills.begin(); it != hills.end(); ++it) {
            const auto& h = it->second;

            if ((h.closestEnemyDist == ~0u) || (h.closestEnemyDist <= state.turnsLeft())){
                numGuardedHills++;
                it->second.numGuards = 0;
            }
            else {it->second.numGuards = 99999;} //Don't assign guards to this hill
        }

        //Assign guards to hills
        if(numGuardedHills && myAnts.size() > numGuardedHills*2)
        {
            const unsigned int avgAnts = ((myAnts.size()-1) / numGuardedHills) - 1;
            ASSERT(avgAnts);

            unsigned int numGuards = (avgAnts < 2) ? avgAnts : 2;
            numGuards += (avgAnts - numGuards) / 5;
            ASSERT(numGuards);
            if (state.storedFood >= hills.size()) {--numGuards;} //One will spawn next turn anyway

            PriorityQueue<std::tuple<Pos, Pos>, unsigned int> queue;

            //Insert possible assignments to queue
            for(auto it = myAnts.begin(); it != myAnts.end(); ++it){
                Ant& ant = it->second;
                Pos p = it->first;

                if (ant.bhill){ //If it is blocking a hill, it will guard that hill too
                    ASSERT(!ant.ghill);
                    ant.ghill = ant.bhill;
                    hills.at(ant.ghill->pos).numGuards++; //const pointers so we can't alter numgaurds directly
                    continue;
                }

                for(auto hill_it = hills.begin(); hill_it != hills.end(); ++hill_it)
                {
                    auto& hill = hill_it->second;
                    if (hill.numGuards >= numGuards) {continue;}
                    if (hill.knownDistance(p)) {queue.push(std::make_tuple(hill.pos, p), (hill.dists[p] + hill.closestEnemyDist));}
                }
            }

            while(queue)
            {
                const auto node = queue.pop();
                auto& hill = hills.at(get<0>(node));
                auto& ant = myAnts.at(get<1>(node));

                if (hill.numGuards < numGuards && !ant.ghill){
                    ant.ghill = &hill;
                    hill.numGuards++;
                    state.bug << ant.pos << " is guarding " << hill.pos << "\n";
                }
            }
        }
    }

    //Move myAnts
    solveEverything(state);

    state.bug << "time taken: " << state.time() << "ms\n";
    state.updateSymmetry(); //Done if there's time left at end of turn
};

//finishes the turn
void Bot::endTurn()
{
    state.turn++;

    //Important to use endl so that stream is flushed properly
    cout << "go" << endl;
};
