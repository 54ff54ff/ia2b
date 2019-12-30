/*========================================================================\
|: [Filename] cmdKey.h                                                   :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Define the enum and checking method for the keypress       :|
<------------------------------------------------------------------------*/

#ifdef HEHE_DEFINE_KEY

#include <limits>

enum Key
{
	ZERO = 0,

	//CTRL_A ~ CTRL_Z => 1 ~ 26
	CTRL_A = 1,
	CTRL_B,
	CTRL_C,
	CTRL_D,
	CTRL_E,
	CTRL_F,
	CTRL_G,
	CTRL_H,
	CTRL_I,
	CTRL_J,
	CTRL_K,
	CTRL_L,
	CTRL_M,
	CTRL_N,
	CTRL_O,
	CTRL_P,
	CTRL_Q,
	CTRL_R,
	CTRL_S,
	CTRL_T,
	CTRL_U,
	CTRL_V,
	CTRL_W,
	CTRL_X,
	CTRL_Y,
	CTRL_Z,

	CTRL_BACKSLASH = 28,

	ESC = 27,
	CHECK_KEY_1 = 91,
	CHECK_KEY_2 = 126,
	CHECK_KEY_3 = 49,
	CHECK_KEY_4 = 59,
	CHECK_KEY_5 = 50,
	CHECK_KEY_6 = 53,

	//27 -> 91 -> { 65, 66, 67, 68, 70, 72 }
	OFFSET_1 = 1 << 8,
	UP       = 65 + OFFSET_1,
	DOWN     = 66 + OFFSET_1,
	RIGHT    = 67 + OFFSET_1,
	LEFT     = 68 + OFFSET_1,
	END      = 70 + OFFSET_1,
	HOME     = 72 + OFFSET_1,

	//27 -> 91 -> { 50, 51, 53, 54 } -> 126
	OFFSET_2 = 1 << 9,
	INSERT   = 50 + OFFSET_2,
	DELETE   = 51 + OFFSET_2,
	PAGEUP   = 53 + OFFSET_2,
	PAGEDOWN = 54 + OFFSET_2,

	//ALT_A ~ ALT_Z => 27 -> { 97 ~ 122 }
	OFFSET_3 = 1 << 10,
	ALT_A    = 97 + OFFSET_3,
	ALT_B,
	ALT_C,
	ALT_D,
	ALT_E,
	ALT_F,
	ALT_G,
	ALT_H,
	ALT_I,
	ALT_J,
	ALT_K,
	ALT_L,
	ALT_M,
	ALT_N,
	ALT_O,
	ALT_P,
	ALT_Q,
	ALT_R,
	ALT_S,
	ALT_T,
	ALT_U,
	ALT_V,
	ALT_W,
	ALT_X,
	ALT_Y,
	ALT_Z,

	//SHIFT_LEFT  => 27 -> 91 -> 49 -> 59 -> 50 -> 68
	//SHIFT_RIGHT => 27 -> 91 -> 49 -> 59 -> 50 -> 67
	OFFSET_4    = 1 << 11,
	SHIFT_RIGHT = 67 + OFFSET_4,
	SHIFT_LEFT  = 68 + OFFSET_4,

	//CTRL + ARROW => 27 -> 91 -> 49 -> 59 -> 53 -> { 65, 66, 67, 68 }
	OFFSET_5   = 1 << 12,
	CTRL_UP    = 65 + OFFSET_5,
	CTRL_DOWN  = 66 + OFFSET_5,
	CTRL_RIGHT = 67 + OFFSET_5,
	CTRL_LEFT  = 68 + OFFSET_5,

	UNDEFINED_KEY = numeric_limits<int>::max(),

	BEEP            = int('\a'), //7,  CTRL_G
	TAB             = int('\t'), //9,  CTRL_I
	NEWLINE         = int('\n'), //10, CTRL_J
	CARRIAGE_RETURN = int('\r'), //13, CTRL_M
	BACKSPACE       = int('\b'), //8,  CTRL_H
	DELETE_CHAR     = 127,
	SPACE           = int(' '),
	HASH            = int('#'),
	BACKSLASH       = int('\\'),

	ENTER = CARRIAGE_RETURN
};

#endif

#ifdef HEHE_RETURN_KEY

Key
ttySetting::returnExactKey()
{
	/* just keep in comment the old implementation
	#define COMPARE(ch, n, op) ch op char(n)
	#define EQ(ch, n)   COMPARE(ch, n, ==) //equal
	#define NE(ch, n)   COMPARE(ch, n, !=) //not equal
	#define GTET(ch, n) COMPARE(ch, n, >=) //greater than or equal to
	#define LTET(ch, n) COMPARE(ch, n, <=) //less than or equal to

	#define isNULL(ch)        ( EQ(ch, 0) )
	#define notNULL(ch)       ( NE(ch, 0) )
	#define isCTRL(ch)        ( ( GTET(ch, CTRL_A) && LTET(ch, CTRL_Z) ) || EQ(ch, CTRL_BACKSLASH) )
	#define isDELETE_CHAR(ch) ( EQ(ch, DELETE_CHAR) )
	#define isALT(ch)         ( GTET(ch, ALT_A) && LTET(ch, ALT_Z) )
	#define isSet1(ch)        ( ( GTET(ch, UP) && LTET(ch, LEFT) ) || EQ(ch, END) || EQ(ch, HOME) )
	#define isSet2(ch)        ( EQ(ch, INSERT) || EQ(ch, DELETE) || EQ(ch, PAGEUP) || EQ(ch, PAGEDOWN) )
	#define isSet3(ch)        ( GTET(ch, ALT_A) && LTET(ch, ALT_Z) )
	#define isSet4(ch)        ( EQ(ch, SHIFT_RIGHT) || EQ(ch, SHIFT_LEFT) )
	#define isSet5(ch)        ( GTET(ch, CTRL_UP) && LTET(ch, CTRL_LEFT) )
	*/

	#define COMPARE(ch, n, op) ch op char(n)
	auto EQ   = [](char ch, Key k) { return COMPARE(ch, k, ==); };
	auto NE   = [](char ch, Key k) { return COMPARE(ch, k, !=); };
	auto GTET = [](char ch, Key k) { return COMPARE(ch, k, >=); };
	auto LTET = [](char ch, Key k) { return COMPARE(ch, k, <=); };
	#undef COMPARE

	auto isNULL        = [=](char ch) { return EQ(ch, ZERO); };
	auto notNULL       = [=](char ch) { return NE(ch, ZERO); };
	auto isCTRL        = [=](char ch) { return (GTET(ch, CTRL_A) && LTET(ch, CTRL_Z)) || EQ(ch, CTRL_BACKSLASH); };
	auto isDELETE_CHAR = [=](char ch) { return EQ(ch, DELETE_CHAR); };
	auto isALT         = [=](char ch) { return GTET(ch, ALT_A) && LTET(ch, ALT_Z); };
	auto isSet1        = [=](char ch) { return (GTET(ch, UP) && LTET(ch, LEFT)) || EQ(ch, END) || EQ(ch, HOME); };
	auto isSet2        = [=](char ch) { return EQ(ch, INSERT) || EQ(ch, DELETE) || EQ(ch, PAGEUP) || EQ(ch, PAGEDOWN); };
	auto isSet3        = [=](char ch) { return isALT(ch); };
	auto isSet4        = [=](char ch) { return EQ(ch, SHIFT_RIGHT) || EQ(ch, SHIFT_LEFT); };
	auto isSet5        = [=](char ch) { return GTET(ch, CTRL_UP) && LTET(ch, CTRL_LEFT); };

	auto complete = [this, isNULL]() { return !enable || isNULL(getOneChar()); };

	if(char ch1 = getOneChar(false); isCTRL(ch1) || isDELETE_CHAR(ch1) || isPrint(ch1)) //BEEP, TAB, NEWLINE, CARRIAGE_RETURN, BACKSPACE has already been in CTRLs
		{ if(complete()) return Key(ch1); }
	else if(EQ(ch1, ESC))
	{
		if(char ch2 = getOneChar(); isNULL(ch2))
			return Key(ESC);
		else
		{
			if(EQ(ch2, CHECK_KEY_1))
			{
				if(char ch3 = getOneChar(); notNULL(ch3))
				{
					if(isSet1(ch3))
					{
						if(complete())
							return Key(int(ch3) + OFFSET_1);
					}
					else if(isSet2(ch3))
					{
						if(char ch4 = getOneChar(); notNULL(ch4) && EQ(ch4, CHECK_KEY_2) && complete())
							return Key(int(ch3) + OFFSET_2);
					}
					else if(EQ(ch3, CHECK_KEY_3))
					{
						if(char ch4 = getOneChar(); notNULL(ch4))
							if(EQ(ch4, CHECK_KEY_4))
							{
								if(char ch5 = getOneChar(); notNULL(ch5))
								{
									if(EQ(ch5, CHECK_KEY_5))
									{
										if(char ch6 = getOneChar(); notNULL(ch6) && isSet4(ch6) && complete())
											return Key(int(ch6) + OFFSET_4);
									}
									else if(EQ(ch5, CHECK_KEY_6))
									{
										if(char ch6 = getOneChar(); notNULL(ch6) && isSet5(ch6) && complete())
											return Key(int(ch6) + OFFSET_5);
									}
								}
							}
					}
				}
			}
			else if(isSet3(ch2) && complete())
				return Key(int(ch2) + OFFSET_3);
		}
	}

	//undefined key
	if(enable) while(getOneChar() != 0);
	//No effect if we use "cin" to get the character
	//But it works while using function "read"
	//tcflush(STDIN_FILENO, TCIFLUSH);
	return UNDEFINED_KEY;
}

#endif
