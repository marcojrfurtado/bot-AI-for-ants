#ifndef BUG_H_
#define BUG_H_

#include <fstream>
#include <sys/stat.h>
#include <sstream>
#include <string.h>

//#define DEBUG
//#define VIZ
using namespace std;
/*
    struct for debugging - this is gross but can be used pretty much like an ofstream,
                           except the debug messages are stripped while compiling if
                           DEBUG is not defined.
    example:
        Bug bug;
        bug.open("./debug.txt");
        bug << state << endl;
        bug << "testing" << 2.0 << '%' << endl;
        bug.close();
*/

struct Bug
{
    std::ofstream file;

    Bug()
    {

    };

    bool fexist(const char *filename )
    {
        ifstream fin(filename , ios::in );;
        if( fin.is_open() ) //File error
        {
            fin.close();
            return true;
        }
        fin.close();
        return false;

    }

    //opens the specified file of specified filetype, plus a number at the end to prevent overwriting
    inline void open(const std::string &filename,const std::string &filetype)
    {
        #ifdef DEBUG
        string finalfile = filename;
        finalfile.append(filetype);
        if (!fexist(finalfile.c_str()))
            file.open(finalfile.c_str());
        else{
            int i = 1;
            while(fexist(finalfile.c_str()))
            {
                stringstream out;
                out << i;
                finalfile = filename;
                finalfile += out.str();
                finalfile.append(filetype);
                i++;
            }

            file.open(finalfile.c_str());
        }
        #endif
    };

    //closes the ofstream
    inline void close()
    {
        #ifdef DEBUG
            file.close();
        #endif
    };
};

//output function for endl
inline Bug& operator<<(Bug &bug, std::ostream& (*manipulator)(std::ostream&))
{
    #ifdef DEBUG
        bug.file << manipulator;
    #endif

    return bug;
};

//output function
template <class T>
inline Bug& operator<<(Bug &bug, const T &t)
{
    #ifdef DEBUG
        bug.file << t;
    #endif

    return bug;
};

#endif //BUG_H_
