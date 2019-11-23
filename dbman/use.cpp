#include <iostream>
#include <cstring>
#include <unistd.h>
using std::cerr;
using std::cout;
using std::endl;

int use(char * dbname) {
    if (chdir(dbname) == -1) {
        cerr << "ERROR: Unable to switch to database. Please use Linux and make sure the disk writable. " << endl;
        return 1;
    }
    return 0;
}