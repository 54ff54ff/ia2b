/*========================================================================\
|: [Filename] stat.h                                                     :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Define a general base class for statistics                 :|
<------------------------------------------------------------------------*/

#ifndef HEHE_STAT_H
#define HEHE_STAT_H

#include <time.h>
#include <type_traits>
using namespace std;

namespace _54ff
{

template <size_t M, size_t N>
class Stat
{
public:
	Stat(): number{}, time{} {}

	void countN(size_t i = 0, size_t n = 1) { number[i] += n; }
	void countOne(size_t i = 0) { countN(i); }
	void doOneTime() { countOne(); }
	void setNum(size_t i = 0, size_t n = 1) { number[i] = n; }
	void startTime()  { tmp = clock(); }
	void finishTime(size_t i = 0) { time[i] += (clock() - tmp); }
	void setTime(size_t i = 0) { time[i] = (clock() - tmp); }

	size_t getNum(size_t i = 0)const { return number[i]; }
	clock_t getClock(size_t i = 0)const { return time[i]; }
	double getTotalTime(size_t i = 0)const { return double(getClock(i)) / CLOCKS_PER_SEC; }
	double getAveTime(size_t i = 0)const { return (double(getClock(i)) / getNum(i)) / CLOCKS_PER_SEC; }

protected:
	size_t   number[M];
	clock_t  time[N];
	clock_t  tmp;
};

template <class StatType>
class StatPtr
{
public:
	template <class... Param>
	StatPtr(bool on, Param&&... param)
	: s(on ? (new StatType(forward<Param>(param)...)) : 0) {}
	~StatPtr() { delete s; }

	bool isON()const { return s != 0; }
	StatType* operator->()const { return s; }

private:
	StatType*  s;
};

}

#endif