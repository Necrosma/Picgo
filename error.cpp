#include <iostream>
#include <string.h>
#include <fstream>
#include <sstream>
#include <stdlib.h>
using namespace std;

#define FILEOPEN_ERROR -1
#define ZEROSTART_ERROR 0 
#define UNDEFFUNC_ERROR 1
#define UNDEFCHAR_ERROR 2
#define UNMATCHSQ_ERROR 3 
#define UNMATCHDQ_ERROR 4 
#define DIGIT_LACK_ERROR 5
#define PRENOTONLY_ERROR 6
#define ESCAPE_QUENCE_ERROR 7
#define COMMENT_FORMAT_ERROR 8
#define COMMENT_LACK_ERROR 9 
#define UNMATCH_ERROR 10
#define RETURN_INT 11
#define RETURN_VOID 12
#define RETURN_BOOL 13
#define UNEXITED_FUN 21
#define UNEXITED_VAR 22
#define UNMATCH_IDENT 23
#define UNMATCH_TYPE 24
#define EXPR_ERROR 31
#define STR_ERROR 32
#define GRAMMER_ERROR 33
#define DUL_FUN 41
#define DUL_VAR 42
#define UNDO 99

void error(int errorcode, char* token){
    cout << token << endl;
    switch(errorcode){
        case -1 :
            cout << "file open failed!" << endl;
            break;
        case 0 :
            cout << "a num should not start with 0 " << endl;
            break;
        case 1 :
            cout << "undefined function name " << endl;
            break;
        case 2 :
            cout << "undefined identifier name " << endl;
            break;
        case 3 :
            cout << "unmatched singal quotes " << endl;
            break;
        case 4 :
            cout << "unmatched double quotes " << endl;
            break;
        case 5 :
            cout << "digit num count should > 0 !" << endl;
            break;
        case 6 :
            cout << "'!' should not appear alone" << endl;
            break;
        case 7 :
            cout << "should be \\n,\\t...." << endl;
            break;
        case 8 :
            cout << "comment has only one '/' " << endl;
            break;
        case 9 :
            cout << "comment is null " << endl;
            break;
        case 10 :
            cout << "input can not matched" << endl;
            break;
        case 11 :
            cout << "Should return INT type" << endl;
            break;
        case 12 :
            cout << "Should return VOID type" << endl;
            break;
        case 13 :
            cout << "Should return BOOL type" << endl;
            break;
        
        case 21 :
            cout << "Can not find function define" << endl;
            break;
        case 22 :
            cout << "Can not find var define" << endl;
            break;
        case 23 :
            cout << "Unreached branch in IDENT" << endl;
            break;
        case 24 :
            cout << "Unreached branch in retType" << endl;
            break;
        case 31 :
            cout << "Should be Expr" << endl;
            break;
        case 32 :
            cout << "Should be Str" << endl;
            break;
        case 99 :
            cout << "AS and BREAK is not implemented" << endl;
            break;
    }
    exit(-1);
}
