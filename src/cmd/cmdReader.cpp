/*========================================================================\
|: [Filename] cmdReader.cpp                                              :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Implement the command line about getting the text          :|
<------------------------------------------------------------------------*/

#include "cmdMgr.h"

namespace _54ff
{

reOpenStatus
fileWithPos::reOpenFile(ifstream& f)
{
	/*
	If the dofile had been modified during the stacking,
	just open it at the original position, then see what it happens
	1. Read normally -> just read it
	2. Fail to read  -> same response as eof, just treat it as eof
	3. Fail to open  -> prompt a warning, then keep going
	*/
	assert(!f.is_open());
	if(f.open(fileName); f.is_open())
	{
		f.seekg(posToStart);
		if(f.fail())
			{ f.close(); return OUT_OF_RANGE; }
		return NORMAL;
	}
	else return FAIL;
}

CmdMgr* CmdMgr::curCmdMgr = 0;

void
CmdMgr::execute()
{
	CmdMgr* oldCmdMgr = getCurCmdMgr(); setCurCmdMgr(this);
	resetCopyBook();
	while((prevExecResult = execOneCmd()) != CMD_EXEC_QUIT)
		cout << '\n';
	history.clear();
	setCurCmdMgr(oldCmdMgr);
}

void
CmdMgr::printPrompt()const
{
	if(isatty(STDOUT_FILENO))
	{
		if(pColor[0] != -1) setProperty(pColor[0]);
		if(pColor[1] != -1) setProperty(pColor[1]);
	}
	cout << prompt;
	if(isatty(STDOUT_FILENO)) setProperty(RESET);
}

void
CmdMgr::prepareForCmd_byHand()
{
//	check();
	printPrompt();
	*(cmdEnd = cursor = cmdBeg) = char(SPACE); resetHLBegin();
	curHisIdx = history.size(); addDummyToHistory();
	setToNewHandler(); getCurWinSize();
}

CmdExecStatus
CmdMgr::execOneCmd_byHand()
{
	prepareForCmd_byHand();
	for(unsigned tabCounter = 1, tabRef = 0; true; ++tabCounter)
		switch(Key curKeyPress = returnExactKey(); curKeyPress)
		{
			default: if(isPrint(curKeyPress)) insertStr(curKeyPress); break;
			case ENTER          : setToOld(); return pressEnter();
			case UP             : retrieveHistory(curHisIdx-1); break;
			case DOWN           : retrieveHistory(curHisIdx+1); break;
			case RIGHT          : moveToRhtOneStep(); break;
			case LEFT           : moveToLftOneStep(); break;
			case BACKSPACE      : deleteBack (); break;
			case DELETE         : deleteFront(); break;
			case SHIFT_RIGHT    : moveToRhtWithHLOneStep(); break;
			case SHIFT_LEFT     : moveToLftWithHLOneStep(); break;
			case CTRL_E         :
			case END            : moveToRhtToSide(); break;
			case CTRL_B         :
			case HOME           : moveToLftToSide(); break;
			case CTRL_RIGHT     : moveToRhtWithHLToSide(); break;
			case CTRL_LEFT      : moveToLftWithHLToSide(); break;
			case PAGEUP         : retrieveHistory(curHisIdx - PG_OFFSET); break;
			case PAGEDOWN       : retrieveHistory(curHisIdx + PG_OFFSET); break;
			case ESC            : setToOld(); return pressESC();
			case TAB            : setToOld(); pressTab(tabCounter == tabRef); tabRef = tabCounter + 1; break;
			case CTRL_A         : selectAll(); break;
			case CTRL_X         : cut();       break;
			case CTRL_C         : copy();      break;
			case CTRL_V         : paste();     break;
			case CTRL_Z         : setToOld(); raise(SIGTSTP); break;
			case CTRL_Q         : setToOld(); raise(SIGINT ); break;
			case CTRL_BACKSLASH : setToOld(); raise(SIGQUIT); break;
			case CTRL_D         : setToOld(); return pressInputEnd();
			case CTRL_U         : setToOld(); printUsage(); break;
			case CTRL_K         : flipEnable(); break;
		}
}

void
CmdMgr::insertStr(const char* str, size_t insNum)
{
	if(isHLBeginSet()) deleteAllHL();
	for(char* tmp = cmdEnd; tmp >= cursor; --tmp)
		*(tmp + insNum) = *tmp;
	for(size_t i = 0; i < insNum; ++i)
		cursor[i] = str[i];
	const char* finalCursor = cursor + insNum;
	goTo(cmdEnd+=insNum); backTo(finalCursor);
}

void
CmdMgr::deleteStr(char* beg, char* end)
{
	assert(beg <= cursor);
	for(char *tmp1 = beg, *tmp2 = end; tmp2 <= cmdEnd; ++tmp1, ++tmp2)
		*tmp1 = *tmp2;
	const size_t delNum = strLen(beg, end);
	backTo(beg); goTo(cmdEnd-=delNum); goToWithSpace(cmdEnd+delNum+1); backTo(beg);
	resetHLBegin();
}

void
CmdMgr::clearAllHL(bool stayAtCursor)
{
	char* tmpC = cursor;
	if(isHLBeginAfterCursor()) { goTo(hlBegin); if(stayAtCursor) backTo(tmpC); }
	else { backTo(hlBegin); goTo(tmpC); if(!stayAtCursor) backTo(hlBegin); }
	resetHLBegin();
}

void
CmdMgr::moveToRhtWithHL(const char* finalCursor)
{
	/*
		(1) no hlBegin
		(1).. cursor ..(2).. final(3)Cursor ..(4)
	*/
	assert(!isHLBeginAtCursor());
	assert(finalCursor > cursor);
	enum { _1, _2, _3, _4 } type;
	if(hlBegin == 0) { setHLBegin(); type = _1; }
	else if(hlBegin < cursor) type = _1;
	else if(hlBegin == finalCursor) type = _3;
	else type = hlBegin < finalCursor ? _2 : _4;
	switch(type)
	{
		case _1: goToWithHL(finalCursor); setUnderScore(); break;
		case _2: goTo(hlBegin); goToWithHL(finalCursor); setUnderScore(); break;
		case _3: goTo(finalCursor); resetHLBegin(); break;
		case _4: goTo(finalCursor); setUnderScore(); break;
	}
}

void
CmdMgr::moveToLftWithHL(const char* finalCursor)
{
	/*
		(1).. final(2)Cursor ..(3).. cursor ..(4)
		                           no hlBegin (4)
	*/
	assert(!isHLBeginAtCursor());
	assert(finalCursor < cursor);
	enum { _1, _2, _3, _4 } type; const char* tmp = cursor;
	if(hlBegin == 0) { setHLBegin(); type = _4; }
	else if(hlBegin > cursor) { type = _4; tmp += 1; }
	else if(hlBegin == finalCursor) type = _2;
	else type = hlBegin > finalCursor ? _3 : _1;
	switch(type)
	{
		case _1: backTo(finalCursor); goTo(tmp); backTo(finalCursor); setUnderScore(); break;
		case _2: backTo(finalCursor); goTo(tmp); backTo(finalCursor); resetHLBegin(); break;
		case _3: backTo(finalCursor); goToWithHL(hlBegin); goTo(tmp); backTo(finalCursor); setUnderScore(); break;
		case _4: backTo(finalCursor); goToWithHL(tmp); backTo(finalCursor); setUnderScore(); break;
	}
}

void
CmdMgr::copyHLToBook()
{
	const char* beg;
	const char* end;
	if(isHLBeginAfterCursor()) { beg = cursor; end = hlBegin; }
	else                       { end = cursor; beg = hlBegin; }
	for(resetCopyBook(); beg < end; ++beg, ++copyBookEnd)
		*copyBookEnd = *beg;
}

void
CmdMgr::storeCursorHlBegin(bool restore)
{
	static char* tmpCursor;
	static char* tmpHlBegin;
	if(restore) { cursor = tmpCursor; hlBegin = tmpHlBegin; }
	else        { tmpCursor = cursor; tmpHlBegin = hlBegin; }
}

void
CmdMgr::moveToTheLast(bool lastNewLine)
{
	moveToRhtToSide();
	bool atTerminalLineBegin = totalCharInCmdLine() % getCurWinWidth() == 0;
	if(lastNewLine) { if(!atTerminalLineBegin) cout << char(NEWLINE); }
	else            { if( atTerminalLineBegin) cout << char(BACKSPACE); }
	cout.flush();
}

void
CmdMgr::reprintCmd()
{
	printPrompt();
	char* tmp = cursor;
	cursor = cmdBeg;
	if(isHLBeginSet())
	{
		const char* beg;
		const char* end;
		if(hlBegin > tmp) { beg = tmp; end = hlBegin; }
		else              { end = tmp; beg = hlBegin; }
		goTo(beg); goToWithHL(end); goTo(cmdEnd); backTo(tmp); setUnderScore();
	}
	else
		{ goTo(cmdEnd); backTo(tmp); }
}

void
CmdMgr::retrieveHistory(size_t targetHisIdx)
{
	if((make_signed_t<size_t>)targetHisIdx < 0)
		targetHisIdx = 0;
	if(targetHisIdx >= history.size())
		targetHisIdx = history.size() - 1;
	if(targetHisIdx != curHisIdx)
	{
		if(curHisIdx == history.size() - 1)
			storeCurCmdToHistory();
		if(contentInCmdLine())
			deleteStr(cmdBeg, cmdEnd);
		if(!history[curHisIdx = targetHisIdx].empty())
			insertStr(history[curHisIdx]);
		else assert(targetHisIdx == history.size() - 1);
	}
}

CmdExecStatus
CmdMgr::addHistoryAndExecCmd()
{
	// Semicolon is used to indicate the end of a command, and maybe another command after that
	// If only spaces before the semicolon, issue an error
	if(onlySpaceInLine())
		return popHistory();
	storeCurCmdToHistory(); cout << endl;
	vector<char*> cmds;
	for(char* tmp = const_cast<char*>(cmdBeg); true; ++tmp)
	{
		char* tmp2 = findFirstNotSpace(tmp);
		if(tmp2 == cmdEnd) break;
		tmp = findFirstSemicolon(tmp2);
		if(tmp == tmp2)
		{
			cerr << "[Error] Only spaces before the semicolon!" << endl;
			return CMD_EXEC_NOP;
		}
		cmds.push_back(tmp2);
		*tmp = char(ZERO);
		if(tmp == cmdEnd) break;
	}
	return parseAndExecCmd(cmds);
}

void
CmdMgr::new_handler_ALL(int sig)
{
	switch(sig)
	{
		case SIGINT  : getCurCmdMgr()->new_handler_INT  (); break;
		case SIGTSTP : getCurCmdMgr()->new_handler_TSTP (); break;
		case SIGTERM : getCurCmdMgr()->new_handler_TERM (); break;
		case SIGCONT : getCurCmdMgr()->new_handler_CONT (); break;
		case SIGQUIT : getCurCmdMgr()->new_handler_QUIT (); break;
		case SIGWINCH: getCurCmdMgr()->new_handler_WINCH(); break;
		default: cerr << "[Error] This signal should not be handled by this function!" << endl; exit(-1);
	}
}

void
CmdMgr::setToOldHandler()
{
	signal(SIGINT  , old_handler_INT  );
	signal(SIGTSTP , old_handler_TSTP );
	signal(SIGTERM , old_handler_TERM );
	signal(SIGCONT , old_handler_CONT );
	signal(SIGQUIT , old_handler_QUIT );
	signal(SIGWINCH, old_handler_WINCH);
}

void
CmdMgr::setToNewHandler()
{
	old_handler_INT   = signal(SIGINT  , new_handler_ALL);
	old_handler_TSTP  = signal(SIGTSTP , new_handler_ALL);
	old_handler_TERM  = signal(SIGTERM , new_handler_ALL);
	old_handler_CONT  = signal(SIGCONT , new_handler_ALL);
	old_handler_QUIT  = signal(SIGQUIT , new_handler_ALL);
	old_handler_WINCH = signal(SIGWINCH, new_handler_ALL);
}

void
CmdMgr::new_handler_WINCH()
{
	const size_t oldWidth = getCurWinWidth();
	getCurWinSize();
	const size_t newWidth = getCurWinWidth();
	const size_t totalChar = totalCharInCmdLine() + 1; //for the possible last highlight
	const size_t curPos = prompt.size() + strLen(cmdBeg, cursor);
	//for ubuntu 18.04
	size_t row, col;
	row = curPos / oldWidth;
	col = curPos % oldWidth;
	if(col > newWidth) col = newWidth - 1;
	for(size_t i = 0; i < col; ++i)
		cout << char(BACKSPACE);
	for(size_t i = 0; i < row; ++i)
		for(size_t j = 0; j < newWidth; ++j)
			cout << char(BACKSPACE);
	row = totalChar / oldWidth;
	col = totalChar % oldWidth;
	if(col > newWidth) col = newWidth - 1;
	for(size_t i = 0; i < col; ++i)
		cout << char(SPACE);
	for(size_t i = 0; i < row; ++i)
		for(size_t j = 0; j < newWidth; ++j)
			cout << char(SPACE);
	for(size_t i = 0; i < col; ++i)
		cout << char(BACKSPACE);
	for(size_t i = 0; i < row; ++i)
		for(size_t j = 0; j < newWidth; ++j)
			cout << char(BACKSPACE);
	reprintCmd(); cout.flush();
}

CmdExecStatus
CmdMgr::execOneCmd_byFile()
{
	prepareForCmd_byFile();
	enum { NORMAL, SINGLE_LINE_COMMENT, ESCAPE_THIS_CHAR } readFileStatus = NORMAL;
	unsigned numUnprintable = 0;
	auto newCommand = [&numUnprintable, this]
	{
		if(numUnprintable != 0)
			cerr << "\n[Warning] Observe total " << numUnprintable
			     << " unprintable character(s) before the execution of current command!";
		return addHistoryAndExecCmd();
	};
	istream& dofile = isDofileOpenInt() ? curDofile : cin;
	while(true)
		if(char ch = dofile.get(); dofile.eof())
		{
			if(readFileStatus == ESCAPE_THIS_CHAR)
				cerr << "\n[Warning] Expect a character after the escape character \'\\\'!";
			cerr << "\n[Warning] Expect a non-escaping newline character at the end of file to activate the command!";
			return newCommand();
		}
		else if(dofile.fail())
		{
			cerr << "\n[Error] An accident happens at reading dofile. Stop here!" << endl;
			closeDofile(); return popHistory();
		}
		else
			switch(readFileStatus)
			{
				case SINGLE_LINE_COMMENT: if(ch == char(NEWLINE)) return newCommand(); break;
				case ESCAPE_THIS_CHAR   : if(isPrint(ch)) cout << (*(cmdEnd++) = ch); readFileStatus = NORMAL; break;
				case NORMAL:
					switch(ch)
					{
						case char(NEWLINE)  : return newCommand();
						case char(HASH)     : readFileStatus = SINGLE_LINE_COMMENT; break;
						case char(BACKSLASH): readFileStatus = ESCAPE_THIS_CHAR;    break;
						default:
							if(isPrint(ch))
								cout << (*(cmdEnd++) = ch);
							else numUnprintable += 1;
							break;
					} break;
			}
}

bool
CmdMgr::openDofile(const char* fileName)
{
	if(isDofileOpenInt() && !pushDofile())
		return false;
	if(curDofile.open(fileName); isDofileOpenInt())
		{ oldDofiles.emplace(fileName); return true; }
	else
	{
		cerr << "[Error] Cannot open dofile \"" << fileName << "\"!" << endl;
		popDofile();
		return false;
	}
}

bool
CmdMgr::pushDofile()
{
	if(oldDofiles.size() == MAX_DOFILE_NUMBER)
		{ cerr << "[Error] Too many dofiles (" << MAX_DOFILE_NUMBER
		       << ") are stored in the stack. The new dofile is omitted!" << endl; return false; }
	oldDofiles.top().setPosToStart(curDofile); closeDofileInt(); return true;
}

void
CmdMgr::popDofile()
{
	while(!oldDofiles.empty())
		switch(oldDofiles.top().reOpenFile(curDofile))
		{
			case NORMAL: return;
			case OUT_OF_RANGE:
				oldDofiles.pop(); break;
			case FAIL:
				cerr << "[Warning] Fail to re-open file \"" << oldDofiles.top() << "\"! Just skip it." << endl;
				oldDofiles.pop(); break;
		}
}

bool
CmdMgr::isDofileOpen()
{
	if(isDofileOpenInt())
	{
		if(curDofile.peek(); curDofile.eof())
			{ closeDofile(); return isDofileOpen(); }
		return true;
	}
	if(!isatty(STDIN_FILENO))
	{
		if(cin.peek(); cin.eof())
		{
			if(close(STDIN_FILENO) != 0)
				{ cerr << "Input Error!" << endl; exit(1); }
			if(open("/dev/tty", O_RDWR) != STDIN_FILENO)
				{ cerr << "Input Error!" << endl; exit(1); }
			return false;
		}
		return true;
	}
	return false;
}

}
