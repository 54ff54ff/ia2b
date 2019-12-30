/*========================================================================\
|: [Filename] test.h                                                     :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Define various testing functions                           :|
<------------------------------------------------------------------------*/

#include <iostream>
#include "cmdCharDef.h"
#include "array.h"
using namespace std;

namespace _54ff
{

void testGetChar()
{
	check();
	Key k;
	do
	{
		cout << "> "; cout.flush();
		switch(k = returnExactKey())
		{
			case CTRL_A         : cout << "CTRL_A"         << endl; break;
			case CTRL_B         : cout << "CTRL_B"         << endl; break;
			case CTRL_C         : cout << "CTRL_C"         << endl; break;
			case CTRL_D         : cout << "CTRL_D"         << endl; break;
			case CTRL_E         : cout << "CTRL_E"         << endl; break;
			case CTRL_F         : cout << "CTRL_F"         << endl; break;
			case CTRL_G         : cout << "CTRL_G"         << endl; break;
			case CTRL_H         : cout << "CTRL_H"         << endl; break;
			case CTRL_I         : cout << "CTRL_I"         << endl; break;
			case CTRL_J         : cout << "CTRL_J"         << endl; break;
			case CTRL_K         : cout << "CTRL_K"         << endl; break;
			case CTRL_L         : cout << "CTRL_L"         << endl; break;
			case CTRL_M         : cout << "CTRL_M"         << endl; break;
			case CTRL_N         : cout << "CTRL_N"         << endl; break;
			case CTRL_O         : cout << "CTRL_O"         << endl; break;
			case CTRL_P         : cout << "CTRL_P"         << endl; break;
			case CTRL_Q         : cout << "CTRL_Q"         << endl; break;
			case CTRL_R         : cout << "CTRL_R"         << endl; break;
			case CTRL_S         : cout << "CTRL_S"         << endl; break;
			case CTRL_T         : cout << "CTRL_T"         << endl; break;
			case CTRL_U         : cout << "CTRL_U"         << endl; break;
			case CTRL_V         : cout << "CTRL_V"         << endl; break;
			case CTRL_W         : cout << "CTRL_W"         << endl; break;
			case CTRL_X         : cout << "CTRL_X"         << endl; break;
			case CTRL_Y         : cout << "CTRL_Y"         << endl; break;
			case CTRL_Z         : cout << "CTRL_Z"         << endl; break;
			case CTRL_BACKSLASH : cout << "CTRL_BACKSLASH" << endl; break;
			case ESC            : cout << "ESC"            << endl; break;
			case UP             : cout << "UP"             << endl; break;
			case DOWN           : cout << "DOWN"           << endl; break;
			case RIGHT          : cout << "RIGHT"          << endl; break;
			case LEFT           : cout << "LEFT"           << endl; break;
			case END            : cout << "END"            << endl; break;
			case HOME           : cout << "HOME"           << endl; break;
			case INSERT         : cout << "INSERT"         << endl; break;
			case DELETE         : cout << "DELETE"         << endl; break;
			case PAGEUP         : cout << "PAGEUP"         << endl; break;
			case PAGEDOWN       : cout << "PAGEDOWN"       << endl; break;
			case ALT_A          : cout << "ALT_A"          << endl; break;
			case ALT_B          : cout << "ALT_B"          << endl; break;
			case ALT_C          : cout << "ALT_C"          << endl; break;
			case ALT_D          : cout << "ALT_D"          << endl; break;
			case ALT_E          : cout << "ALT_E"          << endl; break;
			case ALT_F          : cout << "ALT_F"          << endl; break;
			case ALT_G          : cout << "ALT_G"          << endl; break;
			case ALT_H          : cout << "ALT_H"          << endl; break;
			case ALT_I          : cout << "ALT_I"          << endl; break;
			case ALT_J          : cout << "ALT_J"          << endl; break;
			case ALT_K          : cout << "ALT_K"          << endl; break;
			case ALT_L          : cout << "ALT_L"          << endl; break;
			case ALT_M          : cout << "ALT_M"          << endl; break;
			case ALT_N          : cout << "ALT_N"          << endl; break;
			case ALT_O          : cout << "ALT_O"          << endl; break;
			case ALT_P          : cout << "ALT_P"          << endl; break;
			case ALT_Q          : cout << "ALT_Q"          << endl; break;
			case ALT_R          : cout << "ALT_R"          << endl; break;
			case ALT_S          : cout << "ALT_S"          << endl; break;
			case ALT_T          : cout << "ALT_T"          << endl; break;
			case ALT_U          : cout << "ALT_U"          << endl; break;
			case ALT_V          : cout << "ALT_V"          << endl; break;
			case ALT_W          : cout << "ALT_W"          << endl; break;
			case ALT_X          : cout << "ALT_X"          << endl; break;
			case ALT_Y          : cout << "ALT_Y"          << endl; break;
			case ALT_Z          : cout << "ALT_Z"          << endl; break;
			case SHIFT_RIGHT    : cout << "SHIFT_RIGHT"    << endl; break;
			case SHIFT_LEFT     : cout << "SHIFT_LEFT"     << endl; break;
			case CTRL_UP        : cout << "CTRL_UP"        << endl; break;
			case CTRL_DOWN      : cout << "CTRL_DOWN"      << endl; break;
			case CTRL_RIGHT     : cout << "CTRL_RIGHT"     << endl; break;
			case CTRL_LEFT      : cout << "CTRL_LEFT"      << endl; break;
			case UNDEFINED_KEY  : cout << "UNDEFINED_KEY"  << endl; break;
			case DELETE_CHAR    : cout << "DELETE_CHAR"    << endl; break;
			default: if(isPrint(k)) cout << char(k) << endl; break;
		}
	} while(k != ESC);
}

void testArray()
{
	struct objForTestArray
	{
		 objForTestArray() { cout << "ctor" << endl; }
		~objForTestArray() { cout << "dtor for i = " << i << ", j = " << j << ", k = " << k << endl; }

		int i, j, k;
	};

	sArray<objForTestArray, 5> test;
	for(size_t i = 0; i < 5; ++i)
			{ test[i].i = i;  }
	for(size_t i = 0; i < 5; ++i)
			{ cout << i << " " << test[i].i << endl; }
}

}