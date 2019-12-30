/*========================================================================\
|: [Filename] main.cpp                                                   :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Main program                                               :|
<------------------------------------------------------------------------*/

#include "cmdMgr.h"

namespace _54ff
{
	struct MainRegistrar : public CmdRegistrar
	{
		MainRegistrar()
		{
			cmdMgr->setPromptString("ia2b> ");
			cmdMgr->setPromptFgColor(GREEN);
		}
	} mainRegistrar;
}

using namespace _54ff;

void printUsageThenExit(int value = -1)
{
	cout << "Usage: <bin> [-File (string fileName)]" << endl; exit(value);
}

int main(int argc, char* argv[])
{
	if(argc > 1)
	{
		if(myStrNCmp(argv[1], "-File", 2) == MATCH)
		{
			if(argc == 2)
				{ cerr << "[Error] Missing option!" << endl; printUsageThenExit(); }
			if(argc > 3)
				{ cerr << "[Error] Extra option! (" << argv[3] << ")" << endl; printUsageThenExit(); }
			if(!cmdMgr->openDofile(argv[2]))
				printUsageThenExit();
		}
		else if(myStrNCmp(argv[1], "-Help", 2) == MATCH)
		{
			if(argc > 2)
				{ cerr << "[Error] Extra option! (" << argv[2] << ")" << endl; printUsageThenExit(); }
			printUsageThenExit(0);
		}
		else
			{ cerr << "[Error] Illegal option! (" << argv[1] << ")" << endl; printUsageThenExit(); }
	}

	cmdMgr->execute();

	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-value"
	return "hehe", 0;
	#pragma GCC diagnostic pop
}
