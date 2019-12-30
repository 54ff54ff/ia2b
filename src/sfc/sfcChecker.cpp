/*========================================================================\
|: [Filename] sfcCheck.cpp                                               :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Implement the common utilities of checker                  :|
<------------------------------------------------------------------------*/

#include "sfcChecker.h"
#include "cirSolver.h"
#include "condStream.h"

namespace _54ff
{

SafetyChecker::SafetyChecker(AigNtk* ntkToCheck, size_t outputIdx, bool _trace, size_t timeout)
: ntk       (ntkToCheck->copyNtk())
, property  (ntk->getOutputID(outputIdx))
, trace     (_trace)
, timeBound (timeout == 0 ? 0 : clock() + (timeout * CLOCKS_PER_SEC))
{
	cout << RepeatChar('=', 36) << endl
	     << "Network    : " << ntk->ntkName << endl
	     << "Property   : output " << outputIdx << ", ID " << property << endl
	     << "Report CEX : " << (trace ? "Yes" : "No") << endl
	     << "Timeout    : ";
	if(timeout == 0) cout << "unlimited";
	else             cout << timeout << " seconds";
	cout << endl;
}

void
SafetyChecker::Check()
{ 
	cout << RepeatChar('=', 36) << endl;
	isIntSent = false;
	supportBreak = isBreakSupported();
	oldIntHandler = signal(SIGINT, catchIntsignal);
	check();
	signal(SIGINT, oldIntHandler);
}

bool
SafetyChecker::checkBreakCond()const
{
	if(isIntSent)
	{
		cout << "\rReceive interruption signal";
		return true;
	}
	if(timeBound != 0 && clock() >= timeBound)
	{
		cout << "\rTimeout";
		return true;
	}
	return false;
}

AigGateV
SafetyChecker::buildInit()
{
	vector<AigGateV> initList;
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
		initList.emplace_back(ntk->getLatchNorm(i), true);
	return ntk->createAnd(initList);
}

void (*SafetyChecker::oldIntHandler)(int) = 0;
bool SafetyChecker::isIntSent = false;
bool SafetyChecker::supportBreak = false;

void
CombChecker::check()
{
	SolverPtr<CirSolver> solver(ntk);
	solver->convertToCNF(property, 0);
	solver->clearAssump();
	solver->addAssump(property, 0, false);
	if(solver->solve())
	{
		cout << "Observe a counter example" << endl;
		if(trace) solver->reportTrace(0);
	}
	else
		cout << "Property proved" << endl;
}

}