/*========================================================================\
|: [Filename] progresser.cpp                                             :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Implement the progresser                                   :|
<------------------------------------------------------------------------*/

#include <iomanip>
#include "util.h"
using namespace std;

namespace _54ff
{

void
Progresser::cleanCurLine()const
{
	repeatChar(padding + 2 + strLen, '\b');               //counter -> ": " -> prompt
	repeatChar(strLen + 2 + padding + 1 + padding, ' ');  //prompt -> ": " -> counter -> "/" -> maxNum
	repeatChar(padding + 1 + padding + 2 + strLen, '\b'); //maxNum -> "/" -> counter -> ": " -> prompt
	cout.flush();
}

void
Progresser::printLine()const
{
	cout << prompt << ": "
	     << right << setw(padding) << counter
	     << "/" << maxNum;
	repeatChar(padding + 1, '\b'); //maxNum -> "/"
	cout.flush();
}

void
Progresser::count(size_t diff)
{
	repeatChar(padding, '\b'); //counter
	cout << setw(padding) << (counter += diff) << flush;
}

}