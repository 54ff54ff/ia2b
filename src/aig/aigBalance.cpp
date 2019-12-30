/*========================================================================\
|: [Filename] aigBalance.cpp                                             :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Implement the warppaer class to perform balance            :|
<------------------------------------------------------------------------*/

#include "aigBalance.h"

namespace _54ff
{

AigBalancer* aigBalancer = new AigBalancer;

bool
AigBalancer::balance(AigNtk* ntkToBalance)
{
	if(!ntkToBalance->checkCombLoop(true))
		return false;
	ntk = ntkToBalance;
	newNtk = new AigNtk;
	setIdLitMap();
	checkFanOut();
	createNewCIs();
	doBalance();
	createNewCOs();
	finalize();
	return true;
}

void
AigBalancer::checkFanOut()
{
	const size_t M = ntk->getMaxGateNum();
	Array<unsigned> fanOutNum(M);
	for(size_t i = 0; i < M; ++i)
		fanOutNum[i] = 0;
	for(size_t i = 0; i < M; ++i)
		if(AigGate* g = ntk->getGate(i); g != 0)
			switch(g->getFanInNum())
			{
				case 0: break;
				case 1: fanOutNum[g->getFanIn0ID()] += 1; break;
				case 2: fanOutNum[g->getFanIn0ID()] += 1;
				        fanOutNum[g->getFanIn1ID()] += 1; break;
			}
	hasMultiFanOut.init(M);
	for(size_t i = 0; i < M; ++i)
		hasMultiFanOut[i] = fanOutNum[i] > 1;
}

void
AigBalancer::createNewCIs()
{
	newIdToLevel.init(ntk->getMaxGateNum());
	newIdToLevel[0] = 0;
	for(size_t i = 0, I = ntk->getInputNum(); i < I; ++i)
	{
		oldIdToNewLit[ntk->getInputID(i)] = newNtk->createInput()->getGateID() << 1;
		newIdToLevel[newNtk->getInputID(i)] = 0;
	}
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
	{
		oldIdToNewLit[ntk->getLatchID(i)] = newNtk->createLatch(0)->getGateID() << 1;
		newIdToLevel[newNtk->getLatchID(i)] = 0;
	}
}

void
AigBalancer::doBalance()
{
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
		doBalanceSub(ntk->getLatchNorm(i)->getFanIn0Ptr());
	for(size_t i = 0, O = ntk->getOutputNum(); i < O; ++i)
		doBalanceSub(ntk->getOutputNorm(i)->getFanIn0Ptr());
}

void
AigBalancer::createNewCOs()
{
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
	{
		AigGate* l = ntk->getLatchNorm(i);
		unsigned eqLit = oldIdToNewLit[l->getFanIn0ID()];
		newNtk->getLatchNorm(i)->setFanIn0(newNtk->getGate(eqLit >> 1),
		                                   bool(eqLit & 1) ^ l->isFanIn0Inv());
	}
	for(size_t i = 0, O = ntk->getOutputNum(); i < O; ++i)
	{
		AigGate* o = ntk->getOutputNorm(i);
		unsigned eqLit = oldIdToNewLit[o->getFanIn0ID()];
		newNtk->createOutput(AigGateV(newNtk->getGate(eqLit >> 1),
		                              bool(eqLit & 1) ^ o->isFanIn0Inv()));
	}
}

void
AigBalancer::setIdLitMap()
{
	const size_t M = ntk->getMaxGateNum();
	oldIdToNewLit.init(M);
	oldIdToNewLit[0] = 0;
	for(size_t i = 1; i < M; ++i)
		oldIdToNewLit[i] = UNDEF_GATEID;
}

void
AigBalancer::finalize()
{
	ntk->swap(newNtk);
	ntk->ntkName.swap(newNtk->ntkName);
	hasMultiFanOut.reset();
	oldIdToNewLit.reset();
	newIdToLevel.reset();
	delete newNtk;
}

void
AigBalancer::doBalanceSub(AigGate* g)
{
	if(oldIdToNewLit[g->getGateID()] != UNDEF_GATEID)
		return;
	LevelHeap levelHeap(newIdToLevel);
	for(AigGateV gv: collectCluster(g))
	{
		AigGate* cg = gv.getGatePtr();
		doBalanceSub(cg);
		unsigned eqLit = oldIdToNewLit[cg->getGateID()];
		assert(eqLit != UNDEF_GATEID);
		levelHeap.emplace(newNtk->getGate(eqLit >> 1), bool(eqLit & 1) ^ gv.isInv());
	}
	while(levelHeap.size() > 1)
	{
		AigGateV in0 = levelHeap.top(); levelHeap.pop();
		AigGateV in1 = levelHeap.top(); levelHeap.pop();
		AigGateV gv = newNtk->createAnd(in0, in1);
		newIdToLevel[gv.getGateID()] = maxLevel(in0, in1) + 1;
		levelHeap.emplace(gv);
	}
	oldIdToNewLit[g->getGateID()] = (levelHeap.top().getGateID() << 1) |
	                                unsigned(levelHeap.top().isInv());
}

vector<AigGateV>
AigBalancer::collectCluster(AigGate* g)
{
	assert(g->getGateType() == AIG_AND);
	vector<AigGateV> cluster;
	collectClusterRec(g->getFanIn0(), cluster);
	collectClusterRec(g->getFanIn1(), cluster);
	assert(cluster.size() >= 2);
	return cluster;
}

void
AigBalancer::collectClusterRec(AigGateV gv, vector<AigGateV>& cluster)
{
	AigGate* g = gv.getGatePtr();
	if(gv.isInv() || g->isCI() || hasMultiFanOut[g->getGateID()])
		{ cluster.push_back(gv); return; }
	assert(g->getGateType() == AIG_AND);
	collectClusterRec(g->getFanIn0(), cluster);
	collectClusterRec(g->getFanIn1(), cluster);
}

unsigned
AigBalancer::maxLevel(const AigGateV& gv1, const AigGateV& gv2)
{
	unsigned l1 = newIdToLevel[gv1.getGateID()];
	unsigned l2 = newIdToLevel[gv2.getGateID()];
	return l1 > l2 ? l1 : l2;
}

}