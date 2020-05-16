/*========================================================================\
|: [Filename] aigMisc2.cpp                                               :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Implement the warppaer classes including                   :|
|:            5. AigConster                                              |:
<------------------------------------------------------------------------*/

#include "aigMisc2.h"
#include "pdrChecker.h"

namespace _54ff
{

bool
AigConster::doSimpMono()
{
	if(!ntk->checkCombLoop(true))
		return false;

	vector<AigGateID> zeroCand(ntk->getLatchNum());
	for(size_t i = 0, L = zeroCand.size(); i < L; ++i)
		zeroCand[i] = ntk->getLatchID(i);
	for(size_t iter = 1; true; ++iter)
	{
		SolverPtr<CirSolver> solver(ntk);
		simpMsg << iter << ": " << zeroCand.size() << " -> " << flush;
		for(AigGateID id: zeroCand)
			solver->convertToCNF(id, 0),
			solver->addClause(Lit(solver->getVarInt(id, 0), true));
		size_t n = zeroCand.size();
		size_t s = 0;
		for(size_t i = 0; i < n; ++i)
		{
			solver->convertToCNF(zeroCand[i], 1);
			solver->clearAssump();
			solver->addAssump(zeroCand[i], 1, false);
			if(!solver->solve())
				zeroCand[s++] = zeroCand[i];
		}
		zeroCand.resize(s);
		simpMsg << s << endl;
		if(s == 0 || s == n)
			break;
	}
	simpMsg << RepeatChar('-', 36) << endl;
	cout << "Removed number of latches = " << zeroCand.size() << endl;
	if(!zeroCand.empty())
	{
		simpMsg << "Removed latches:";
		for(AigGateID id: zeroCand)
			simpMsg << " " << id;
		simpMsg << endl;
	}

	replaceWithZero(zeroCand);
	return true;
}

bool
AigConster::doSimpPdr(size_t satLimit)
{
	if(!ntk->checkCombLoop(true))
		return false;

	// TODO, more testing
	const string resultStr[] = { "PASS", "FAIL", "ABORT" }; size_t r;
	vector<AigGateID> zeroLatch;
	vector<AigGateLit> singleLit(1);
	vector<PdrCube> indSet;
	sfcMsg.unsetActive();
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
	{
		PdrChecker* checker = getDefaultPdr(ntk);
		checker->disablePrintFrame();
		checker->setSatLimit(satLimit);
		singleLit[0] = makeToLit(ntk->getLatchID(i), false);
		checker->setTargetCube(PdrCube(singleLit));
		checker->startWithIndSet(indSet);
		simpMsg << "Test latch with ID " << ntk->getLatchID(i) << flush;
		switch(checker->checkInt())
		{
			case PDR_RESULT_UNSAT : r = 0; break;
			case PDR_RESULT_SAT   : r = 1; break;
			default               : r = 2; break;
		}
		indSet = checker->getCurIndSet(false);
		simpMsg << " -> " << resultStr[r] << ", #Inf = " << indSet.size() << endl;
		if(r == 0) zeroLatch.push_back(ntk->getLatchID(i));
		delete checker;
	}
	sfcMsg.setActive();

	simpMsg << RepeatChar('-', 36) << endl;
	cout << "Removed number of latches = " << zeroLatch.size() << endl;
	if(!zeroLatch.empty())
	{
		simpMsg << "Removed latches:";
		for(AigGateID id: zeroLatch)
			simpMsg << " " << id;
		simpMsg << endl;
	}

	replaceWithZero(zeroLatch);
	return true;
}

void
AigConster::replaceWithZero(const vector<AigGateID>& zeros)
{
	AigGate::setGlobalRef();
	for(AigGateID id: zeros)
		ntk->getGate(id)->setToGlobalRef();
	for(size_t i = 0, M = ntk->getMaxGateNum(); i < M; ++i)
		if(AigGate* g = ntk->getGate(i); g != 0)
			switch(g->getFanInNum())
			{
				case 1:
					if(g->getFanIn0Ptr()->isGlobalRef())
						g->getFanIn0() = g->isFanIn0Inv() ? ntk->getConst1V()
						                                  : ntk->getConst0V();
					break;
				case 2:
					if(g->getFanIn0Ptr()->isGlobalRef())
						g->getFanIn0() = g->isFanIn0Inv() ? ntk->getConst1V()
						                                  : ntk->getConst0V();
					if(g->getFanIn1Ptr()->isGlobalRef())
						g->getFanIn1() = g->isFanIn1Inv() ? ntk->getConst1V()
						                                  : ntk->getConst0V();
					break;

				default: break;
			}
}

}
