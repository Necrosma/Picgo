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
void program();
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
void param(int callPos);

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
void parse()
{
  init_begin();
  program();
  init_end();
  
  u32_instruction(rangeList[0].vars.size() + funcList.size(), o0);
  for (int i = 0; i < rangeList[0].vars.size(); i++)
  {
    if (rangeList[0].vars[i].dataType == "string"){
      o0.push_back(0x01);
      u32_instruction(rangeList[0].vars[i].name.size(), o0); 
      for (int j = 0; j < rangeList[0].vars[i].name.size(); j++)
        o0.push_back(rangeList[0].vars[i].name[j]);
    }
    else if (rangeList[0].vars[i].dataType == "int"){
      if (rangeList[0].vars[i].is_const)
        o0.push_back(0x01);
      else o0.push_back(0x00);
      u32_instruction(8, o0);
      for (int j = 0; j < 8; j++)
        o0.push_back(0x00);
    }
    else error(UNMATCH_TYPE,token);
  }
  
  for (int i = 0; i < funcList.size(); i++)
  {
    funcList[i].pos = rangeList[0].vars.size() + i;
    o0.push_back(0x01);
    int arrayNum = funcList[i].name.size();
    u32_instruction(arrayNum, o0);
    for (int j = 0; j < arrayNum; j++)
      o0.push_back(funcList[i].name[j]);
  }

  u32_instruction(funcList.size(), o0);
  for (int i = 0; i < funcList.size(); i++)
  {
    int name = funcList[i].pos;
    u32_instruction(name, o0);
    if (funcList[i].retType == "int")
      u32_instruction(1, o0);
    else if (funcList[i].retType == "void")
      u32_instruction(0, o0);
    else error(UNMATCH_TYPE,token);

    u32_instruction(funcList[i].paramSlot, o0);
    u32_instruction(funcList[i].varSlot, o0);
    u32_instruction(funcList[i].insNum, o0);
    for (int j = 0; j < funcList[i].instruct.size(); j++)
      o0.push_back(funcList[i].instruct[j]);
  }
  
  for (int i = 0; i < o0.size();i++) out += o0[i];
  fwrite(out.c_str(), out.size(), 1, outFile);
}

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
  funcPos = funcList.size();
  getsym();
  check(IDENT);
  if (!strcmp(token, "main")) 
    funcPos = 1;
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
    vector<Var> params;
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
      params.push_back(tempParam);
      getsym();
    } while (symId == COMMA);

    funcList[funcPos].params = params;
    funcList[funcPos].paramSlot = funcList[funcPos].params.size();
  }

  check(R_PAREN);
  getsym();
  check(ARROW);
  getsym();
  check(IDENT);
  if (!strcmp(token, "int") || !strcmp(token, "void"))
    funcList[funcPos].retType = token;
  else error(UNMATCH_TYPE, token);

  getsym();
  block_stmt(0);
  Fun_instruction(funcPos,0x49);
}


/*==========================================*
- - - - - Statements - - - - - 
*==========================================*/
void block_stmt(int upRange)
{
  rangePos = rangeList.size();
  int savePos = rangePos;
  rangeList.push_back(Range(funcPos, upRange));
  check(L_BRACE);

  getsym();
  while (true)
  {
    if (symId == CONST_KW)
      const_stmt(funcPos,rangePos);
    else if (symId == LET_KW)
      let_stmt(funcPos,rangePos);
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
        Fun_instruction(funcPos,0x03);
        u32_instruction(1, funcList[funcPos].instruct);
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
  funcPos = funPos;
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
  if(funcPos == 0)
    varPos = rangeList[0].vars.size();
  else 
    varPos = funcList[funcPos].varSlot++;
  rangeList[rangePos].vars.push_back(tempVar);

  getsym();
  check(ASSIGN);

  if (funcPos == 0) Fun_instruction(funcPos,0x0c);
  else Fun_instruction(funcPos,0x0a);
  u32_instruction(varPos, funcList[funcPos].instruct);

  getsym();
  expr(&retType);
  Fun_instruction(funcPos,0x17);
  
  check(SEMICOLON);
  getsym();
}

void let_stmt(int funPos, int rangePos_)
{
  funcPos = funPos;
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
  if(funcPos == 0)
    varPos = rangeList[0].vars.size();
  else 
    varPos = funcList[funcPos].varSlot++;
  tempVar.funSlot = varPos;
  rangeList[rangePos].vars.push_back(tempVar);

  getsym();
  if (symId == ASSIGN)
  {
    if (funcPos == 0)
      Fun_instruction(funcPos,0x0c);
    else
      Fun_instruction(funcPos,0x0a);
    u32_instruction(varPos, funcList[funcPos].instruct);

    getsym();
    expr(&retType);
    Fun_instruction(funcPos,0x17);
    
  }
  check(SEMICOLON);
  getsym();
}

void if_stmt()
{
  int retType = ASSIGN_;
  getsym();
  expr(&retType);
  Fun_instruction(funcPos,0x43);
  u32_instruction(1, funcList[funcPos].instruct);
  Fun_instruction(funcPos,0x41);
  int wait = funcList[funcPos].instruct.size();
  u32_instruction(0, funcList[funcPos].instruct);
  int ifLeft = funcList[funcPos].insNum;

  block_stmt(rangePos);
  int right = funcList[funcPos].insNum;

  if (symId == ELSE_KW)
  {
    Fun_instruction(funcPos,0x41);
    int wait = funcList[funcPos].instruct.size();
    u32_instruction(0, funcList[funcPos].instruct);
    int elseLeft = funcList[funcPos].insNum;
    right++;

    getsym();
    if (symId == IF_KW) if_stmt();
    else if (symId == L_BRACE) block_stmt(rangePos);
    else error(GRAMMER_ERROR, token);

    int x = funcList[funcPos].insNum - elseLeft;
    funcList[funcPos].instruct[wait] = x>>24;
    funcList[funcPos].instruct[wait+1] = x>>16;
    funcList[funcPos].instruct[wait+2] = x>>8;
    funcList[funcPos].instruct[wait+3] = x;
  }
  int x = right - ifLeft;
  funcList[funcPos].instruct[wait] = x>>24;
  funcList[funcPos].instruct[wait+1] = x>>16;
  funcList[funcPos].instruct[wait+2] = x>>8;
  funcList[funcPos].instruct[wait+3] = x;

  Fun_instruction(funcPos,0x41);
  u32_instruction(0, funcList[funcPos].instruct);
}

void while_stmt()
{
  int retType = ASSIGN_;
  Fun_instruction(funcPos,0x41);
  int whilePos = funcList[funcPos].instruct.size();
  u32_instruction(0, funcList[funcPos].instruct);
  int whileNum = funcList[funcPos].insNum;
  getsym();
  expr(&retType);
    
  Fun_instruction(funcPos,0x43);
  u32_instruction(1, funcList[funcPos].instruct);
  
  Fun_instruction(funcPos,0x41);
  int wait = funcList[funcPos].instruct.size();
  u32_instruction(0, funcList[funcPos].instruct);
  int tmp = funcList[funcPos].insNum;

  block_stmt(rangePos);
  
  Fun_instruction(funcPos,0x41);
  u32_instruction(whileNum - funcList[funcPos].insNum, funcList[funcPos].instruct);
  
  int x = funcList[funcPos].insNum - tmp;
  funcList[funcPos].instruct[wait] = x>>24;
  funcList[funcPos].instruct[wait+1] = x>>16;
  funcList[funcPos].instruct[wait+2] = x>>8;
  funcList[funcPos].instruct[wait+3] = x;
}

void return_stmt()
{
  getsym();
  if (symId != SEMICOLON){
    Fun_instruction(funcPos,0x0b);
    u32_instruction(0, funcList[funcPos].instruct);
    
    int retType = INT_;
    if (funcList[funcPos].retType != "int")
      error(RETURN_INT, token);
    expr(&retType);
    Fun_instruction(funcPos,0x17); 
  }
  else{
    if (funcList[funcPos].retType != "void")
      error(RETURN_VOID, token);
  }
  Fun_instruction(funcPos,0x49);
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
      if (*retType == INT_) Fun_instruction(funcPos,0x30);
      else Fun_instruction(funcPos,0x32);
      Fun_instruction(funcPos,0x39);
      *retType = BOOL_;
    }
    else if (symId == LE)
    {
      getsym();
      HighExpr(retType);
      if (*retType == INT_) Fun_instruction(funcPos,0x30);
      else Fun_instruction(funcPos,0x32);
      Fun_instruction(funcPos,0x3a);
      Fun_instruction(funcPos,0x2e);
      *retType = BOOL_;
    }
    else if (symId == GT)
    {
      getsym();
      HighExpr(retType);
      if (*retType == INT_) Fun_instruction(funcPos,0x30);
      else Fun_instruction(funcPos,0x32);
      Fun_instruction(funcPos,0x3a);
      *retType = BOOL_;
    }
    else if (symId == GE)
    {
      getsym();
      HighExpr(retType);
      if (*retType == INT_) Fun_instruction(funcPos,0x30);
      else Fun_instruction(funcPos,0x32);
      Fun_instruction(funcPos,0x39);
      Fun_instruction(funcPos,0x2e);
      *retType = BOOL_;
    }
    else if (symId == EQ)
    {
      getsym();
      HighExpr(retType);
      if (*retType == INT_) Fun_instruction(funcPos,0x30);
      else Fun_instruction(funcPos,0x32);
      Fun_instruction(funcPos,0x2e);
      *retType = BOOL_;
    }
    else if (symId == NEQ){
      getsym();
      HighExpr(retType);
      if (*retType == INT_) Fun_instruction(funcPos,0x30);
      else Fun_instruction(funcPos,0x32);  
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
        Fun_instruction(funcPos,0x20);
      else
        error(RETURN_INT, token);
    }
    else if (symId == MINUS){
      getsym();
      MediumExpr(retType);
      if (*retType == INT_)
        Fun_instruction(funcPos,0x21);
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
        Fun_instruction(funcPos,0x22);
    }
    else if (symId == DIV) {
      getsym();
      LowExpr(retType);
      if (*retType == INT_)
        Fun_instruction(funcPos,0x23);
    }
    else break;
  }
}

void LowExpr(int *retType)
{
  int i = -1;
  int saveRange = rangePos;
  if (symId == IDENT)
  {
    string preToken = token;
    getsym();

    /*========== Function Call =========*/
    if (symId == L_PAREN)
    {
      int callPos = findFun(preToken);
      if(callPos<0) error(UNEXITED_FUN,token);
      
      if (funcList[callPos].retType == "void"){
        if (*retType != ASSIGN_ && *retType != VOID_)
          error(RETURN_VOID, token);
        Fun_instruction(funcPos,0x1a);
        u32_instruction(0, funcList[funcPos].instruct);
        
        if (*retType == ASSIGN_) *retType = VOID_;
      }
      else if (funcList[callPos].retType == "int"){
        if (*retType != ASSIGN_ && *retType != INT_)
          error(RETURN_INT, token);
        Fun_instruction(funcPos,0x1a);
        u32_instruction(1, funcList[funcPos].instruct);
        
        if (*retType == ASSIGN_) *retType = INT_;
      }

      getsym();
      if (symId == R_PAREN){
        Fun_instruction(funcPos,0x48);
        u32_instruction(callPos, funcList[funcPos].instruct);
      }
      else{
        param(callPos);
        Fun_instruction(funcPos,0x48);
        u32_instruction(callPos, funcList[funcPos].instruct);
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
      bool flag = false;
      //local vars
      while (rangeList[saveRange].upRange != -1)
      {
        i = findVar(saveRange,preToken,&varType,ASSIGN_);
        if(i!=-1){
          int slot = rangeList[saveRange].vars[i].funSlot;
          flag = true;
          Fun_instruction(funcPos,0x0a);
          u32_instruction(slot, funcList[funcPos].instruct);
          break;
        }
        saveRange = rangeList[saveRange].upRange;
      }
      if (!flag) {//fun params
        i = findParam(funcPos,preToken,&varType,ASSIGN_);
        if(i!=-1){
          Fun_instruction(funcPos,0x0b);
          if (funcList[funcPos].retType == "void")
            u32_instruction(i, funcList[funcPos].instruct);
          else
            u32_instruction(i + 1, funcList[funcPos].instruct);
        }
        else { //global vars
          i = findVar(0,preToken,&varType,ASSIGN_);
          if(i!=-1){
            Fun_instruction(funcPos,0x0c);
            u32_instruction(i, funcList[funcPos].instruct);
          }
          else error(UNEXITED_VAR, token);
        }
      }
      getsym();
      expr(&varType);
      if (*retType == ASSIGN_) *retType = VOID_;
      Fun_instruction(funcPos,0x17);
      
    }
    /*========== Ident =========*/
    else
    {
      bool flag = false;
      while (rangeList[saveRange].upRange != -1)
      {
        i = findVar(saveRange,preToken,retType,LOAD_);
        if(i != -1){
          int slot = rangeList[saveRange].vars[i].funSlot;
          flag = true;
          Fun_instruction(funcPos,0x0a);
          u32_instruction(slot, funcList[funcPos].instruct);
          break;
        }
        saveRange = rangeList[saveRange].upRange;
      }
      if (!flag) {
        i = findParam(funcPos,preToken,retType,LOAD_);
        if(i!=-1){
          Fun_instruction(funcPos,0x0b);
          if (funcList[funcPos].retType == "void")
            u32_instruction(i, funcList[funcPos].instruct);
          else
            u32_instruction(i + 1, funcList[funcPos].instruct);
        }
        else {
          i = findVar(0,preToken,retType,LOAD_);
          if(i!=-1){
            Fun_instruction(funcPos,0x0c);
            u32_instruction(i, funcList[funcPos].instruct);
          }
          else error(UNEXITED_VAR, token);
        }
      }
      Fun_instruction(funcPos,0x13);
    }
  }
  else if (symId == UINT_LITERAL)
  {
    if (*retType != ASSIGN_ && *retType != INT_)
      error(RETURN_INT, token);
    int64_t temp = atoi(token);
    Fun_instruction(funcPos,0x01);
    u64_instruction(temp, funcList[funcPos].instruct);
    
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
    int64_t tmp = rangeList[0].vars.size();
    rangeList[0].vars.push_back(temp);
    funcList[0].instruct.push_back(0x01);
    u64_instruction(tmp, funcList[0].instruct);
    funcList[0].insNum++;
    if (*retType == ASSIGN_) *retType = INT_;
    getsym();
  }
  else if (symId == CHAR_LITERAL)
  {
    if (*retType != ASSIGN_ && *retType != INT_)
      error(RETURN_INT, token);
    int64_t tmp = token[0];
    Fun_instruction(funcPos,0x01);
    u64_instruction(tmp, funcList[funcPos].instruct);
    
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
      Fun_instruction(funcPos,0x34);
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
    Fun_instruction(funcPos,0x50);
    
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
    Fun_instruction(funcPos,0x51);
    
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
    Fun_instruction(funcPos,0x54);
    
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
    Fun_instruction(funcPos,0x55);
    
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
    int64_t tmp = rangeList[0].vars.size();
    rangeList[0].vars.push_back(temp);
    Fun_instruction(funcPos,0x01);
    u64_instruction(tmp, funcList[funcPos].instruct);
    
    getsym();
    check(R_PAREN);
    Fun_instruction(funcPos,0x57);
    
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
    Fun_instruction(funcPos,0x58);
    
    if (*retType == ASSIGN_) *retType = VOID_;
    getsym();
  }
  else error(UNMATCH_IDENT, token);
  if(symId == AS_KW){
    error(UNDO,token);
  }
}

void param(int callPos)
{
  int retType = ASSIGN_;
  expr(&retType);
  if (retType == INT_)
    if (funcList[callPos].params[0].dataType != "int")
      error(RETURN_INT, token);
  else if (retType == VOID_)
    if (funcList[callPos].params[0].dataType != "void")
      error(RETURN_VOID, token);
  else
    error(UNMATCH_TYPE, token);
  
  for (int i = 1; i < funcList[callPos].params.size(); ++i){
    check(COMMA);
    getsym();
    retType = ASSIGN_;
    expr(&retType);
    if (retType == INT_)
      if (funcList[callPos].params[i].dataType != "int")
        error(RETURN_INT, token);
    else if (retType == VOID_)
      if (funcList[callPos].params[i].dataType != "void")
        error(RETURN_VOID, token);
    else
      error(UNMATCH_TYPE, token);
  }
}