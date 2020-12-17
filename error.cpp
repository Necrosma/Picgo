#include <iostream>
#include <string.h>
#include <fstream>
#include <sstream>
#include <stdlib.h>
using namespace std;
#define FILEOPEN_ERROR -1 //file open failed
#define ZEROSTART_ERROR 0 //
#define UNDEFFUNC_ERROR 1
#define UNDEFCHAR_ERROR 2 //undefined IDEN
#define UNMATCHSQ_ERROR 3 //右单引号不匹配
#define UNMATCHDQ_ERROR 4 //右双引号不匹配
#define DIGIT_LACK_ERROR 5 //数字为0位
#define PRENOTONLY_ERROR 6 //!不能单独出现
#define ESCAPE_QUENCE_ERROR 7 //转义符不完整
#define COMMENT_FORMAT_ERROR 8 //注释格式错误
#define COMMENT_LACK_ERROR 9 //注释为空
#define UNMATCH_ERROR 10 //无匹配项
#define E 99 // develop

char WARNING[] = {'W','a','r','n','i','n','g',':',' ','\0'};
char ERROR[] =  {'E','r','r','o','r',':',' ','\0'};

void error(int errorcode, char* token){
    cout << token << endl;
    switch(errorcode){
        case -1 :
            cout << "file open failed!" << endl;
            exit(-1);
            break;
        case 0 ://warning
            cout <<  WARNING << "a num should not start with 0 " << endl;
            exit(-1);
            break;
        case 1 :
            cout << ERROR << "undefined function name " << endl;
            exit(-1);
            break;
        case 2 :
            cout << ERROR << "undefined identifier name " << endl;
            exit(-1);
            break;
        case 3 :
            cout << ERROR << "unmatched singal quotes " << endl;
            exit(-1);
            break;
        case 4 :
            cout << ERROR << "unmatched double quotes " << endl;
            exit(-1);
            break;
        case 5 :
            cout << ERROR << "digit num count should > 0 !" << endl;
            exit(-1);
            break;
        case 6 :
            cout << ERROR << "'!' should not appear alone" << endl;
            exit(-1);
            break;
        case 7 :
            cout << ERROR << "should be \\n,\\t...." << endl;
            exit(-1);
            break;
        case 8 :
            cout << ERROR << "comment has only one '/' " << endl;
            exit(-1);
            break;
        case 9 :
            cout << ERROR << "comment is null " << endl;
            exit(-1);
            break;
        case 10 :
            cout << ERROR << "input can not matched" << endl;
            exit(-1);
            break;
        case 99 :
            cout << ERROR << "ERROR" << endl;
            exit(-1);
            break;
    }
}
