/*========================================================================\
|: [Filename] condStream.h                                               :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] 1. Define a wrapper class for ostream to activate it       :|
:|               conditionally based on the current setting              |:
|:            2. Define some utilities for CondStream to facilitate the  :|
:|               usage and make the code cleaner                         |:
<------------------------------------------------------------------------*/

#ifndef HEHE_CONDSTREAM_H
#define HEHE_CONDSTREAM_H

#include <iostream>
#include "util.h"
using namespace std;

namespace _54ff
{

class CondStream
{};

class RepeatChar
{
friend ostream& operator<<(ostream&, const RepeatChar&);

public:
	RepeatChar(char c, unsigned n): ch(c), num(n) {}

private:
	char      ch;
	unsigned  num;
};

class CleanStrOnTerminal
{
friend ostream& operator<<(ostream&, const CleanStrOnTerminal&);

public:
	CleanStrOnTerminal(const char* s): str(s) {}

private:
	const char*  str;
};

template <class UIntType>
class CleanIntOnTerminal
{
static_assert(is_unsigned_v<UIntType> && is_integral_v<UIntType>);

friend ostream& operator<<(ostream& os, const CleanIntOnTerminal& ci)
{
	const size_t len = calDigit(ci.num);
	for(size_t i = 0; i < len; ++i) os << "\b";
	for(size_t i = 0; i < len; ++i) os << " ";
	return os;
}

public:
	CleanIntOnTerminal(UIntType n): num(n) {}

private:
	UIntType  num;
};

}

#endif