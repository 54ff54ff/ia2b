/*========================================================================\
|: [Filename] sfcChecker.h                                               :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Define the basic checker for safety property               :|
<------------------------------------------------------------------------*/

#ifndef HEHE_SFCCHECKER_H
#define HEHE_SFCCHECKER_H

#include <iostream>
#include <iomanip>
#include "aigNtk.h"
#include "condStream.h"
using namespace std;

namespace _54ff
{

class SafetyChecker
{
public:
	SafetyChecker(AigNtk*, size_t, bool, size_t);
	virtual ~SafetyChecker() { delete ntk; }

	void Check();

	bool checkBreakCond()const;

protected:
	virtual void check() = 0;
	virtual bool isBreakSupported()const { return false; }

	AigGateV buildInit();

	static void catchIntsignal(int) { if(!supportBreak || isIntSent) exit(1); else isIntSent = true; }
	static void (*oldIntHandler)(int);

protected:
	AigNtk*     ntk;
	AigGateID   property;
	bool        trace;
	clock_t     timeBound;

	static bool  isIntSent;
	static bool  supportBreak;
};

class SafetyBChecker : public SafetyChecker
{
public:
	using SafetyChecker::SafetyChecker;

protected:
	bool isBreakSupported()const { return true; }
};

class CombChecker : public SafetyChecker
{
public:
	CombChecker(AigNtk* ntkToCheck, size_t outputIdx, bool _trace, size_t timeout)
	: SafetyChecker(ntkToCheck, outputIdx, _trace, timeout) {}

protected:
	void check();
};

class CheckerErr
{
public:
	CheckerErr(const char* err): errMsg(err) {}

	const string& getErrMsg()const { return errMsg; }

private:
	string  errMsg;
};

class CheckerBreak {};

template <class Checker, class... Param>
SafetyChecker* getChecker(AigNtk* ntkToCheck, size_t outputIdx, bool _trace, size_t timeout, Param&&... param)
{
	static_assert(is_convertible_v<Checker*, SafetyChecker*>);
	if(!ntkToCheck->checkCombLoop(true))
		return 0;
	if(outputIdx >= ntkToCheck->getOutputNum())
		{ cerr << "[Error] The output index (" << outputIdx
		       << ") is out of range!" << endl; return 0; }
	if(ntkToCheck->noLatchInCone(outputIdx))
	{
		cout << "No latch related to the property. Reduce to combinational checker!" << endl;
		return (new CombChecker(ntkToCheck, outputIdx, _trace, timeout));
	}
	try { return (new Checker(ntkToCheck, outputIdx, _trace, timeout, forward<Param>(param)...)); }
	catch(const CheckerErr& ce) { cerr << ce.getErrMsg() << endl; return 0; }
}

}

#endif
