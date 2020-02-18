/*========================================================================\
|: [Filename] cmdCommon.cpp                                              :|
:| [Author]   Chung-Yang (Ric) Huang, Chiang Chun-Yi                     |:
|: [Synopsis] 1. Implement the interface for command execution           :|
:|            2. Define and implement some common commands               |:
<------------------------------------------------------------------------*/

#include "cmdMgr.h"

namespace _54ff
{

const string CmdTypeStr[CMD_TYPE_TOTAL] =
{
	"Common",
	"System",
	"I/O",
	"Display",
	"Synthesis",
	"Verification",
	"Experiment"
};

void
CmdExec::printUsage()const
{
	const char* usagePrompt = "Usage: ";
	constexpr size_t usageLen = 7;
	const char* separation = " ";
	constexpr size_t sepLen = 1;
	cout << usagePrompt << getCmdStr() << separation;
	printWithPadding(cmdMgr->getCurWinWidth(), usageLen+getTotalLen()+sepLen, getUsageStr());
}

void
CmdExec::printHelp(bool original)const
{
	if(original) cout                                                         << getCmdStr();
	else         cout << left << setw(cmdMgr->getMaxCmdLen()) << setfill(' ') << getCmdStr();
	const char* separation = " : ";
	constexpr size_t sepLen = 3;
	cout << separation;
	printWithPadding(cmdMgr->getCurWinWidth(),
	                 (original ? getTotalLen() : cmdMgr->getMaxCmdLen())+sepLen, getHelpStr());
}

CmdExecStatus
CmdExec::errorOption(CmdOptionError err, const char* opt)
{
	switch(err)
	{
		case CMD_OPT_MISSING:
			cerr << "[Error] Missing option!"; if(opt) cerr << " (" << opt << ")"; cerr << endl; break;
		case CMD_OPT_EXTRA: cerr << "[Error] Extra option! (" << opt << ")" << endl; break;
		case CMD_OPT_ILLEGAL: cerr << "[Error] Illegal option! (" << opt << ")" << endl; break;
		case CMD_OPT_FOPEN_FAIL: cerr << "[Error] Cannot open file \"" << opt << "\"!" << endl; break;
		case CMD_OPT_INVALID_INT:
			cerr << "[Error] \"" << opt << "\" is not a valid interger!" << endl; break;
		case CMD_OPT_INVALID_UINT:
			cerr << "[Error] \"" << opt << "\" is not a valid unsigned interger!" << endl; break;
	}
	return CMD_EXEC_ERROR_EXT;
}

RegMsg regMsg;

bool
OptListBase::checkValidity(const char* optStr, size_t matchNum)
{
	/* Criteria
	1. optStr should contain at least one '-' and the other alphabet(s)
	2. The length of optStr should be greater than or equal to matchNum
	3. The part within matchNum should be uppercase; otherwise, lowercase
	*/
	if(matchNum <= 1)
		{ regMsg() << "[Error] For option \"" << optStr << "\", the matching Number ("
		           << matchNum << ") should be at least 2!" << endl; return false; }
	if(optStr[0] == '\0' || optStr[0] != '-')
		{ regMsg() << "[Error] For option \"" << optStr << "\""
		              ", the mandatory part should contain one \"-\" tailing with alphabet(s)!" << endl; return false; }
	for(size_t i = 1; i < matchNum; ++i)
		if(optStr[i] == '\0')
			{ regMsg() << "[Error] The length of \"" << optStr << "\" is smaller than " << matchNum << "!" << endl; return false; }
		else if(!isalpha(optStr[i]))
			{ regMsg() << "[Error] Non alphabet at position " << i << " of \"" << optStr << "\"!" << endl; return false; }
		else if(!isupper(optStr[i]))
			{ regMsg() << "[Error] A lowercase at position " << i << " of \"" << optStr << "\" in its mandatory part!" << endl; return false; }
	for(size_t i = matchNum; optStr[i] != '\0'; ++i)
		if(!isalpha(optStr[i]))
			{ regMsg() << "[Error] Non alphabet at position " << i << " of \"" << optStr << "\"!" << endl; return false; }
		else if(!islower(optStr[i]))
			{ regMsg() << "[Error] An uppercase at position " << i << " of \"" << optStr << "\" in its optional part!" << endl; return false; }
	return true;
}

bool
OptListBase::checkAmbiguity(const char* optStr1, size_t matchNum1,
                            const char* optStr2, size_t matchNum2)
{
	if(matchNum1 > matchNum2)
	{
		union { const char* tmpS; size_t tmpN; };
		tmpS = optStr1; optStr1 = optStr2; optStr2 = tmpS;
		tmpN = matchNum1; matchNum1 = matchNum2; matchNum2 = tmpN;
	}

	/*
	# equal position can be handled as well
	# matchNum1 <= matchNum2
	# matchNum1 <= end1
	# matchNum2 <= end2
	# Strong ambiguity -> Weak ambiguity -> Non ambiguity
	========================================================================
	# cases of position
	------------------------------------------------------------------------
	case 1.          matchNum1  end1  matchNum2  end2
	        optStr1      |       |
	        optStr2                       |       |
	------------------------------------------------------------------------
	case 2.          matchNum1  matchNum2  end1  end2
	        optStr1      |                  |
	        optStr2                 |             |
	------------------------------------------------------------------------
	case 3.          matchNum1  matchNum2  end2  end1
	        optStr1      |                        |
	        optStr2                 |       |
	========================================================================
	# cases of ambiguity
	------------------------------------------------------------------------
	case 1. Inequality                  before matchNum1 -> Non    ambiguity
	case 2. Inequality or reaching end1 before matchNum2 -> Weak   ambiguity -> match1 but not match2
	case 3. The rest                                     -> Strong ambiguity -> match1 and     match2
	*/

	for(size_t i = 0; i < matchNum1; ++i)
		if(optStr1[i] != optStr2[i])
			return true;

	/*====================================*/

	auto repAmbiguity = [=](bool type, size_t commLen)
	{
		auto setToRed = [] { regMsg() << "\033[31m"; };
		auto reset    = [] { regMsg() << "\033[0m"; };
		auto printOpt = [=](const char* optStr, size_t matchNum)->stringstream&
		{
			size_t i = 0;
			if(commLen > matchNum)
			{
				setToRed();
				for(; i < matchNum; ++i) regMsg() << char(toupper(optStr[i]));
				for(; i < commLen; ++i) regMsg() << optStr[i];
				reset();
			}
			else
			{
				setToRed();
				for(; i < commLen; ++i) regMsg() << char(toupper(optStr[i]));
				reset();
				for(; i < matchNum; ++i) regMsg() << char(toupper(optStr[i]));
			}
			for(; optStr[i] != '\0'; ++i) regMsg() << optStr[i];
			return regMsg();
		};
		auto printOpt1 = [=]()->stringstream& { return printOpt(optStr1, matchNum1); };
		auto printOpt2 = [=]()->stringstream& { return printOpt(optStr2, matchNum2); };

		/*====================================*/

		regMsg() << "[Error] " << (type ? "Strong" : "Weak") << " ambiguity between two options! "
		            "{\""; printOpt1() << "\", \""; printOpt2() << "\"}" << endl;
		return false;
	};

	/*====================================*/

	for(size_t i = matchNum1; i < matchNum2; ++i)
		//the case of "optStr1[i] == '\0'" is implicitly included in the condition
		//since optStr2[i] cannot be '\0' here
		if(optStr1[i] != optStr2[i])
			return repAmbiguity(false, matchNum1);
	return repAmbiguity(true, matchNum2);
}

/************************************************************************/
/************************************************************************/
/************************************************************************/
/************************************************************************/
/************************************************************************/
/************************* Some Common Commands *************************/
/************************************************************************/
/************************************************************************/
/************************************************************************/
/************************************************************************/
/************************************************************************/
/************************************************************************/

CmdClass           (Dofile,  CMD_TYPE_COMMON, 0);
CmdClass           (Quit,    CMD_TYPE_COMMON, 1, "-Force",       2);
CmdClass           (Time,    CMD_TYPE_COMMON, 0);
CmdClass           (History, CMD_TYPE_COMMON, 0);
CmdClass           (Info,    CMD_TYPE_COMMON, 0);
CmdClass           (Prev,    CMD_TYPE_COMMON, 0);
CmdClassCustomMatch(Help,    CMD_TYPE_COMMON, 3, "-Flattened",   2,
                                                 "-Categorized", 2,
                                                 "-Description", 2);

CmdClass(SetKeyboard, CMD_TYPE_SYSTEM, 3, "-Normal",  2,
                                          "-Special", 2,
                                          "-Get",     2);

struct CommonRegistrar : public CmdRegistrar
{
	CommonRegistrar()
	{
		setFile();
		setLine(); cmdMgr->regCmd< DofileCmd>("DOfile",      2);
		setLine(); cmdMgr->regCmd<   QuitCmd>("Quit",        1);
		setLine(); cmdMgr->regCmd<   TimeCmd>("TIMe",        3);
		setLine(); cmdMgr->regCmd<HistoryCmd>("HIStory",     3);
		setLine(); cmdMgr->regCmd<   InfoCmd>("INFOrmation", 4);
		setLine(); cmdMgr->regCmd<   PrevCmd>("PREVious",    4);
		setLine(); cmdMgr->regCmd<   HelpCmd>("HELp",        3);

		setLine(); cmdMgr->regCmd<SetKeyboardCmd>("SET KEyboard", 3, 2);
	}
} static commonRegistrar;

/*========================================================================
	DOfile <(string fileName)>
========================================================================*/

CmdExecStatus
DofileCmd::exec(char* options)const
{
	PureStrList tokens = breakToTokens(options);
	if(tokens.size() < 1)
		return errorOption(CMD_OPT_MISSING);
	if(tokens.size() > 1)
		return errorOption(CMD_OPT_EXTRA, tokens[1]);
	WrapStr s(replaceHomeDir(tokens[0]), false);
	const char* dofileName = (const char*)s ? (const char*)s : tokens[0];
	return cmdMgr->openDofile(dofileName) ? CMD_EXEC_DONE : CMD_EXEC_ERROR_INT;
}

const char*
DofileCmd::getUsageStr()const
{
	return "<(string fileName)>\n";
}

const char*
DofileCmd::getHelpStr()const
{
	return "Read commands from the dofile\n";
}

/*========================================================================
	Quit [-Force]
--------------------------------------------------------------------------
	0: -Force, 2
========================================================================*/

CmdExecStatus
QuitCmd::exec(char* options)const
{
	bool force = false;
	for(const char* token: breakToTokens(options))
		if(optMatch<0>(token))
			{ if(force) return errorOption(CMD_OPT_EXTRA, token); force = true; }
		else return errorOption(CMD_OPT_ILLEGAL, token);

	if(force || !isatty(STDIN_FILENO))
		return CMD_EXEC_QUIT;
	else
	{
		const char* askToQuit = "Do you want to quit? [";
		constexpr size_t numChoices = 2;
		const char* yesOrNo[numChoices] = { "Yes", "No]" };
		return Menu(askToQuit, yesOrNo, numChoices) == 1 ? CMD_EXEC_DONE : CMD_EXEC_QUIT;
	}
}

const char*
QuitCmd::getUsageStr()const
{
	return "[-Force]\n";
}

const char*
QuitCmd::getHelpStr()const
{
	return "Terminate the execution\n";
}

/*========================================================================
	TIMe
========================================================================*/

CmdExecStatus
TimeCmd::exec(char* options)const
{
	if(PureStrList tokens = breakToTokens(options);
	   tokens.size() > 0)
		return errorOption(CMD_OPT_ILLEGAL, tokens[0]);

	timer.printTime();
	return CMD_EXEC_DONE;
}

const char*
TimeCmd::getUsageStr()const
{
	return "\n";
}

const char*
TimeCmd::getHelpStr()const
{
	return "Get the runtime statistics of the current process\n";
}

/*========================================================================
	HIStory [(unsigned numFromBot)]
========================================================================*/

CmdExecStatus
HistoryCmd::exec(char* options)const
{
	PureStrList tokens = breakToTokens(options);
	size_t numFromBot = cmdMgr->getHisNum();
	if(tokens.size() >= 1)
	{
		if(!myStrToUInt(tokens[0], numFromBot))
			return errorOption(CMD_OPT_INVALID_UINT, tokens[0]);
		if(tokens.size() > 1)
			return errorOption(CMD_OPT_EXTRA, tokens[1]);
	}

	cmdMgr->printHistory(numFromBot);
	return CMD_EXEC_DONE;
}

const char*
HistoryCmd::getUsageStr()const
{
	return "[(unsigned numFromBot)]\n";
}

const char*
HistoryCmd::getHelpStr()const
{
	return "Print previous commands in the history list\n";
}

/*========================================================================
	INFOrmation
========================================================================*/

CmdExecStatus
InfoCmd::exec(char* options)const
{
	if(PureStrList tokens = breakToTokens(options);
	   tokens.size() > 0)
		return errorOption(CMD_OPT_ILLEGAL, tokens[0]);

	getInfoAboutCurProc();
	return CMD_EXEC_DONE;
}

const char*
InfoCmd::getUsageStr()const
{
	return "\n";
}

const char*
InfoCmd::getHelpStr()const
{
	return "Get some information about the current process\n";
}

/*========================================================================
	PREVious
========================================================================*/

CmdExecStatus
PrevCmd::exec(char* options)const
{
	if(PureStrList tokens = breakToTokens(options);
	   tokens.size() > 0)
		return errorOption(CMD_OPT_ILLEGAL, tokens[0]);

	const string statStr[CMD_EXEC_TOTAL] =
		{ "Done", "Execution Error", "Parsing Error", "Quit", "No Operation" };
	cout << statStr[cmdMgr->getPrevResult()] << "!" << endl;
	return CMD_EXEC_DONE;
}

const char*
PrevCmd::getUsageStr()const
{
	return "\n";
}

const char*
PrevCmd::getHelpStr()const
{
	return "Get the execution status of the previous command\n";
}

/*========================================================================
	HELp [(strings cmd) [-Description] | -Flattened | -Categorized]
--------------------------------------------------------------------------
	0: -Flattened,   2
	1: -Categorized, 2
	2: -Description, 2
========================================================================*/

CmdExecStatus
HelpCmd::exec(char* options)const
{
	enum { NONE, FLATTENED, CATEGORIZED } type = NONE;
	switch(*options)
	{
		case '\0':
			type = FLATTENED;
			break;

		case '-':
			for(const char* token: breakToTokens(options))
				if(optMatch<0>(token))
					{ if(type != NONE) return errorOption(CMD_OPT_EXTRA, token); type = FLATTENED; }
				else if(optMatch<1>(token))
					{ if(type != NONE) return errorOption(CMD_OPT_EXTRA, token); type = CATEGORIZED; }
				else return errorOption(CMD_OPT_ILLEGAL, token);
			break;

		default:
		{
			char* tmp = options;
			if(CmdExec* e = cmdMgr->cmdMap[options]; e != 0)
			{
				bool description = false;
				for(const char* token: breakToTokens(options))
					if(optMatch<0>(token) || optMatch<1>(token))
						return errorOption(CMD_OPT_EXTRA, token);
					else if(optMatch<2>(token))
						{ if(description) return errorOption(CMD_OPT_EXTRA, token); description = true; }
					else return errorOption(CMD_OPT_ILLEGAL, token);
				description ? e->printHelp(true) : e->printUsage();
				return CMD_EXEC_DONE;
			}
			else return errorOption(CMD_OPT_ILLEGAL, tmp);
		} break;
	}

	cmdMgr->printHelps(type == FLATTENED);
	return CMD_EXEC_DONE;
}

const char*
HelpCmd::getUsageStr()const
{
	return "[(strings cmd) [-Description] | -Flattened | -Categorized]\n";
}

const char*
HelpCmd::getHelpStr()const
{
	return "Print help message for various commands\n";
}

WrapStrList
HelpCmd::listThing(char*& options)const
{
	WrapStrList strList;
	if(*options == '-' ||
	   cmdMgr->cmdMap.completeCmd(options, strList) != 0)
	{
		char* tmp = findLastToken(options);
		if(*tmp == '-')
			{ strList = matchOptions(tmp); strList.emplace_back(tmp); }
	}
	return strList;
}

/*========================================================================
	SET KEyboard <-Normal | -Special | -Get>
--------------------------------------------------------------------------
	0: -Normal,  2
	1: -Special, 2
	2: -Get,     2
========================================================================*/

CmdExecStatus
SetKeyboardCmd::exec(char* options)const
{
	enum { NONE, NORMAL, SPECIAL, GET } type = NONE;
	for(const char* token: breakToTokens(options))
		if(optMatch<0>(token))
		{
			if(type != NONE)
				return errorOption(CMD_OPT_EXTRA, token);
			type = NORMAL;
		}
		else if(optMatch<1>(token))
		{
			if(type != NONE)
				return errorOption(CMD_OPT_EXTRA, token);
			type = SPECIAL;
		}
		else if(optMatch<2>(token))
		{
			if(type != NONE)
				return errorOption(CMD_OPT_EXTRA, token);
			type = GET;
		}
		else return errorOption(CMD_OPT_ILLEGAL, token);

	const string comboName[] = { "normal", "special" };
	switch(type)
	{
		case NONE: return errorOption(CMD_OPT_MISSING);
		case GET:
			cout << "Current combo type is " << comboName[enabled()] << endl;
			break;

		default:
			cout << (changeEnable(type == SPECIAL) ? "Change combo type to be "
			                                       : "The combo type is already ")
			     << comboName[enabled()] << endl;
			break;
	}
	return CMD_EXEC_DONE;
}

const char*
SetKeyboardCmd::getUsageStr()const
{
	return "<-Normal | -Special | -Get>\n";
}

const char*
SetKeyboardCmd::getHelpStr()const
{
	return "Set the type of keyboard combo\n";
}

}
