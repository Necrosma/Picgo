#include <iostream>
#include <vector>
#include <stdlib.h>
#include <string.h>
using namespace std;

// 符号表 =============================================
//全局变量常量、字符串字面量交错存储，函数在二者之后
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
  LOCAL(int funtionPos, int upRange, int isWhile) : vars(), funtionPos(funtionPos), upRange(upRange) {}
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

void F_instruction(int funtionPos, int n)
{
  Fmap[funtionPos].instructions.push_back(n);
  Fmap[funtionPos].insNum++;
}

int findVar(int tempRangePos, string preToken, int *retType, bool isLoad)
{
  for (int i = 0; i < Lmap[tempRangePos].vars.size(); i++)
  {
    if (preToken == Lmap[tempRangePos].vars[i].name)
    {
      if(isLoad)
        checkVarType(Lmap[tempRangePos].vars[i],retType);
      else setVarType(Lmap[tempRangePos].vars[i],retType);
      return i;
    }
  }
  return -1;
}

int findParam(int funtionPos, string preToken, int *retType, bool isLoad)
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
  else if (param.dataType == "double")
    *retType = 2;
  else{
    puts("setVarType ERROR");
    error(99, token);
  }
}

void checkVarType(Global var, int* retType)
{
  if (var.dataType == "int")
  {
    if (*retType != 0 && *retType != 1){
      puts("return isn't int");
      error(99, token);
    }
    if (*retType == 0)
      *retType = 1;
  }
  else if (var.dataType == "double")
  {
    if (*retType != 0 && *retType != 2){
      puts("return isn't int");
      error(99, token);
    }
    if (*retType == 0)
      *retType = 2;
  }
  else if (var.dataType == "void")
  {
    if (*retType != 0 && *retType != 3){
      puts("return isn't int");
      error(99, token);
    }
    if (*retType == 0)
      *retType = 3;
  }
  else if (var.dataType == "string")
    puts("string type???");
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