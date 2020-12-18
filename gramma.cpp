#include <iostream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <fstream>
#include <sstream>
#include <string.h>
#include <cstring>
#include "lexer.cpp"
using namespace std;

// 符号表 =============================================
//TODO:按指令使用情况检查
uint64_t doubleToRawBits(double x)
{
  uint64_t bits;
  memcpy(&bits, &x, sizeof bits);
  return bits;
}
void intToTwoBits(int x, unsigned char *str)
{
  str[1] = x;
  str[2] = x >> 8;
}
void intToFourBits(int x, unsigned char *str)
{
  str[1] = x;
  str[2] = x >> 8;
  str[3] = x >> 16;
  str[4] = x >> 24;
}
void intToEightBits(int64_t x, unsigned char *str)
{
  str[1] = x;
  str[2] = x >> 8;
  str[3] = x >> 16;
  str[4] = x >> 24;
  str[5] = x >> 32;
  str[6] = x >> 40;
  str[7] = x >> 48;
  str[8] = x >> 56;
}
void pushIns(int x, vector<unsigned char> &instructions)
{
  unsigned char str[5];
  memset(str, 0, sizeof(str));
  intToFourBits(x, str);
  instructions.push_back(str[4]);
  instructions.push_back(str[3]);
  instructions.push_back(str[2]);
  instructions.push_back(str[1]);
}
void pushIns(int64_t x, vector<unsigned char> &instructions)
{
  unsigned char str[9];
  memset(str, 0, sizeof(str));
  intToEightBits(x, str);
  instructions.push_back(str[8]);
  instructions.push_back(str[7]);
  instructions.push_back(str[6]);
  instructions.push_back(str[5]);
  instructions.push_back(str[4]);
  instructions.push_back(str[3]);
  instructions.push_back(str[2]);
  instructions.push_back(str[1]);
}
void pushIns(int16_t x, vector<unsigned char> &instructions)
{
  unsigned char str[3];
  memset(str, 0, sizeof(str));
  intToTwoBits(x, str);
  instructions.push_back(str[2]);
  instructions.push_back(str[1]);
}
//全局变量常量、字符串字面量交错存储，函数在二者之后
typedef struct GLOBAL
{
  string name;
  string dataType;
  bool is_const;
  GLOBAL() : name(), dataType(), is_const() {}
  GLOBAL(string name, string dataType, bool is_const) : name(name), dataType(dataType), is_const(is_const) {}
} Global;
vector<Global> Gmap;
typedef struct LOCAL
{
  vector<Global> vars;
  vector<int> postionInFuntion;
  //所属函数
  int funtionPos;
  //父域，-1表示父域为全局变量，其余为Lmap下表
  int upRange;
  /*
    存放待填写的breaks，first是Fmap[funtionPos].instructions[insPos]，代码待填写的位置
    second是当前指令数，用于被insNum减，得到放入first位置的值
    */
  // vector<pair<int, int>> breaks;
  // bool is_while;
  // int continueNum; //TODO:初始化
  LOCAL(int funtionPos, int upRange) : vars(), postionInFuntion(), funtionPos(funtionPos), upRange(upRange) {}
} Local;
vector<Local> Lmap;
typedef struct FUNTION
{
  int pos;
  int localSlotNum;
  int paramSlotNum; //TODO:检查是否增加Num
  string name;
  string retType;
  //!=instructions.size()
  int insNum;
  vector<unsigned char> instructions;
  vector<Global> params;
  FUNTION(string name) : pos(0), localSlotNum(0), paramSlotNum(0), name(name), retType(""), insNum(0), instructions(), params() {}
} Funtion;
vector<Funtion> Fmap;

vector<unsigned char> instructions;
//======================================

/*===========================================/
Define
/===========================================*/
bool analyse();
void function();
void let_decl_stmt(int funtionPos, int rangePos);
void const_decl_stmt(int funtionPos, int rangePos);
void stmt(int funtionPos, int rangePos);
void if_stmt(int funtionPos, int rangePos);
void while_stmt(int funtionPos, int rangePos);
void break_stmt(int funtionPos, int rangePos);
void continue_stmt(int funtionPos, int rangePos);
void return_stmt(int funtionPos, int rangePos);
void block_stmt(int funtionPos, int upRange);
void expr(int funtionPos, int rangePos, int *retType);

void pushSymTab(char *name, int type, int value, int address, int para);
void insertSymTab(char *name, int type, int value, int address, int para);
int searchSymTab(char *name);
void insertPara(int para);
void delSymTab();

/*===========================================/
Utils
/===========================================*/
void check(int id)
{
  if (symId != id)
  {
    printf("Excepted Id: %d, Actual Id: %d\n", id, symId);
    error(99, token);
  }
}
bool isExpr()
{
  if (symId==IDENT || symId==MINUS || symId==L_PAREN || symId == UINT_LITERAL || symId == DOUBLE_LITERAL || symId == STRING_LITERAL || symId == CHAR_LITERAL 
    || symId==GETINT || symId==GETDOUBLE || symId==GETCHAR || symId==PUTINT || symId==PUTDOUBLE || symId==PUTCHAR || symId==PUTSTR || symId==PUTLN 
  ) return true;
  else
    return false;
}
bool init_start()
{
  //stackalloc(1)
  Fmap[0].instructions.push_back(0x1a);
  Fmap[0].instructions.push_back(0x00);
  Fmap[0].instructions.push_back(0x00);
  Fmap[0].instructions.push_back(0x00);
  Fmap[0].instructions.push_back(0x01);
  Fmap[0].insNum++;
  //call(1)
  Fmap[0].instructions.push_back(0x48);
  Fmap[0].instructions.push_back(0x00);
  Fmap[0].instructions.push_back(0x00);
  Fmap[0].instructions.push_back(0x00);
  Fmap[0].instructions.push_back(0x01);
  Fmap[0].insNum++;
  //popn(1)
  Fmap[0].instructions.push_back(0x03);
  Fmap[0].instructions.push_back(0x00);
  Fmap[0].instructions.push_back(0x00);
  Fmap[0].instructions.push_back(0x00);
  Fmap[0].instructions.push_back(0x01);
  Fmap[0].insNum++;
  return true;
}
/*===========================================/
Functions
/===========================================*/
void program()
{
  //默认Fmap[0]是_start
  Fmap.push_back(Funtion("_start"));
  Fmap[0].retType = "void";
  //默认Fmap[1]是main
  Fmap.push_back(Funtion("main"));
  while (true)
  {
    if (symId == FN_KW)
    {
      // printf("[LOGGER] FUN: %s\n",token);
      function();
    }
    else if (symId == LET_KW)
      let_decl_stmt(0, 0);
    else if (symId == CONST_KW)
      const_decl_stmt(0, 0);
    else
      break;
  }
}

//function -> 'fn' IDENT '(' function_param_list? ')' '->' ty block_stmt
//function_param_list -> function_param (',' function_param)*
//function_param -> 'const'? IDENT ':' ty
void function()
{
  int funtionPos = Fmap.size();
  getsym();
  check(IDENT);
  if (!strcmp(token, "main")) //确定位置
    funtionPos = 1;
  else //重名检查
  {
    for (int i = 0; i < Fmap.size(); i++)
      if (!strcmp(token, Fmap[i].name.c_str()))
        error(99, token);
    // printf("Fmap.push: %s\n",token);
    Fmap.push_back(Funtion(token));
  }

  getsym();
  check(L_PAREN);

  getsym(); // CONST | IDENT | ')'
  /*----- function_param_list -----*/
  if (symId == CONST_KW || symId == IDENT)
  {
    vector<Global> tempParams;
    do
    {
      if (symId == COMMA)
        getsym(); //TODO fn(,a,b)
      /*----- function_param -----*/
      Global tempParam;
      tempParam.is_const = false;
      if (symId == CONST_KW)
      {
        tempParam.is_const = true;
        getsym(); // IDENT
      }
      check(IDENT);
      tempParam.name = token;

      getsym(); // :
      check(COLON);
      getsym(); // ty
      check(IDENT);
      if (!strcmp(token, "int") || !strcmp(token, "double") || !strcmp(token, "void"))
        tempParam.dataType = token;
      else
        error(99, token);
      tempParams.push_back(tempParam);
      getsym(); // ',' | ')'
    } while (symId == COMMA);

    Fmap[funtionPos].params = tempParams;
    Fmap[funtionPos].paramSlotNum = Fmap[funtionPos].params.size();
  }
  //null_list
  check(R_PAREN);

  getsym(); // ->
  check(ARROW);

  getsym(); // ty
  check(IDENT);
  if (!strcmp(token, "int") || !strcmp(token, "double") || !strcmp(token, "void")){
    Fmap[funtionPos].retType = token;
    // printf("FMAP TYPE: %s\n",Fmap[funtionPos].retType.c_str());
  }
  else
    error(99, token);

  getsym();
  block_stmt(funtionPos, -1);
  //默认ret
  Fmap[funtionPos].instructions.push_back(0x49);
  Fmap[funtionPos].insNum++;
}
/* '{' stmt* '}'
	stmt ->
      expr_stmt
    | decl_stmt
    | if_stmt
    | while_stmt
    | break_stmt
    | continue_stmt
    | return_stmt
    | block_stmt
    | empty_stmt
*/
void block_stmt(int funtionPos, int upRange)
{
  int rangePos = Lmap.size();
  Lmap.push_back(Local(funtionPos, upRange));

  check(L_BRACE);

  getsym();
  while (true)
  {
    if (symId == CONST_KW)
      const_decl_stmt(funtionPos, rangePos);
    else if (symId == LET_KW)
      let_decl_stmt(funtionPos, rangePos);
    else if (symId == IF_KW)
      if_stmt(funtionPos, rangePos);
    else if (symId == WHILE_KW)
      while_stmt(funtionPos, rangePos);
    else if (symId == BREAK_KW)
      error(99,token);
    else if (symId == CONTINUE_KW)
      error(99,token);
    else if (symId == RETURN_KW)
      return_stmt(funtionPos, rangePos);
    else if (symId == SEMICOLON)
      getsym();
    else if (symId == L_BRACE)
      block_stmt(funtionPos, rangePos);
    else if (isExpr())
    {
      int retType = 0;
      expr(funtionPos, rangePos, &retType);
      if (retType != 3)
      {
        //popn
        Fmap[funtionPos].instructions.push_back(0x03);
        pushIns(1, Fmap[funtionPos].instructions);
        Fmap[funtionPos].insNum++;
      }
      check(SEMICOLON);
      getsym();
    }
    else
    {
      break;
    }
  }
  check(R_BRACE); //stmt尾已读
  getsym();
}

/* 'const' IDENT ':' ty '=' expr ';' */
void const_decl_stmt(int funtionPos, int rangePos)
{
  getsym(); // IDENT
  check(IDENT);
  string preToken = token;
  getsym(); // :
  check(COLON);
  getsym();     // ty
  check(IDENT); // 'int' 'double' 'void'
  int retType = 0;
  if (!strcmp(token, "int"))
    retType = 1;
  else if (!strcmp(token, "double"))
    retType = 2;
  else
  {
    puts("定义数据类型不是int或double");
    error(99, token);
  }
  Global tempVar(preToken, token, true);
  int varPos = 0;
  //查重后放入符号表
  if (funtionPos == 0)
  {
    //全局变量查重
    for (int i = 0; i < Gmap.size(); i++)
    {
      if (Gmap[i].dataType == "string")
        continue;
      else if (Gmap[i].name == tempVar.name)
        error(99, token);
    }
    varPos = Gmap.size();
    Gmap.push_back(tempVar);
  }
  else
  {
    //所在域查重
    for (int i = 0; i < Lmap[rangePos].vars.size(); i++)
    {
      if (Lmap[rangePos].vars[i].dataType == "string")
        continue;
      else if (Lmap[rangePos].vars[i].name == tempVar.name)
        error(99, token);
    }
    if (Lmap[rangePos].upRange == -1)
    {
      //和函数参数查重
      for (int i = 0; i < Fmap[funtionPos].params.size(); i++)
      {
        if (Fmap[funtionPos].params[i].dataType == "string")
          continue;
        else if (Fmap[funtionPos].params[i].name == tempVar.name)
          error(99, token);
      }
    }
    varPos = Fmap[funtionPos].localSlotNum++;
    Lmap[rangePos].vars.push_back(tempVar);
    int postionInFuntion = Lmap[rangePos].postionInFuntion.size();
    Lmap[rangePos].postionInFuntion.push_back(postionInFuntion);
  }

  getsym(); // =
  check(ASSIGN);

  if (funtionPos == 0)
  {
    //globa
    Fmap[funtionPos].instructions.push_back(0x0c);
    pushIns(varPos, Fmap[funtionPos].instructions);
  }
  else
  {
    //loca
    Fmap[funtionPos].instructions.push_back(0x0a);
    pushIns(varPos, Fmap[funtionPos].instructions);
  }
  Fmap[funtionPos].insNum++;

  getsym();
  expr(funtionPos, rangePos, &retType); // ;
  //store64
  Fmap[funtionPos].instructions.push_back(0x17);
  Fmap[funtionPos].insNum++;

  check(SEMICOLON);
  getsym();
}
/* 'let' IDENT ':' ty ('=' expr)? ';' */
void let_decl_stmt(int funtionPos, int rangePos)
{
  getsym(); // IDENT
  check(IDENT);
  string preToken = token;
  getsym(); // :
  check(COLON);
  getsym(); // ty
  check(IDENT);
  int retType = 0;
  if (!strcmp(token, "int"))
    retType = 1;
  else if (!strcmp(token, "double"))
    retType = 2;
  else
    error(99, token);
  Global tempVar(preToken, token, false);
  int varPos = 0;
  //查重后放入符号表
  if (funtionPos == 0)
  {
    //全局变量查重
    for (int i = 0; i < Gmap.size(); i++)
    {
      if (Gmap[i].dataType == "string")
        continue;
      else if (Gmap[i].name == tempVar.name)
        error(99, token);
    }
    varPos = Gmap.size();
    Gmap.push_back(tempVar);
  }
  else
  {
    //所在域查重
    for (int i = 0; i < Lmap[rangePos].vars.size(); i++)
    {
      if (Lmap[rangePos].vars[i].dataType == "string")
        continue;
      else if (Lmap[rangePos].vars[i].name == tempVar.name)
        error(99, token);
    }
    if (Lmap[rangePos].upRange == -1)
    {
      //和函数参数查重
      for (int i = 0; i < Fmap[funtionPos].params.size(); i++)
      {
        if (Fmap[funtionPos].params[i].dataType == "string")
          continue;
        else if (Fmap[funtionPos].params[i].name == tempVar.name)
          error(99, token);
      }
    }
    varPos = Fmap[funtionPos].localSlotNum++;
    Lmap[rangePos].vars.push_back(tempVar);
    int postionInFuntion = Lmap[rangePos].postionInFuntion.size();
    Lmap[rangePos].postionInFuntion.push_back(postionInFuntion);
  }

  getsym(); // = | ;
  if (symId == ASSIGN)
  {
    if (funtionPos == 0)
    {
      //globa
      Fmap[funtionPos].instructions.push_back(0x0c);
      pushIns(varPos, Fmap[funtionPos].instructions);
    }
    else
    {
      //loca
      Fmap[funtionPos].instructions.push_back(0x0a);
      pushIns(varPos, Fmap[funtionPos].instructions);
    }
    Fmap[funtionPos].insNum++;
    getsym();
    expr(funtionPos, rangePos, &retType); // ;
    //store64
    Fmap[funtionPos].instructions.push_back(0x17);
    Fmap[funtionPos].insNum++;
  }
  check(SEMICOLON);
  getsym();
}
//if_stmt -> 'if' expr block_stmt ('else' block_stmt|if_stmt)?
void if_stmt(int funtionPos, int rangePos)
{
  int retType = 0;
  getsym();
  expr(funtionPos, rangePos, &retType); // {
  //brtrue(1)
  Fmap[funtionPos].instructions.push_back(0x43);
  pushIns(1, Fmap[funtionPos].instructions);
  Fmap[funtionPos].insNum++;
  //br(0)0等待替换;
  Fmap[funtionPos].instructions.push_back(0x41);
  int waitPos = Fmap[funtionPos].instructions.size();
  pushIns(0, Fmap[funtionPos].instructions);
  int ifLowNum = ++Fmap[funtionPos].insNum;

  block_stmt(funtionPos, rangePos);
  int ifHighNum = Fmap[funtionPos].insNum;

  //预读else
  if (symId == ELSE_KW)
  {
    //br(0)0等待替换;
    Fmap[funtionPos].instructions.push_back(0x41);
    int waitPos = Fmap[funtionPos].instructions.size();
    pushIns(0, Fmap[funtionPos].instructions);
    int elseLowNum = ++Fmap[funtionPos].insNum;
    ifHighNum++;

    getsym();
    if (symId == IF_KW)
    {
      if_stmt(funtionPos, rangePos);
    }
    else if (symId == L_BRACE)
    {
      block_stmt(funtionPos, rangePos);
    }
    else
      error(99, token);
    //修改else等待替换的br(0)
    unsigned char str[5];
    memset(str, 0, sizeof(str));
    intToFourBits(Fmap[funtionPos].insNum - elseLowNum, str);
    for (int i = 0; i < 4; i++)
    {
      Fmap[funtionPos].instructions[waitPos + i] = str[4 - i];
    }
  }
  //修改if等待替换的br(0)
  unsigned char str[5];
  memset(str, 0, sizeof(str));
  intToFourBits(ifHighNum - ifLowNum, str);
  for (int i = 0; i < 4; i++)
  {
    Fmap[funtionPos].instructions[waitPos + i] = str[4 - i];
  }
  //结尾添加一个br(0)
  Fmap[funtionPos].instructions.push_back(0x41);
  pushIns(0, Fmap[funtionPos].instructions);
  Fmap[funtionPos].insNum++;
}
/* 'while' expr block_stmt */
void while_stmt(int funtionPos, int rangePos)
{
  int retType = 0;
  //br(0)
  Fmap[funtionPos].instructions.push_back(0x41);
  int whilePos = Fmap[funtionPos].instructions.size();
  pushIns(0, Fmap[funtionPos].instructions);
  int whileNum = ++Fmap[funtionPos].insNum;
  getsym();
  expr(funtionPos, rangePos, &retType); // {
                                        //brtrue(1)
  Fmap[funtionPos].instructions.push_back(0x43);
  pushIns(1, Fmap[funtionPos].instructions);
  Fmap[funtionPos].insNum++;
  //br(0)0等待替换，在符号表中添加continueNum;
  Fmap[funtionPos].instructions.push_back(0x41);
  int waitPos = Fmap[funtionPos].instructions.size();
  pushIns(0, Fmap[funtionPos].instructions);
  int tempNum = ++Fmap[funtionPos].insNum;
  // Lmap[rangePos].continueNum = tempNum;

  block_stmt(funtionPos, rangePos);
  //添加循环br(-?)
  Fmap[funtionPos].instructions.push_back(0x41);
  pushIns(whileNum - Fmap[funtionPos].insNum - 1, Fmap[funtionPos].instructions);
  Fmap[funtionPos].insNum++;
  //修改开头等待替换的0
  unsigned char str[5];
  memset(str, 0, sizeof(str));
  intToFourBits(Fmap[funtionPos].insNum - tempNum, str); //TODO:check
  for (int i = 0; i < 4; i++)
  {
    Fmap[funtionPos].instructions[waitPos + i] = str[4 - i];
  }
  //修改break等待替换的0
  // for (int i = 0; i < Lmap[rangePos].breaks.size(); i++)
  // {
  //   unsigned char str[5];
  //   memset(str, 0, sizeof(str));
  //   intToFourBits(Fmap[funtionPos].insNum - Lmap[rangePos].breaks[i].second, str);
  //   for (int i = 0; i < 4; i++)
  //   {
  //     Fmap[funtionPos].instructions[Lmap[rangePos].breaks[i].first + i] = str[4 - i];
  //   }
  // }
}
/* 'return' expr? ';' */
void return_stmt(int funtionPos, int rangePos)
{
  getsym();
  if (symId != SEMICOLON)
  {
    //arga(0)
    Fmap[funtionPos].instructions.push_back(0x0b);
    pushIns(0, Fmap[funtionPos].instructions);
    Fmap[funtionPos].insNum++;
    //返回值
    int retType;
    if (Fmap[funtionPos].retType == "int")
      retType = 1;
    else if (Fmap[funtionPos].retType == "double")
      retType = 2;
    else
      error(99, token);
    expr(funtionPos, rangePos, &retType);
    //store64
    Fmap[funtionPos].instructions.push_back(0x17);
    Fmap[funtionPos].insNum++;
  }
  else
  {
    if (Fmap[funtionPos].retType != "void")
      error(99, token);
  }
  //ret
  Fmap[funtionPos].instructions.push_back(0x49);
  Fmap[funtionPos].insNum++;
  check(SEMICOLON); //TODO
  getsym();
}

//=============================

/*
expr -> 
    operator_expr
    | negate_expr
    | assign_expr
    | as_expr
    | call_expr
    | literal_expr
    | ident_expr
    | group_expr      
expr -> high_expr(比较运算符high_expr)*
high_expr -> medium_expr(加减运算符medium_expr)*
medium_expr -> low_expr(乘除运算符low_expr)*
//应先预读到IDENT、'-'、'('、literal 才进入analyseExpr
*/
void HighExpr(int funtionPos, int rangePos, int *retType);
void MediumExpr(int funtionPos, int rangePos, int *retType);
void LowExpr(int funtionPos, int rangePos, int *retType);
void CallParamList(int funtionPos, int rangePos, int callFuntionPos);
void expr(int funtionPos, int rangePos, int *retType)
{
  HighExpr(funtionPos, rangePos, retType);
  while (true)
  {
    if (symId == LT)
    {
      getsym();
      HighExpr(funtionPos, rangePos, retType);
      if (*retType == 1)
        Fmap[funtionPos].instructions.push_back(0x30);
      else
        Fmap[funtionPos].instructions.push_back(0x32);
      Fmap[funtionPos].insNum++;
      Fmap[funtionPos].instructions.push_back(0x39);
      Fmap[funtionPos].insNum++;
      *retType = 4;
    }
    else if (symId == LE)
    {
      getsym();
      HighExpr(funtionPos, rangePos, retType);
      if (*retType == 1)
        Fmap[funtionPos].instructions.push_back(0x30);
      else
        Fmap[funtionPos].instructions.push_back(0x32);
      Fmap[funtionPos].insNum++;
      Fmap[funtionPos].instructions.push_back(0x3a);
      Fmap[funtionPos].insNum++;
      Fmap[funtionPos].instructions.push_back(0x2e);
      Fmap[funtionPos].insNum++;
      *retType = 4;
    }
    else if (symId == GT)
    {
      getsym();
      HighExpr(funtionPos, rangePos, retType);
      if (*retType == 1)
        Fmap[funtionPos].instructions.push_back(0x30);
      else
        Fmap[funtionPos].instructions.push_back(0x32);
      Fmap[funtionPos].insNum++;
      Fmap[funtionPos].instructions.push_back(0x3a);
      Fmap[funtionPos].insNum++;
      *retType = 4;
    }
    else if (symId == GE)
    {
      getsym();
      HighExpr(funtionPos, rangePos, retType);
      if (*retType == 1)
        Fmap[funtionPos].instructions.push_back(0x30);
      else
        Fmap[funtionPos].instructions.push_back(0x32);
      Fmap[funtionPos].insNum++;
      Fmap[funtionPos].instructions.push_back(0x39);
      Fmap[funtionPos].insNum++;
      Fmap[funtionPos].instructions.push_back(0x2e);
      Fmap[funtionPos].insNum++;
      *retType = 4;
    }
    else if (symId == EQ)
    {
      getsym();
      HighExpr(funtionPos, rangePos, retType);
      if (*retType == 1)
        Fmap[funtionPos].instructions.push_back(0x30);
      else
        Fmap[funtionPos].instructions.push_back(0x32);
      Fmap[funtionPos].insNum++;
      Fmap[funtionPos].instructions.push_back(0x2e);
      Fmap[funtionPos].insNum++;
      *retType = 4;
    }
    else if (symId == NEQ)
    {
      getsym();
      HighExpr(funtionPos, rangePos, retType);
      if (*retType == 1)
        Fmap[funtionPos].instructions.push_back(0x30);
      else
        Fmap[funtionPos].instructions.push_back(0x32);
      Fmap[funtionPos].insNum++;
      *retType = 4;
    }
    else
      break;
  }
}

void HighExpr(int funtionPos, int rangePos, int *retType)
{
  MediumExpr(funtionPos, rangePos, retType);
  while (true)
  {
    if (symId == PLUS)
    {
      getsym();
      MediumExpr(funtionPos, rangePos, retType);
      if (*retType == 1)
      {
        Fmap[funtionPos].instructions.push_back(0x20);
        Fmap[funtionPos].insNum++;
      }
      else if (*retType == 2)
      {
        Fmap[funtionPos].instructions.push_back(0x24);
        Fmap[funtionPos].insNum++;
      }
      else
        error(99, token);
    }
    else if (symId == MINUS)
    {
      getsym();
      MediumExpr(funtionPos, rangePos, retType);
      if (*retType == 1)
      {
        Fmap[funtionPos].instructions.push_back(0x21);
        Fmap[funtionPos].insNum++;
      }
      else if (*retType == 2)
      {
        Fmap[funtionPos].instructions.push_back(0x25);
        Fmap[funtionPos].insNum++;
      }
      else
        error(99, token);
    }
    else
      break;
  }
}

void MediumExpr(int funtionPos, int rangePos, int *retType)
{
  LowExpr(funtionPos, rangePos, retType);
  while (true)
  {
    if (symId == MUL)
    {
      getsym();
      LowExpr(funtionPos, rangePos, retType);
      if (*retType == 1)
      {
        Fmap[funtionPos].instructions.push_back(0x22);
        Fmap[funtionPos].insNum++;
      }
      else if (*retType == 2)
      {
        Fmap[funtionPos].instructions.push_back(0x26);
        Fmap[funtionPos].insNum++;
      }
    }
    else if (symId == DIV)
    {
      getsym();
      LowExpr(funtionPos, rangePos, retType);
      if (*retType == 1)
      {
        Fmap[funtionPos].instructions.push_back(0x23);
        Fmap[funtionPos].insNum++;
      }
      else if (*retType == 2)
      {
        Fmap[funtionPos].instructions.push_back(0x27);
        Fmap[funtionPos].insNum++;
      }
    }
    else
      break;
  }
}

void LowExpr(int funtionPos, int rangePos, int *retType)
{
  if (symId == IDENT)
  {
    string preToken = token;
    //TODO 当前已读IDENT，在读下一个之前需要保存
    getsym(); // '(' | '=' | AFTER
    // 函数调用 IDENT '(' call_param_list? ')'
    if (symId == L_PAREN)
    {
      //查找funtion，从main开始
      int callFuntionPos = 0;
      for (int i = 1; i < Fmap.size(); i++)
        if (preToken == Fmap[i].name)
        {
          callFuntionPos = i;
          break;
        }
      //没找到
      if (callFuntionPos == 0)
        error(99, token);
      //压入_ret
      if (Fmap[callFuntionPos].retType == "void")
      {
        //找到的函数返回值不是要求的返回值
        if (*retType != 0 && *retType != 3)
          error(99, token);
        //压入
        //stackalloc(0)
        Fmap[funtionPos].instructions.push_back(0x1a);
        pushIns(0, Fmap[funtionPos].instructions);
        Fmap[funtionPos].insNum++;
        //返回调用者想要查看的返回值类型
        if (*retType == 0)
          *retType = 3;
      }
      else if (Fmap[callFuntionPos].retType == "int")
      {
        // printf("%d\n", *retType);
        //找到的函数返回值不是要求的返回值
        if (*retType != 0 && *retType != 1)
          error(99, token);
        //压入
        //stackalloc(1)
        Fmap[funtionPos].instructions.push_back(0x1a);
        pushIns(1, Fmap[funtionPos].instructions);
        Fmap[funtionPos].insNum++;
        //返回调用者想要查看的返回值类型
        if (*retType == 0)
          *retType = 1;
      }
      else if (Fmap[callFuntionPos].retType == "double")
      {
        //找到的函数返回值不是要求的返回值
        if (*retType != 0 && *retType != 2)
          error(99, token);
        //压入
        //stackalloc(1)
        Fmap[funtionPos].instructions.push_back(0x1a);
        pushIns(1, Fmap[funtionPos].instructions);
        Fmap[funtionPos].insNum++;
        //返回调用者想要查看的返回值类型
        if (*retType == 0)
          *retType = 2;
      }

      getsym(); // call_param_list | ')'
      if (symId == R_PAREN)
      {           // 空列表
        Fmap[funtionPos].instructions.push_back(0x48);
        pushIns(callFuntionPos, Fmap[funtionPos].instructions);
        Fmap[funtionPos].insNum++;
        getsym(); // read next
      }
      else
      {
        CallParamList(funtionPos, rangePos, callFuntionPos);
        Fmap[funtionPos].instructions.push_back(0x48);
        pushIns(callFuntionPos, Fmap[funtionPos].instructions);
        Fmap[funtionPos].insNum++;

        check(R_PAREN);
        getsym(); // read next
      }
    }

    // 赋值语句 l_expr '=' expr
    else if (symId == ASSIGN)
    {
      //调用者想要的返回值不是void
      if (*retType != 0 && *retType != 3)
        error(99, token);
      //查找变量
      int tempRangePos = rangePos;
      int varType = 0;
      bool local = false, param = false, global = false;
      //向上域进行查找
      while (Lmap[tempRangePos].upRange != -1)
      {
        for (int i = 0; i < Lmap[tempRangePos].vars.size(); i++)
        {
          if (preToken == Lmap[tempRangePos].vars[i].name)
          {
            if (Lmap[tempRangePos].vars[i].is_const)
              error(99, token);
            if (Lmap[tempRangePos].vars[i].dataType == "int")
              varType = 1;
            else if (Lmap[tempRangePos].vars[i].dataType == "double")
              varType = 2;
            else
              error(99, token);
            local = true;
            //loca()
            Fmap[funtionPos].instructions.push_back(0x0a);
            pushIns(Lmap[tempRangePos].postionInFuntion[i], Fmap[funtionPos].instructions);
            Fmap[funtionPos].insNum++;
            break;
          }
        }
        if (local)
          break;
        else
          tempRangePos = Lmap[tempRangePos].upRange;
      }
      //函数的block
      if (!local)
      {
        for (int i = 0; i < Lmap[tempRangePos].vars.size(); i++)
        {
          if (preToken == Lmap[tempRangePos].vars[i].name)
          {
            if (Lmap[tempRangePos].vars[i].is_const)
              error(99, token);
            if (Lmap[tempRangePos].vars[i].dataType == "int")
              varType = 1;
            else if (Lmap[tempRangePos].vars[i].dataType == "double")
              varType = 2;
            else
              error(99, token);
            local = true;
            //loca()
            Fmap[funtionPos].instructions.push_back(0x0a);
            pushIns(Lmap[tempRangePos].postionInFuntion[i], Fmap[funtionPos].instructions);
            Fmap[funtionPos].insNum++;
            break;
          }
        }
      }
      //函数的参数
      if (!local)
      {
        for (int i = 0; i < Fmap[funtionPos].params.size(); i++)
        {
          if (preToken == Fmap[funtionPos].params[i].name)
          {
            if (Fmap[funtionPos].params[i].is_const)
              error(99, token);
            if (Fmap[funtionPos].params[i].dataType == "int")
              varType = 1;
            else if (Fmap[funtionPos].params[i].dataType == "double")
              varType = 2;
            else
              error(99, token);
            param = true;
            //arga()
            Fmap[funtionPos].instructions.push_back(0x0b);
            if (Fmap[funtionPos].retType == "void")
              pushIns(i, Fmap[funtionPos].instructions);
            else
              pushIns(i + 1, Fmap[funtionPos].instructions);
            Fmap[funtionPos].insNum++;
            break;
          }
        }
      }
      //全局变量
      if (!local && !param)
      {
        for (int i = 0; i < Gmap.size(); i++)
        {
          if (Gmap[i].dataType != "string")
          {
            if (preToken == Gmap[i].name)
            {
              if (Gmap[i].is_const)
                error(99, token);
              if (Gmap[i].dataType == "int")
                varType = 1;
              else if (Gmap[i].dataType == "double")
                varType = 2;
              else
                error(99, token);
              global = true;
              //globa()
              Fmap[funtionPos].instructions.push_back(0x0c);
              pushIns(i, Fmap[funtionPos].instructions);
              Fmap[funtionPos].insNum++;
              break;
            }
          }
        }
      }
      if (!local && !param && !global)
        error(99, token);
      getsym(); //expr
      expr(funtionPos, rangePos, &varType);
      if (*retType == 0)
        *retType = 3;
      //store64
      Fmap[funtionPos].instructions.push_back(0x17);
      Fmap[funtionPos].insNum++;
    }
    // 变量调用 IDENT 注：此时以读了下一个token
    else
    {
      // printf("[LOGGER] 变量调用: %s\n", preToken.c_str());
      //查找变量
      int tempRangePos = rangePos;
      bool local = false, param = false, global = false;
      //向上域进行查找
      while (Lmap[tempRangePos].upRange != -1)
      { //达到0层直接跳出
        for (int i = 0; i < Lmap[tempRangePos].vars.size(); i++)
        {
          if (preToken == Lmap[tempRangePos].vars[i].name)
          {
            if (Lmap[tempRangePos].vars[i].dataType == "int")
            {
              if (*retType != 0 && *retType != 1)
                error(99, token);
              if (*retType == 0)
                *retType = 1;
            }
            else if (Lmap[tempRangePos].vars[i].dataType == "double")
            {
              if (*retType != 0 && *retType != 2)
                error(99, token);
              if (*retType == 0)
                *retType = 2;
            }
            else if (Lmap[tempRangePos].vars[i].dataType == "void")
            {
              if (*retType != 0 && *retType != 3)
                error(99, token);
              if (*retType == 0)
                *retType = 3;
            }
            else
              error(99, token);
            local = true;
            //loca()
            Fmap[funtionPos].instructions.push_back(0x0a);
            pushIns(Lmap[tempRangePos].postionInFuntion[i], Fmap[funtionPos].instructions);
            Fmap[funtionPos].insNum++;
            break;
          }
        }
        if (local)
          break; // 向浅层找到了变量
        tempRangePos = Lmap[tempRangePos].upRange;
      }
      //函数的block //查找第0层？？？意义？
      if (!local)
      {
        for (int i = 0; i < Lmap[tempRangePos].vars.size(); i++)
        {
          if (preToken == Lmap[tempRangePos].vars[i].name)
          {
            if (Lmap[tempRangePos].vars[i].dataType == "int")
            {
              if (*retType != 0 && *retType != 1)
              {
                printf("返回类型错误：Excepted 1 Actual %d\n", *retType);
                error(99, token);
              }
              if (*retType == 0)
                *retType = 1;
            }
            else if (Lmap[tempRangePos].vars[i].dataType == "double")
            {
              if (*retType != 0 && *retType != 2)
              {

                printf("返回类型错误：Excepted 2 Actual %d\n", *retType);
                error(99, token);
              }
              if (*retType == 0)
                *retType = 2;
            }
            else if (Lmap[tempRangePos].vars[i].dataType == "void")
            {
              if (*retType != 0 && *retType != 3)
              {
                printf("返回类型错误：Excepted 0 Actual %d\n", *retType);
                error(99, token);
              }
              if (*retType == 0)
                *retType = 3;
            }
            else
            {
              printf("返回类型错误：Actual %d\n", *retType);
              error(99, token);
            }
            local = true;
            //loca()
            Fmap[funtionPos].instructions.push_back(0x0a);
            pushIns(Lmap[tempRangePos].postionInFuntion[i], Fmap[funtionPos].instructions);
            Fmap[funtionPos].insNum++;
            break;
          }
        }
      }
      //函数的参数
      if (!local)
      {
        for (int i = 0; i < Fmap[funtionPos].params.size(); i++)
        {
          if (preToken == Fmap[funtionPos].params[i].name)
          {
            if (Fmap[funtionPos].params[i].dataType == "int")
            {
              if (*retType != 0 && *retType != 1)
                error(99, token);
              if (*retType == 0)
                *retType = 1;
            }
            else if (Fmap[funtionPos].params[i].dataType == "double")
            {
              if (*retType != 0 && *retType != 2)
                error(99, token);
              if (*retType == 0)
                *retType = 2;
            }
            else if (Fmap[funtionPos].params[i].dataType == "void")
            {
              if (*retType != 0 && *retType != 3)
                error(99, token);
              if (*retType == 0)
                *retType = 3;
            }
            else
              error(99, token);
            param = true;
            //arga()
            Fmap[funtionPos].instructions.push_back(0x0b);
            if (Fmap[funtionPos].retType == "void")
              pushIns(i, Fmap[funtionPos].instructions);
            else
              pushIns(i + 1, Fmap[funtionPos].instructions);
            Fmap[funtionPos].insNum++;
            break;
          }
        }
      }
      //全局变量
      if (!local && !param)
      {
        for (int i = 0; i < Gmap.size(); i++)
        {
          if (Gmap[i].dataType != "string")
          {
            if (preToken == Gmap[i].name)
            {

              if (Gmap[i].dataType == "int")
              {
                if (*retType != 0 && *retType != 1)
                  error(99, token);
                if (*retType == 0)
                  *retType = 1;
              }
              else if (Gmap[i].dataType == "double")
              {
                if (*retType != 0 && *retType != 2)
                  error(99, token);
                if (*retType == 0)
                  *retType = 2;
              }
              else if (Gmap[i].dataType == "void")
              {
                if (*retType != 0 && *retType != 3)
                  error(99, token);
                if (*retType == 0)
                  *retType = 3;
              }
              else
              {
                puts(Gmap[i].dataType.c_str());
                error(99, token);
              }
              global = true;

              //globa()
              Fmap[funtionPos].instructions.push_back(0x0c);
              pushIns(i, Fmap[funtionPos].instructions);
              Fmap[funtionPos].insNum++;
              break;
            }
          }
        }
      }
      if (!local && !param && !global)
        error(99, token);
      //load64
      Fmap[funtionPos].instructions.push_back(0x13);
      Fmap[funtionPos].insNum++;
      // getsym(); 无需向后读，因为已预读了一个
    }
  }
  else if (symId == UINT_LITERAL)
  {
    // printf("[LOGGER] UINT: %lf\n", num);
    //要求该lowexpr的返回类型不是int
    if (*retType != 0 && *retType != 1)
      error(99, token);
    //TODO:int64_t的上限？
    int64_t temp = atoi(token);
    Fmap[funtionPos].instructions.push_back(0x01);
    pushIns(temp, Fmap[funtionPos].instructions);
    Fmap[funtionPos].insNum++;
    if (*retType == 0)
      *retType = 1;
    getsym();
  }
  else if (symId == DOUBLE_LITERAL)
  {
    // printf("[LOGGER] DOUBLE: %lf\n", num);
    if (*retType != 0 && *retType != 2)
      error(99, token);
    double tempd = atof(token);
    int64_t temp = doubleToRawBits(tempd);
    Fmap[funtionPos].instructions.push_back(0x01);
    pushIns(temp, Fmap[funtionPos].instructions);
    Fmap[funtionPos].insNum++;
    if (*retType == 0)
      *retType = 2;
    getsym();
  }
  else if (symId == STRING_LITERAL)
  {
    // printf("[LOGGER] STRING: %s\n", token);
    //要求该lowexpr的返回类型不是int
    if (*retType != 0 && *retType != 1)
      error(99, token);
    Global temp = Global(token, "string", true);
    int64_t tempNum = Gmap.size();
    Gmap.push_back(temp);
    Fmap[0].instructions.push_back(0x01);
    pushIns(tempNum, Fmap[0].instructions);
    Fmap[0].insNum++;
    if (*retType == 0)
      *retType = 1;
    getsym();
  }
  else if (symId == CHAR_LITERAL)
  {
    // printf("[LOGGER] CHAR: %c\n", token[0]);
    //要求该lowexpr的返回类型不是int
    if (*retType != 0 && *retType != 1)
      error(99, token);
    int64_t tempNum = token[0];
    Fmap[funtionPos].instructions.push_back(0x01);
    pushIns(tempNum, Fmap[funtionPos].instructions);
    Fmap[funtionPos].insNum++;
    if (*retType == 0)
      *retType = 1;
    getsym();
  }
  else if (symId == MINUS)
  { // '-' expr
    if (*retType != 0 && *retType != 1 && *retType != 2)
      error(99, token);
    getsym(); //expr
    int retT = 0;
    expr(funtionPos, rangePos, &retT);
    if (retT == 1)
    {
      Fmap[funtionPos].instructions.push_back(0x34);
      Fmap[funtionPos].insNum++;
      if (*retType == 0)
        *retType = 1;
    }
    else if (retT == 2)
    {
      Fmap[funtionPos].instructions.push_back(0x35);
      Fmap[funtionPos].insNum++;
      if (*retType == 0)
        *retType = 2;
    }
    else
    {
      printf("%d ", retT);
      puts("Minus expr");
      error(99, token);
    }
  }
  else if (symId == L_PAREN)
  { // '(' expr ')'
    getsym();
    if (isExpr())
      expr(funtionPos, rangePos, retType);
    else
      error(99, token);
    check(R_PAREN);
    getsym(); // ;
  }
  else if (symId == GETINT)
  {
    if (*retType != 0 && *retType != 1)
      error(99,token);
    //'('
    getsym();
    check(L_PAREN);
    //')'
    getsym();
    check(R_PAREN);
    //scan.i
    Fmap[funtionPos].instructions.push_back(0x50);
    Fmap[funtionPos].insNum++;
    if (*retType == 0)
      *retType = 1;
    getsym();
  }
  else if (symId == GETDOUBLE)
  {
    if (*retType != 0 && *retType != 2)
      error(99,token);
    //'('
    getsym();
    check(L_PAREN);
    //')'
    getsym();
    check(R_PAREN);
    //scan.f
    Fmap[funtionPos].instructions.push_back(0x52);
    Fmap[funtionPos].insNum++;
    if (*retType == 0)
      *retType = 2;
    getsym();
  }
  else if (symId == GETCHAR)
  {
    if (*retType != 0 && *retType != 1)
      error(99,token);
    //'('
    getsym();
    check(L_PAREN);
    //')'
    getsym();
    check(R_PAREN);
    //scan.f
    Fmap[funtionPos].instructions.push_back(0x51);
    Fmap[funtionPos].insNum++;
    if (*retType == 0)
      *retType = 1;
    getsym();
  }
  else if (symId == PUTINT)
  {
    if (*retType != 0 && *retType != 3){
      printf("PUTINT type error: %d\n",*retType);
      error(99,token);
    }
    int retT = 1;
    //'('
    getsym();
    check(L_PAREN);
    //int
    getsym();
    expr(funtionPos, rangePos, &retT);
    //')'
    check(R_PAREN);
    //print.i
    Fmap[funtionPos].instructions.push_back(0x54);
    Fmap[funtionPos].insNum++;
    if (*retType == 0)
      *retType = 3;
    getsym();
  }
  else if (symId == PUTDOUBLE)
  {
    if (*retType != 0 && *retType != 3)
      error(99,token);
    int retT = 2;
    //'('
    getsym();
    check(L_PAREN);
    //double
    getsym();
    expr(funtionPos, rangePos, &retT);
    //')'
    getsym();
    check(R_PAREN);
    //print.f
    Fmap[funtionPos].instructions.push_back(0x56);
    Fmap[funtionPos].insNum++;
    if (*retType == 0)
      *retType = 3;
    getsym();
  }
  else if (symId == PUTCHAR)
  {
    if (*retType != 0 && *retType != 3)
      error(99,token);
    int retT = 1;
    //'('
    getsym();
    check(L_PAREN);
    //int
    getsym();
    expr(funtionPos, rangePos, &retT);
    //')'
    check(R_PAREN);
    //print.c
    Fmap[funtionPos].instructions.push_back(0x55);
    Fmap[funtionPos].insNum++;
    if (*retType == 0)
      *retType = 3;
    getsym();
  }
  else if (symId == PUTSTR)
  {
    if (*retType != 0 && *retType != 3)
      error(99,token);
    int retT = 1;
    //'('
    getsym();
    check(L_PAREN);
    //STRING
    getsym();
    if (symId != STRING_LITERAL)
      error(99,token);
    Global temp = Global(token, "string", true);
    int64_t tempNum = Gmap.size();
    Gmap.push_back(temp);
    Fmap[funtionPos].instructions.push_back(0x01);
    pushIns(tempNum, Fmap[funtionPos].instructions);
    Fmap[funtionPos].insNum++;
    //')'
    getsym();
    check(R_PAREN);
    //print.s
    Fmap[funtionPos].instructions.push_back(0x57);
    Fmap[funtionPos].insNum++;
    if (*retType == 0)
      *retType = 3;
    getsym();
  }
  else if (symId == PUTLN)
  {
    if (*retType != 0 && *retType != 3)
      error(99,token);
    //'('
    getsym();
    check(L_PAREN);
    //')'
    getsym();
    check(R_PAREN);
    //println
    Fmap[funtionPos].instructions.push_back(0x58);
    Fmap[funtionPos].insNum++;
    if (*retType == 0)
      *retType = 3;
    getsym();
  }
  else
    error(99, token);

  // as 的部分不管了 TODO
  if(symId == AS_KW){
    puts("as is not implement");
    error(99,token);
  } 
}

// expr (',' expr)* [非空才进入]
void CallParamList(int funtionPos, int rangePos, int callFuntionPos)
{
  int retType = 0;
  expr(funtionPos, rangePos, &retType);
  if (retType == 1)
  {
    if (Fmap[callFuntionPos].params[0].dataType != "int")
      error(99, token);
  }
  else if (retType == 2)
  {
    if (Fmap[callFuntionPos].params[0].dataType != "double")
      error(99, token);
  }
  else if (retType == 3)
  {
    if (Fmap[callFuntionPos].params[0].dataType != "void")
      error(99, token);
  }
  else
    error(99, token);
  // 【调用】函数已定义，可查到其参数个数
  int size = Fmap[callFuntionPos].params.size();
  for (int i = 1; i < size; ++i)
  {
    check(COMMA);
    getsym();
    retType = 0;
    expr(funtionPos, rangePos, &retType);
    if (retType == 1)
    {
      if (Fmap[callFuntionPos].params[i].dataType != "int")
        error(99, token);
    }
    else if (retType == 2)
    {
      if (Fmap[callFuntionPos].params[i].dataType != "double")
        error(99, token);
    }
    else if (retType == 3)
    {
      if (Fmap[callFuntionPos].params[i].dataType != "void")
        error(99, token);
    }
    else
      error(99, token);
  }
}

//前提：将所有符号表搞定
bool analyse()
{ //TODO: 写入文件
  //magic
  instructions.push_back(0x72);
  instructions.push_back(0x30);
  instructions.push_back(0x3b);
  instructions.push_back(0x3e);
  //version
  pushIns(1, instructions);
  program();
  //_start的指令集
  if (!init_start())
    return false;
  //Array<GlobalDef>：全局变量常量（全设为0），字符串字面量，函数名
  //Array<GlobalDef>.count
  int globalNum = Gmap.size() + Fmap.size();
  pushIns(globalNum, instructions);
  //Array<GlobalDef>.item 全局变量部分（包括字符串字面量和标准库函数）
  for (int i = 0; i < Gmap.size(); i++)
  {
    if (Gmap[i].dataType == "string")
    {
      //Array<GlobalDef>.item[i].is_const = 1
      instructions.push_back(0x01);
      int arrayNum = Gmap[i].name.size();
      //Array<GlobalDef>.item[i].value.count
      pushIns(arrayNum, instructions);
      for (int j = 0; j < arrayNum; j++)
      {
        //Array<GlobalDef>.item[i].value.item[j]
        instructions.push_back(Gmap[i].name[j]);
      }
    }
    else if (Gmap[i].dataType == "int" || Gmap[i].dataType == "double")
    {
      //Array<GlobalDef>.item[i].is_const
      if (Gmap[i].is_const)
        instructions.push_back(0x01);
      else
        instructions.push_back(0x00);
      //Array<GlobalDef>.item[i].value.count=8
      pushIns(8, instructions);
      for (int j = 0; j < 8; j++)
      {
        //Array<GlobalDef>.item[i].value.item[j]
        instructions.push_back(0x00);
      }
    }
    else{
      return false;
    }
  }
  //Array<GlobalDef>.item 函数部分
  for (int i = 0; i < Fmap.size(); i++)
  {
    Fmap[i].pos = Gmap.size() + i;
    //Array<GlobalDef>.item[Gmap.size()+i].is_const = 1
    instructions.push_back(0x01);
    int arrayNum = Fmap[i].name.size();
    //Array<GlobalDef>.item[Gmap.size()+i].value.count
    pushIns(arrayNum, instructions);
    for (int j = 0; j < arrayNum; j++)
    {
      //Array<GlobalDef>.item[Gmap.size()+i].value.item[j]
      instructions.push_back(Fmap[i].name[j]);
    }
  }

  //Array<FunctionDef>.count
  int funtionNum = Fmap.size();
  pushIns(funtionNum, instructions);
  for (int i = 0; i < funtionNum; i++)
  {
    //Array<FunctionDef>.item[i].name
    int name = Fmap[i].pos;
    pushIns(name, instructions);
    //Array<FunctionDef>.item[i].return_slots
    if (Fmap[i].retType == "int" || Fmap[i].retType == "double")
    {
      int return_slots = 1;
      pushIns(return_slots, instructions);
    }
    else if (Fmap[i].retType == "void")
    {
      int return_slots = 0;
      pushIns(return_slots, instructions);
    }
    else
    {
      printf("Fmap[%d] error: %s\n",i,Fmap[i].retType.c_str());
      return false;
    }
    //Array<FunctionDef>.item[i].param_slots
    pushIns(Fmap[i].paramSlotNum, instructions);
    //Array<FunctionDef>.item[i].loc_slots
    pushIns(Fmap[i].localSlotNum, instructions);
    //Array<FunctionDef>.item[i].Array<Instruction>.count
    pushIns(Fmap[i].insNum, instructions);
    //Array<FunctionDef>.item[i].Array<Instruction>.item
    for (int j = 0; j < Fmap[i].instructions.size(); j++)
    {
      instructions.push_back(Fmap[i].instructions[j]);
    }
  }
  string str;
  // puts("=====================");
  for (int i = 0; i < instructions.size();i++)
  {
    str += instructions[i];

    int n = i+1;
    cout.width(2);
    cout.fill('0');
    cout<<hex<<(int)instructions[i];
    if(!(n%2) && n%16) cout<<' ';
    if(!(n%16)) cout<<'\n';
  }
  fwrite(str.c_str(), str.size(), 1, outFile);
  return true;
}