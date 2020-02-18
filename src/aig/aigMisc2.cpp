/*========================================================================\
|: [Filename] aigMisc2.cpp                                               :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Implement the warppaer classes including                   :|
|:            5. AigConster
<------------------------------------------------------------------------*/

#include "aigMisc2.h"

namespace _54ff
{

bool
AigConster::doSimp()
{
	if(!ntk->checkCombLoop(true))
		return false;

	for(size_t iter = 1; true; ++iter)
	{
		solver = getSolver(ntk);
		cout << iter << ": " << zeroCand.size() << " -> " << flush;
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
		cout << s << endl;
		delete solver;
		if(s == 0 || s == n)
			break;
	}
	cout << "Removed number of latches = " << zeroCand.size() << endl;
	if(!zeroCand.empty())
	{
		cout << "Removed latches:";
		for(AigGateID id: zeroCand)
			cout << " " << id;
		cout << endl;
	}

	AigGate::setGlobalRef();
	for(AigGateID id: zeroCand)
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

	return true;
}

}
