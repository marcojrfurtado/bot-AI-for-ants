#include "Bot.h"

using namespace std;

//constructor
Bot::Bot()
{

    bug.open("debug_bot",".txt");
};
Bot::~Bot()
{
    bug.close();
};

//plays a single game of Ants.
void Bot::playGame()
{
    //reads the game parameters and sets up
    cin >> state;
    state.setup();
    state.defineGuardians();
    #ifdef VIZ
    //cout << "v setFillColor 0 0 0 0.5"<<endl;
    //cout<< "v setLineWidth 5"<<endl;
    //cout<< "v setLineColor 0 0 0 1.0"<<endl;
    #endif
    endTurn();

    //continues making moves while the game is not over
    while(cin >> state)
    {
        state.bug << "turn " << turn << ":" << endl;
        state.updateVisionInformation();
    	state.defineGuardians();
        battleCheck();
        state.bug << "setup: " << state.timer.getTime() << "ms" << endl;
        tactics();
        state.bug << "tactics: " << state.timer.getTime() << "ms" << endl;
        state.preDiffuse();
        state.diffuse(8);
        state.preDiffuse();
        state.diffuse(3);
        makeMoves();
        state.bug << "diffuse: " << state.timer.getTime() << "ms" << endl;
	state.print_status();
        endTurn();
    }
    //state.bug.close();
};
void Bot::battleCheck()
{
    Location antLoc;
    std::queue<Location> checkMyAnts, checkEnemyAnts;
    FightGroup currentFightGroup;

    // Divide the ants into fighting groups and non-fighting ants.
    while (!state.enemyAnts.empty())
    {
        currentFightGroup.myAnts.clear();
        currentFightGroup.enemyAnts.clear();
        antLoc = state.enemyAnts.front();
        bool addToFightGroup_aEnemyAnt = false;
        int counter = 0;
        // Go through the list of my ants and see which ones are in currentFightGroup
        list<Location>::iterator b;
        for (b = state.myAnts.begin(); b != state.myAnts.end();)
        {

//		if ( b->isGuardian ) {
//			bug << " battleCheck: Guardian found " << endl;
//			continue;
//		}

            if(state.distance(*b, antLoc) <= ENG_RAD)
            {
                checkMyAnts.push(*b); // myAnt is in currentFightGroup
                currentFightGroup.myAnts.push_back(*b);
                //state.myAnts.remove(*b);    // Remove my ant from the list of myAnts
                b = state.myAnts.erase(b);
                addToFightGroup_aEnemyAnt = true;
            }
            else
            {
                b++;
            }
        }
        if (addToFightGroup_aEnemyAnt)
        {
            currentFightGroup.enemyAnts.push_back(state.enemyAnts.front());
        }
        else
        {
            state.enemyNonFightingAnts.push_back(state.enemyAnts.front());
            state.enemyFightingAnts.push_back(state.enemyAnts.front());
        }
        state.enemyAnts.pop_front();  // Remove enemy ant from the list of enemyAnts
        bool doneBuildingFightGroup = false;


        while (!doneBuildingFightGroup)
        {
            counter++;

            // Go through queue of checkMyAnts and see which enemy ants are part of currentFightGroup
            while (checkMyAnts.size() > 0)
            {
                antLoc = checkMyAnts.front();
                list<Location>::iterator c;
                for (c = state.enemyAnts.begin(); c != state.enemyAnts.end();)
                {
                    if(state.distance(*c, antLoc) <= ENG_RAD)
                    {
                        checkEnemyAnts.push(*c); // enemyAnt is in currentFightGroup
                        currentFightGroup.enemyAnts.push_back(*c);
                        c = state.enemyAnts.erase(c); // Remove enemy ant from the list of enemyAnts
                    }
                    else
                    {
                        c++;
                    }
                }
                checkMyAnts.pop();
            }

            while (checkEnemyAnts.size() > 0)
            {
                antLoc = checkEnemyAnts.front();
                list<Location>::iterator d;
                for (d = state.myAnts.begin(); d != state.myAnts.end();)
                {
                    if(state.distance(*d, antLoc) <= ENG_RAD)
                    {
                        checkMyAnts.push(*d); // myAnt is in currentFightGroup
                        currentFightGroup.myAnts.push_back(*d);
                        d = state.myAnts.erase(d); // Remove my ant from the list of myAnts
                    }
                    else
                    {
                        d++;
                    }
                }
                checkEnemyAnts.pop();
            }

            if ((checkMyAnts.size() == 0) && (checkEnemyAnts.size() == 0))
            {
                doneBuildingFightGroup = true;
            }
        }
        if (currentFightGroup.myAnts.size() > 0)
        {
            state.fightingGroups.push_back(currentFightGroup);  // Done with this fighting group
            state.bug << "Fight group added to vector: enemyAnts=" << currentFightGroup.enemyAnts.size() << ", myAnts=" << currentFightGroup.myAnts.size() << endl;
        }
    }
/*    for(int i=0;i<(int)state.fightingGroups.size();i++)
    {
        for(int b=0;b<(int)state.fightingGroups[i].myAnts.size();b++){
            bug<<"v circle "<<state.fightingGroups[i].myAnts[b].isGuardian << endl;
        }
    }*/
    #ifdef VIZ
    for(int i=0;i<(int)state.fightingGroups.size();i++)
    {
        for(int b=0;b<(int)state.fightingGroups[i].myAnts.size();b++){
            cout<<"v circle "<<state.fightingGroups[i].myAnts[b].row<<" "<<state.fightingGroups[i].myAnts[b].col<<" "<<(i+1)/2.<<" false"<<endl;
        }
        for(int b=0;b<(int)state.fightingGroups[i].enemyAnts.size();b++){
            cout<<"v circle "<<state.fightingGroups[i].enemyAnts[b].row<<" "<<state.fightingGroups[i].enemyAnts[b].col<<" "<<(i+1)/2.<<" false"<<endl;
        }
    }
    #endif
}
int Bot::moveTo(Location loc1, Location loc2){
    //returns the best direction for moving from loc1 to loc2
    //const char CDIRECTIONS[4] = {'N', 'E', 'S', 'W'};
    int d1 = abs(loc1.row-loc2.row),
        d2 = abs(loc1.col-loc2.col),
        dr = min(d1, rows-d1),
        dc = min(d2, cols-d2);
    if (dr>dc){
        if (loc1.row-loc2.row < 0)
            return 2;
        else
            return 0;
    }
    else{
        if (loc1.col-loc2.col < 0)
            return 1;
        else
            return 3;
    }
}
int Bot::moveAway(Location loc1, Location loc2){
    //returns the best direction for moving from loc1 away from loc2
    //const char CDIRECTIONS[4] = {'N', 'E', 'S', 'W'};
    int d1 = abs(loc1.row-loc2.row),
        d2 = abs(loc1.col-loc2.col),
        dr = min(d1, rows-d1),
        dc = min(d2, cols-d2);
    if (dr>dc){
        if (loc1.row-loc2.row < 0)
            return 0;
        else
            return 2;
    }
    else{
        if (loc1.col-loc2.col < 0)
            return 3;
        else
            return 1;
    }
}

Orders Bot::getfit(FightGroup B)
{
    Location loc;
    int mydeath, theirdeath, dist;
    float beclose=0;
    mydeath=theirdeath=0;
    //first tally up enemy count around the ant
    for(int a=0; a<(int) B.enemyAnts.size(); a++){
        B.en_enemies.push_back(0);
    }
    for(int a=0; a<(int)B.myAnts.size(); a++){
        loc =B.N_my[a];
        B.my_enemies.push_back(0);
        for(int b=0; b<(int) B.enemyAnts.size(); b++){
            dist = state.distance(loc,B.N_en[b]);
            if (dist <= ATK_RAD){
               B.my_enemies[a]++;
               B.en_enemies[b]++;
            }
            beclose += 1./dist;
        }
    }

    //then find out who dies
    for(int a=0; a<(int)B.myAnts.size(); a++){
        loc =B.N_my[a];
        for(int b=0; b<(int) B.enemyAnts.size(); b++){
            if (state.distance(loc,B.N_en[b]) <= ATK_RAD && B.en_enemies[b] <=B.my_enemies[a])
                mydeath++;
        }
    }
    for(int a=0; a<(int) B.enemyAnts.size(); a++){
        loc = B.N_en[a];
        for(int b=0; b<(int)B.myAnts.size(); b++){
            if (state.distance(loc,B.N_my[b]) <= ATK_RAD && B.my_enemies[b]<= B.en_enemies[a])
                theirdeath++;
        }
    }
    //state.bug<<"my:"<<mydeath<<" their:"<<theirdeath<<" close"<<beclose<<" tot:"<<theirdeath*10-(mydeath*10.5)+beclose<<endl;
    return Orders(theirdeath*1000-(mydeath*1150)+beclose);
}
Orders Bot::bestof(int i,FightGroup B)
{//i = # of ant in vector currently done
    Orders best,tmp;
    Location loc,nloc;
    if (i == (int)(B.myAnts.size()+B.enemyAnts.size())){
        //state.bug<<"fitting"<<endl;
        return getfit(B);
    } else {
        //state.bug<<"i:"<<i;
        if (i < (int)B.myAnts.size())
        {
            int bdir=0;
            loc = B.myAnts[i];
            for(int d=0; d<5; d++)
            {
                nloc = state.getLoc(loc,d);
                if (state.grid[nloc.row][nloc.col].movable() && state.grid[nloc.row][nloc.col].AntHereCheck==false){
                    state.grid[nloc.row][nloc.col].AntHereCheck=true;
                    B.N_my.push_back(nloc);
                    //state.bug<<" "<<d<<" nloc:"<<nloc.row<<","<<nloc.col<<endl;
                    tmp = bestof(i+1,B);
                    if (tmp.fitness > best.fitness)
                    {
                        best = tmp;
                        bdir=d;
                    }
                    B.N_my.pop_back();
                    state.grid[nloc.row][nloc.col].AntHereCheck=false;
                }
            }
//	    if ( loc.isGuardian )
//		    bug << "Guard to fight group." << endl;
            best.add(loc,bdir);
        }else{
            best.fitness = 9999999;
            loc = B.enemyAnts[i-B.myAnts.size()];
            for(int d=0; d<5; d++)
            {
                nloc = state.getLoc(loc,d);
                if (state.grid[nloc.row][nloc.col].movable() && state.grid[nloc.row][nloc.col].AntHereCheck==false){
                    state.grid[nloc.row][nloc.col].AntHereCheck=true;
                    B.N_en.push_back(nloc);
                    //state.bug<<" "<<d<<" n:"<<nloc.row<<","<<nloc.col<<endl;
                    tmp = bestof(i+1,B);
                    if (tmp.fitness < best.fitness)
                    {
                        best = tmp;
                    }
                    B.N_en.pop_back();
                    state.grid[nloc.row][nloc.col].AntHereCheck=false;
                }
            }
        }
        //state.bug<<"i"<<i<<":"<<best.fitness<<endl;
        return best;
    }
}
void Bot::smallbatt(FightGroup B)
{
    Orders final = bestof(0,B);
    //state.bug<<"("<<final.ants[0].row<<","<<final.ants[0].col<<")->my:"<<final.md<<" their:"<<final.ed<<" close"<<final.dist<<" tot:"<<final.fitness<<"  "<<state.timer.getTime()<<"ms"<<endl;
    for(int i = 0;i<(int)final.dir.size();i++)
    {
//	   if ( !final.ants[i].isGuardian  )
           state.makeMove(final.ants[i],final.dir[i]);
//	   if ( final.ants[i].isGuardian  )
  //      	bug << "Guardian small battle" << endl;
    }
}
void Bot::largebatt(FightGroup B)
{
    Location closest,loc;
    vector<Order> orderlist;//allow multiple ants to work together better
    vector<Location> bigallies,allies,mustfight,mustfightto;//if one ant charges in with an ally, the ally must join that fight
    for(int a=0; a<(int)B.myAnts.size(); a++)
    {
        int mindist,dirMove,enemies;
        bool mustgo=false;
        allies.clear();
        bigallies.clear();
        enemies = 0;
        loc = B.myAnts[a];
        mindist = 100;//mindist = ENG_RAD;
        for(int b=0; b<(int) mustfight.size(); b++){//check if this guy must fight
            if (mustfight[b]==loc)
            {
                mustgo=true;
               closest=mustfightto[b];//and move him towards that fight
                dirMove = moveTo(loc,closest);
                break;
            }
        }
        if (!mustgo)//otherwise, check enemy/ally count
        {
            for(int b=0; b<(int) B.enemyAnts.size(); b++){
                int dist;
                dist = state.distance(loc,B.enemyAnts[b]);
                if (dist <= mindist){
                    closest = B.enemyAnts[b];
                    mindist = state.distance(loc,B.enemyAnts[b]);
                }
                if (dist <= ENG_RAD){
                    enemies++;
                    for(int c=0; c<(int)B.myAnts.size(); c++)//update allies list
                    {
                        dist = state.distance(B.enemyAnts[b],B.myAnts[c]);
                        if (dist <= ENG_RAD){
                            bigallies.push_back(B.myAnts[c]);
                            if (dist <= MID_RAD){
                                allies.push_back(B.myAnts[c]);
                                #ifdef VIZ
                                cout<< "v circle " << B.myAnts[c].row<<" "<<B.myAnts[c].col<<" .3 false"<<endl;
                                #endif
                            }
                        }
                    }
                }
            }
            vector<Location>::iterator pte = unique(allies.begin(), allies.end());
            allies.erase(pte, allies.end());//remove duplicate allies- there will be a lot of them
            pte = unique(bigallies.begin(), bigallies.end());
            bigallies.erase(pte, bigallies.end());//remove duplicate allies- there will be a lot of them
            if(enemies < (int)allies.size())
            {
                dirMove=moveTo(loc,closest);
                //if you're attacking, make sure all allies come in to fight
                for(int b=0; b<(int) allies.size(); b++){
                    mustfight.push_back(allies[b]);
                    mustfightto.push_back(closest);//and make sure they aim for the right enemy
                    #ifdef VIZ
                    cout<< "v line " << loc.row<<" "<<loc.col<<" "<<allies[b].row<<" "<<allies[b].col<<endl;
                    #endif
                }
            }
            else if(allies.size() > 0 && enemies > (int)allies.size())
                dirMove=moveAway(loc,closest);
            else if(enemies*3 <= (int)bigallies.size())
            {
                dirMove=moveTo(loc,closest);
                //if you're attacking, make sure all allies come in to fight
                for(int b=0; b<(int) bigallies.size(); b++){
                    mustfight.push_back(bigallies[b]);
                    mustfightto.push_back(closest);//and make sure they aim for the right enemy
                }
            }
            else
                dirMove = -1;
        }
        state.grid[closest.row][closest.col].foodDif += 35;
        Location nloc = state.getLoc(loc, dirMove);
        #ifdef VIZ
        cout<< "v line " << loc.row<<" "<<loc.col<<" "<<nloc.row<<" "<<nloc.col<<endl;
        cout<< "v arrow " << loc.row<<" "<<loc.col<<" "<<closest.row<<" "<<closest.col<<endl;
        #endif
        orderlist.push_back(Order(loc,dirMove));

        unsigned int lastsize=orderlist.size()+1;
        while(orderlist.size() != lastsize)//this orderlist also handles mustfighting for ants that went before the calling ally
        {
            lastsize=orderlist.size();
            for(int a=0; a<(int) orderlist.size(); a++)
            {
                loc = orderlist[a].loc;
                for(int b=0; b<(int) mustfight.size(); b++){//check if this guy must fight
                    if (mustfight[b]==loc)
                    {
                        orderlist[a].dir = moveTo(loc,mustfightto[b]);
                        break;
                    }
                }
                dirMove = orderlist[a].dir;
                if (dirMove != -1){
                    nloc = state.getLoc(loc, dirMove);
                    if (state.grid[nloc.row][nloc.col].isOpen() ){
                        state.makeMove(loc,dirMove);
                        orderlist.erase(orderlist.begin()+a);
                        a--;
                    }
	  // 	    if ( loc.isGuardian  )
      	//	  	bug << "Guardian large battle" << endl;
                }
            }
        }
    }
}
void Bot::tactics()
{//splits battles into small and big to be called seprately
    for(int i=0;i<(int)state.fightingGroups.size();i++)
    {
        int fighters = state.fightingGroups[i].myAnts.size()+state.fightingGroups[i].enemyAnts.size();
        if ((state.timer.getTime()>(state.turntime-150) && fighters>5) || fighters > 7)
            largebatt(state.fightingGroups[i]);
        else
            smallbatt(state.fightingGroups[i]);
    }
}
void Bot::makeMoves()
{
    //if (turn < 100) state.bug << state << endl;
    //picks out moves for each food ant
    int bestd;
    float maxDif, maxTotalDif;
    Location loc;

    list<Location>::iterator ant;
    for (ant = state.myAnts.begin(); ant != state.myAnts.end(); ant++)
    //for(int ant=0; ant<(int)state.myAnts.size(); ant++)
    {
        loc = *ant;
        bestd = 4;
        maxDif = state.grid[loc.row][loc.col].foodDif-100;
        maxTotalDif = maxDif + state.grid[loc.row][loc.col].guardDif;
        //state.bug<<"from "<<state.grid[loc.row][loc.col].foodDif<<endl;
        for(int d=0; d<4; d++)
        {
	    int count = 0;
            Location nloc = state.getLoc(loc, d);
            if (state.grid[nloc.row][nloc.col].AntHereCheck) break;


	    if ( ant->isGuardian ) {
//		    bug << "Teste " << state.grid[nloc.row][nloc.col].guardDif << " " << state.grid[nloc.row][nloc.col].foodDif << endl;
            	    float totalDif = state.grid[nloc.row][nloc.col].foodDif + state.grid[nloc.row][nloc.col].guardDif;
		    if(state.grid[nloc.row][nloc.col].isOpen() ) {
				    
			if ( totalDif >= maxTotalDif )  {
		    
			    maxTotalDif = totalDif;
			    bestd = d;
		    	}
/*
			if ( state.grid[nloc.row][nloc.col].foodDif >= maxGuardDif)  {
		    
			    maxGuardDif = state.grid[nloc.row][nloc.col].foodDif;
			    bestd = d;
		    	}*/
		    }
		    
		    if (state.grid[nloc.row][nloc.col].isFood)
		    {//if there's food next to us, collect it now
			bestd = d;
			state.grid[nloc.row][nloc.col].foodDif=0;//and then run away from it, since we don't need its scent anymore
			break;
		    }
            	    
	    } else {

		    if (state.grid[nloc.row][nloc.col].hillPlayer > 0)
		    {//if there is an enemy hill next to us, KILL IT NAO
			bestd = d;
			break;
		    }
		    // PROBLEMA: DUAS FORMIGAS PODEM BUSCAR O MESMO ALIMENTO

		    if (state.grid[nloc.row][nloc.col].isFood)
		    {//if there's food next to us, collect it now
			bestd = d;
			state.grid[nloc.row][nloc.col].foodDif=0;//and then run away from it, since we don't need its scent anymore
			break;
		    }
		    if(state.grid[nloc.row][nloc.col].isOpen() && state.grid[nloc.row][nloc.col].foodDif >= maxDif)
		    {//otherwise, go in the direction of need
			maxDif = state.grid[nloc.row][nloc.col].foodDif;
			bestd = d;
		    }
	    }
        }

//	if ( !loc.isGuardian )
	state.makeMove(loc,bestd);
        //state.bug<<state.grid[loc.row][loc.col].foodDif<<" ";
    }

};

//finishes the turn
void Bot::endTurn()
{
    state.bug << "total: " << state.timer.getTime() << "ms" << endl << endl;
    #ifdef VIZ
    float max,min;
    max = -999999999; min=999999999;
    if (turn > 0 && turn < 150)
    {
        rowRep{colRep{
            if (state.grid[row][col].isOpen()){
                if (state.grid[row][col].foodDif < min)
                    min = state.grid[row][col].foodDif;
                if (state.grid[row][col].foodDif > max)
                    max = state.grid[row][col].foodDif;
            }
        }}
        /*rowRep{colRep{
            if (state.grid[row][col].isOpen()){
                cout<<"v setFillColor 0 0 0 "<< (((state.grid[row][col].foodDif)+min)/(max-min))<<endl;
                cout<< "v tile " << row << " " << col<< endl;
            }
        }}*/
        state.bug << "GFX: " << state.timer.getTime() << "ms  :"<<min<<"/"<<max << endl << endl;
    }
    #endif
    if(turn > 0)
        state.reset();
    turn++;

    cout << "go" << endl;
};
