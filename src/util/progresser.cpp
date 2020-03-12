/*========================================================================\
|: [Filename] progresser.cpp                                             :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Implement the progresser                                   :|
<------------------------------------------------------------------------*/

#include <iomanip>
#include "util.h"
#include "condStream.h"
using namespace std;

namespace _54ff
{

void
Progresser::cleanCurLine()const
{
	cout << RepeatChar('\b', padding + 2 + strLen)               // counter -> ": " -> prompt
	     << RepeatChar(' ',  strLen + 2 + padding + 1 + padding) // prompt  -> ": " -> counter -> "/"  -> maxNum
	     << RepeatChar('\b', padding + 1 + padding + 2 + strLen) // maxNum  -> "/"  -> counter -> ": " -> prompt
	     << flush;
}

void
Progresser::printLine()const
{
	cout << prompt << ": "
	     << right << setw(padding) << counter
	     << "/" << maxNum
	     << RepeatChar('\b', padding + 1) // maxNum -> "/"
	     << flush;
}

void
Progresser::count(size_t diff)
{
	cout << RepeatChar('\b', padding) // counter
	     << setw(padding) << (counter += diff) << flush;
}

}