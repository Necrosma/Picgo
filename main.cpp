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
    // outFile = fopen("out.txt","wt");

    outFile = fopen(argv[3],"wt");
    src.open(argv[1], ios::in);
    
    streampos pos =src.tellg();
    src.seekg(0,ios::end);
    if(src.tellg()<21) return 111;
    src.seekg(pos);  

    tokens.open("tokens.txt", ios::out);

    getch();
    getsym();
    // program();

    if(!analyse()) return -1;

    tokens.close();
    src.close();
    fclose(outFile);
    return 0;
}

