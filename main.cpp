#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <string.h>
#include "gramma.cpp"
using namespace std;
int main(int argc,char** argv)
{
    // src.open("src.txt", ios::in);
    src.open(argv[1], ios::in);
    tokens.open("tokens.txt", ios::out);
    // outFile = fopen("out.txt","wt");
    outFile = fopen(argv[3],"wt");
    if(!src.is_open()){
        cout << "file open failed" << endl;
        return -1;
    }
    getch();
    getsym();
    // program();

    if(!analyse()) return -1;

    tokens.close();
    src.close();
    fclose(outFile);
    return 0;
}

