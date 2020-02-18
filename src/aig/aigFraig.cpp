/*========================================================================\
|: [Filename] aigFraig.cpp                                               :|
:| [Author]   Chung-Yang (Ric) Huang, Chiang Chun-Yi                     |:
|: [Synopsis] Implement the wrapper class to perform FRAIG               :|
<------------------------------------------------------------------------*/

#include <iomanip>
#include "aigFraig.h"
#include "condStream.h"
using namespace std;

namespace _54ff
{

const char* AigFraiger::fraigHeader[FRAIG_TOTAL] = { "Optimize", "Rewrite ", "Strash  ", "Fraig   " };
unsigned AigFraiger::fraigCount[FRAIG_TOTAL];
AigFraiger* aigFraiger = new AigFraiger;

void
AigFraiger::randomSim()
{
	size_t fail = calMaxFail();
	cout << "Perform Random Simulation. MAX FAIL = " << fail << endl;
	initFecGrp();
	unsigned patTime = 1;
	cout << "#FEC Group = " << fecGroups.size() << ", #Rest fail = " << fail << flush;
	for(; !fecGroups.empty() && fail > 0; ++patTime)
	{
		if(!updateFecGrpRand()) fail -= 1;
		cout << "\r" << setw(13+20+15+20) << ""
		     << "\r" << "#FEC Group = " << fecGroups.size()
		     << ", #Rest fail = " << fail << flush;
	}
	feqTarget.init(ntk->getMaxGateNum()); setFeqTarget();
	cout << "\r" << setw(13+20+15+20) << ""
		 << "\r" << "#FEC Group = " << fecGroups.size()
	     << ", total " << patTime * 64 << " patterns simulated"
	     << " (" << patTime << " times)" << endl
	     << RepeatChar('-', 72) << endl;
}

bool
AigFraiger::simpNtk(AigNtk* ntkToSimp, bool noFraig, bool doTwoLevel)
{
	if(!ntkToSimp->checkCombLoop(true, dfsList))
		return false;
	ntk = ntkToSimp;
	resetCount();
	if(!noFraig)
	{
		randomSim();
		solver = getSolver(ntk);
		for(size_t i = 0, I = ntk->getInputNum(); i < I; ++i)
			solver->convertToCNF(ntk->getInputID(i), 0);
		for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
			solver->convertToCNF(ntk->getLatchID(i), 0);
	}
	StrashSet strashSet(dfsList.size());
	unsigned satCount[5] = { 0, 0, 0, 0, 0 };
	for(AigAnd* a: dfsList)
	{
		a->getFanIn0().checkEqGate();
		a->getFanIn1().checkEqGate();
		a->checkFanInOrder();

		/* constant propagation
		1. 0 &  F = 0
		2. 1 &  F = F
		3. F &  F = F
		4. F & ~F = 0
		*/
		if(a->getFanIn1Ptr()->getGateType() == AIG_CONST0)
			a->setEqGate(a->isFanIn1Inv() ?  a->getFanIn0() : ntk->getConst0V(), getHeader(FRAIG_OPTIMIZE));
		else if(a->getFanIn1Ptr() == a->getFanIn0Ptr())
			a->setEqGate(a->isFanIn1Inv() ^ a->isFanIn0Inv() ? ntk->getConst0V() : a->getFanIn1(), getHeader(FRAIG_OPTIMIZE));

		/* two level rule based minimization */
		else if(doTwoLevel && doTwoLevelSimp(a)) {}

		/* structural hash */
		else if(auto [iter, distinct] = strashSet.emplace(a);
		        !distinct) iter->helpSetEq(a);

		if(noFraig || a->toDelete()) continue;
		/* fraig */
		const AigGateID id = a->getGateID();
		const AigGateID candId = feqTarget[id];
		if(candId == UNDEF_GATEID) continue;
		const bool inv = candId & 1;
		const_cast<AigGateID&>(candId) >>= 1;
		if(candId == 0)
		{
			solver->convertToCNF(id, 0);
			const Var var = solver->getVarInt(id, 0);
			solver->clearAssump();
			solver->addAssump(var, false ^ inv);
			if(solver->solve())
				{ satCount[3] += 1; collectPattern(); updateFecGrpSat(1); }
			else
			{
				satCount[4] += 1; solver->addConflict();
				a->setEqGate(ntk->getGate(candId), inv, getHeader(FRAIG_FRAIG));
			}
		}
		else
		{
			solver->convertToCNF(id,     0);
			const Var var     = solver->getVarInt(id,     0);
			solver->convertToCNF(candId, 0);
			const Var candVar = solver->getVarInt(candId, 0);
			size_t SAT = 0;
			solver->clearAssump();
			solver->addAssump(var, false);
			solver->addAssump(candVar, true ^ inv);
			if(solver->solve()) { SAT += 1; collectPattern(); } else solver->addConflict();
			solver->clearAssump();
			solver->addAssump(var, true);
			solver->addAssump(candVar, false ^ inv);
			if(solver->solve()) { SAT += 1; collectPattern(); } else solver->addConflict();
			satCount[SAT] += 1;
			SAT == 0 ? a->setEqGate(ntk->getGate(candId), inv, getHeader(FRAIG_FRAIG)) : updateFecGrpSat(SAT);
		}
	}
	for(size_t i = 0, O = ntk->getOutputNum(); i < O; ++i)
		ntk->getOutput(i)->getFanIn0().checkEqGate();
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
		ntk->getLatch(i)->getFanIn0().checkEqGate();
	for(AigAnd* a: dfsList)
		if(a->toDelete())
			ntk->removeGate(a->getGateID());
	unsigned totalCount = 0;
	cout << RepeatChar('-', 72) << endl
	     << "Optimize = " << fraigCount[FRAIG_OPTIMIZE]
	     << ", Strash = " << fraigCount[FRAIG_STRASH];
	totalCount += fraigCount[FRAIG_OPTIMIZE];
	totalCount += fraigCount[FRAIG_STRASH];
	if(doTwoLevel) { cout << ", Rewrite = " << fraigCount[FRAIG_REWRITE]; totalCount += fraigCount[FRAIG_REWRITE]; }
	if(!noFraig) { cout << ", Fraig = " << fraigCount[FRAIG_FRAIG]; totalCount += fraigCount[FRAIG_FRAIG]; }
	cout << ", Total = " << totalCount << endl;
	if(!noFraig)
	{
		cout << RepeatChar('-', 72) << endl
		     << "Const     : SAT = " << satCount[3] << ", UNSAT = " << satCount[4] << endl
		     << "Non Const : Two SAT = " << satCount[2]
		     << ", One SAT = " << satCount[1] << ", No SAT = " << satCount[0] << endl;
		feqTarget.reset(); simValue.reset(); delete solver;
		for(LitVec* p: fecGroups) delete p;
		vector<LitVec*>().swap(fecGroups);
	}
	vector<AigAnd*>().swap(dfsList);
	return true;
}

size_t
AigFraiger::calMaxFail()const
{
	double maxFailD = 3.0;
	size_t n = ntk->getInputNum() + ntk->getLatchNum();
	for(n = n < 6 ? 0 : n - 6; n != 0; n >>= 1) maxFailD *= 1.3;
	for(n = ntk->getGateNum(); n != 0; n >>= 2) maxFailD *= 1.4;
	return size_t(maxFailD);
}

void
AigFraiger::simAllAnd()
{
	for(AigAnd* a: dfsList)
	{
		simValue[a->getGateID()]  = a->isFanIn0Inv() ? ~simValue[a->getFanIn0ID()]:
		                                                simValue[a->getFanIn0ID()];
		simValue[a->getGateID()] &= a->isFanIn1Inv() ? ~simValue[a->getFanIn1ID()]:
		                                                simValue[a->getFanIn1ID()];
	}
}

void
AigFraiger::initFecGrp()
{
	const size_t I = ntk->getInputNum();
	const size_t L = ntk->getLatchNum();
	//To let all the CIs have different pattern
	const size_t interval = numeric_limits<size_t>::max() / (I + L);
	simValue.init(ntk->getMaxGateNum());
	FecHash fecHash(dfsList.size() + I + L + 1);
	simValue[0] = 0, fecHash[0] = new LitVec(1, 0 << 1);
	size_t counter = interval;
	for(size_t i = 0; i < I; ++i, counter += interval)
		simValue[ntk->getInputID(i)] = counter,
		fecHash[counter] = new LitVec(1, ntk->getInputID(i) << 1);
	for(size_t i = 0; i < L; ++i, counter += interval)
		simValue[ntk->getLatchID(i)] = counter,
		fecHash[counter] = new LitVec(1, ntk->getLatchID(i) << 1);
	simAllAnd();
	for(AigAnd* a: dfsList)
	{
		AigGateID id = a->getGateID();
		size_t value = simValue[id];
		if(auto iter = fecHash.find(value); iter != fecHash.end())
			iter->second->push_back((id << 1) | 0);
		else if(iter = fecHash.find(~value); iter != fecHash.end())
			iter->second->push_back((id << 1) | 1);
		else
			fecHash[value] = new LitVec(1, id << 1);
	}
	for(auto fecPair: fecHash)
		if(LitVec*& fecVecPtr = fecPair.second; fecVecPtr->size() == 1)
			delete fecVecPtr;
		else
			fecGroups.push_back(fecVecPtr);
}

bool
AigFraiger::updateFecGrpRand()
{
	setRandPat(); simAllAnd();
	vector<LitVec*> oldFecGroups;
	oldFecGroups.swap(fecGroups);
	bool result = false;
	for(LitVec* curFecGrpP: oldFecGroups)
	{
		FecHash fecHash(curFecGrpP->size());
		for(unsigned lit: *curFecGrpP)
		{
			size_t value = lit & 1 ? ~simValue[lit>>1] : simValue[lit>>1];
			if(auto iter = fecHash.find(value); iter != fecHash.end())
				iter->second->push_back(lit);
			else fecHash[value] = new LitVec(1, lit);
		}
		unsigned newGrpNum = 0;
		for(auto fecPair: fecHash)
		{
			newGrpNum += 1;
			if(LitVec*& fecVecPtr = fecPair.second; fecVecPtr->size() == 1)
				delete fecVecPtr;
			else
				fecGroups.push_back(fecVecPtr);
		}
		result = result || (newGrpNum > 1);
		delete curFecGrpP;
	}
	return result;
}

void
AigFraiger::collectPattern()
{
	for(size_t i = 0, I = ntk->getInputNum(); i < I; ++i)
	{
		const AigGateID id = ntk->getInputID(i);
		simValue[id] <<= 1;
		simValue[id] |= size_t(solver->getValueBool(id, 0));
	}
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
	{
		const AigGateID id = ntk->getLatchID(i);
		simValue[id] <<= 1;
		simValue[id] |= size_t(solver->getValueBool(id, 0));
	}
}

void
AigFraiger::updateFecGrpSat(size_t SAT)
{
	assert(SAT == 1 || SAT == 2);
	const size_t maxGrpNum = 1 << SAT;
	const size_t GET_PATTERN_MASK = maxGrpNum - 1;
	simAllAnd();
	vector<LitVec*> newFecGroups;
	newFecGroups.reserve(fecGroups.size()*maxGrpNum);
	unsigned litNum = 0;
	for(const LitVec* curFecPtr: fecGroups)
	{
		const LitVec& curFecGroup = *curFecPtr;
		Array<LitVec*> newFec(maxGrpNum);
		for(size_t i = 0; i < maxGrpNum; ++i)
			newFec[i] = new LitVec;
		for(const unsigned lit: curFecGroup)
		{
			const AigGateID id = lit >> 1;
			union { AigGate* g; AigAnd* a; };
			if(g = ntk->getGate(id);
			   g->getGateType() != AIG_AND || a->noEqGate())
			{
				size_t v = lit & 1 ? ~simValue[id] : simValue[id];
				v &= GET_PATTERN_MASK;
				newFec[v]->push_back(lit);
			}
		}
		for(size_t i = 0; i < maxGrpNum; ++i)
			if(LitVec* newFecP = newFec[i]; newFecP->size() > 1)
				{ newFecGroups.push_back(newFecP); litNum += newFecP->size(); }
			else delete newFecP;
		delete curFecPtr;
	}
	newFecGroups.swap(fecGroups);
	setFeqTarget();
	cout << "Update by CEX, #FEC Group = " << fecGroups.size()
	     << ", #Literal in Group = " << litNum << endl;
}

void
AigFraiger::setRandPat()
{
	auto randPatForCI = [this](AigGateID id)
	{
		constexpr size_t GET_16BITS_MASK = 1 << 16;
		constexpr size_t shiftBitsNum    = 16;
		simValue[id] = 0;
		for(size_t i: { 0, 1, 2, 3 })
			simValue[id] |= ((size_t(rand()) % GET_16BITS_MASK) << (shiftBitsNum * i));
	};

	for(size_t i = 0, I = ntk->getInputNum(); i < I; ++i)
		randPatForCI(ntk->getInputID(i));
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
		randPatForCI(ntk->getLatchID(i));
}

void
AigFraiger::setFeqTarget()
{
	for(size_t i = 0, M = ntk->getMaxGateNum(); i < M; ++i)
		feqTarget[i] = UNDEF_GATEID;
	for(const LitVec* curFecPtr: fecGroups)
	{
		const LitVec& curFecGroup = *curFecPtr;
		const AigGateID target = curFecGroup[0] >> 1;
		const unsigned inv = curFecGroup[0] & 1;
		for(size_t i = 1, n = curFecGroup.size(); i < n; ++i)
			feqTarget[curFecGroup[i]>>1] = (target << 1) | (inv ^ (curFecGroup[i] & 1));
	}
}

bool
AigFraiger::doTwoLevelSimp(AigAnd* a)
{
	auto repMsg = [a] { cout << "Rewrite  : replace the fanin of And (" << a->getGateID() << ")..." << endl; };
	while(true)
	{
		const AigGateV in0V = a->getFanIn0();
		const AigGateV in1V = a->getFanIn1();
		const AigGate* in0 = in0V.getGatePtr();
		const AigGate* in1 = in1V.getGatePtr();
		bool isAnd0, isAnd1;
		//initialize to remove "maybe uninitialized" warning
		AigGateV in00V = 0, in01V = 0, in10V = 0, in11V = 0;
		bool in0Inv = false, in1Inv = false;
		if(isAnd0 = (in0->getGateType() == AIG_AND); isAnd0)
		{
			in00V = in0->getFanIn0();
			in01V = in0->getFanIn1();
			in0Inv = in0V.isInv();
			if(in00V == ~in1V || in01V == ~in1V)
				{ a->setEqGate(in0Inv ? in1V : ntk->getConst0V(), getHeader(FRAIG_REWRITE)); return true; }
			else
				if(in0Inv)
				{
					if(in00V == in1V)
						{ a->getFanIn0() =  in00V; a->getFanIn1() = ~in01V; repMsg(); continue; }
					if(in01V == in1V)
						{ a->getFanIn0() = ~in00V; a->getFanIn1() =  in01V; repMsg(); continue; }
				}
				else
					if(in00V == in1V || in01V == in1V)
						{ a->setEqGate(in0V, getHeader(FRAIG_REWRITE)); return true; }
		}
		if(isAnd1 = (in1->getGateType() == AIG_AND); isAnd1)
		{
			in10V = in1->getFanIn0();
			in11V = in1->getFanIn1();
			in1Inv = in1V.isInv();
			if(in10V == ~in0V || in11V == ~in0V)
				{ a->setEqGate(in1Inv ? in0V : ntk->getConst0V(), getHeader(FRAIG_REWRITE)); return true; }
			else
				if(in1Inv)
				{
					if(in10V == in0V)
						{ a->getFanIn0() =  in10V; a->getFanIn1() = ~in11V; repMsg(); continue; }
					if(in11V == in0V)
						{ a->getFanIn0() = ~in10V; a->getFanIn1() =  in11V; repMsg(); continue; }
				}
				else
					if(in10V == in0V || in11V == in0V)
						{ a->setEqGate(in1V, getHeader(FRAIG_REWRITE)); return true; }
		}
		if(isAnd0 && isAnd1)
		{
			if(in0Inv && in1Inv)
			{
				if(in00V == in10V && in01V == ~in11V)
					{ a->setEqGate(~in00V, getHeader(FRAIG_REWRITE)); return true; }
				if(in01V == in11V && in00V == ~in10V)
					{ a->setEqGate(~in01V, getHeader(FRAIG_REWRITE)); return true; }
			}
			else
			{
				if(in00V == ~in10V || in00V == ~in11V || in01V == ~in10V || in01V == ~in11V)
					{ a->setEqGate(!in0Inv ? !in1Inv ? ntk->getConst0V() : in0V : in1V, getHeader(FRAIG_REWRITE)); return true; }
				if(!in0Inv && !in1Inv)
				{
					if(in00V == in10V || in01V == in10V)
						{ a->getFanIn1() = in11V; a->checkFanInOrder(); continue; }
					if(in00V == in11V || in01V == in11V)
						{ a->getFanIn1() = in10V; a->checkFanInOrder(); continue; }
				}
				else if(in0Inv)
				{
					if(in00V == in10V || in00V == in11V)
						{ a->getFanIn0() = ~in01V; a->checkFanInOrder(); continue; }
					if(in01V == in10V || in01V == in11V)
						{ a->getFanIn0() = ~in00V; a->checkFanInOrder(); continue; }
				}
				else //in1Inv
				{
					if(in10V == in00V || in10V == in01V)
						{ a->getFanIn1() = ~in11V; a->checkFanInOrder(); continue; }
					if(in11V == in00V || in11V == in01V)
						{ a->getFanIn1() = ~in10V; a->checkFanInOrder(); continue; }
				}
			}
		}
		return false;
	}
}

}
