#include <iostream>
#include <vector>
#include <stdlib.h>
#include <string.h>
using namespace std;

// 符号表 =============================================
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

void F_instruction(int funtionPos, int n){
  Fmap[funtionPos].instructions.push_back(n);
  Fmap[funtionPos].insNum++;
}