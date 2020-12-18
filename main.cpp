#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <string.h>
#include "gramma.cpp"
using namespace std;
int main(int argc,char** argv)
{
//     src.open("src.txt", ios::in);
//     outFile = fopen("out.txt","wt");

    outFile = fopen(argv[3],"wt");
    src.open(argv[1], ios::in);
    
    streampos pos =src.tellg();
    src.seekg(0,ios::end);
    if(src.tellg()<24) return -1;
    src.seekg(pos);  

    tokens.open("tokens.txt", ios::out);

    getch();
    getsym();
    parse();

    tokens.close();
    src.close();
    fclose(outFile);
    return 0;
}

