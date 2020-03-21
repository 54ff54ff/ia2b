/*========================================================================\
|: [Filename] cmdCharDef.cpp                                             :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Implement the interface for the keypress                   :|
<------------------------------------------------------------------------*/

#include "cmdCharDef.h"

namespace _54ff
{

ttySetting mainTTY;

void
ttySetting::check()
{
	//get current terminal setting
	tcgetattr(STDIN_FILENO, &old_setting);

	//set the raw mode based on the current setting
	raw_setting = old_setting;
	raw_setting.c_iflag &= ~ ( IGNBRK |
	                           BRKINT |
	                           PARMRK |
	                           ISTRIP |
	                           INLCR  |
	                           IGNCR  |
	                           ICRNL  |
	                           IXON );
	raw_setting.c_oflag &= ~   OPOST;
	raw_setting.c_lflag &= ~ ( ECHO   |
	                           ECHONL |
	                           ICANON |
	                           ISIG   |
	                           IEXTEN );
	raw_setting.c_cflag &= ~ ( CSIZE  |
	                           PARENB );
	raw_setting.c_cflag |=     CS8;
	raw_setting.c_cc[VTIME] = 0;
}

#define HEHE_RETURN_KEY
#include "cmdKey.h"
#undef HEHE_RETURN_KEY

char
ttySetting::getOneChar(bool combo)
{
	raw_setting.c_cc[VMIN] = enable && combo ? 0 : 1;
	char ch;
	waitNow = true;
	setToRaw();
	cin.unsetf(ios_base::skipws);
	cin >> ch;
	cin.  setf(ios_base::skipws);
//	cout.flush();
//	ssize_t actual = read(STDIN_FILENO, &ch, 1);
//	setToOld(); If setToOld here, some undesirable characters will be printed out if the keypress is too frequent
	waitNow = false;
//	return actual ? ch : 0;
	return cin.fail() ? cin.clear(), 0 : ch;
}

size_t
Menu(const char* question, const char* options[], size_t numOpt, const char* separator, bool tutorial)
{
	//pre-condition:
	//1. numOpt >= 2
	//2. Only letter, digit, punctuation and space (printable) is allowed in question, options and separator
	//   Newline can also be allowed in question though
	//3. The end of question should contain proper separator like space or newline
	auto  forward = [](const char* str) { for(; *str != 0; ++str) cout << *str; };
	auto backward = [](const char* str) { for(; *str != 0; ++str) cout << '\b'; };
//	check();
	if(tutorial)
		cout << "(Please press left/right arrow to select...)\n";
	forward(question);
	forward(options[0]);
	for(size_t i = 1; i < numOpt; ++i)
		forward(separator), forward(options[i]);
	for(size_t i = numOpt - 1; i > 0; --i)
		backward(options[i]), backward(separator);
	backward(options[0]);
	for(size_t curIdx = 0; true;)
		switch(returnExactKey())
		{
			case ENTER:
				forward(options[curIdx]);
				for(size_t i = curIdx + 1; i < numOpt; ++i)
					forward(separator), forward(options[i]);
				cout << endl;
				return curIdx;

			case LEFT:
				if(curIdx != 0)
					backward(separator), backward(options[--curIdx]);
				break;

			case RIGHT:
				if(curIdx != numOpt - 1)
					forward(options[curIdx++]), forward(separator);
				break;

			default:
				break;
		}
}

}
