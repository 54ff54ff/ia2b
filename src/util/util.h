/*========================================================================\
|: [Filename] util.h                                                     :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Define some utilities                                      :|
<------------------------------------------------------------------------*/

#ifndef HEHE_UTIL_H
#define HEHE_UTIL_H

#include <unistd.h>
#include <sys/times.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <iostream>
#include "array.h"
using namespace std;

namespace _54ff
{

inline char* copyCStr(const char* str) { return strcpy(new char[strlen(str)+1], str); }
inline char* copyCStr(const string& str) { return copyCStr(str.c_str()); }
inline size_t strLen(const char* beg, const char* end) { return (size_t(end) - size_t(beg)) / sizeof(char); }

/*====================================*/

template <int (*toWhat)(int)>
inline char* copyToWhat(const char* str)
{
	char* s = copyCStr(str);
	for(char* tmp = s; *tmp != '\0'; ++tmp)
		*tmp = toWhat(*tmp);
	return s;
}
inline char* copyToLower(const char* str) { return copyToWhat<tolower>(str); }
inline char* copyToUpper(const char* str) { return copyToWhat<toupper>(str); }

/*====================================*/

class WrapStr
{
friend ostream& operator<<(ostream& os, const WrapStr& ws) { return os << ws.str; }

public:
	WrapStr(): str(0) {}
	WrapStr(const char* s, bool c = true) { setStrInt(s, c); }
	~WrapStr() { clearInt(); }

	WrapStr(const WrapStr& ws): WrapStr(ws.str, true) {}
	WrapStr(WrapStr&& ws): WrapStr(ws.str, false) { ws.str = 0; }

	WrapStr& operator=(const WrapStr& ws) { setStrInt(ws.str, true); return *this; }
	WrapStr& operator=(WrapStr&& ws) { setStrInt(ws.str, false); ws.str = 0; return *this; }

	bool operator==(const WrapStr& ws)const { return strcmp(str, ws.str) == 0; }

	void clear() { clearInt(); str = 0; }
	void setStr(const char* s, bool copy) { clearInt(); setStrInt(s, copy); }
	operator const char*()const { return str; }
	const char& operator[](size_t i)const { return str[i]; }
	bool operator<(const WrapStr& ws)const { return strcmp(str, ws.str) < 0; }

private:
	void clearInt() { delete []str; }
	void setStrInt(const char* s, bool copy) { str = copy ? copyCStr(s) : s; }

private:
	const char*  str;
};

using WrapStrList = vector<WrapStr>;
using PureStrList = vector<const char*>;

/*====================================*/

inline size_t calLen(const char* str)               { return strlen(str); }
template <class... Strs>
inline size_t calLen(const char* str, Strs... strs) { return strlen(str) + 1 + calLen(strs...); }

inline void copyMultiCStrSub(char* tmp, const char* str)
	{ for(; *str != '\0'; ++tmp, ++str) *tmp = *str; *tmp = '\0'; }
template <class... Strs>
inline void copyMultiCStrSub(char* tmp, const char* str, Strs... strs)
	{ for(; *str != '\0'; ++tmp, ++str) *tmp = *str; *(tmp++) = ' '; copyCStrSub(tmp, strs...); }

template <class... Strs>
inline char* copyMultiCStr(Strs... strs) { char* tmp = new char[calLen(strs...)+1]; copyMultiCStrSub(tmp, strs...); return tmp; }

/*====================================*/

constexpr size_t GET_FLAG_MASK = 1;
constexpr size_t FLIP_FLAG_MASK = GET_FLAG_MASK;
constexpr size_t GET_PTR_MASK = ~GET_FLAG_MASK;
template <class T> size_t setFlagToValue(T* ptr, bool flag) { return size_t(ptr) | size_t(flag); }
template <class T> T*     setFlagToPtr  (T* ptr, bool flag) { return (T*)setFlagToValue(ptr, flag); }
template <class T> T* getPtr(size_t value) { return (T*)(value & GET_PTR_MASK); } //In this function prototype,
                                                                                  //there is nothing related to "class T".
                                                                                  //Hence if we want to use it,
                                                                                  //the "class T" must be explicitly specified!
template <class T> T* getPtr(T* ptr)       { return getPtr<T>(size_t(ptr)); }
inline             bool getFlag(size_t value) { return value & GET_FLAG_MASK; }
template <class T> bool getFlag(T* ptr)       { return getFlag(size_t(ptr)); }

/*================== strUtil.cpp ==================*/

enum MatchType { MATCH, NOTMATCH, PARTIAL };
MatchType myStrNCmp(const char*, const char*, size_t);
char* getToken(char*, bool* = 0);
PureStrList breakToTokens(char*);
char* findLastToken(char*);
bool isValidVarName(const char*);

/*====================================*/

constexpr size_t MAX_SIZE_T = numeric_limits<size_t>::max();

template<class IntType>
bool myStrToInt(const char* str, IntType& num)
{
	static_assert(is_signed_v<IntType> && is_integral_v<IntType>);

	if(*str == '\0')
		return false;
	num = 0;
	bool sign = false;
	if(*str == '-')
		{ if(*(++str) == '\0') return false; sign = true; }
	bool leadZero = false;
	bool overflow = false;
	for(size_t i = 0; str[i] != '\0'; ++i)
		if(isdigit(str[i]))
		{
			IntType tmp = num;
			num *= 10;
			num += IntType(str[i] - '0');
			if(tmp > num) overflow = true;
			if(tmp == 0 && i != 0)
				leadZero = true;
		}
		else return false;
	if(sign) num = -num;
	if(leadZero) cerr << "[Warning] Leading zero(s) on a number (" << str << ") should be avoided!" << endl;
	if(overflow) cerr << "[Warning] Overflow detected, the resulting number is " << num << "!" << endl;
	return true;
}

template<class UIntType>
bool myStrToUInt(const char* str, UIntType& num)
{
	static_assert(is_unsigned_v<UIntType> && is_integral_v<UIntType>);

	if(*str == '\0')
		return false;
	num = 0;
	bool leadZero = false;
	bool overflow = false;
	for(size_t i = 0; str[i] != '\0'; ++i)
		if(isdigit(str[i]))
		{
			UIntType tmp = num;
			num *= 10;
			num += UIntType(str[i] - '0');
			if(tmp > num) overflow = true;
			if(tmp == 0 && i != 0)
				leadZero = true;
		}
		else return false;
	if(leadZero) cerr << "[Warning] Leading zero(s) on a number (" << str << ") should be avoided!" << endl;
	if(overflow) cerr << "[Warning] Overflow detected, the resulting number is " << num << "!" << endl;
	return true;
}

/*================== listFile.cpp ==================*/

WrapStrList listFile(const char*, const char*);
void printStrsByOrder(const size_t, const WrapStrList&);
string findCommonPart(size_t, const WrapStrList&);
void printWithPadding(const size_t, const size_t, const char*);

/*====================================*/

template <class UIntType>
size_t calDigit(UIntType n)
{
	static_assert(is_unsigned_v<UIntType> && is_integral_v<UIntType>);

	size_t ans = 0;
	for(; n != 0; n /= 10)
		ans += 1;
	return ans;
}

/*================== progresser.cpp ==================*/

class Progresser
{
// $(prompt): $(counter)/$(maxNum)

public:
	Progresser(const char* p, size_t n, size_t begin = 0)
	: prompt   (copyCStr(p))
	, strLen   (strlen(prompt))
	, maxNum   (n)
	, padding  (calDigit(maxNum))
	, counter  (begin) {}

	~Progresser() { delete []prompt; }

	void cleanCurLine()const;
	void printLine()const;
	void count(size_t = 1);

private:
	const char* const  prompt;
	const size_t       strLen;
	const size_t       maxNum;
	const size_t       padding;
	size_t             counter;
};

/*================== util.cpp ==================*/
time_t getCurTime();
void getInfoAboutCurProc();
const char* getHomeDir();
const char* replaceHomeDir(const char*);
size_t getHashSize(size_t);

class Timer
{
public:
	Timer() { checkClock(); initClock = curClock; }
	void printTime();

private:
	void checkClock() { times(&clockData); curClock = clock(); }

private:
	clock_t     initClock;
	clock_t     curClock;
	struct tms  clockData;
} extern timer;

}

#endif
