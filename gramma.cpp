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
  funtionPos = funcList.size();
  getsym();
  check(IDENT);
  if (!strcmp(token, "main")) 
    funtionPos = 1;
  else { 
    string str = token;
    if(findFun(str) >= 0) error(DUL_FUN,token);
    funcList.push_back(Funtion(token));
  }

  getsym();
  check(L_PAREN);

  getsym();
  /*======= function_param_list =======*/
  if (symId == CONST_KW || symId == IDENT)
  {
    vector<Var> tempParams;
    do
    {
      if (symId == COMMA) getsym();
      /*======= function_param =======*/
      Var tempParam;
      tempParam.is_const = false;
      if (symId == CONST_KW){
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

    funcList[funtionPos].params = tempParams;
    funcList[funtionPos].paramSlotNum = funcList[funtionPos].params.size();
  }

  check(R_PAREN);
  getsym();
  check(ARROW);
  getsym();
  check(IDENT);
  if (!strcmp(token, "int") || !strcmp(token, "void"))
    funcList[funtionPos].retType = token;
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
  rangePos = rangeList.size();
  int savePos = rangePos;
  rangeList.push_back(Range(funtionPos, upRange));
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
        u32_instruction(1, funcList[funtionPos].instructions);
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
  if (!strcmp(token, "int")) retType = INT_;
  else error(RETURN_INT, token);

  Var tempVar(preToken, token, true);
  int varPos = 0;
  
  checkDefine(rangePos,tempVar.name);
  if(funtionPos == 0)
    varPos = rangeList[0].vars.size();
  else 
    varPos = funcList[funtionPos].localSlotNum++;
  rangeList[rangePos].vars.push_back(tempVar);

  getsym();
  check(ASSIGN);

  if (funtionPos == 0) Fun_instruction(funtionPos,0x0c);
  else  Fun_instruction(funtionPos,0x0a);
  u32_instruction(varPos, funcList[funtionPos].instructions);

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
  Var tempVar(preToken, token, false);
  int varPos = 0;

  checkDefine(rangePos,tempVar.name);
  if(funtionPos == 0)
    varPos = rangeList[0].vars.size();
  else 
    varPos = funcList[funtionPos].localSlotNum++;
  tempVar.funSlot = varPos;
  rangeList[rangePos].vars.push_back(tempVar);

  getsym();
  if (symId == ASSIGN)
  {
    if (funtionPos == 0)
      Fun_instruction(funtionPos,0x0c);
    else
      Fun_instruction(funtionPos,0x0a);
    u32_instruction(varPos, funcList[funtionPos].instructions);

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
  Fun_instruction(funtionPos,0x43);
  u32_instruction(1, funcList[funtionPos].instructions);
  Fun_instruction(funtionPos,0x41);
  int waitPos = funcList[funtionPos].instructions.size();
  u32_instruction(0, funcList[funtionPos].instructions);
  int ifLowNum = funcList[funtionPos].insNum;

  block_stmt(rangePos);
  int ifHighNum = funcList[funtionPos].insNum;

  if (symId == ELSE_KW)
  {
    Fun_instruction(funtionPos,0x41);
    int waitPos = funcList[funtionPos].instructions.size();
    u32_instruction(0, funcList[funtionPos].instructions);
    int elseLowNum = funcList[funtionPos].insNum;
    ifHighNum++;

    getsym();
    if (symId == IF_KW) if_stmt();
    else if (symId == L_BRACE) block_stmt(rangePos);
    else error(GRAMMER_ERROR, token);

    int x = funcList[funtionPos].insNum - elseLowNum;
    funcList[funtionPos].instructions[waitPos] = x>>24;
    funcList[funtionPos].instructions[waitPos+1] = x>>16;
    funcList[funtionPos].instructions[waitPos+2] = x>>8;
    funcList[funtionPos].instructions[waitPos+3] = x;
  }
  int x = ifHighNum - ifLowNum;
  funcList[funtionPos].instructions[waitPos] = x>>24;
  funcList[funtionPos].instructions[waitPos+1] = x>>16;
  funcList[funtionPos].instructions[waitPos+2] = x>>8;
  funcList[funtionPos].instructions[waitPos+3] = x;

  Fun_instruction(funtionPos,0x41);
  u32_instruction(0, funcList[funtionPos].instructions);
}

void while_stmt()
{
  int retType = ASSIGN_;
  Fun_instruction(funtionPos,0x41);
  int whilePos = funcList[funtionPos].instructions.size();
  u32_instruction(0, funcList[funtionPos].instructions);
  int whileNum = funcList[funtionPos].insNum;
  getsym();
  expr(&retType);
    
  Fun_instruction(funtionPos,0x43);
  u32_instruction(1, funcList[funtionPos].instructions);
  
  Fun_instruction(funtionPos,0x41);
  int waitPos = funcList[funtionPos].instructions.size();
  u32_instruction(0, funcList[funtionPos].instructions);
  int tempNum = funcList[funtionPos].insNum;

  block_stmt(rangePos);
  
  Fun_instruction(funtionPos,0x41);
  u32_instruction(whileNum - funcList[funtionPos].insNum, funcList[funtionPos].instructions);
  
  int x = funcList[funtionPos].insNum - tempNum;
  funcList[funtionPos].instructions[waitPos] = x>>24;
  funcList[funtionPos].instructions[waitPos+1] = x>>16;
  funcList[funtionPos].instructions[waitPos+2] = x>>8;
  funcList[funtionPos].instructions[waitPos+3] = x;
}

void return_stmt()
{
  getsym();
  if (symId != SEMICOLON){
    Fun_instruction(funtionPos,0x0b);
    u32_instruction(0, funcList[funtionPos].instructions);
    
    int retType = INT_;
    if (funcList[funtionPos].retType != "int")
      error(RETURN_INT, token);
    expr(&retType);
    Fun_instruction(funtionPos,0x17); 
  }
  else{
    if (funcList[funtionPos].retType != "void")
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
      if (*retType == INT_) Fun_instruction(funtionPos,0x30);
      else Fun_instruction(funtionPos,0x32);
      Fun_instruction(funtionPos,0x39);
      *retType = BOOL_;
    }
    else if (symId == LE)
    {
      getsym();
      HighExpr(retType);
      if (*retType == INT_) Fun_instruction(funtionPos,0x30);
      else Fun_instruction(funtionPos,0x32);
      Fun_instruction(funtionPos,0x3a);
      Fun_instruction(funtionPos,0x2e);
      *retType = BOOL_;
    }
    else if (symId == GT)
    {
      getsym();
      HighExpr(retType);
      if (*retType == INT_) Fun_instruction(funtionPos,0x30);
      else Fun_instruction(funtionPos,0x32);
      Fun_instruction(funtionPos,0x3a);
      *retType = BOOL_;
    }
    else if (symId == GE)
    {
      getsym();
      HighExpr(retType);
      if (*retType == INT_) Fun_instruction(funtionPos,0x30);
      else Fun_instruction(funtionPos,0x32);
      Fun_instruction(funtionPos,0x39);
      Fun_instruction(funtionPos,0x2e);
      *retType = BOOL_;
    }
    else if (symId == EQ)
    {
      getsym();
      HighExpr(retType);
      if (*retType == INT_) Fun_instruction(funtionPos,0x30);
      else Fun_instruction(funtionPos,0x32);
      Fun_instruction(funtionPos,0x2e);
      *retType = BOOL_;
    }
    else if (symId == NEQ){
      getsym();
      HighExpr(retType);
      if (*retType == INT_) Fun_instruction(funtionPos,0x30);
      else Fun_instruction(funtionPos,0x32);  
      *retType = BOOL_;
    }
    else break;
  }
}

void HighExpr(int *retType)
{
  MediumExpr(retType);
  while (true)
  {
    if (symId == PLUS){
      getsym();
      MediumExpr(retType);
      if (*retType == INT_)
        Fun_instruction(funtionPos,0x20);
      else
        error(RETURN_INT, token);
    }
    else if (symId == MINUS){
      getsym();
      MediumExpr(retType);
      if (*retType == INT_)
        Fun_instruction(funtionPos,0x21);
      else
        error(RETURN_INT, token);
    }
    else break;
  }
}

void MediumExpr(int *retType)
{
  LowExpr(retType);
  while (true)
  {
    if (symId == MUL) {
      getsym();
      LowExpr(retType);
      if (*retType == INT_)
        Fun_instruction(funtionPos,0x22);
    }
    else if (symId == DIV) {
      getsym();
      LowExpr(retType);
      if (*retType == INT_)
        Fun_instruction(funtionPos,0x23);
    }
    else break;
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
      
      if (funcList[callFuntionPos].retType == "void")
      {
        if (*retType != ASSIGN_ && *retType != VOID_)
          error(RETURN_VOID, token);
        Fun_instruction(funtionPos,0x1a);
        u32_instruction(0, funcList[funtionPos].instructions);
        
        if (*retType == ASSIGN_) *retType = VOID_;
      }
      else if (funcList[callFuntionPos].retType == "int")
      {
        if (*retType != ASSIGN_ && *retType != INT_)
          error(RETURN_INT, token);
        Fun_instruction(funtionPos,0x1a);
        u32_instruction(1, funcList[funtionPos].instructions);
        
        if (*retType == ASSIGN_) *retType = INT_;
      }

      getsym();
      if (symId == R_PAREN)
      {
        Fun_instruction(funtionPos,0x48);
        u32_instruction(callFuntionPos, funcList[funtionPos].instructions);
      }
      else
      {
        CallParamList(callFuntionPos);
        Fun_instruction(funtionPos,0x48);
        u32_instruction(callFuntionPos, funcList[funtionPos].instructions);
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
      bool local = false, param = false;
      //local vars
      while (rangeList[tempRangePos].upRange != -1)
      {
        i = findVar(tempRangePos,preToken,&varType,ASSIGN_);
        if(i!=-1){
          int slot = rangeList[tempRangePos].vars[i].funSlot;
          local = true;
          Fun_instruction(funtionPos,0x0a);
          u32_instruction(slot, funcList[funtionPos].instructions);
          break;
        }
        tempRangePos = rangeList[tempRangePos].upRange;
      }
      //fun params
      if (!local)
      {
        i = findParam(funtionPos,preToken,&varType,ASSIGN_);
        if(i!=-1){
          param = true;
          Fun_instruction(funtionPos,0x0b);
          if (funcList[funtionPos].retType == "void")
            u32_instruction(i, funcList[funtionPos].instructions);
          else
            u32_instruction(i + 1, funcList[funtionPos].instructions);
        }
      }
      //global vars
      if (!local && !param)
      {
        i = findVar(0,preToken,&varType,ASSIGN_);
        if(i!=-1){
          Fun_instruction(funtionPos,0x0c);
          u32_instruction(i, funcList[funtionPos].instructions);
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
      bool local = false, param = false;

      while (rangeList[tempRangePos].upRange != -1)
      {
        i = findVar(tempRangePos,preToken,retType,LOAD_);
        if(i != -1){
          int slot = rangeList[tempRangePos].vars[i].funSlot;
          local = true;
          Fun_instruction(funtionPos,0x0a);
          u32_instruction(slot, funcList[funtionPos].instructions);
          break;
        }
        tempRangePos = rangeList[tempRangePos].upRange;
      }
      if (!local)
      {
        i = findParam(funtionPos,preToken,retType,LOAD_);
        if(i!=-1){
          param = true;
          Fun_instruction(funtionPos,0x0b);
          if (funcList[funtionPos].retType == "void")
            u32_instruction(i, funcList[funtionPos].instructions);
          else
            u32_instruction(i + 1, funcList[funtionPos].instructions);
        }
      }
      if (!local && !param)
      {
        i = findVar(0,preToken,retType,LOAD_);
        if(i!=-1){
          Fun_instruction(funtionPos,0x0c);
          u32_instruction(i, funcList[funtionPos].instructions);
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
    int64_t temp = atoi(token);
    Fun_instruction(funtionPos,0x01);
    u64_instruction(temp, funcList[funtionPos].instructions);
    
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
    Var temp = Var(token, "string", true);
    int64_t tempNum = rangeList[0].vars.size();
    rangeList[0].vars.push_back(temp);
    funcList[0].instructions.push_back(0x01);
    u64_instruction(tempNum, funcList[0].instructions);
    funcList[0].insNum++;
    if (*retType == ASSIGN_) *retType = INT_;
    getsym();
  }
  else if (symId == CHAR_LITERAL)
  {
    if (*retType != ASSIGN_ && *retType != INT_)
      error(RETURN_INT, token);
    int64_t tempNum = token[0];
    Fun_instruction(funtionPos,0x01);
    u64_instruction(tempNum, funcList[funtionPos].instructions);
    
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
    Var temp = Var(token, "string", true);
    int64_t tempNum = rangeList[0].vars.size();
    rangeList[0].vars.push_back(temp);
    Fun_instruction(funtionPos,0x01);
    u64_instruction(tempNum, funcList[funtionPos].instructions);
    
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
    if (funcList[callFuntionPos].params[0].dataType != "int")
      error(RETURN_INT, token);
  else if (retType == VOID_)
    if (funcList[callFuntionPos].params[0].dataType != "void")
      error(RETURN_VOID, token);
  else
    error(UNMATCH_TYPE, token);
  
  int size = funcList[callFuntionPos].params.size();
  for (int i = 1; i < size; ++i)
  {
    check(COMMA);
    getsym();
    retType = ASSIGN_;
    expr(&retType);
    if (retType == INT_)
      if (funcList[callFuntionPos].params[i].dataType != "int")
        error(RETURN_INT, token);
    else if (retType == VOID_)
      if (funcList[callFuntionPos].params[i].dataType != "void")
        error(RETURN_VOID, token);
    else
      error(UNMATCH_TYPE, token);
  }
}

void parse()
{
  init_begin();
  program();
  init_end();
  
  int VarNum = rangeList[0].vars.size() + funcList.size();
  u32_instruction(VarNum, instructions);
  for (int i = 0; i < rangeList[0].vars.size(); i++)
  {
    if (rangeList[0].vars[i].dataType == "string")
    {
      instructions.push_back(0x01);
      int arrayNum = rangeList[0].vars[i].name.size();
      u32_instruction(arrayNum, instructions);
      for (int j = 0; j < arrayNum; j++)
        instructions.push_back(rangeList[0].vars[i].name[j]);
    }
    else if (rangeList[0].vars[i].dataType == "int" || rangeList[0].vars[i].dataType == "double")
    {
      if (rangeList[0].vars[i].is_const)
        instructions.push_back(0x01);
      else
        instructions.push_back(0x00);
      u32_instruction(8, instructions);
      for (int j = 0; j < 8; j++)
        instructions.push_back(0x00);
    }
    else{
      error(UNMATCH_TYPE,token);
    }
  }
  
  for (int i = 0; i < funcList.size(); i++)
  {
    funcList[i].pos = rangeList[0].vars.size() + i;
    instructions.push_back(0x01);
    int arrayNum = funcList[i].name.size();
    u32_instruction(arrayNum, instructions);
    for (int j = 0; j < arrayNum; j++)
      instructions.push_back(funcList[i].name[j]);
  }

  int funtionNum = funcList.size();
  u32_instruction(funtionNum, instructions);
  for (int i = 0; i < funtionNum; i++)
  {
    int name = funcList[i].pos;
    u32_instruction(name, instructions);
    if (funcList[i].retType == "int")
      u32_instruction(1, instructions);
    else if (funcList[i].retType == "void")
      u32_instruction(0, instructions);
    else error(UNMATCH_TYPE,token);

    u32_instruction(funcList[i].paramSlotNum, instructions);
    u32_instruction(funcList[i].localSlotNum, instructions);
    u32_instruction(funcList[i].insNum, instructions);
    for (int j = 0; j < funcList[i].instructions.size(); j++)
      instructions.push_back(funcList[i].instructions[j]);
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