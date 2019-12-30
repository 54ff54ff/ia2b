/*========================================================================\
|: [Filename] cmdExec.h                                                  :|
:| [Author]   Chung-Yang (Ric) Huang, Chiang Chun-Yi                     |:
|: [Synopsis] Define the interface for command execution                 :|
<------------------------------------------------------------------------*/

#ifndef HEHE_CMDEXEC_H
#define HEHE_CMDEXEC_H

#include <stdlib.h>
#include <string.h>
#include <utility>
#include <tuple>
#include <initializer_list>
#include <sstream>
#include <functional>
#include "util.h"
using namespace std;

namespace _54ff
{

//idea from academic verification tool "v3" by Cheng-Yin Wu
enum CmdType
{
	CMD_TYPE_COMMON = 0,
	CMD_TYPE_SYSTEM,
	CMD_TYPE_IO,
	CMD_TYPE_DISPLAY,
	CMD_TYPE_SYNTHESIS,
	CMD_TYPE_VERIFICATION,
	CMD_TYPE_TOTAL,
	CMD_TYPE_HIDDEN
};

extern const string CmdTypeStr[CMD_TYPE_TOTAL];

enum CmdExecStatus
{
	CMD_EXEC_DONE = 0,
	CMD_EXEC_ERROR_INT,
	CMD_EXEC_ERROR_EXT,
	CMD_EXEC_QUIT,
	CMD_EXEC_NOP,
	CMD_EXEC_TOTAL
};

enum CmdOptionError
{
	CMD_OPT_MISSING,
	CMD_OPT_EXTRA,
	CMD_OPT_ILLEGAL,
	CMD_OPT_FOPEN_FAIL,
	CMD_OPT_INVALID_INT,
	CMD_OPT_INVALID_UINT
};

class CmdExec
{
public:
	virtual ~CmdExec() { delete []cmdStr; };

	virtual CmdExecStatus exec(char*)const = 0;
	virtual const char* getUsageStr()const = 0;
	virtual const char* getHelpStr()const = 0;

	void printUsage()const;
	void printHelp(bool)const;

	virtual bool hasCustomMatch()const { return false; }
	virtual WrapStrList listThing(char*&)const { return WrapStrList(); }
	virtual WrapStrList matchOptions(const char*)const = 0;

	const char* getCmdStr()const { return cmdStr; }
	size_t getTotalLen()const { return strlen(getCmdStr()); }
	CmdType getCmdType()const { return cmdType; }

protected:
	CmdExec(const char* s, CmdType t)
	: cmdStr(copyCStr(s)), cmdType(t) {}

	static CmdExecStatus errorOption(CmdOptionError, const char* = 0);

private:
	const char* const  cmdStr;
	const CmdType      cmdType;
};

class RegMsg
{
public:
	void setFileInt(const char* fn) { fileName = fn; }
	void setLineInt(unsigned    ln) { lineNo   = ln; }
	void printStatus()const
		{ cerr << "In file \"" << fileName << "\", at line " << lineNo << ":" << endl; }

	void setWarn() { warning.emplace_back(getStr()); resetStr(); }
	bool hasWarn()const { return !warning.empty(); }
	void printWarn() { printStatus(); for(const string& s: warning) cerr << s; warning.clear(); }
	string getStr()const { return messenger.str(); }
	void resetStr() { messenger.str(""); }

	stringstream& operator()() { return messenger; }

private:
	string          fileName;
	unsigned        lineNo;
	vector<string>  warning;
	stringstream    messenger;
} extern regMsg;
#define setFile() regMsg.setFileInt(__FILE__)
#define setLine() regMsg.setLineInt(__LINE__)

class RegError {};

class OptListBase
{
protected:
	static bool checkValidity(const char*, size_t);
	static bool checkAmbiguity(const char*, size_t, const char*, size_t);
};

template <size_t S>
class OptList : public OptListBase
{
protected:
	template <class... Opts>
	OptList(Opts... opts)
	{
		static_assert(sizeof...(opts) / 2 == S && sizeof...(opts) % 2 == 0);
		fillList(0, opts...); checkTotalValidity(); checkTotalAmbiguity();
	}

	~OptList() { for(size_t i = 0; i < S; ++i) delete []optStrs[i]; }

	template <size_t idx>
	bool optMatch(const char* cand)const { static_assert(idx < S); return myStrNCmp(cand, optStrs[idx], matchNums[idx]) == MATCH; }

	WrapStrList matchOptions(const char* prefix)const
	{
		WrapStrList matchedOpts; //trust the RVO
		for(size_t i = 0; i < S; ++i)
			if(myStrNCmp(prefix, optStrs[i], matchNums[i]) != NOTMATCH)
				matchedOpts.emplace_back(optStrs[i]);
		return matchedOpts;
	}

private:
	sArray<const char*, S>  optStrs;
	sArray<size_t,      S>  matchNums;

	void checkTotalValidity()const
	{
		for(size_t i = 0; i < S; ++i)
			if(!OptListBase::checkValidity(optStrs[i], matchNums[i]))
				throw RegError();
			else
				for(size_t j = 0; j < matchNums[i]; ++j)
					const_cast<char&>(optStrs[i][j]) = tolower(optStrs[i][j]);
	}

	void checkTotalAmbiguity()const
	{
		for(size_t i = 0; i < S; ++i)
			for(size_t j = i + 1; j < S; ++j)
				if(!OptListBase::checkAmbiguity(optStrs[i], matchNums[i],
				                                optStrs[j], matchNums[j])) throw RegError();
	}

	template <class... Opts>
	void fillList(size_t i, const char* optStr, size_t matchNum, Opts... opts)
		{ optStrs[i] = copyCStr(optStr); matchNums[i] = matchNum; fillList(i + 1, opts...); }
	void fillList(size_t) {}
};

#define CmdClass(func, type, num, ...)                 \
class func##Cmd : public CmdExec, OptList<num>         \
{                                                      \
public:                                                \
	func##Cmd(const char* s)                           \
	: CmdExec(s, type)                                 \
	, OptList<num>(__VA_ARGS__) {}                     \
                                                       \
	CmdExecStatus exec(char*)const;                    \
	const char* getUsageStr()const;                    \
	const char* getHelpStr()const;                     \
                                                       \
	WrapStrList matchOptions(const char* prefix)const  \
		{ return OptList<num>::matchOptions(prefix); } \
}

#define CmdClassCustomMatch(func, type, num, ...)      \
class func##Cmd : public CmdExec, OptList<num>         \
{                                                      \
public:                                                \
	func##Cmd(const char* s)                           \
	: CmdExec(s, type)                                 \
	, OptList<num>(__VA_ARGS__) {}                     \
                                                       \
	CmdExecStatus exec(char*)const;                    \
	const char* getUsageStr()const;                    \
	const char* getHelpStr()const;                     \
                                                       \
	WrapStrList matchOptions(const char* prefix)const  \
		{ return OptList<num>::matchOptions(prefix); } \
                                                       \
	bool hasCustomMatch()const { return true; }        \
	WrapStrList listThing(char*&)const;                \
}

class CmdMap
{
public:
	CmdMap(): maxCmdLen(0) {}

	void regCmd(CmdExec*, const Array<size_t>&, size_t);
	CmdExec* completeCmd(char*&, WrapStrList&)const;
	size_t getMaxCmdLen()const { return maxCmdLen; }
	vector<CmdExec*> getAllExec()const
		{ vector<CmdExec*> execList; cmdTreeRoot.traverseOnCmdStr(execList); return execList; }

	using matchRange = tuple<size_t, size_t, MatchType>;
	CmdExec* operator[](char*&)const;

private:
	class CmdNode
	{
	public:
		matchRange findMatchCmd(const char*)const;

		size_t checkAmbiguity(const char*, size_t)const;
		void insertCmdStrNode(size_t pos, const char* cmdStr, size_t mandLen, void* ptr, bool isLeaf)
			{ cmdStrNodeList.emplace(cmdStrNodeList.begin()+pos, cmdStr, mandLen, ptr, isLeaf); }
		bool getIntPtr(size_t idx, void*& ptr)const
			{ const CmdStrNode& n = cmdStrNodeList[idx]; ptr = n.getIntPtr(); return n.isLeaf(); }
		void traverseOnCmdStr(vector<CmdExec*>&)const;

		size_t getCmdNum()const { return cmdStrNodeList.size(); }
		const char* getCmdStr(size_t i)const { return cmdStrNodeList[i].getCmdStr(); }
		string getStr8Cmd(size_t)const; //str8 = straight

	private:
		class CmdStrNode
		{
		public:
			CmdStrNode(const char* s, size_t len, void* p, bool flag)
			: cmdStr      (copyToLower(s))
			, mandLen     (len)
			, ptrWithFlag (setFlagToValue(p, flag)) {}
			~CmdStrNode() { delete []cmdStr; if(isLeaf()) delete getCmdExec(); else delete getNextNode(); }

			/*====================================*/

			CmdStrNode(const CmdStrNode&) = delete;
			CmdStrNode(CmdStrNode&& c) noexcept //for vector, rule of 5
			: cmdStr      (c.cmdStr)
			, mandLen     (c.mandLen)
			, ptrWithFlag (c.ptrWithFlag) { c.cmdStr = 0; c.ptrWithFlag = 0; }

			CmdStrNode& operator=(const CmdStrNode&) = delete;
			CmdStrNode& operator=(CmdStrNode&& c) noexcept //for vector, rule of 5
			{
				cmdStr = c.cmdStr; c.cmdStr = 0;
				const_cast<size_t&>(mandLen) = c.mandLen;
				ptrWithFlag = c.ptrWithFlag; c.ptrWithFlag = 0;
				return *this;
			}

			/*====================================*/

			bool isLeaf()const { return getFlag(ptrWithFlag); }
			void* getIntPtr()const { return getPtr<void>(ptrWithFlag); }
			CmdExec* getCmdExec()const { return (CmdExec*)getIntPtr(); }
			CmdNode* getNextNode()const { return (CmdNode*)getIntPtr(); }

			/*====================================*/

			const char* getCmdStr()const { return cmdStr; }
			size_t getMandLen()const { return mandLen; }
			char operator[](size_t i)const { return cmdStr[i]; }
			MatchType checkPartialCmd(const char* curToken, size_t len)const
				{ assert(len <= mandLen); return myStrNCmp(curToken+len, cmdStr+len, mandLen-len); }

		private:
			const char*   cmdStr;
			const size_t  mandLen;
			size_t        ptrWithFlag;
		};

	private:
		size_t lowerBound(size_t, size_t, char, size_t)const;
		size_t upperBound(size_t lo, size_t hi, char value, size_t idx)const
			{ return lowerBound(lo, hi, value+1, idx); }

	private:
		vector<CmdStrNode>  cmdStrNodeList;
	};

private:
	static bool checkValidity(const char*, size_t);

private:
	CmdNode  cmdTreeRoot;
	size_t   maxCmdLen;
};

}

#endif
