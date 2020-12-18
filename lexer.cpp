#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <cmath>
#include <string.h>
#include "error.cpp"
using namespace std;

#define FN_KW 1        // fn
#define LET_KW 2       // let
#define CONST_KW 3     // const
#define AS_KW 4        // as
#define WHILE_KW 5     // while
#define IF_KW 6        // if
#define ELSE_KW 7      // else
#define RETURN_KW 8    // return
#define BREAK_KW 9     // break
#define CONTINUE_KW 10 // continue
#define GETINT 11
#define GETDOUBLE 12
#define GETCHAR 13
#define PUTINT 14
#define PUTDOUBLE 15
#define PUTCHAR 16
#define PUTSTR 17
#define PUTLN 18

#define PLUS 19     // +
#define MUL 20      // *
#define MINUS 21     // -
#define DIV 22       // /
#define ASSIGN 23    // =
#define EQ 24        // ==
#define NEQ 25       // !=
#define LT 26        // <
#define GT 27        // >
#define LE 28        // <=
#define GE 29        // >=
#define L_PAREN 30   // (
#define R_PAREN 31   // )
#define L_BRACE 32   // {
#define R_BRACE 33   // }
#define ARROW 34     // ->
#define COMMA 35     // ,
#define COLON 36     // :
#define SEMICOLON 37 //;
#define COMMENT 38   // //

#define UINT_LITERAL 41
#define DOUBLE_LITERAL 42
#define STRING_LITERAL 43
#define CHAR_LITERAL 44
#define IDENT 50

string wSymbol[] = {"FN_KW", "LET_KW", "CONST_KW", "AS_KW", "WHILE_KW",
                    "IF_KW", "ELSE_KW", "RETURN_KW", "BREAK_KW", "CONTINUE_KW",
                    "GETINT","GETDOUBLE","GETCHAR","PUTINT","PUTDOUBLE","PUTCHAR","PUTSTR","PUTLN"};
string word[] = {"fn", "let", "const", "as", "while", "if", "else", "return", "break", "continue",
                  "getint","getdouble","getchar","putint","putdouble","putchar","putstr","putln"};

char token[512];
int ip;
string sym = "";
int symId;
char ch = ' ';
double num;
ifstream src;
ofstream tokens;
ofstream _gramma;
FILE *outFile = NULL;

int isLetter()
{
  return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch == '_') ? 1 : 0;
}
int isChar()
{
  return ch == 32 || ch == 33 || (ch >= 35 && ch <= 126) ? 1 : 0;
}
int isDigit()
{
  return ch <= '9' && ch >= '0' ? 1 : 0;
}

int getch()
{
  if (src.peek() != EOF)
    src.get(ch);
  else
    ch = 0;
}
int getsym()
{
  while (ch == '\t' || ch == ' ' || ch == '\r' || ch == '\n')
  {
    getch();
  }
  if (ch == 0)
    return -1;
  ip = 0;
  if (isLetter())
  {
    while (isLetter() || isDigit())
    {
      token[ip++] = ch;
      getch();
      if (ch == 0)
        break;
    }
    token[ip] = '\0';
    //关键字
    int i;
    for (i = 0; i < 18; i++)
    {
      if (!strcmp(word[i].c_str(), token))
      {
        sym = wSymbol[i];
        symId = i + 1;
        break;
      }
    }
    //标识符
    if (i == 18)
    {
      sym = "IDENT";
      symId = IDENT;
    }
  }
  else if (isDigit())
  {
    sym = "UINT_LITERAL";
    symId = UINT_LITERAL;
    num = 0;
    int flag = ch == '0' ? 1 : 0;
    while (isDigit())
    {
      token[ip++] = ch;
      num = num * 10 + (int)(ch - '0');
      getch();
    }
    // 小数  TODO 003, 00.2
    if (ch == '.')
    {
      double bit = 1.0;
      getch();
      if (!isDigit())
        error(DIGIT_LACK_ERROR, token);
      while (isDigit())
      {
        bit /= 10;
        token[ip++] = ch;
        num += (int)(ch - '0') * bit;
        getch();
      }
      // 科学计数法
      if (ch == 'e' || ch == 'E')
      {
        int minus = 0, e = 0;
        getch();
        if (ch == '+' || ch == '-')
        {
          if (ch == '-')
            minus = 1;
          getch();
        }
        if (!isDigit())
          error(DIGIT_LACK_ERROR, token);
        while (isDigit())
        {
          token[ip++] = ch;
          e = e * 10 + (int)(ch - '0');
          getch();
        }
        num = minus ? num / pow(10.0, e) : num * pow(10.0, e);
      }
      sym = "DOUBLE_LITERAL";
      symId = DOUBLE_LITERAL;
    }
    token[ip] = '\0';
  }
  else if (ch == '=')
  {
    token[ip++] = ch;
    sym = "ASSIGN";
    symId = ASSIGN;
    getch();
    if (ch == '=')
    {
      token[ip++] = ch;
      sym = "EQ";
      symId = EQ;
      getch();
    }
    token[ip] = '\0';
  }
  else if (ch == '>')
  {
    token[ip++] = ch;
    sym = "GT";
    symId = GT;
    getch();
    if (ch == '=')
    {
      token[ip++] = ch;
      sym = "GE";
      symId = GE;
      getch();
    }
    token[ip] = '\0';
  }
  else if (ch == '<')
  {
    token[ip++] = ch;
    sym = "LT";
    symId = LT;
    getch();
    if (ch == '=')
    {
      token[ip++] = ch;
      sym = "LE";
      symId = LE;
      getch();
    }
    token[ip] = '\0';
  }
  else if (ch == '-')
  {
    token[ip++] = ch;
    sym = "MINUS";
    symId = MINUS;
    getch();
    if (ch == '>')
    {
      token[ip++] = ch;
      sym = "ARROW";
      symId = ARROW;
      getch();
    }
    token[ip] = '\0';
  }
  else if (ch == '!')
  {
    token[ip++] = ch;
    getch();
    if (ch == '=')
    {
      token[ip++] = ch;
      sym = "NEQ";
      symId = NEQ;
      getch();
    }
    else
    {
      error(PRENOTONLY_ERROR, token); // ! can not appear by only
    }
    token[ip] = '\0';
  }
  /** 
     * CHAR_LITERAL
     * '\'' ([^'\\] | '\' [\\"'nrt]) '\''
     */
  else if (ch == '\'')
  {
    getch();
    if (ch == '\\')
    { //escape_sequence  TODO?
      getch();
      if (ch == '\\')
        token[ip++] = (char)92;
      else if (ch == '\"')
        token[ip++] = (char)34;
      else if (ch == '\'')
        token[ip++] = (char)39;
      else if (ch == 'n')
        token[ip++] = (char)10;
      else if (ch == 'r')
        token[ip++] = (char)13;
      else if (ch == 't')
        token[ip++] = (char)9;
      else
        error(ESCAPE_QUENCE_ERROR, token);
    }
    else if (ch != '\'' && ch != '\\')
    { //char
      token[ip++] = ch;
    }
    else
    {
      token[ip] = '\0';
      error(ESCAPE_QUENCE_ERROR, token); // ' '
    }
    getch();
    if (ch == '\'')
    {
      sym = "CHAR_LITERAL";
      symId = CHAR_LITERAL;
      getch();
    }
    else
    {
      error(UNMATCHSQ_ERROR, token);
    }
  }
  /**
     * STRING_LITERAL
     * '"' ([^"\\] | '\' [\\"'nrt])* '"'
     */
  else if (ch == '\"')
  {
    char tempStr[512];
    int index = 0;
    getch();

    // except "
    while (isChar())
    {
      if (ch == '\\')
      { //escape_sequence  TODO?
        getch();
        if (ch == '\\')
          tempStr[index++] = (char)92;
        else if (ch == '\"')
          tempStr[index++] = (char)34;
        else if (ch == '\'')
          tempStr[index++] = (char)39;
        else if (ch == 'n')
          tempStr[index++] = (char)10;
        else if (ch == 'r')
          tempStr[index++] = (char)13;
        else if (ch == 't')
          tempStr[index++] = (char)9;
        else
          error(ESCAPE_QUENCE_ERROR, token);
      }
      else
        tempStr[index++] = ch;
      getch();
    }
    tempStr[index] = '\0';
    if (ch == '\"')
    {
      strcpy(token, tempStr);
      sym = "STRING_LITERAL";
      symId = STRING_LITERAL;
      getch();
    }
    else
    {
      error(UNMATCHDQ_ERROR, token); // a string must be surrounded by " "
    }
  }
  /**
     * COMMENT
     * '//' regex(.*) '\n'
     */
  else if (ch == '/')
  {
    token[ip++] = ch;
    token[ip] = '\0';
    sym = "DIV";
    symId = DIV;

    getch();
    if (ch == '/')
    {
      getch();
      if (ch == '\n') //TODO
        error(COMMENT_LACK_ERROR, token);
      while (ch != '\n')
      {
        getch();
      }
      sym = "COMMENT";
      symId = COMMENT;
      getch(); //TODO
    }
    // else error(COMMENT_FORMAT_ERROR,token);
  }
  else
  {
    if (ch == ',')
    {
      sym = "COMMA";
      symId = COMMA;
    }
    else if (ch == ':')
    {
      sym = "COLON";
      symId = COLON;
    }
    else if (ch == ';')
    {
      sym = "SEMICOLON";
      symId = SEMICOLON;
    }
    else if (ch == '(')
    {
      sym = "L_PAREN";
      symId = L_PAREN;
    }
    else if (ch == ')')
    {
      sym = "R_PAREN";
      symId = R_PAREN;
    }
    else if (ch == '{')
    {
      sym = "L_BRACE";
      symId = L_BRACE;
    }
    else if (ch == '}')
    {
      sym = "R_BRACE";
      symId = R_BRACE;
    }
    else if (ch == '+')
    {
      sym = "PLUS";
      symId = PLUS;
    }
    else if (ch == '*')
    {
      sym = "MUL";
      symId = MUL;
    }
    else{
      printf("Unmatched char: [%d] '%c'\n",(int)ch,ch);
      error(UNMATCH_ERROR, token);
    }
    token[ip++] = ch;
    token[ip] = '\0';
    getch();
  }

  if (!sym.compare("UINT_LITERAL") || !sym.compare("DOUBLE_LITERAL"))
  {
    tokens << sym << ':' << ' ' << num << endl;
  }
  else if (sym.compare("COMMENT"))
  {
    tokens << sym << ':' << ' ' << token << endl;
  }
  else
    getsym();
  return 0;
}
