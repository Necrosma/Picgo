#include <iostream>
#include <vector>
#include <stdlib.h>
#include <string.h>
using namespace std;
#define LOAD_ 1
#define ASSIGN_ 0
#define INT_ 1
#define VOID_ 3
#define BOOL_ 4

typedef struct VAR
{
  string name;
  string dataType;
  bool const_;
  int funSlot;
  VAR() : name(), dataType(), const_(), funSlot() {}
  VAR(string name, string dataType, bool const_) : name(name), dataType(dataType), const_(const_), funSlot() {}
} Var;

typedef struct RANGE
{
  vector<Var> vars;
  int funcPos; //所属函数
  int upRange; //全局变量-1 函数0 逐增
  RANGE(int funcPos, int upRange) : vars(), funcPos(funcPos), upRange(upRange) {}
} Range;

typedef struct FUNTION
{
  vector<Var> params;
  int pos;
  string name;
  string retType;
  int insNum;  
  int varSlot;
  int paramSlot;
  vector<unsigned char> instruct;
  FUNTION(string name) : pos(0), varSlot(0), paramSlot(0), name(name), retType(""), insNum(0), instruct(), params() {}
} Funtion;


vector<Range> rangeList;
vector<Funtion> funcList;
vector<unsigned char> o0;

void Fun_instruction(int funcPos, int n)
{
  funcList[funcPos].instruct.push_back(n);
  funcList[funcPos].insNum++;
}

void u32_instruction(int x, vector<unsigned char> &instruct)
{
  instruct.push_back(x>>24);
  instruct.push_back(x>>16);
  instruct.push_back(x>>8);
  instruct.push_back(x);
}

void u64_instruction(int64_t x, vector<unsigned char> &instruct)
{
  instruct.push_back(x>>56);
  instruct.push_back(x>>48);
  instruct.push_back(x>>40);
  instruct.push_back(x>>32);
  instruct.push_back(x>>24);
  instruct.push_back(x>>16);
  instruct.push_back(x>>8);
  instruct.push_back(x);
}

void init_begin()
{
  o0.push_back(0x72);
  o0.push_back(0x30);
  o0.push_back(0x3b);
  o0.push_back(0x3e);
  u32_instruction(1, o0);
  funcList.push_back(Funtion("_start"));
  funcList[0].retType = "void";
  funcList.push_back(Funtion("main"));
  rangeList.push_back(Range(0, -1));
}

void init_end()
{
  
  funcList[0].instruct.push_back(0x1a);
  funcList[0].instruct.push_back(0x00);
  funcList[0].instruct.push_back(0x00);
  funcList[0].instruct.push_back(0x00);
  funcList[0].instruct.push_back(0x01);
  funcList[0].insNum++;
  
  funcList[0].instruct.push_back(0x48);
  funcList[0].instruct.push_back(0x00);
  funcList[0].instruct.push_back(0x00);
  funcList[0].instruct.push_back(0x00);
  funcList[0].instruct.push_back(0x01);
  funcList[0].insNum++;
  
  funcList[0].instruct.push_back(0x03);
  funcList[0].instruct.push_back(0x00);
  funcList[0].instruct.push_back(0x00);
  funcList[0].instruct.push_back(0x00);
  funcList[0].instruct.push_back(0x01);
  funcList[0].insNum++;
}

void setVarType(Var param, int* retType){
  if (param.const_)
    error(99, token);
  if (param.dataType == "int")
    *retType = 1;
  else error(UNMATCH_TYPE, token);
}

void checkVarType(Var var, int* retType)
{
  if (var.dataType == "int"){
    if (*retType != ASSIGN_ && *retType != 1)
      error(RETURN_INT, token);
    if (*retType == ASSIGN_) *retType = 1;
  }
  else if (var.dataType == "void") {
    if (*retType != ASSIGN_ && *retType != VOID_)
      error(RETURN_VOID, token);
    if (*retType == ASSIGN_) *retType = VOID_;
  }
  else error(UNMATCH_TYPE, token);
}

void checkDefine(int range, string name){
  for (int i = 0; i < rangeList[range].vars.size(); i++)
    if (rangeList[range].vars[i].name == name)
      error(DUL_VAR, token);
}


int findFun(string token)
{
  for (int i = 1; i < funcList.size(); i++)
    if (token == funcList[i].name)
      return i;
  return -1;
}

int findVar(int range, string token, int *retType, int isLoad)
{
  for (int i = 0; i < rangeList[range].vars.size(); i++)
  {
    if (token == rangeList[range].vars[i].name){
      if(isLoad) checkVarType(rangeList[range].vars[i],retType);
      else setVarType(rangeList[range].vars[i],retType);
      return i;
    }
  }
  return -1;
}

int findParam(int funcPos, string token, int *retType, int isLoad)
{
  for (int i = 0; i < funcList[funcPos].params.size(); i++)
  {
    if (token == funcList[funcPos].params[i].name){
      if(isLoad) checkVarType(funcList[funcPos].params[i],retType);
      else setVarType(funcList[funcPos].params[i],retType);
      return i;
    }
  }
  return -1;
}