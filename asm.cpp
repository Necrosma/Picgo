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

// 符号表 ===================
typedef struct GLOBAL
{
  string name;
  string dataType;
  bool is_const;
  int funSlot;
  GLOBAL() : name(), dataType(), is_const() {}
  GLOBAL(string name, string dataType, bool is_const) : name(name), dataType(dataType), is_const(is_const) {}
} Global;
typedef struct LOCAL
{
  vector<Global> vars;
  //所属函数
  int funtionPos;
  //全局变量-1 函数0 逐增
  int upRange;
  LOCAL(int funtionPos, int upRange) : vars(), funtionPos(funtionPos), upRange(upRange) {}
} Local;
vector<Local> Lmap;
typedef struct FUNTION
{
  int pos;
  int localSlotNum;
  int paramSlotNum; //TODO:检查是否增加Num
  string name;
  string retType;
  int insNum;
  vector<unsigned char> instructions;
  vector<Global> params;
  FUNTION(string name) : pos(0), localSlotNum(0), paramSlotNum(0), name(name), retType(""), insNum(0), instructions(), params() {}
} Funtion;
vector<Funtion> Fmap;

vector<unsigned char> instructions;

//==============================
void checkVarType(Global type, int* retType);
void setVarType(Global param, int* retType);
//==============================
void intToFourBits(int x,unsigned char *str){
    str[1]=x;
    str[2]=x>>8;
    str[3]=x>>16;
    str[4]=x>>24;
}

void u32_instruction(int x, vector<unsigned char> &instructions)
{
  instructions.push_back(x>>24);
  instructions.push_back(x>>16);
  instructions.push_back(x>>8);
  instructions.push_back(x);
}
void u64_instruction(int64_t x, vector<unsigned char> &instructions)
{
  instructions.push_back(x>>56);
  instructions.push_back(x>>48);
  instructions.push_back(x>>40);
  instructions.push_back(x>>32);
  instructions.push_back(x>>24);
  instructions.push_back(x>>16);
  instructions.push_back(x>>8);
  instructions.push_back(x);
}

void init_start()
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
}

void Fun_instruction(int funtionPos, int n)
{
  Fmap[funtionPos].instructions.push_back(n);
  Fmap[funtionPos].insNum++;
}
int findFun(string preToken)
{
  for (int i = 1; i < Fmap.size(); i++){
    if (preToken == Fmap[i].name)
      return i;
  }
  return -1;
}

int findVar(int tempRangePos, string preToken, int *retType, int isLoad)
{
  for (int i = 0; i < Lmap[tempRangePos].vars.size(); i++)
  {
    if (preToken == Lmap[tempRangePos].vars[i].name)
    {
      if(isLoad) checkVarType(Lmap[tempRangePos].vars[i],retType);
      else setVarType(Lmap[tempRangePos].vars[i],retType);
      return i;
    }
  }
  return -1;
}

int findParam(int funtionPos, string preToken, int *retType, int isLoad)
{
  for (int i = 0; i < Fmap[funtionPos].params.size(); i++)
  {
    if (preToken == Fmap[funtionPos].params[i].name)
    {
      if(isLoad) checkVarType(Fmap[funtionPos].params[i],retType);
      else setVarType(Fmap[funtionPos].params[i],retType);
      return i;
    }
  }
  return -1;
}

void setVarType(Global param, int* retType){
  if (param.is_const)
    error(99, token);
  if (param.dataType == "int")
    *retType = 1;
  else{
    puts("setVarType ERROR");
    error(99, token);
  }
}

void checkVarType(Global var, int* retType)
{
  if (var.dataType == "int")
  {
    if (*retType != ASSIGN_ && *retType != 1){
      puts("return isn't int");
      error(99, token);
    }
    if (*retType == ASSIGN_)
      *retType = 1;
  }
  else if (var.dataType == "void")
  {
    if (*retType != ASSIGN_ && *retType != VOID_){
      puts("return isn't int");
      error(99, token);
    }
    if (*retType == ASSIGN_)
      *retType = VOID_;
  }
  else{
    puts("checkType unreached ERROR");
    error(99, token);
  }
}

void checkDefine(int rangePos, string name){
  for (int i = 0; i < Lmap[rangePos].vars.size(); i++)
  {
    if (Lmap[rangePos].vars[i].name == name)
      error(99, token);
  }
}