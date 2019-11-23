#include <iostream>
#include <cstring>
#include <unistd.h>
using std::cerr;
using std::endl;

int main(int argc, char ** argv) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " DB_NAME" << endl;
        return 1;
    }
    const char * dbname = argv[1];
    char command[128] = "mkdir ";

    system(strcat(command, dbname));

    return 0;
}