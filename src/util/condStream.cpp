/*========================================================================\
|: [Filename] condStream.cpp                                             :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Implement CondStream and the other utilities               :|
<------------------------------------------------------------------------*/

#include "condStream.h"

namespace _54ff
{

ostream& operator<<(ostream& os, const RepeatChar& rc)
{
	for(unsigned i = 0; i < rc.num; ++i)
		os << rc.ch;
	return os;
}

ostream& operator<<(ostream& os, const CleanStrOnTerminal& cs)
{
	const size_t len = strlen(cs.str);
	for(size_t i = 0; i < len; ++i) os << "\b";
	for(size_t i = 0; i < len; ++i) os << " ";
	return os;
}

}