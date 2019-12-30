/*========================================================================\
|: [Filename] cmdParser.cpp                                              :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Implement the command line about analyzing the text        :|
<------------------------------------------------------------------------*/

#include <stack>
#include "cmdMgr.h"
using namespace std;

namespace _54ff
{

void
CmdMap::regCmd(CmdExec* e, const Array<size_t>& mandLens, size_t mandNum)
{
	auto failToReg = [e] { delete e; throw RegError(); };
	char* cmdStr = copyCStr(e->getCmdStr());
	WrapStr releaseCmdStr(cmdStr, false);
	vector<char*> cmdTokens;
	for(size_t i = 0; true; ++i)
	{
		auto extraSpace = [e, failToReg](size_t _i)
			{ regMsg() << "[Error] Extra space at column " << _i << " of \"" << e->getCmdStr() << "\"!" << endl; failToReg(); };
		auto emptyCommand = [failToReg]
			{ regMsg() << "[Error] Empty command is illegal!" << endl; failToReg(); };
		if(cmdStr[i] == ' ') extraSpace(i);
		if(cmdStr[i] == '\0') i == 0 ? emptyCommand() : extraSpace(i-1);
		cmdTokens.push_back(&cmdStr[i]);
		for(; cmdStr[i] != '\0' && cmdStr[i] != ' '; ++i);
		if(cmdStr[i] == '\0') break;
		cmdStr[i] = '\0';
	}
	if(cmdTokens.size() != mandNum)
	{
		regMsg() << "[Error] The number of command tokens (" << cmdTokens.size()
		         << ") is different from the given (" << mandNum << ")!" << endl; failToReg();
	}

	CmdNode* node = &cmdTreeRoot;
	constexpr size_t MAX_SIZE_T = numeric_limits<size_t>::max();
	size_t startIdx = cmdTreeRoot.getCmdNum() == 0 ? 0 : MAX_SIZE_T;
	size_t matchIdx = 0;
	auto noTokenDiff = [e, failToReg]
		{ regMsg() << "[Error] For \"" << e->getCmdStr()
		           << "\", cannot tell the difference of tokens before the end!" << endl; failToReg(); };
	for(size_t i = 0; i < mandNum; ++i)
		if(!checkValidity(cmdTokens[i], mandLens[i]))
			failToReg();
		else if(startIdx == MAX_SIZE_T)
		{
			for(size_t j = 0; j < mandLens[i]; ++j)
				cmdTokens[i][j] = tolower(cmdTokens[i][j]);
			matchIdx = node->checkAmbiguity(cmdTokens[i], mandLens[i]);
			if(matchIdx == MAX_SIZE_T)
				failToReg();
			else
			{
				bool sameCmd = matchIdx & 1;
				matchIdx >>= 1;
				if(sameCmd)
				{
					union { CmdNode* next; void* ptr; };
					if(node->getIntPtr(matchIdx, ptr))
						noTokenDiff();
					else node = next;
				}
				else startIdx = i;
			}
		}
	if(startIdx == MAX_SIZE_T)
		noTokenDiff();

	auto regCmdToken = [=, &cmdTokens, &mandLens]
	{
		function<tuple<void*, bool>(size_t)> getNodeOrExecPtr;
		getNodeOrExecPtr = [=, &getNodeOrExecPtr, &cmdTokens, &mandLens](size_t idx)->tuple<void*, bool>
		{
			if(idx == mandNum)
				return { e, true };
			CmdNode* newNode = new CmdNode;
			auto [ptr, isLeaf] = getNodeOrExecPtr(idx+1);
			newNode->insertCmdStrNode(0, cmdTokens[idx], mandLens[idx], ptr, isLeaf);
			return { newNode, false };
		};

		auto [ptr, isLeaf] = getNodeOrExecPtr(startIdx+1);
		node->insertCmdStrNode(matchIdx, cmdTokens[startIdx], mandLens[startIdx], ptr, isLeaf);
	};
	regCmdToken();

	if(size_t cmdLen = e->getTotalLen(); maxCmdLen < cmdLen)
		maxCmdLen = cmdLen;
}

CmdExec*
CmdMap::completeCmd(char*& tmp, WrapStrList& strList)const
{
	for(const CmdNode* curNode = &cmdTreeRoot; true;)
	{
		const char* curToken = tmp;
		bool isEnd;
		tmp = getToken(tmp, &isEnd);
		matchRange r = curNode->findMatchCmd(curToken);
		const auto& [lo, hi, result] = r;
		switch(result)
		{
			case NOTMATCH:
				return 0;

			case MATCH:
				if(isEnd)
				{
					strList.emplace_back(curNode->getStr8Cmd(lo).c_str());
					strList.emplace_back(curToken);
					return 0;
				}
				else
				{
					union { CmdExec* e; CmdNode* next; void* ptr; };
					if(curNode->getIntPtr(lo, ptr))
						return e;
					else curNode = next;
				} break;

			case PARTIAL:
				if(isEnd)
				{
					if(lo + 1 == hi)
						strList.emplace_back(curNode->getStr8Cmd(lo).c_str());
					else
						for(size_t i = lo; i < hi; ++i)
							strList.emplace_back(curNode->getCmdStr(i));
					strList.emplace_back(curToken);
				}
				return 0;
		}
	}
}

CmdExec*
CmdMap::operator[](char*& tmp)const
{
	for(const CmdNode* curNode = &cmdTreeRoot; true;)
	{
		const char* curToken = tmp;
		bool isEnd;
		tmp = getToken(tmp, &isEnd);
		matchRange r = curNode->findMatchCmd(curToken);
		const size_t& lo = get<0>(r);
		const MatchType& result = get<2>(r);
		if(result != MATCH) return 0;
		else
		{
			union { CmdExec* e; CmdNode* next; void* ptr; };
			if(curNode->getIntPtr(lo, ptr))
				return e;
			else if(isEnd)
				return 0;
			else curNode = next;
		}
	}
}

auto //Trailing Return Type
CmdMap::CmdNode::findMatchCmd(const char* curToken)const->matchRange
{
	//the range: [lo, hi)
	size_t lo = 0;           //first match
	size_t hi = getCmdNum(); //first not match
	size_t len = 0;

	auto updateRange = [&, curToken]()->bool
	{
		if(curToken[len] == '\0') return false;
		lo = lowerBound(lo, hi, tolower(curToken[len]), len);
		hi = upperBound(lo, hi, tolower(curToken[len]), len);
		len += 1; return true;
	};

	//to handle the special case that nothing in cmdStrNodeList
	//though this should be avoided in this application
	//another reason is to handle that only one command exists
	do
		switch(hi - lo)
		{
			case 0: return { lo, hi, NOTMATCH };
			case 1: return { lo, hi, cmdStrNodeList[lo].checkPartialCmd(curToken, len) };
			default: break;
		}
	while(updateRange());
	return { lo, hi, PARTIAL };
}

size_t
CmdMap::CmdNode::checkAmbiguity(const char* cmdStr, size_t matchNum)const
{
	constexpr size_t MAX_SIZE_T = numeric_limits<size_t>::max();
	size_t lo = 0;
	size_t hi = getCmdNum();
	size_t len = 0;

	/*====================================*/

	auto repAmbiguity = [=](bool type, size_t commLen, size_t failIdx)
	{
		auto setToRed = [] { regMsg() << "\033[31m"; };
		auto reset    = [] { regMsg() << "\033[0m"; };
		auto printCmd = [=](const char* cmdStr, size_t matchNum)->stringstream&
		{
			size_t i = 0;
			if(commLen > matchNum)
			{
				setToRed();
				for(; i < matchNum; ++i) regMsg() << char(toupper(cmdStr[i]));
				for(; i < commLen; ++i) regMsg() << cmdStr[i];
				reset();
			}
			else
			{
				setToRed();
				for(; i < commLen; ++i) regMsg() << char(toupper(cmdStr[i]));
				reset();
				for(; i < matchNum; ++i) regMsg() << char(toupper(cmdStr[i]));
			}
			for(; cmdStr[i] != '\0'; ++i) regMsg() << cmdStr[i];
			return regMsg();
		};
		auto printCmdCand = [=]()->stringstream& { return printCmd(cmdStr, matchNum); };
		auto printCmdFail = [=]()->stringstream&
			{ const CmdStrNode& failNode = cmdStrNodeList[failIdx];
			  return printCmd(failNode.getCmdStr(), failNode.getMandLen()); };

		/*====================================*/

		regMsg() << "[Error] " << (type ? "Strong" : "Weak") << " ambiguity between two command tokens! "
		            "{\""; printCmdCand() << "\", \""; printCmdFail() << "\"}" << endl;
		return MAX_SIZE_T;
	};

	auto checkAmbiguitySub = [=, &lo, &len]
	{
		const char* cmdStr1;
		const char* cmdStr2;
		size_t matchNum1, matchNum2;
		const CmdStrNode& n = cmdStrNodeList[lo];
		if(matchNum > n.getMandLen()) { cmdStr1 = n.getCmdStr(); matchNum1 = n.getMandLen();
		                                cmdStr2 = cmdStr;        matchNum2 = matchNum; }
		else                          { cmdStr1 = cmdStr;        matchNum1 = matchNum;
		                                cmdStr2 = n.getCmdStr(); matchNum2 = n.getMandLen(); }

		for(; len < matchNum1; ++len)
			if(cmdStr1[len] != cmdStr2[len])
				return ((cmdStr[len] > n[len] ? lo + 1 : lo) << 1) | 0;
		for(; len < matchNum2; ++len)
			if(cmdStr1[len] != cmdStr2[len])
				return repAmbiguity(false, matchNum1, lo);
		for(; cmdStr1[len] == cmdStr2[len]; ++len)
			if(cmdStr1[len] == '\0')
				return matchNum1 == matchNum2 ? ((lo << 1) | 1) : repAmbiguity(true, matchNum2, lo);
		return repAmbiguity(true, matchNum2, lo);
	};

	/*====================================*/

	/*
	# There are no ambiguity among the commands in the list before checking
	# While there are multiple candidates during matching, they must not reach their matchNum yet
	  Otherwise:
	  case 1. One reaches, one not: weak ambiguity
	  case 2. Both reach: strong ambiguity
	*/

	/*
	# During this section, only weak ambiguity is possible
	  since we just examine the mandatry part of cmdStr without the last character
	# At most one weak ambiguity is possible here, happening at "case 1"
	*/
	do
	{
		switch(hi - lo)
		{
			case 0: return (lo << 1) | 0;
			case 1: return checkAmbiguitySub();
			default: break;
		}

		if(len == matchNum) break;
		lo = lowerBound(lo, hi, cmdStr[len], len);
		hi = upperBound(lo, hi, cmdStr[len], len);
		len += 1;
	} while(true);

	/*
	# If entering this section, the rest commands in the list must be at least weak ambiguity
	# At most one strong ambiguity in the list
	  Otherwise, any two of the strongly ambiguous commands will be at least weak ambiguity
	*/
	const size_t weakIdx = lo;
	for(; true; ++len)
	{
		if(cmdStr[len] == '\0')
			return repAmbiguity(false, matchNum, weakIdx);
		lo = lowerBound(lo, hi, cmdStr[len], len);
		hi = upperBound(lo, hi, cmdStr[len], len);

		switch(hi - lo)
		{
			case 0: return repAmbiguity(false, matchNum, weakIdx);
			case 1: return checkAmbiguitySub();
			default: break;
		}
	}
}

void
CmdMap::CmdNode::traverseOnCmdStr(vector<CmdExec*>& execList)const
{
	for(size_t i = 0; i < cmdStrNodeList.size(); ++i)
	{
		union { CmdExec* e; CmdNode* next; void* ptr; };
		if(getIntPtr(i, ptr))
			{ if(e->getCmdType() != CMD_TYPE_HIDDEN) execList.push_back(e); }
		else next->traverseOnCmdStr(execList);
	}
}

string
CmdMap::CmdNode::getStr8Cmd(size_t i)const
{
	union { CmdNode* next; void* ptr; };
	string cmdStr(getCmdStr(i));
	if(!getIntPtr(i, ptr))
	{
		const CmdNode* curNode;
		do
		{
			if(next->getCmdNum() > 1)
				break;
			curNode = next;
			cmdStr += " ";
			cmdStr += curNode->getCmdStr(0);
		} while(!curNode->getIntPtr(0, ptr));
	}
	return cmdStr;
}

size_t
CmdMap::CmdNode::lowerBound(size_t lo, size_t hi, char value, size_t len)const
{
	while(lo != hi)
		if(size_t mid = (lo + hi) / 2;
		   cmdStrNodeList[mid][len] < value) lo = mid + 1;
		else                                 hi = mid;
	return lo;
}

bool
CmdMap::checkValidity(const char* cmdStr, size_t matchNum)
{
	/* Criteria
	1. cmdStr should contain at least one alphabet
	2. The length of cmdStr should be greater than or equal to matchNum
	3. The part within matchNum should be uppercase; otherwise, lowercase
	*/
	if(matchNum == 0)
		{ regMsg() << "[Error] The matching Number (" << matchNum << ") should be at least 1!" << endl; return false; }
	for(size_t i = 0; i < matchNum; ++i)
		if(cmdStr[i] == '\0')
			{ regMsg() << "[Error] The length of \"" << cmdStr << "\" is smaller than " << matchNum << "!" << endl; return false; }
		else if(!isalpha(cmdStr[i]))
			{ regMsg() << "[Error] Non alphabet at position " << i << " of \"" << cmdStr << "\"!" << endl; return false; }
		else if(!isupper(cmdStr[i]))
			{ regMsg() << "[Error] A lowercase at position " << i << " of \"" << cmdStr << "\" in its mandatory part!" << endl; return false; }
	for(size_t i = matchNum; cmdStr[i] != '\0'; ++i)
		if(!isalpha(cmdStr[i]))
			{ regMsg() << "[Error] Non alphabet at position " << i << " of \"" << cmdStr << "\"!" << endl; return false; }
		else if(!islower(cmdStr[i]))
			{ regMsg() << "[Error] An uppercase at position " << i << " of \"" << cmdStr << "\" in its optional part!" << endl; return false; }
	return true;
}

CmdMgr* cmdMgr;                      //do not assign 0 here!
unsigned CmdRegistrar::niftyCounter; //do not assign 0 here!

void
CmdMgr::printHelps(bool flattened)const
{
	vector<CmdExec*> execList = cmdMap.getAllExec();
	if(flattened)
		for(CmdExec* e: execList)
			e->printHelp(false);
	else
	{
		stable_sort(execList.begin(), execList.end(),
		            [](CmdExec* e1, CmdExec* e2) { return e1->getCmdType() < e2->getCmdType(); });
		CmdType curType = CMD_TYPE_TOTAL;
		for(CmdExec* e: execList)
		{
			if(e->getCmdType() != curType)
			{
				if(curType != CMD_TYPE_TOTAL)
					cout << endl;
				constexpr size_t headerLen = 72;
				const char* cmdPrompt = "Commands";
				constexpr size_t cmdPromptLen = 8;
				constexpr size_t fiveSpace = 5;
				const size_t numToPrint = headerLen - cmdPromptLen - fiveSpace - CmdTypeStr[e->getCmdType()].size();
				size_t i = 0;
				for(size_t half = numToPrint/2; i < half; ++i) cout << "=";
				cout << "  " << CmdTypeStr[e->getCmdType()] << " " << cmdPrompt << "  ";
				for(; i < numToPrint; ++i) cout << "=";
				cout << endl;
				curType = e->getCmdType();
			}
			e->printHelp(false);
		}
	}
}

void
CmdMgr::printHistory(size_t numFromBot)const
{
	const size_t hisSize = getHisNum();
	if(numFromBot > hisSize)
		{ cerr << "[Warning] Not that many commands in the history list!" << endl;
		  numFromBot = hisSize; }
	if(hisSize == 0)
		{ cout << "Empty history list!" << endl; return; }
	for(size_t i = hisSize-numFromBot; i < hisSize; ++i)
		cout << " " << i << ": " << history[i] << endl;
}

CmdExecStatus
CmdMgr::parseAndExecCmd(char*& cmd_opt)
{
	if(CmdExec* e = cmdMap[cmd_opt]; e != 0)
		return e->exec(cmd_opt);
	else
	{
		cerr << "[Error] Unknown command!" << endl;
		return CMD_EXEC_NOP;
	}
}

CmdMgr::CharArr CmdMgr::listCmdArea;

void
CmdMgr::copyToList()const
{
	const char* tmp = findFirstNotSpace();
	size_t copyCharNum = tmp < cursor ? strLen(tmp, cursor) : 0;
	memcpy(listCmdArea.begin(), tmp, copyCharNum);
	listCmdArea.begin()[copyCharNum] = '\0';
}

void
CmdMgr::printUsage()
{
	copyToList();
	if(CmdExec* e = cmdMap[listCmdArea.workPlace()]; e != 0)
	{
		storeCursorHlBegin(false);
		moveToTheLast();
		e->printUsage();
		cout << endl;
		storeCursorHlBegin(true);
		reprintCmd();
	}
}

WrapStrList
CmdMgr::findMatchStr()const
{
	copyToList();
	char*& tmp = listCmdArea.workPlace();
	WrapStrList strList;
	if(CmdExec* e = cmdMap.completeCmd(tmp, strList); e != 0)
	{
		if(e->hasCustomMatch())
			strList = e->listThing(tmp);
		else
		{
			tmp = findLastToken(tmp);
			if(*tmp != '\0' && *tmp == '-')
			{
				strList = e->matchOptions(tmp);
				strList.emplace_back(tmp);
			}
			else
			{
				if(*tmp != '\0' && *tmp == '~' && *(tmp+1) != '\0' && *(tmp+1) == '/')
				{
					const char* homeDir = getHomeDir();
					if(homeDir != 0)
					{
						const size_t homeLen = strlen(homeDir);
						char* tmp2 = tmp + 2;
						for(; *tmp2 != '\0'; ++tmp2);
						for(; tmp2 > tmp; --tmp2)
							*(tmp2 + homeLen - 1) = *tmp2;
						for(size_t i = 0; i < homeLen; ++i)
							tmp2[i] = homeDir[i];
					}
				}
				const char* dir = tmp;
				const char* prefix = 0;
				for(; *tmp != '\0'; ++tmp)
					if(*tmp == '/')
						prefix = tmp + 1;
				if(prefix != 0)
				{
					for(; tmp >= prefix; --tmp)
						*(tmp + 1) = *tmp;
					*(tmp + 1) = '\0';
					prefix += 1;
				}
				else
					{ prefix = dir; dir = "."; }
				strList = listFile(dir, prefix);
				strList.emplace_back(prefix);
			}
		}
	}
	return strList;
}

void
CmdMgr::printMatchStr(const WrapStrList& strList, bool multiTab)
{
	static bool manyMatch = false;
	switch(strList.size())
	{
		case 0:
		case 1: manyMatch = false; break;

		case 2:
		{
			const size_t prefixLen = strlen(strList[1]);
			const size_t totalLen  = strlen(strList[0]);
			if(isHLBeginSet()) clearAllHL(true);
			insertStr(strList[0]+prefixLen, totalLen-prefixLen);
			if(*(cursor-1) != '/') //just workaround, not that good
				insertStr(SPACE);
		} manyMatch = false; break;

		default:
		{
			const size_t prefixLen = strlen(strList.back());
			const_cast<WrapStrList&>(strList).pop_back();
			const string common = findCommonPart(prefixLen, strList);
			if(common.empty())
			{
				if(multiTab && manyMatch)
				{
					storeCursorHlBegin(false);
					moveToTheLast();
					printStrsByOrder(getCurWinWidth(), strList);
					storeCursorHlBegin(true);
					reprintCmd();
				}
				manyMatch = true;
			}
			else
			{
				if(isHLBeginSet())
					clearAllHL(true);
				insertStr(common);
				manyMatch = false;
			}
		} break;
	}
}

}
