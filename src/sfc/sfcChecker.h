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

extern CondStream  sfcMsg;

class SafetyChecker
{
public:
	SafetyChecker(AigNtk*, size_t, bool, size_t, bool, bool);
	virtual ~SafetyChecker() { if(ntkIsCopied) delete ntk; }

	void Check();

	virtual bool isComb()const { return false; }

protected:
	virtual void check() = 0;

	AigGateV buildInit();

	bool checkBreakCond()const;
	bool checkIsStopped()const { return isStpSent; }
	void resetStop()const { isStpSent = false; }
	static void catchIntsignal(int);
	static void (*oldIntHandler)(int);
	static void catchStpsignal(int);
	static void (*oldStpHandler)(int);

protected:
	AigNtk*     ntk;
	AigGateID   property;
	bool        trace;
	bool        supportBreak;
	bool        ntkIsCopied;
	clock_t     timeBound;

	// TODO, support suspending
	static bool  isIntSent;
	static bool  isStpSent;
	static bool  supportBreakNow;
};

#define SC_Derived(checkerName, supportB, ntkIsC)                                  \
class checkerName : public SafetyChecker                                           \
{                                                                                  \
public:                                                                            \
	checkerName(AigNtk* ntkToCheck, size_t outputIdx, bool _trace, size_t timeout) \
	: SafetyChecker(ntkToCheck, outputIdx, _trace, timeout, supportB, ntkIsC) {}   \
}

SC_Derived(SafetyBCChecker, true , true );
SC_Derived(SafetyBNChecker, true , false);
SC_Derived(SafetyNCChecker, false, true );
SC_Derived(SafetyNNChecker, false, false);

class CombChecker : public SafetyNNChecker
{
public:
	CombChecker(AigNtk* ntkToCheck, size_t outputIdx, bool _trace, size_t timeout)
	: SafetyNNChecker(ntkToCheck, outputIdx, _trace, timeout) {}

	bool isComb()const { return true; }

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
