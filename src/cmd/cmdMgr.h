/*========================================================================\
|: [Filename] cmdMgr.h                                                   :|
:| [Author]   Chung-Yang (Ric) Huang, Chiang Chun-Yi                     |:
|: [Synopsis] Define the interface for the command line                  :|
<------------------------------------------------------------------------*/

#ifndef HEHE_CMDMGR_H
#define HEHE_CMDMGR_H

#include <sys/ioctl.h>
#include <fcntl.h>
#include <assert.h>
#include <signal.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>
#include <stack>
#include <utility>
#include <type_traits>
#include "cmdCharDef.h"
#include "cmdExec.h"
#include "util.h"
using namespace std;

namespace _54ff
{

constexpr int RESET      = 0;
constexpr int HIGHLIGHT  = 7;
constexpr int UNDERSCORE = 4;
enum CmdColor { BLACK = 0, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE, RESETCOLOR };

enum reOpenStatus { NORMAL, FAIL, OUT_OF_RANGE };
class fileWithPos
{
friend ostream& operator<<(ostream& os, const fileWithPos& f) { return os << f.fileName; }

public:
	fileWithPos(const char* n): fileName(copyCStr(n)) {}
	~fileWithPos() { delete []fileName; }

	fileWithPos(const fileWithPos&) = delete;
	fileWithPos(fileWithPos&& fwp) noexcept //for stack, rule of 5
	: fileName   (fwp.fileName)
	, posToStart (::move(fwp.posToStart)) { fwp.fileName = 0; }

	fileWithPos& operator=(const fileWithPos&) = delete;
	fileWithPos& operator=(fileWithPos&& fwp) noexcept //for stack, rule of 5
	{
		fileName = fwp.fileName; fwp.fileName = 0;
		posToStart = ::move(fwp.posToStart); return *this;
	}

	void setPosToStart(ifstream& f) { posToStart = f.tellg(); }
	reOpenStatus reOpenFile(ifstream& f);

private:
	const char*  fileName;
	streampos    posToStart;
};

class CmdMgr
{
friend class HelpCmd;

static constexpr size_t  READ_BUFFER_SIZE = 16384;
static constexpr size_t  COPY_BUFFER_SIZE =  8192;
static constexpr size_t  LIST_BUFFER_SIZE =  8192;
static constexpr size_t         PG_OFFSET =    10;
static constexpr size_t MAX_DOFILE_NUMBER =  2048;

using handler = void(*)(int);
using oldDofileStack = stack<fileWithPos>;

public:
	CmdMgr(): prompt("cmd> "), pColor{-1, -1} { curDofile.unsetf(ios::skipws); }

	/*====================================*/

	template <class CmdExecChild, typename... MandLens>
	void regCmd(const char* cmdStr, MandLens... mandLens)
	{
		static_assert(is_convertible_v<CmdExecChild*, CmdExec*>, "CmdExecChild should be publicly inherited from CmdExec!");
		constexpr size_t mandNum = sizeof...(mandLens); Array<size_t> mandLenList(mandNum);
		fillInMandLen(0, mandLenList, mandLens...);
		try { cmdMap.regCmd(new CmdExecChild(cmdStr), mandLenList, mandNum); }
		catch(RegError re) { regMsg.printStatus(); cerr << regMsg.getStr(); exit(-1); }
		if(regMsg.hasWarn()) regMsg.printWarn();
	}

	void setPromptString(const string& _prompt) { prompt = _prompt; }
	void setPromptFgColor(CmdColor fg = RESETCOLOR) { pColor[0] = ( fg == RESETCOLOR ? -1 : FG(fg) ); }
	void setPromptBgColor(CmdColor bg = RESETCOLOR) { pColor[1] = ( bg == RESETCOLOR ? -1 : BG(bg) ); }
	void execute();

	bool openDofile(const char*);

	size_t getCurWinWidth()const { return curWinSize.ws_col; }
	size_t getHisNum()const { return history.size(); }
	size_t getMaxCmdLen()const { return cmdMap.getMaxCmdLen(); }
	CmdExecStatus getPrevResult()const { return prevExecResult; }
	void printHelps(bool)const;
	void printHistory(size_t)const;

	/*====================================*/

	static CmdMgr* getCurCmdMgr() { return curCmdMgr; }

private:
	class CharArr
	{
	public:
		CharArr(): arr(new char[LIST_BUFFER_SIZE]) {}
		~CharArr() { delete []arr; }

		char* begin()const { return arr; }
		char*& workPlace() { return tmp = arr; }

	private:
		char* const  arr;
		char*        tmp;
	};

private:
	template <typename... MandLens>
	void fillInMandLen(size_t i, Array<size_t>& mandLenList, size_t l, MandLens... mandLens)
		{ mandLenList[i] = l; fillInMandLen(i+1, mandLenList, mandLens...); }
	void fillInMandLen(size_t i, Array<size_t>& mandLenList, size_t l)
		{ mandLenList[i] = l; }

	/*====================================*/

	void printPrompt()const;
	void addDummyToHistory() { history.emplace_back(); }
	void updateCurChar()const { cout << *cursor << char(BACKSPACE); }
	bool atLineBeg()const { return cursor == cmdBeg; }
	bool atLineEnd()const { return cursor == cmdEnd; }
	bool notAtLineBeg()const { return cursor != cmdBeg; }
	bool notAtLineEnd()const { return cursor != cmdEnd; }
	bool isHLBeginAtCursor()const { return hlBegin == cursor; }
	bool isHLBeginAfterCursor()const { return hlBegin > cursor; }
	bool isHLBeginBeforeCursor()const { return hlBegin < cursor; }
	void setHLBegin() { hlBegin = cursor; }
	void resetHLBegin() { hlBegin = 0; }
	bool isHLBeginSet()const { return hlBegin != 0; }
	void resetCopyBook() { copyBookEnd = copyBook; }
	bool contentInCopy()const { return copyBookEnd != copyBook; }
	bool contentInCmdLine()const { return cmdEnd != cmdBeg; }
	size_t charNumInCmdLine()const { return strLen(cmdBeg, cmdEnd); }
	size_t totalCharInCmdLine()const { return prompt.size() + charNumInCmdLine(); }

	/*====================================*/

	CmdExecStatus execOneCmd() { return isDofileOpen() ? execOneCmd_byFile() : execOneCmd_byHand(); }

	// If the cursor is at the end of one line, somehow it will be stuck at there and does not go to a new line
	// Workaround now is to print the character and go back, be careful if new feature is added
	CmdExecStatus execOneCmd_byHand();
	void prepareForCmd_byHand();
	void backTo       (const char* finalCursor) { for(; cursor > finalCursor; --cursor) cout << char(BACKSPACE); }
	void goTo         (const char* finalCursor) { for(; cursor < finalCursor; ++cursor) cout << *cursor; updateCurChar(); }
	void goToWithHL   (const char* finalCursor) { setProperty(HIGHLIGHT); for(; cursor < finalCursor; ++cursor) cout << *cursor; setProperty(RESET); }
	void goToWithSpace(const char* finalCursor) { for(; cursor < finalCursor; ++cursor) cout << char(SPACE); }
	void setUnderScore()const { setProperty(HIGHLIGHT); setProperty(UNDERSCORE); updateCurChar(); setProperty(RESET); }
	void insertStr(const char*, size_t = 1);
	void insertStr(const char c) { insertStr(&c); }
	void insertStr(const char* begin, const char* end) { insertStr(begin, strLen(begin, end)); }
	void insertStr(const string& str) { insertStr(str.c_str(), str.size()); }
	void deleteStr(char*, char*);
	void clearAllHL(bool);
	void deleteAllHL() { isHLBeginAfterCursor() ? deleteStr(cursor, hlBegin) : deleteStr(hlBegin, cursor); }
	void deleteBack () { if(isHLBeginSet()) deleteAllHL(); else if(notAtLineBeg()) deleteStr(cursor - 1, cursor    ); }
	void deleteFront() { if(isHLBeginSet()) deleteAllHL(); else if(notAtLineEnd()) deleteStr(cursor    , cursor + 1); }
	void moveToRhtWithHL(const char*);
	void moveToLftWithHL(const char*);
	void moveToRhtOneStep() { if(isHLBeginSet()) clearAllHL(hlBegin < cursor); else if(notAtLineEnd())   goTo(cursor + 1); }
	void moveToLftOneStep() { if(isHLBeginSet()) clearAllHL(hlBegin > cursor); else if(notAtLineBeg()) backTo(cursor - 1); }
	void moveToRhtWithHLOneStep() { if(notAtLineEnd()) moveToRhtWithHL(cursor + 1); }
	void moveToLftWithHLOneStep() { if(notAtLineBeg()) moveToLftWithHL(cursor - 1); }
	void moveToRhtToSide() { if(isHLBeginSet()) clearAllHL(hlBegin < cursor);   goTo(cmdEnd); }
	void moveToLftToSide() { if(isHLBeginSet()) clearAllHL(hlBegin > cursor); backTo(cmdBeg); }
	void moveToRhtWithHLToSide() { if(notAtLineEnd()) moveToRhtWithHL(cmdEnd); }
	void moveToLftWithHLToSide() { if(notAtLineBeg()) moveToLftWithHL(cmdBeg); }

	void selectAll() { if(contentInCmdLine()) { backTo(hlBegin = cmdBeg); goToWithHL(cmdEnd); setUnderScore(); } }
	void cut() { if(isHLBeginSet()) { copyHLToBook(); deleteAllHL(); } }
	void copy() { if(isHLBeginSet()) copyHLToBook(); }
	void copyHLToBook();
	void paste() { if(contentInCopy()) insertStr(copyBook, copyBookEnd); }

	void storeCursorHlBegin(bool);
	void moveToTheLast(bool = true);
	void reprintCmd();

	void retrieveHistory(size_t);
	CmdExecStatus popHistory() { history.pop_back(); return CMD_EXEC_NOP; }
	CmdExecStatus pressInputEnd() { moveToTheLast(false); setToOldHandler(); return popHistory(); }
	CmdExecStatus pressEnter() { moveToTheLast(false); setToOldHandler(); return addHistoryAndExecCmd(); }
	CmdExecStatus pressESC() { moveToTheLast(); setToOldHandler(); return CMD_EXEC_QUIT; }

	/*====================================*/

	CmdExecStatus execOneCmd_byFile();
	void prepareForCmd_byFile() { printPrompt(); cmdEnd = cmdBeg; addDummyToHistory(); }
	void closeDofileInt() { curDofile.close(); }
	void closeDofile() { closeDofileInt(); oldDofiles.pop(); popDofile(); }
	bool pushDofile();
	void popDofile();
	bool isDofileOpenInt() { return curDofile.is_open(); }
	bool isDofileOpen();

	/*====================================*/

	char* findFirstNotSpace()const { char* tmp = const_cast<char*>(cmdBeg); for(; tmp < cmdEnd && *tmp == char(SPACE); ++tmp); return tmp; }
	void storeCurCmdToHistory() { history.back().assign(cmdBeg, charNumInCmdLine()); }
	CmdExecStatus addHistoryAndExecCmd();
	CmdExecStatus parseAndExecCmd(char*&);

	static CharArr listCmdArea;
	void copyToList()const;
	void printUsage();
	void pressTab(bool multiTab) { printMatchStr(findMatchStr(), multiTab); }
	WrapStrList findMatchStr()const;
	void printMatchStr(const WrapStrList&, bool);

	/*====================================*/

	static CmdMgr* curCmdMgr;
	static void setCurCmdMgr(CmdMgr* newCmdMgr) { curCmdMgr = newCmdMgr; }
	static void new_handler_ALL(int);

	void setToOldHandler();
	void setToNewHandler();
	void new_handler_INT  () { moveToTheLast(); cout << "Receiving interruption signal! Exit the process now..." << endl; exit(-1); }
	void new_handler_TSTP () { storeCursorHlBegin(false); moveToTheLast(false); raise(SIGSTOP); }
	void new_handler_TERM () { moveToTheLast(); signal(SIGTERM, SIG_DFL); raise(SIGTERM); }
	void new_handler_CONT () { if(isWaiting) setToRaw(); storeCursorHlBegin(true); reprintCmd(); getCurWinSize(); }
	void new_handler_QUIT () { moveToTheLast(); signal(SIGQUIT, SIG_DFL); raise(SIGQUIT); }
	void new_handler_WINCH();

	/*====================================*/

	static void setProperty(int p) { cout << "\033[" << p << "m"; }
	static int FG(CmdColor color) { return 30 + int(color); }
	static int BG(CmdColor color) { return 40 + int(color); }
	void getCurWinSize() { ioctl(STDIN_FILENO, TIOCGWINSZ, &curWinSize); }

private:
	string          prompt;
	int             pColor[2];
	char            cmdBeg[READ_BUFFER_SIZE];
    char*           cursor;
    char*           cmdEnd;
    char*           hlBegin;
    char            copyBook[COPY_BUFFER_SIZE];
    char*           copyBookEnd;
    vector<string>  history;
    size_t          curHisIdx;
	struct winsize  curWinSize;
	ifstream        curDofile;
	oldDofileStack  oldDofiles;
	CmdMap          cmdMap;
	handler         old_handler_INT;
	handler         old_handler_TSTP;
	handler         old_handler_TERM;
	handler         old_handler_CONT;
	handler         old_handler_QUIT;
	handler         old_handler_WINCH;
	CmdExecStatus   prevExecResult;
};

extern CmdMgr* cmdMgr;
class CmdRegistrar
{
protected:
	CmdRegistrar()  { if(niftyCounter++ == 0) cmdMgr = new CmdMgr; }
	~CmdRegistrar() { if(--niftyCounter == 0) delete cmdMgr; }

private:
	static unsigned niftyCounter; //Schwarz Counter
};

}

#endif
