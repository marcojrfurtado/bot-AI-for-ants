#include "State.h"

int rows,cols,turn;//these are global variables


using namespace std;
State::State()
{
    gameover = 0;
    turn = 0;
    bug.open("debug",".txt");
};
State::~State()
{
    bug.close();
};

//sets up the game grid
void State::setup()
{
    grid = vector<vector<Square> >(rows, vector<Square>(cols, Square()));
    invisigrid = vector<vector<double> >(rows, vector<double>(cols, 0));
    invisigrid2 = vector<vector<double> >(rows, vector<double>(cols, 0));
    for(int row=0; row<rows; row++)
        for(int col=0; col<cols; col++)
            grid[row][col].init();
};

//resets everything for the next turn
void State::reset()
{
    //std::vector<Location> myHills, enemyHills, food,hillsNowSeen, enemyNonFightingAnts;
    //std::list<Location> myAnts, enemyAnts;
    myAnts.clear();
    enemyAnts.clear();
    myHills.clear();
    enemyNonFightingAnts.clear();
    enemyFightingAnts.clear();
    hillsNowSeen.clear();
    fightingGroups.clear();
    //enemyHills.clear();
    food.clear();
    for(int row=0; row<rows; row++)
        for(int col=0; col<cols; col++)
            grid[row][col].reset();
};

//outputs move information to the engine
void State::makeMove(const Location &loc, int direction)
{
    if (direction < 4 && direction > -1)
    {
        cout << "o " << loc.row << " " << loc.col << " " << CDIRECTIONS[direction] << endl;
    } else direction = 4;
    //this code shouldn't do anything if
    Location nLoc = getLoc(loc, direction);
    grid[loc.row][loc.col].ant = -1;
    grid[nLoc.row][nLoc.col].ant = 0;
    grid[nLoc.row][nLoc.col].AntHereCheck = true;
    if (turn < 50)
        bug<< "from "<<grid[loc.row][loc.col].foodDif<<" to "<<grid[nLoc.row][nLoc.col].foodDif<<" "<<CDIRECTIONS[direction]<<direction<<endl;
};

//returns the [SQUARED] euclidean distance between two locations with the edges wrapped
int State::distance(const Location &loc1, const Location &loc2)
{
    int d1 = abs(loc1.row-loc2.row),
        d2 = abs(loc1.col-loc2.col),
        dr = min(d1, rows-d1),
        dc = min(d2, cols-d2);
    return (dr*dr + dc*dc);
};
//wraps x and y to min and max values
int State::yw(int y){
    if (y < 0)
        return (y+cols);
    else if (y >= cols)
        return (y-cols);
    else
        return y;
}
int State::xw(int x){
    if (x < 0)
        return (x+rows);
    else if (x >= rows)
        return (x-rows);
    else
        return x;
}
//returns the new location from moving in a given direction with the edges wrapped
Location State::getLoc(const Location &loc, int direction)
{
    return Location( xw(loc.row + DIRECTIONS[direction][0]),
                     yw(loc.col + DIRECTIONS[direction][1]));
};

void State::preDiffuse()
{
    Location loc;
    //exploration
    rowRep{
        colRep{
            grid[row][col].foodDif-=2;
            if (grid[row][col].sinceSeen>0){
                //grid[row][col].foodDif++;//since there are more ants than food, this balances it out
                //exploration and looking at old spaces Diffusion isVisible,hasSeen
                if ((!grid[xw(row+1)][col].hasSeen||!grid[xw(row-1)][col].hasSeen||!grid[row][yw(col+1)].hasSeen||!grid[row][yw(col-1)].hasSeen))
                    grid[row][col].foodDif+=2;
                grid[row][col].foodDif += grid[row][col].sinceSeen*.2;
            }
        }
    }
    for(int a=0; a<(int) myHills.size(); a++){//run away from ant hills early on, stay near them later
        loc = myHills[a];
	grid[loc.row][loc.col].guardDif = 20000;
    }

    list<Location>::iterator b;
    for (b = myAnts.begin(); b != myAnts.end(); b++){
        loc = *b;
        grid[loc.row][loc.col].foodDif -= 100;
        if (grid[loc.row][loc.col].foodDif > 0)
            grid[loc.row][loc.col].foodDif = 0;
    }

    for(int a=0; a<(int) enemyNonFightingAnts.size(); a++){//move towards enemies
        loc = enemyNonFightingAnts[a];
        for(int d=0; d<(int) myHills.size(); d++){//especially if near one of my hills
            grid[loc.row][loc.col].foodDif += 6000/(distance(myHills[d],loc)+10);
        }
    }
    for(int a=0; a<(int) enemyFightingAnts.size(); a++){//move towards enemies
        loc = enemyFightingAnts[a];
        grid[loc.row][loc.col].foodDif += 100;
        for(int d=0; d<(int) myHills.size(); d++){//especially if near one of my hills
            grid[loc.row][loc.col].foodDif += 6000/(distance(myHills[d],loc)+10);
        }
    }
    for(int a=0; a<(int) food.size(); a++){//move towards food
        loc = food[a];
        if (grid[loc.row][loc.col].foodDif < 150)
            grid[loc.row][loc.col].foodDif = 150;
        grid[loc.row][loc.col].foodDif += 300;
    }
    for(int a=0; a<(int) enemyHills.size(); a++){//charge ememy anthills
        loc = enemyHills[a];
    //    bug<<"enemy:"<<loc.row<<","<<loc.col<<endl;
        grid[loc.row][loc.col].foodDif += 200;
        if (grid[loc.row][loc.col].foodDif < 1000)
            grid[loc.row][loc.col].foodDif = 1000;
    }
}
#define DiffusionStuffa(x,y) if (!grid[x][y].isWater){count++;tmp_1 += grid[x][y].foodDif; tmp_2 +=grid[x][y].guardDif;}
#define DiffusionStuffb(x,y) if (!grid[x][y].isWater){count++;tmp_1 += invisigrid[x][y]; tmp_2 +=grid[x][y].guardDif;}
void State::diffuse(int repetitions)//diffuses food and (some?)recrution of nearby ants
{//actually does 2x repetition count, using invisigrid
    float tmp_1,tmp_2,count;
    for(int i=0;i<repetitions;i++)
    {
        rowRep{colRep{
            tmp_1=tmp_2=count=0;
            if (grid[row][col].land()){
                DiffusionStuffa(xw(row+1),col);
                DiffusionStuffa(xw(row-1),col);
                DiffusionStuffa(row,yw(col+1));
                DiffusionStuffa(row,yw(col-1));
                invisigrid[row][col] = (tmp_1+(grid[row][col].foodDif*2))/(count+2);
                invisigrid2[row][col] = (tmp_2+(grid[row][col].guardDif*2))/(count+2);
            }
        }}
        rowRep{colRep{
            tmp_1=tmp_2=count=0;
            if (grid[row][col].land()){
                DiffusionStuffb(xw(row+1),col);
                DiffusionStuffb(xw(row-1),col);
                DiffusionStuffb(row,yw(col+1));
                DiffusionStuffb(row,yw(col-1));
                grid[row][col].foodDif = (tmp_1+(invisigrid[row][col]*2))/(count+2);
                grid[row][col].guardDif = (tmp_2+(invisigrid2[row][col]*2))/(count+2);
            }
        }}
    }

 /*   bug << "Food dif: turn " << turns << endl;
    rowRep {
	    colRep {
		    bug << "x: " << row << ", y: " << col << "," << grid[row][col].foodDif << endl ;
	    }
    }
    */
}
void State::updateVisionInformation()
{
    /*Location sLoc;
    for(int a=0; a<(int) myAnts.size(); a++)
    {
        sLoc = myAnts[a];

    }*/
    std::queue<Location> locQueue;
    Location sLoc, cLoc, nLoc;
    list<Location>::iterator a;
    for (a = myAnts.begin(); a != myAnts.end(); a++)
    //for(int a=0; a<(int) myAnts.size(); a++)
    {
        //sLoc = myAnts[a];
        sLoc = *a;
        locQueue.push(sLoc);

        std::vector<std::vector<bool> > visited(rows, std::vector<bool>(cols, 0));
        grid[sLoc.row][sLoc.col].hasSeen = 1;
        grid[sLoc.row][sLoc.col].sinceSeen = 0;
        visited[sLoc.row][sLoc.col] = 1;

        while(!locQueue.empty())
        {
            cLoc = locQueue.front();
            locQueue.pop();

            for(int d=0; d<TDIRECTIONS; d++)
            {
                nLoc = getLoc(cLoc, d);

                if(!visited[nLoc.row][nLoc.col] && distance(sLoc, nLoc) <= viewradius)
                {
                    grid[nLoc.row][nLoc.col].sinceSeen = 0;
                    grid[nLoc.row][nLoc.col].hasSeen = 1;
                    locQueue.push(nLoc);
                }
                visited[nLoc.row][nLoc.col] = 1;
            }
        }
    }

    //after viewing information is updated, check enemy anthills
    Location loc;
    for(int a=0; a<(int) enemyHills.size(); a++)
    {
        loc = enemyHills[a];
        if (grid[loc.row][loc.col].sinceSeen == 0)
        {
            bool hasseen = false;
            for(int b=0; b<(int) hillsNowSeen.size(); b++)
            {
                if (hillsNowSeen[b] == enemyHills[a])
                {
                    hasseen = true;
                    break;
                }
            }
            if (!hasseen)
            {
                enemyHills.erase(enemyHills.begin()+a);
                a--;
            }
        }
    }
};

//input function
istream& operator>>(istream &is, State &state)
{
    int row, col, player;
    string inputType, junk;

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
            is >> turn;
            break;
        }
        else //unknown line
            getline(is, junk);
    }

    if(turn == 0)
    {
        //reads game parameters
        while(is >> inputType)
        {
            if(inputType == "loadtime")
                is >> state.loadtime;
            else if(inputType == "turntime")
                is >> state.turntime;
            else if(inputType == "rows")
                is >> rows;
            else if(inputType == "cols")
                is >> cols;
            else if(inputType == "turns")
                is >> state.turns;
            else if(inputType == "player_seed")
                is >> state.seed;
            else if(inputType == "viewradius2")
                is >> state.viewradius;
            else if(inputType == "attackradius2")
                is >> state.attackradius;
            else if(inputType == "spawnradius2")
                is >> state.spawnradius;
            else if(inputType == "ready") //end of parameter input
            {
                state.timer.start();
                break;
            }
            else    //unknown line
                getline(is, junk);
        }
    }
    else
    {

	int countMyAnts =0;
	
        //reads information about the current turn
        while(is >> inputType)
        {
            if(inputType == "w") //water square
            {
                is >> row >> col;
                state.grid[row][col].isWater = 1;
            }
            else if(inputType == "f") //food square
            {
                is >> row >> col;
                state.grid[row][col].isFood = 1;
                state.food.push_back(Location(row, col));
            }
            else if(inputType == "a") //live ant square
            {
                is >> row >> col >> player;
                state.grid[row][col].ant = player;
                if(player == 0)
                {

	  	  // One fifth of the newly created ants are guardians
   		    countMyAnts++;

		    Location loc = Location(row,col);
		    loc.isGuardian = ( countMyAnts%5 == 0  )?true:false;
                    state.myAnts.push_back(loc);
                }
                else
                    state.enemyAnts.push_back(Location(row, col));
            }
            else if(inputType == "d") //dead ant square
            {
                is >> row >> col >> player;
                //state.grid[row][col].deadAnts.push_back(player);
                //I don't plan on ever using this, and it may make squares a non-PoD struct
            }
            else if(inputType == "h")
            {
                is >> row >> col >> player;
                state.grid[row][col].isHill = 1;
                state.grid[row][col].hillPlayer = player;
                if(player == 0)
                    state.myHills.push_back(Location(row, col));
                else{
                    state.hillsNowSeen.push_back(Location(row, col));
                    if (state.grid[row][col].hasSeen == false)
                        state.enemyHills.push_back(Location(row, col));
                }

            }
            else if(inputType == "players") //player information
                is >> state.noPlayers;
            else if(inputType == "scores") //score information
            {
                state.scores = vector<double>(state.noPlayers, 0.0);
                for(int p=0; p<state.noPlayers; p++)
                    is >> state.scores[p];
            }
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
    }

    return is;
};
