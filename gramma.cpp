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
#include "asm.cpp"
#define LOAD true
#define ASSIGN_ false
using namespace std;

void parse();
void function();
void let_decl_stmt(int funtionPos,int rangePos_);
void const_decl_stmt(int funtionPos,int rangePos_);
void stmt(int funtionPos);
void if_stmt(int funtionPos);
void while_stmt(int funtionPos);
void break_stmt(int funtionPos);
void continue_stmt(int funtionPos);
void return_stmt(int funtionPos);
void block_stmt(int funtionPos, int upRange);
void expr(int funtionPos, int *retType);

int rangePos;
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
  Lmap.push_back(Local(0, -1, 0)); //todo
  while (true)
  {
    if (symId == FN_KW)
      function();
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
  }
  else
    error(99, token);

  getsym();
  //函数父域为全局变量，0
  block_stmt(funtionPos, 0);
  //默认ret
  F_instruction(funtionPos,0x49);
  
}
/* '{' stmt* '}'
	stmt -> expr_stmt | decl_stmt | if_stmt | while_stmt | return_stmt | block_stmt
*/
void block_stmt(int funtionPos, int upRange)
{
  rangePos = Lmap.size();
  int savePos = rangePos;
  Lmap.push_back(Local(funtionPos, upRange));
  check(L_BRACE);

  getsym();
  while (true)
  {
    if (symId == CONST_KW)
      const_decl_stmt(funtionPos,rangePos);
    else if (symId == LET_KW)
      let_decl_stmt(funtionPos,rangePos);
    else if (symId == IF_KW)
      if_stmt(funtionPos);
    else if (symId == WHILE_KW)
      while_stmt(funtionPos);
    else if (symId == BREAK_KW)
      error(99,token);
    else if (symId == CONTINUE_KW)
      error(99,token);
    else if (symId == RETURN_KW)
      return_stmt(funtionPos);
    else if (symId == SEMICOLON)
      getsym();
    else if (symId == L_BRACE)
      block_stmt(funtionPos, rangePos);
    else if (isExpr())
    {
      int retType = 0;
      expr(funtionPos, &retType);
      if (retType != 3)
      {
        //popn
        F_instruction(funtionPos,0x03);
        pushIns(1, Fmap[funtionPos].instructions);
        
      }
      check(SEMICOLON);
      getsym();
    }
    else
    {
      break;
    }
    // 恢复域级
    rangePos = savePos;
  }
  check(R_BRACE); //stmt尾已读
  getsym();
}
/* 'const' IDENT ':' ty '=' expr ';' */
void const_decl_stmt(int funtionPos, int rangePos_)
{
  rangePos = rangePos_;
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
  else error(99, token);

  Global tempVar(preToken, token, true);
  int varPos = 0;
  //查重后放入符号表
  checkDefine(rangePos,tempVar.name);
  if(funtionPos == 0)
    varPos = Lmap[0].vars.size();
  else 
    varPos = Fmap[funtionPos].localSlotNum++;
  Lmap[rangePos].vars.push_back(tempVar);

  getsym(); // =
  check(ASSIGN);

  if (funtionPos == 0) //global
    F_instruction(funtionPos,0x0c);
  else //loca
    F_instruction(funtionPos,0x0a);
  pushIns(varPos, Fmap[funtionPos].instructions);

  getsym();
  expr(funtionPos,&retType); // ;
  F_instruction(funtionPos,0x17); //store64
  
  check(SEMICOLON);
  getsym();
}
/* 'let' IDENT ':' ty ('=' expr)? ';' */
void let_decl_stmt(int funtionPos, int rangePos_)
{
  rangePos = rangePos_;
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
  checkDefine(rangePos,tempVar.name);
  if(funtionPos == 0)
    varPos = Lmap[0].vars.size();
  else 
    varPos = Fmap[funtionPos].localSlotNum++;
  tempVar.funSlot = varPos;
  Lmap[rangePos].vars.push_back(tempVar);

  getsym(); // = | ;
  if (symId == ASSIGN)
  {
    if (funtionPos == 0)
      F_instruction(funtionPos,0x0c);
    else
      F_instruction(funtionPos,0x0a);
    pushIns(varPos, Fmap[funtionPos].instructions);

    getsym();
    expr(funtionPos, &retType); // ;
    F_instruction(funtionPos,0x17); //store64
    
  }
  check(SEMICOLON);
  getsym();
}
//if_stmt -> 'if' expr block_stmt ('else' block_stmt|if_stmt)?
void if_stmt(int funtionPos)
{
  int retType = 0;
  getsym();
  expr(funtionPos, &retType); // {
  //brtrue(1)
  F_instruction(funtionPos,0x43);
  pushIns(1, Fmap[funtionPos].instructions);
  //br(0)0等待替换;
  F_instruction(funtionPos,0x41);
  int waitPos = Fmap[funtionPos].instructions.size();
  pushIns(0, Fmap[funtionPos].instructions);
  int ifLowNum = Fmap[funtionPos].insNum;

  block_stmt(funtionPos, rangePos);
  int ifHighNum = Fmap[funtionPos].insNum;

  //预读else
  if (symId == ELSE_KW)
  {
    //br(0)0等待替换;
    F_instruction(funtionPos,0x41);
    int waitPos = Fmap[funtionPos].instructions.size();
    pushIns(0, Fmap[funtionPos].instructions);
    int elseLowNum = Fmap[funtionPos].insNum;
    ifHighNum++;

    getsym();
    if (symId == IF_KW)
    {
      if_stmt(funtionPos);
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
  F_instruction(funtionPos,0x41);
  pushIns(0, Fmap[funtionPos].instructions);
}
/* 'while' expr block_stmt */
void while_stmt(int funtionPos)
{
  int retType = 0;
  //br(0)
  F_instruction(funtionPos,0x41);
  int whilePos = Fmap[funtionPos].instructions.size();
  pushIns(0, Fmap[funtionPos].instructions);
  int whileNum = Fmap[funtionPos].insNum;
  getsym();
  expr(funtionPos, &retType); // {
                                        //brtrue(1)
  F_instruction(funtionPos,0x43);
  pushIns(1, Fmap[funtionPos].instructions);
  //br(0)0等待替换，在符号表中添加continueNum;
  F_instruction(funtionPos,0x41);
  int waitPos = Fmap[funtionPos].instructions.size();
  pushIns(0, Fmap[funtionPos].instructions);
  int tempNum = Fmap[funtionPos].insNum;

  block_stmt(funtionPos, rangePos);
  //添加循环br(-?)
  F_instruction(funtionPos,0x41);
  pushIns(whileNum - Fmap[funtionPos].insNum, Fmap[funtionPos].instructions);
  //修改开头等待替换的0
  unsigned char str[5];
  memset(str, 0, sizeof(str));
  intToFourBits(Fmap[funtionPos].insNum - tempNum, str); //TODO:check
  for (int i = 0; i < 4; i++)
  {
    Fmap[funtionPos].instructions[waitPos + i] = str[4 - i];
  }
}
/* 'return' expr? ';' */
void return_stmt(int funtionPos)
{
  getsym();
  if (symId != SEMICOLON)
  {
    //arga(0)
    F_instruction(funtionPos,0x0b);
    pushIns(0, Fmap[funtionPos].instructions);
    
    //返回值
    int retType;
    if (Fmap[funtionPos].retType == "int")
      retType = 1;
    else if (Fmap[funtionPos].retType == "double")
      retType = 2;
    else
      error(99, token);
    expr(funtionPos, &retType);
    //store64
    F_instruction(funtionPos,0x17);
    
  }
  else
  {
    if (Fmap[funtionPos].retType != "void")
      error(99, token);
  }
  //ret
  F_instruction(funtionPos,0x49);
  
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
void HighExpr(int funtionPos, int *retType);
void MediumExpr(int funtionPos, int *retType);
void LowExpr(int funtionPos, int *retType);
void CallParamList(int funtionPos, int callFuntionPos);
void expr(int funtionPos, int *retType)
{
  HighExpr(funtionPos, retType);
  while (true)
  {
    if (symId == LT)
    {
      getsym();
      HighExpr(funtionPos, retType);
      if (*retType == 1)
        F_instruction(funtionPos,0x30);
      else
        F_instruction(funtionPos,0x32);
      F_instruction(funtionPos,0x39);
      *retType = 4;
    }
    else if (symId == LE)
    {
      getsym();
      HighExpr(funtionPos, retType);
      if (*retType == 1)
        F_instruction(funtionPos,0x30);
      else
        F_instruction(funtionPos,0x32);
      F_instruction(funtionPos,0x3a);
      F_instruction(funtionPos,0x2e);
      *retType = 4;
    }
    else if (symId == GT)
    {
      getsym();
      HighExpr(funtionPos, retType);
      if (*retType == 1)
        F_instruction(funtionPos,0x30);
      else
        F_instruction(funtionPos,0x32);
      F_instruction(funtionPos,0x3a);
      
      *retType = 4;
    }
    else if (symId == GE)
    {
      getsym();
      HighExpr(funtionPos, retType);
      if (*retType == 1)
        F_instruction(funtionPos,0x30);
      else
        F_instruction(funtionPos,0x32);
      F_instruction(funtionPos,0x39);
      F_instruction(funtionPos,0x2e);
      
      *retType = 4;
    }
    else if (symId == EQ)
    {
      getsym();
      HighExpr(funtionPos, retType);
      if (*retType == 1)
        F_instruction(funtionPos,0x30);
      else
        F_instruction(funtionPos,0x32);
      F_instruction(funtionPos,0x2e);
      
      *retType = 4;
    }
    else if (symId == NEQ)
    {
      getsym();
      HighExpr(funtionPos, retType);
      if (*retType == 1)
        F_instruction(funtionPos,0x30);
      else
        F_instruction(funtionPos,0x32);  
      *retType = 4;
    }
    else
      break;
  }
}

void HighExpr(int funtionPos, int *retType)
{
  MediumExpr(funtionPos,  retType);
  while (true)
  {
    if (symId == PLUS)
    {
      getsym();
      MediumExpr(funtionPos, retType);
      if (*retType == 1)
        F_instruction(funtionPos,0x20);
      else if (*retType == 2)
        F_instruction(funtionPos,0x24);
      else
        error(99, token);
    }
    else if (symId == MINUS)
    {
      getsym();
      MediumExpr(funtionPos, retType);
      if (*retType == 1)
        F_instruction(funtionPos,0x21);
      else if (*retType == 2)
        F_instruction(funtionPos,0x25);
      else
        error(99, token);
    }
    else
      break;
  }
}

void MediumExpr(int funtionPos, int *retType)
{
  LowExpr(funtionPos, retType);
  while (true)
  {
    if (symId == MUL)
    {
      getsym();
      LowExpr(funtionPos, retType);
      if (*retType == 1)
        F_instruction(funtionPos,0x22);
      else if (*retType == 2)
        F_instruction(funtionPos,0x26);
    }
    else if (symId == DIV)
    {
      getsym();
      LowExpr(funtionPos, retType);
      if (*retType == 1)
        F_instruction(funtionPos,0x23);
      else if (*retType == 2)
        F_instruction(funtionPos,0x27);
    }
    else
      break;
  }
}

void LowExpr(int funtionPos, int *retType)
{
  int tempRangePos = rangePos ,i = -1, flag = 0;
  if (symId == IDENT)
  {
    string preToken = token;
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
        F_instruction(funtionPos,0x1a);
        pushIns(0, Fmap[funtionPos].instructions);
        
        //返回调用者想要查看的返回值类型
        if (*retType == 0)
          *retType = 3;
      }
      else if (Fmap[callFuntionPos].retType == "int")
      {
        //找到的函数返回值不是要求的返回值
        if (*retType != 0 && *retType != 1)
          error(99, token);
        //压入
        //stackalloc(1)
        F_instruction(funtionPos,0x1a);
        pushIns(1, Fmap[funtionPos].instructions);
        
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
        F_instruction(funtionPos,0x1a);
        pushIns(1, Fmap[funtionPos].instructions);
        
        //返回调用者想要查看的返回值类型
        if (*retType == 0)
          *retType = 2;
      }

      getsym(); // call_param_list | ')'
      if (symId == R_PAREN)
      {           // 空列表
        F_instruction(funtionPos,0x48);
        pushIns(callFuntionPos, Fmap[funtionPos].instructions);
        getsym(); // read next
      }
      else
      {
        CallParamList(funtionPos, callFuntionPos);
        F_instruction(funtionPos,0x48);
        pushIns(callFuntionPos, Fmap[funtionPos].instructions);
        check(R_PAREN);
        getsym(); // read next
      }
    }

    // 赋值语句 l_expr '=' expr
    else if (symId == ASSIGN)
    {
      flag = 1;
      //调用者想要的返回值不是void
      if (*retType != 0 && *retType != 3)
        error(99, token);
      //查找变量
      int varType = 0;
      bool local = false, param = false, global = false;
      //向上域进行查找
      while (Lmap[tempRangePos].upRange != -1)
      {
        i = findVar(tempRangePos,preToken,&varType,ASSIGN_);
        if(i!=-1){
          int slot = Lmap[tempRangePos].vars[i].funSlot;
          local = true;
          F_instruction(funtionPos,0x0a);
          pushIns(slot, Fmap[funtionPos].instructions);
          break;
        }
        tempRangePos = Lmap[tempRangePos].upRange;
      }
      //函数的参数
      if (!local)
      {
        i = findParam(funtionPos,preToken,&varType,ASSIGN_);
        if(i!=-1){
          param = true;
          F_instruction(funtionPos,0x0b);
          if (Fmap[funtionPos].retType == "void")
            pushIns(i, Fmap[funtionPos].instructions);
          else
            pushIns(i + 1, Fmap[funtionPos].instructions);
        }
      }
      //全局变量
      if (!local && !param)
      {
        i = findVar(0,preToken,&varType,ASSIGN_);
        if(i!=-1){
          global = true;
          F_instruction(funtionPos,0x0c);
          pushIns(i, Fmap[funtionPos].instructions);
        }
        else error(99, token);
      }
      
      getsym(); //expr
      expr(funtionPos, &varType);
      if (*retType == 0)
        *retType = 3;
      F_instruction(funtionPos,0x17); //store64
      
    }
    // 变量调用 IDENT 注：此时以读了下一个token
    else
    {
      flag = 1;
      //查找变量
      bool local = false, param = false, global = false;
      //向上域进行查找
      while (Lmap[tempRangePos].upRange != -1)
      { //达到0层直接跳出
        i = findVar(tempRangePos,preToken,retType,LOAD);
        if(i != -1){
          int slot = Lmap[tempRangePos].vars[i].funSlot;
          // printf("\nFIND i = %d; Slot = %d",i,slot);
          local = true;
          F_instruction(funtionPos,0x0a);
          pushIns(slot, Fmap[funtionPos].instructions);
          break;
        }
        tempRangePos = Lmap[tempRangePos].upRange;
      }
      //函数的参数
      if (!local)
      {
        i = findParam(funtionPos,preToken,retType,LOAD);
        if(i!=-1){
          param = true;
          F_instruction(funtionPos,0x0b);
          if (Fmap[funtionPos].retType == "void")
            pushIns(i, Fmap[funtionPos].instructions);
          else
            pushIns(i + 1, Fmap[funtionPos].instructions);
        }
      }
      //全局变量
      if (!local && !param)
      {
        i = findVar(0,preToken,retType,LOAD);
        if(i!=-1){
          global = true;
          F_instruction(funtionPos,0x0c);
          pushIns(i, Fmap[funtionPos].instructions);
        }
        else error(99, token);
      }
      //load64
      F_instruction(funtionPos,0x13);
    }
  }
  else if (symId == UINT_LITERAL)
  {
    //要求该lowexpr的返回类型不是int
    if (*retType != 0 && *retType != 1)
      error(99, token);
    //TODO:int64_t的上限？
    int64_t temp = atoi(token);
    F_instruction(funtionPos,0x01);
    pushIns(temp, Fmap[funtionPos].instructions);
    
    if (*retType == 0)
      *retType = 1;
    getsym();
  }
  else if (symId == DOUBLE_LITERAL)
  {
    // if (*retType != 0 && *retType != 2)
      error(99, token);
    int64_t temp = doubleToRawBits(num);
    F_instruction(funtionPos,0x01);
    pushIns(temp, Fmap[funtionPos].instructions);
    
    if (*retType == 0)
      *retType = 2;
    getsym();
  }
  else if (symId == STRING_LITERAL)
  {
    //要求该lowexpr的返回类型不是int
    if (*retType != 0 && *retType != 1)
      error(99, token);
    Global temp = Global(token, "string", true);
    int64_t tempNum = Lmap[0].vars.size();
    Lmap[0].vars.push_back(temp);
    Fmap[0].instructions.push_back(0x01);
    pushIns(tempNum, Fmap[0].instructions);
    Fmap[0].insNum++;
    if (*retType == 0)
      *retType = 1;
    getsym();
  }
  else if (symId == CHAR_LITERAL)
  {
    //要求该lowexpr的返回类型不是int
    if (*retType != 0 && *retType != 1)
      error(99, token);
    int64_t tempNum = token[0];
    F_instruction(funtionPos,0x01);
    pushIns(tempNum, Fmap[funtionPos].instructions);
    
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
    expr(funtionPos, &retT);
    if (retT == 1)
    {
      F_instruction(funtionPos,0x34);
      
      if (*retType == 0)
        *retType = 1;
    }
    else if (retT == 2)
    {
      F_instruction(funtionPos,0x35);
      
      if (*retType == 0)
        *retType = 2;
    }
    else
    {
      printf("%d ", retT);
      error(99, token);
    }
  }
  else if (symId == L_PAREN)
  { // '(' expr ')'
    getsym();
    if (isExpr())
      expr(funtionPos, retType);
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
    F_instruction(funtionPos,0x50);
    
    if (*retType == 0)
      *retType = 1;
    getsym();
  }
  else if (symId == GETDOUBLE)
  {
    // if (*retType != 0 && *retType != 2)
      error(99,token);
    //'('
    getsym();
    check(L_PAREN);
    //')'
    getsym();
    check(R_PAREN);
    //scan.f
    F_instruction(funtionPos,0x52);
    
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
    F_instruction(funtionPos,0x51);
    
    if (*retType == 0)
      *retType = 1;
    getsym();
  }
  else if (symId == PUTINT)
  {
    if (*retType != 0 && *retType != 3)
      error(99,token);
    int retT = 1;
    //'('
    getsym();
    check(L_PAREN);
    //int
    getsym();
    expr(funtionPos, &retT);
    //')'
    check(R_PAREN);
    //print.i
    F_instruction(funtionPos,0x54);
    
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
    expr(funtionPos, &retT);
    //')'
    check(R_PAREN);
    //print.f
    F_instruction(funtionPos,0x56);
    
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
    expr(funtionPos, &retT);
    //')'
    check(R_PAREN);
    //print.c
    F_instruction(funtionPos,0x55);
    
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
    int64_t tempNum = Lmap[0].vars.size();
    Lmap[0].vars.push_back(temp);
    F_instruction(funtionPos,0x01);
    pushIns(tempNum, Fmap[funtionPos].instructions);
    
    //')'
    getsym();
    check(R_PAREN);
    //print.s
    F_instruction(funtionPos,0x57);
    
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
    F_instruction(funtionPos,0x58);
    
    if (*retType == 0)
      *retType = 3;
    getsym();
  }
  else
    error(99, token);

  // as 的部分不管了 
  if(symId == AS_KW){
    error(99,token);
  }
}

// expr (',' expr)* [非空才进入]
void CallParamList(int funtionPos, int callFuntionPos)
{
  int retType = 0;
  expr(funtionPos, &retType);
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
    expr(funtionPos, &retType);
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

void parse()
{
  //magic
  instructions.push_back(0x72);
  instructions.push_back(0x30);
  instructions.push_back(0x3b);
  instructions.push_back(0x3e);
  //version
  pushIns(1, instructions);
  program();
  init_start(); //_start的指令集
  //Array<GlobalDef>：全局变量常量（全设为0），字符串字面量，函数名
  //Array<GlobalDef>.count
  int globalNum = Lmap[0].vars.size() + Fmap.size();
  pushIns(globalNum, instructions);
  //Array<GlobalDef>.item 全局变量部分（包括字符串字面量和标准库函数）
  for (int i = 0; i < Lmap[0].vars.size(); i++)
  {
    if (Lmap[0].vars[i].dataType == "string")
    {
      //Array<GlobalDef>.item[i].is_const = 1
      instructions.push_back(0x01);
      int arrayNum = Lmap[0].vars[i].name.size();
      //Array<GlobalDef>.item[i].value.count
      pushIns(arrayNum, instructions);
      for (int j = 0; j < arrayNum; j++)
      {
        //Array<GlobalDef>.item[i].value.item[j]
        instructions.push_back(Lmap[0].vars[i].name[j]);
      }
    }
    else if (Lmap[0].vars[i].dataType == "int" || Lmap[0].vars[i].dataType == "double")
    {
      //Array<GlobalDef>.item[i].is_const
      if (Lmap[0].vars[i].is_const)
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
      error(99,token);
    }
  }
  //Array<GlobalDef>.item 函数部分
  for (int i = 0; i < Fmap.size(); i++)
  {
    Fmap[i].pos = Lmap[0].vars.size() + i;
    //Array<GlobalDef>.item[Lmap[0].vars.size()+i].is_const = 1
    instructions.push_back(0x01);
    int arrayNum = Fmap[i].name.size();
    //Array<GlobalDef>.item[Lmap[0].vars.size()+i].value.count
    pushIns(arrayNum, instructions);
    for (int j = 0; j < arrayNum; j++)
    {
      //Array<GlobalDef>.item[Lmap[0].vars.size()+i].value.item[j]
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
      error(99,token);
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
}