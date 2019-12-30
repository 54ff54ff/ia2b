/*========================================================================\
|: [Filename] cmdCharDef.h                                               :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Define the interface for the keypress                      :|
<------------------------------------------------------------------------*/

#ifndef HEHE_CMDCHARDEF_H
#define HEHE_CMDCHARDEF_H

#include <termios.h>
#include <unistd.h>
#include <iostream>
using namespace std;

namespace _54ff
{

#define HEHE_DEFINE_KEY
#include "cmdKey.h"
#undef HEHE_DEFINE_KEY

class ttySetting
{
public:
	ttySetting(): waitNow(false), enable(true) {}

	void check();
	Key returnExactKey();
	bool isWaiting()const { return waitNow; }
	bool enabled()const { return enable; }
	bool changeEnable(bool e) { return enable ^ e ? (enable = e, true) : false; }
	void flipEnable() { enable = !enable; }

	void setToRaw()const { tcsetattr(STDIN_FILENO, TCSANOW, &raw_setting); }
	void setToOld()const { tcsetattr(STDIN_FILENO, TCSANOW, &old_setting); }

private:
	struct termios  old_setting;
	struct termios  raw_setting;
	bool            waitNow;
	bool            enable;

	char getOneChar(bool combo = true); //pre-condition: cin is connected to termianl
};

extern ttySetting mainTTY;
inline void check() { mainTTY.check(); }
inline Key returnExactKey() { return mainTTY.returnExactKey(); }
inline bool isWaiting() { return mainTTY.isWaiting(); }
inline bool enabled() { return mainTTY.enabled(); }
inline bool changeEnable(bool e) { return mainTTY.changeEnable(e); }
inline void flipEnable() { return mainTTY.flipEnable(); }
inline void setToRaw() { mainTTY.setToRaw(); }
inline void setToOld() { mainTTY.setToOld(); }

template<class T> bool isPrint(T t) { return t >= 32 && t <= 126; }

size_t Menu(const char*, const char*[], size_t, const char* = " / ", bool = false);

}

#endif
