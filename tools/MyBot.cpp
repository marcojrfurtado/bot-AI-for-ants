#include "Bot.h"

using namespace std;

/*
    This program will play a single game of Ants while communicating with
    the engine via standard input and output.

    The function "makeMoves()" in Bot.cc is where it makes the moves
    each turn and is probably the best place to start exploring. You are
    allowed to edit any part of any of the files, remove them, or add your
    own, provided you continue conforming to the input and output format
    outlined on the specifications page at:
        http://www.ai-contest.com
*/
int main(int argc, char *argv[])
{
    cout.sync_with_stdio(false); //this line makes your bot faster

    Bot bot;

#ifdef HARDCODEINPUT
    #include <fstream>
    std::ifstream stream("0.bot0.input");

    if (!stream.is_open() || !stream.good()){
        std::cout << "Unable to open input stream!\n";
        return 042;
    }

    bot.playGame( stream );
#else
    bot.playGame( std::cin );
#endif

    return 0;
}
