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
using namespace std;

void parse();
void function();
void block_stmt(int upRange);
void let_stmt(int funPos, int rangePos_);
void const_stmt(int funPos, int rangePos_);
void if_stmt();
void while_stmt();
void return_stmt();
void expr(int *retType);
void HighExpr(int *retType);
void MediumExpr(int *retType);
void LowExpr(int *retType);
void CallParamList(int callFuntionPos);

int rangePos;
int funtionPos;

/*===========================================/
- - - - - Utils - - - - - 
/===========================================*/
void check(int id)
{
  if (symId != id)
  {
    printf("Excepted Id: %d, Actual Id: %d\n", id, symId);
    error(GRAMMER_ERROR, token);
  }
}
bool isExpr()
{
  if (symId==IDENT || symId==MINUS || symId==L_PAREN || symId == UINT_LITERAL || symId == DOUBLE_LITERAL 
    || symId == STRING_LITERAL || symId == CHAR_LITERAL || symId==GETINT || symId==GETDOUBLE 
    || symId==GETCHAR || symId==PUTINT || symId==PUTDOUBLE || symId==PUTCHAR || symId==PUTSTR || symId==PUTLN 
  ) return true;
  else
    return false;
}


/*===========================================/
- - - - - Functions - - - - - 
/===========================================*/
void program()
{
  while (true)
  {
    if (symId == FN_KW)
      function();
    else if (symId == LET_KW)
      let_stmt(0,0);
    else if (symId == CONST_KW)
      const_stmt(0,0);
    else break;
  }
}

void function()
{
  funtionPos = Fmap.size();
  getsym();
  check(IDENT);
  if (!strcmp(token, "main")) 
    funtionPos = 1;
  else { 
    string str = token;
    if(findFun(str) >= 0) error(DUL_FUN,token);
    Fmap.push_back(Funtion(token));
  }

  getsym();
  check(L_PAREN);

  getsym();
  /*======= function_param_list =======*/
  if (symId == CONST_KW || symId == IDENT)
  {
    vector<Global> tempParams;
    do
    {
      if (symId == COMMA)
        getsym();
      /*======= function_param =======*/
      Global tempParam;
      tempParam.is_const = false;
      if (symId == CONST_KW)
      {
        tempParam.is_const = true;
        getsym();
      }
      check(IDENT);
      tempParam.name = token;

      getsym();
      check(COLON);
      getsym();
      check(IDENT);
      if (!strcmp(token, "int") || !strcmp(token, "void"))
        tempParam.dataType = token;
      else
        error(UNMATCH_TYPE, token);
      tempParams.push_back(tempParam);
      getsym();
    } while (symId == COMMA);

    Fmap[funtionPos].params = tempParams;
    Fmap[funtionPos].paramSlotNum = Fmap[funtionPos].params.size();
  }
  //null_list
  check(R_PAREN);
  getsym();
  check(ARROW);
  getsym();
  check(IDENT);
  if (!strcmp(token, "int") || !strcmp(token, "void"))
    Fmap[funtionPos].retType = token;
  else error(UNMATCH_TYPE, token);

  getsym();
  block_stmt(0);
  Fun_instruction(funtionPos,0x49);
}


/*==========================================*
- - - - - Statements - - - - - 
*==========================================*/
void block_stmt(int upRange)
{
  rangePos = Lmap.size();
  int savePos = rangePos;
  Lmap.push_back(Local(funtionPos, upRange));
  check(L_BRACE);

  getsym();
  while (true)
  {
    if (symId == CONST_KW)
      const_stmt(funtionPos,rangePos);
    else if (symId == LET_KW)
      let_stmt(funtionPos,rangePos);
    else if (symId == IF_KW)
      if_stmt();
    else if (symId == WHILE_KW)
      while_stmt();
    else if (symId == BREAK_KW)
      error(UNDO,token);
    else if (symId == CONTINUE_KW)
      error(UNDO,token);
    else if (symId == RETURN_KW)
      return_stmt();
    else if (symId == SEMICOLON)
      getsym();
    else if (symId == L_BRACE)
      block_stmt(rangePos);
    else if (isExpr())
    {
      int retType = ASSIGN_;
      expr(&retType);
      if (retType != VOID_)
      {
        Fun_instruction(funtionPos,0x03);
        u32_instruction(1, Fmap[funtionPos].instructions);
      }
      check(SEMICOLON);
      getsym();
    }
    else break;
    rangePos = savePos;
  }
  check(R_BRACE);
  getsym();
}

void const_stmt(int funPos, int rangePos_)
{
  funtionPos = funPos;
  rangePos = rangePos_;
  getsym();
  check(IDENT);
  string preToken = token;
  getsym();
  check(COLON);
  getsym();
  check(IDENT);
  int retType = ASSIGN_;
  if (!strcmp(token, "int"))
    retType = INT_;
  else error(RETURN_INT, token);

  Global tempVar(preToken, token, true);
  int varPos = 0;
  
  checkDefine(rangePos,tempVar.name);
  if(funtionPos == 0)
    varPos = Lmap[0].vars.size();
  else 
    varPos = Fmap[funtionPos].localSlotNum++;
  Lmap[rangePos].vars.push_back(tempVar);

  getsym();
  check(ASSIGN);

  if (funtionPos == 0) 
    Fun_instruction(funtionPos,0x0c);
  else 
    Fun_instruction(funtionPos,0x0a);
  u32_instruction(varPos, Fmap[funtionPos].instructions);

  getsym();
  expr(&retType);
  Fun_instruction(funtionPos,0x17);
  
  check(SEMICOLON);
  getsym();
}

void let_stmt(int funPos, int rangePos_)
{
  funtionPos = funPos;
  rangePos = rangePos_;
  getsym();
  check(IDENT);
  string preToken = token;
  getsym();
  check(COLON);
  getsym();
  check(IDENT);
  int retType = ASSIGN_;
  if (!strcmp(token, "int"))
    retType = INT_;
  else
    error(RETURN_INT, token);
  Global tempVar(preToken, token, false);
  int varPos = 0;

  checkDefine(rangePos,tempVar.name);
  if(funtionPos == 0)
    varPos = Lmap[0].vars.size();
  else 
    varPos = Fmap[funtionPos].localSlotNum++;
  tempVar.funSlot = varPos;
  Lmap[rangePos].vars.push_back(tempVar);

  getsym();
  if (symId == ASSIGN)
  {
    if (funtionPos == 0)
      Fun_instruction(funtionPos,0x0c);
    else
      Fun_instruction(funtionPos,0x0a);
    u32_instruction(varPos, Fmap[funtionPos].instructions);

    getsym();
    expr(&retType);
    Fun_instruction(funtionPos,0x17);
    
  }
  check(SEMICOLON);
  getsym();
}

void if_stmt()
{
  int retType = ASSIGN_;
  getsym();
  expr(&retType);
  //brtrue(1)
  Fun_instruction(funtionPos,0x43);
  u32_instruction(1, Fmap[funtionPos].instructions);
  //br(0)0等待替换;
  Fun_instruction(funtionPos,0x41);
  int waitPos = Fmap[funtionPos].instructions.size();
  u32_instruction(0, Fmap[funtionPos].instructions);
  int ifLowNum = Fmap[funtionPos].insNum;

  block_stmt(rangePos);
  int ifHighNum = Fmap[funtionPos].insNum;

  if (symId == ELSE_KW)
  {
    //br(0)0等待替换;
    Fun_instruction(funtionPos,0x41);
    int waitPos = Fmap[funtionPos].instructions.size();
    u32_instruction(0, Fmap[funtionPos].instructions);
    int elseLowNum = Fmap[funtionPos].insNum;
    ifHighNum++;

    getsym();
    if (symId == IF_KW)
      if_stmt();
    else if (symId == L_BRACE)
      block_stmt(rangePos);
    else
      error(GRAMMER_ERROR, token);
    int x = Fmap[funtionPos].insNum - elseLowNum;
    Fmap[funtionPos].instructions[waitPos] = x>>24;
    Fmap[funtionPos].instructions[waitPos+1] = x>>16;
    Fmap[funtionPos].instructions[waitPos+2] = x>>8;
    Fmap[funtionPos].instructions[waitPos+3] = x;
  }
  int x = ifHighNum - ifLowNum;
  Fmap[funtionPos].instructions[waitPos] = x>>24;
  Fmap[funtionPos].instructions[waitPos+1] = x>>16;
  Fmap[funtionPos].instructions[waitPos+2] = x>>8;
  Fmap[funtionPos].instructions[waitPos+3] = x;
  //结尾添加一个br(0)
  Fun_instruction(funtionPos,0x41);
  u32_instruction(0, Fmap[funtionPos].instructions);
}

void while_stmt()
{
  int retType = ASSIGN_;
  //br(0)
  Fun_instruction(funtionPos,0x41);
  int whilePos = Fmap[funtionPos].instructions.size();
  u32_instruction(0, Fmap[funtionPos].instructions);
  int whileNum = Fmap[funtionPos].insNum;
  getsym();
  expr(&retType); // {
   //brtrue(1)
  Fun_instruction(funtionPos,0x43);
  u32_instruction(1, Fmap[funtionPos].instructions);
  //br(0)0等待替换，在符号表中添加continueNum;
  Fun_instruction(funtionPos,0x41);
  int waitPos = Fmap[funtionPos].instructions.size();
  u32_instruction(0, Fmap[funtionPos].instructions);
  int tempNum = Fmap[funtionPos].insNum;

  block_stmt(rangePos);
  //添加循环br(-?)
  Fun_instruction(funtionPos,0x41);
  u32_instruction(whileNum - Fmap[funtionPos].insNum, Fmap[funtionPos].instructions);
  //修改开头等待替换的0
  int x = Fmap[funtionPos].insNum - tempNum;
  Fmap[funtionPos].instructions[waitPos] = x>>24;
  Fmap[funtionPos].instructions[waitPos+1] = x>>16;
  Fmap[funtionPos].instructions[waitPos+2] = x>>8;
  Fmap[funtionPos].instructions[waitPos+3] = x;
}

void return_stmt()
{
  getsym();
  if (symId != SEMICOLON)
  {
    Fun_instruction(funtionPos,0x0b); //arga(0)
    u32_instruction(0, Fmap[funtionPos].instructions);
    
    int retType = INT_;
    if (Fmap[funtionPos].retType != "int")
      error(RETURN_INT, token);
    expr(&retType);
    Fun_instruction(funtionPos,0x17); 
  }
  else
  {
    if (Fmap[funtionPos].retType != "void")
      error(RETURN_VOID, token);
  }
  Fun_instruction(funtionPos,0x49);
  
  check(SEMICOLON);
  getsym();
}


/*=========================================*
- - - - - Expression - - - - - 
*==========================================*/

void expr(int *retType)
{
  HighExpr(retType);
  while (true)
  {
    if (symId == LT)
    {
      getsym();
      HighExpr(retType);
      if (*retType == INT_)
        Fun_instruction(funtionPos,0x30);
      else
        Fun_instruction(funtionPos,0x32);
      Fun_instruction(funtionPos,0x39);
      *retType = BOOL_;
    }
    else if (symId == LE)
    {
      getsym();
      HighExpr(retType);
      if (*retType == INT_)
        Fun_instruction(funtionPos,0x30);
      else
        Fun_instruction(funtionPos,0x32);
      Fun_instruction(funtionPos,0x3a);
      Fun_instruction(funtionPos,0x2e);
      *retType = BOOL_;
    }
    else if (symId == GT)
    {
      getsym();
      HighExpr(retType);
      if (*retType == INT_)
        Fun_instruction(funtionPos,0x30);
      else
        Fun_instruction(funtionPos,0x32);
      Fun_instruction(funtionPos,0x3a);
      
      *retType = BOOL_;
    }
    else if (symId == GE)
    {
      getsym();
      HighExpr(retType);
      if (*retType == INT_)
        Fun_instruction(funtionPos,0x30);
      else
        Fun_instruction(funtionPos,0x32);
      Fun_instruction(funtionPos,0x39);
      Fun_instruction(funtionPos,0x2e);
      
      *retType = BOOL_;
    }
    else if (symId == EQ)
    {
      getsym();
      HighExpr(retType);
      if (*retType == INT_)
        Fun_instruction(funtionPos,0x30);
      else
        Fun_instruction(funtionPos,0x32);
      Fun_instruction(funtionPos,0x2e);
      
      *retType = BOOL_;
    }
    else if (symId == NEQ)
    {
      getsym();
      HighExpr(retType);
      if (*retType == INT_)
        Fun_instruction(funtionPos,0x30);
      else
        Fun_instruction(funtionPos,0x32);  
      *retType = BOOL_;
    }
    else
      break;
  }
}

void HighExpr(int *retType)
{
  MediumExpr(retType);
  while (true)
  {
    if (symId == PLUS)
    {
      getsym();
      MediumExpr(retType);
      if (*retType == INT_)
        Fun_instruction(funtionPos,0x20);
      else
        error(RETURN_INT, token);
    }
    else if (symId == MINUS)
    {
      getsym();
      MediumExpr(retType);
      if (*retType == INT_)
        Fun_instruction(funtionPos,0x21);
      else
        error(RETURN_INT, token);
    }
    else
      break;
  }
}

void MediumExpr(int *retType)
{
  LowExpr(retType);
  while (true)
  {
    if (symId == MUL)
    {
      getsym();
      LowExpr(retType);
      if (*retType == INT_)
        Fun_instruction(funtionPos,0x22);
    }
    else if (symId == DIV)
    {
      getsym();
      LowExpr(retType);
      if (*retType == INT_)
        Fun_instruction(funtionPos,0x23);
    }
    else
      break;
  }
}

void LowExpr(int *retType)
{
  int tempRangePos = rangePos ,i = -1;
  if (symId == IDENT)
  {
    string preToken = token;
    getsym();

    /*========== Function Call =========*/
    if (symId == L_PAREN)
    {
      int callFuntionPos = findFun(preToken);
      if(callFuntionPos<0) error(UNEXITED_FUN,token);
      
      if (Fmap[callFuntionPos].retType == "void")
      {
        if (*retType != ASSIGN_ && *retType != VOID_)
          error(RETURN_VOID, token);
        Fun_instruction(funtionPos,0x1a);
        u32_instruction(0, Fmap[funtionPos].instructions);
        
        if (*retType == ASSIGN_) *retType = VOID_;
      }
      else if (Fmap[callFuntionPos].retType == "int")
      {
        if (*retType != ASSIGN_ && *retType != INT_)
          error(RETURN_INT, token);
        Fun_instruction(funtionPos,0x1a);
        u32_instruction(1, Fmap[funtionPos].instructions);
        
        if (*retType == ASSIGN_) *retType = INT_;
      }

      getsym();
      if (symId == R_PAREN)
      { // null_list
        Fun_instruction(funtionPos,0x48);
        u32_instruction(callFuntionPos, Fmap[funtionPos].instructions);
      }
      else
      {
        CallParamList(callFuntionPos);
        Fun_instruction(funtionPos,0x48);
        u32_instruction(callFuntionPos, Fmap[funtionPos].instructions);
        check(R_PAREN);
      }
      getsym();
    }

    /*========== Assign stmt =========*/
    else if (symId == ASSIGN)
    {
      if (*retType != ASSIGN_ && *retType != VOID_)
        error(RETURN_VOID, token);
      
      int varType = ASSIGN_;
      bool local = false, param = false, global = false;
      //local vars
      while (Lmap[tempRangePos].upRange != -1)
      {
        i = findVar(tempRangePos,preToken,&varType,ASSIGN_);
        if(i!=-1){
          int slot = Lmap[tempRangePos].vars[i].funSlot;
          local = true;
          Fun_instruction(funtionPos,0x0a);
          u32_instruction(slot, Fmap[funtionPos].instructions);
          break;
        }
        tempRangePos = Lmap[tempRangePos].upRange;
      }
      //fun params
      if (!local)
      {
        i = findParam(funtionPos,preToken,&varType,ASSIGN_);
        if(i!=-1){
          param = true;
          Fun_instruction(funtionPos,0x0b);
          if (Fmap[funtionPos].retType == "void")
            u32_instruction(i, Fmap[funtionPos].instructions);
          else
            u32_instruction(i + 1, Fmap[funtionPos].instructions);
        }
      }
      //global vars
      if (!local && !param)
      {
        i = findVar(0,preToken,&varType,ASSIGN_);
        if(i!=-1){
          global = true;
          Fun_instruction(funtionPos,0x0c);
          u32_instruction(i, Fmap[funtionPos].instructions);
        }
        else error(UNEXITED_VAR, token);
      }
      
      getsym();
      expr(&varType);
      if (*retType == ASSIGN_) *retType = VOID_;
      Fun_instruction(funtionPos,0x17);
      
    }
    /*========== Ident =========*/
    else
    {
      bool local = false, param = false, global = false;

      while (Lmap[tempRangePos].upRange != -1)
      {
        i = findVar(tempRangePos,preToken,retType,LOAD_);
        if(i != -1){
          int slot = Lmap[tempRangePos].vars[i].funSlot;
          local = true;
          Fun_instruction(funtionPos,0x0a);
          u32_instruction(slot, Fmap[funtionPos].instructions);
          break;
        }
        tempRangePos = Lmap[tempRangePos].upRange;
      }
      if (!local)
      {
        i = findParam(funtionPos,preToken,retType,LOAD_);
        if(i!=-1){
          param = true;
          Fun_instruction(funtionPos,0x0b);
          if (Fmap[funtionPos].retType == "void")
            u32_instruction(i, Fmap[funtionPos].instructions);
          else
            u32_instruction(i + 1, Fmap[funtionPos].instructions);
        }
      }
      if (!local && !param)
      {
        i = findVar(0,preToken,retType,LOAD_);
        if(i!=-1){
          global = true;
          Fun_instruction(funtionPos,0x0c);
          u32_instruction(i, Fmap[funtionPos].instructions);
        }
        else error(UNEXITED_VAR, token);
      }
      Fun_instruction(funtionPos,0x13);
    }
  }
  else if (symId == UINT_LITERAL)
  {
    if (*retType != ASSIGN_ && *retType != INT_)
      error(RETURN_INT, token);
    //TODO
    int64_t temp = atoi(token);
    Fun_instruction(funtionPos,0x01);
    u64_instruction(temp, Fmap[funtionPos].instructions);
    
    if (*retType == ASSIGN_) *retType = INT_;
    getsym();
  }
  else if (symId == DOUBLE_LITERAL)
  {
    error(UNDO, token);
  }
  else if (symId == STRING_LITERAL)
  {
    if (*retType != ASSIGN_ && *retType != INT_)
      error(RETURN_INT, token);
    Global temp = Global(token, "string", true);
    int64_t tempNum = Lmap[0].vars.size();
    Lmap[0].vars.push_back(temp);
    Fmap[0].instructions.push_back(0x01);
    u64_instruction(tempNum, Fmap[0].instructions);
    Fmap[0].insNum++;
    if (*retType == ASSIGN_)
      *retType = INT_;
    getsym();
  }
  else if (symId == CHAR_LITERAL)
  {
    if (*retType != ASSIGN_ && *retType != INT_)
      error(RETURN_INT, token);
    int64_t tempNum = token[0];
    Fun_instruction(funtionPos,0x01);
    u64_instruction(tempNum, Fmap[funtionPos].instructions);
    
    if (*retType == ASSIGN_)
      *retType = INT_;
    getsym();
  }
  else if (symId == MINUS)
  {
    if (*retType != ASSIGN_ && *retType != INT_ )
      error(RETURN_INT, token);
    getsym();
    int retT = ASSIGN_;
    expr(&retT);
    if (retT == INT_)
    {
      Fun_instruction(funtionPos,0x34);
      if (*retType == ASSIGN_) *retType = INT_;
    }
    else error(RETURN_INT, token);
  }
  else if (symId == L_PAREN)
  {
    getsym();
    if (isExpr())
      expr(retType);
    else
      error(EXPR_ERROR, token);
    check(R_PAREN);
    getsym();
  }
  else if (symId == GETINT)
  {
    if (*retType != ASSIGN_ && *retType != INT_)
      error(RETURN_INT,token);
      
    getsym();
    check(L_PAREN);
    
    getsym();
    check(R_PAREN);
    Fun_instruction(funtionPos,0x50);
    
    if (*retType == ASSIGN_) *retType = INT_;
    getsym();
  }
  else if (symId == GETDOUBLE)
  {
    error(UNDO,token);
  }
  else if (symId == GETCHAR)
  {
    if (*retType != ASSIGN_ && *retType != INT_)
      error(RETURN_INT,token);
    
    getsym();
    check(L_PAREN);
    
    getsym();
    check(R_PAREN);
    Fun_instruction(funtionPos,0x51);
    
    if (*retType == ASSIGN_) *retType = INT_;
    getsym();
  }
  else if (symId == PUTINT)
  {
    if (*retType != ASSIGN_ && *retType != VOID_)
      error(RETURN_VOID,token);
    int retT = INT_;
    
    getsym();
    check(L_PAREN);
    
    getsym();
    expr(&retT);
    
    check(R_PAREN);
    Fun_instruction(funtionPos,0x54);
    
    if (*retType == ASSIGN_) *retType = VOID_;
    getsym();
  }
  else if (symId == PUTDOUBLE)
  {
    error(UNDO,token);
  }
  else if (symId == PUTCHAR)
  {
    if (*retType != ASSIGN_ && *retType != VOID_)
      error(RETURN_VOID,token);
    int retT = INT_;
    
    getsym();
    check(L_PAREN);
    
    getsym();
    expr(&retT);
    
    check(R_PAREN);
    Fun_instruction(funtionPos,0x55);
    
    if (*retType == ASSIGN_) *retType = VOID_;
    getsym();
  }
  else if (symId == PUTSTR)
  {
    if (*retType != ASSIGN_ && *retType != VOID_)
      error(RETURN_VOID,token);
    int retT = INT_;
    getsym();
    check(L_PAREN);

    getsym();
    if (symId != STRING_LITERAL)
      error(STR_ERROR,token);
    Global temp = Global(token, "string", true);
    int64_t tempNum = Lmap[0].vars.size();
    Lmap[0].vars.push_back(temp);
    Fun_instruction(funtionPos,0x01);
    u64_instruction(tempNum, Fmap[funtionPos].instructions);
    
    getsym();
    check(R_PAREN);
    Fun_instruction(funtionPos,0x57);
    
    if (*retType == ASSIGN_) *retType = VOID_;
    getsym();
  }
  else if (symId == PUTLN)
  {
    if (*retType != ASSIGN_ && *retType != VOID_)
      error(RETURN_VOID,token);
    getsym();
    check(L_PAREN);
    getsym();
    check(R_PAREN);
    Fun_instruction(funtionPos,0x58);
    
    if (*retType == ASSIGN_) *retType = VOID_;
    getsym();
  }
  else
    error(UNMATCH_IDENT, token);

  if(symId == AS_KW){
    error(UNDO,token);
  }
}

void CallParamList(int callFuntionPos)
{
  int retType = ASSIGN_;
  expr(&retType);
  if (retType == INT_)
    if (Fmap[callFuntionPos].params[0].dataType != "int")
      error(RETURN_INT, token);
  else if (retType == VOID_)
    if (Fmap[callFuntionPos].params[0].dataType != "void")
      error(RETURN_VOID, token);
  else
    error(UNMATCH_TYPE, token);
  
  int size = Fmap[callFuntionPos].params.size();
  for (int i = 1; i < size; ++i)
  {
    check(COMMA);
    getsym();
    retType = ASSIGN_;
    expr(&retType);
    if (retType == INT_)
      if (Fmap[callFuntionPos].params[i].dataType != "int")
        error(RETURN_INT, token);
    else if (retType == VOID_)
      if (Fmap[callFuntionPos].params[i].dataType != "void")
        error(RETURN_VOID, token);
    else
      error(UNMATCH_TYPE, token);
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
  u32_instruction(1, instructions);
  //初始化默认函数
  Fmap.push_back(Funtion("_start"));
  Fmap[0].retType = "void";
  Fmap.push_back(Funtion("main"));
  //初始化全局变量
  Lmap.push_back(Local(0, -1)); //todo
  
  program();
  init_start(); //_start的指令集
  //Array<GlobalDef>：全局变量常量（全设为0），字符串字面量，函数名
  //Array<GlobalDef>.count
  int globalNum = Lmap[0].vars.size() + Fmap.size();
  u32_instruction(globalNum, instructions);
  //Array<GlobalDef>.item 全局变量部分（包括字符串字面量和标准库函数）
  for (int i = 0; i < Lmap[0].vars.size(); i++)
  {
    if (Lmap[0].vars[i].dataType == "string")
    {
      //Array<GlobalDef>.item[i].is_const = 1
      instructions.push_back(0x01);
      int arrayNum = Lmap[0].vars[i].name.size();
      //Array<GlobalDef>.item[i].value.count
      u32_instruction(arrayNum, instructions);
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
      u32_instruction(8, instructions);
      for (int j = 0; j < 8; j++)
      {
        //Array<GlobalDef>.item[i].value.item[j]
        instructions.push_back(0x00);
      }
    }
    else{
      error(UNMATCH_TYPE,token);
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
    u32_instruction(arrayNum, instructions);
    for (int j = 0; j < arrayNum; j++)
      instructions.push_back(Fmap[i].name[j]);
  }

  //Array<FunctionDef>.count
  int funtionNum = Fmap.size();
  u32_instruction(funtionNum, instructions);
  for (int i = 0; i < funtionNum; i++)
  {
    //Array<FunctionDef>.item[i].name
    int name = Fmap[i].pos;
    u32_instruction(name, instructions);
    //Array<FunctionDef>.item[i].return_slots
    if (Fmap[i].retType == "int")
      u32_instruction(1, instructions);
    else if (Fmap[i].retType == "void")
      u32_instruction(0, instructions);
    else error(UNMATCH_TYPE,token);

    //Array<FunctionDef>.item[i].param_slots
    u32_instruction(Fmap[i].paramSlotNum, instructions);
    //Array<FunctionDef>.item[i].loc_slots
    u32_instruction(Fmap[i].localSlotNum, instructions);
    //Array<FunctionDef>.item[i].Array<Instruction>.count
    u32_instruction(Fmap[i].insNum, instructions);
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