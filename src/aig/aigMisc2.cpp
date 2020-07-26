/*========================================================================\
|: [Filename] aigMisc2.cpp                                               :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Implement the warppaer classes including                   :|
:|            5. AigConster                                              |:
|:            6. AigCutter                                               :|
<------------------------------------------------------------------------*/

#include "aigMisc2.h"
#include "cirSolver.h"
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

/**************************************/
/**************************************/
/**************************************/
/**************************************/
/**************************************/

bool
AigCut::operator<(const AigCut& c)const
{
	if(numLeaves != c.numLeaves)
		return numLeaves > c.numLeaves;
	// Compare average number of fanout here
	// Since numLeaves are the same, just compare the sum
	return fanOutSum < c.fanOutSum;
}

AigCut&
AigCut::operator=(const AigCut& c)
{
	absValue   = c.absValue;
    truthTable = c.truthTable;
    fanOutSum  = c.fanOutSum;
    numLeaves  = c.numLeaves;
	for(unsigned i = 0; i < numLeaves; ++i)
		leaves[i] = c.leaves[i];
	return *this;
}

bool
AigCut::dominate(const AigCut& c)const
{
	if(numLeaves > c.numLeaves ||
	   (absValue & ~c.absValue) != 0)
		return false;
/*
	for(unsigned i = 0, j = 0; i < numLeaves; ++i, ++j)
	{
		for(; j < c.numLeaves; ++j)
			if(leaves[i] == c.leaves[j])
				goto OK;
			else if(leaves[i] < c.leaves[j])
				return false;
		return false;
		OK: {}
	}
*/
	for(unsigned i = 0; i < numLeaves; ++i)
	{
		for(unsigned j = 0; j < c.numLeaves; ++j)
			if(leaves[i] == c.leaves[j])
				goto OK;
		return false;
		OK: {}
	}
	return true;
}

unsigned
AigCut::numOnes()const
{
	constexpr bool useAbcStyle = true;
	unsigned tmp = absValue;
	if constexpr(useAbcStyle)
	{
		tmp = (tmp & 0x55555555) + ((tmp >> 1) & 0x55555555);
		tmp = (tmp & 0x33333333) + ((tmp >> 2) & 0x33333333);
		tmp = (tmp & 0x0F0F0F0F) + ((tmp >> 4) & 0x0F0F0F0F);
		tmp = (tmp & 0x00FF00FF) + ((tmp >> 8) & 0x00FF00FF);
		return (tmp & 0x0000FFFF) + (tmp >> 16);
	}
	else
	{
		unsigned ans = 0;
		for(; tmp != 0; tmp >>= 1)
			ans += (tmp & unsigned(1));
		return ans;
	}
}

bool
AigCut::isRedundant(unsigned idx)const
{
	return (truthTable & varPosMask[idx]) == ((truthTable & ~varPosMask[idx]) << (1 << idx));
}

void
AigCut::calAbsValue()
{
	absValue = 0;
	for(unsigned i = 0; i < numLeaves; ++i)
		absValue |= (unsigned(1) << (leaves[i] & 31));
}

void
AigCut::calFanOutNum(const Array<unsigned>& fanOutNum)
{
	unsigned tmp = 0;
	for(unsigned i = 0; i < numLeaves; ++i)
		tmp += fanOutNum[leaves[i]];
	if(tmp >= (unsigned(1) << 11))
		tmp = (unsigned(1) << 11) - 1;
	fanOutSum = tmp;
}

AigCutter::AigCutter(AigNtk* n, unsigned maxL, unsigned maxCN)
: ntk       (n)
, maxLeaves (maxL)
, maxCutNum (maxCN)
{
	assert(maxLeaves > 0 && maxLeaves < (unsigned(1) << 5));
	assert(maxCutNum > 0);
	prepare();
	buildCut();
}

void
AigCutter::prepare()
{
	/*
	|<- maxN * L AigCut(4) ->|<- one AigCut(4) ->|<- maxN * L unsigned ->|
	          allCuts                tmpCut               cutIdx
	*/
	size_t sizePerCut = AigCut::calSize(maxLeaves);
	size_t numBytes1 = 1;
	numBytes1 *= ntk->getMaxGateNum();
	numBytes1 *= maxCutNum;
	numBytes1 += 1;
	numBytes1 *= sizePerCut;
	size_t numBytes2 = 1;
	numBytes2 *= ntk->getMaxGateNum();
	numBytes2 *= maxCutNum;
	numBytes2 *= sizeof(unsigned);
	allCuts = (AigCut*)operator new(numBytes1 + numBytes2);
	cutIdx = (unsigned*)((char*)allCuts + numBytes1);
	tmpCut = (AigCut*)((char*)cutIdx - sizePerCut);

	fanOutNum.init(ntk->getMaxGateNum());
	for(size_t i = 0, M = ntk->getMaxGateNum(); i < M; ++i)
		fanOutNum[i] = 0;
	for(size_t i = 0, M = ntk->getMaxGateNum(); i < M; ++i)
		if(AigGate* g = ntk->getGate(i); g != 0)
			switch(g->getFanInNum())
			{
				case 1: fanOutNum[g->getFanIn0ID()] += 1; break;
				case 2: fanOutNum[g->getFanIn0ID()] += 1;
				        fanOutNum[g->getFanIn1ID()] += 1; break;
				default: break;
			}
}

void
AigCutter::buildCut()
{
	vector<AigAnd*> dfsList;
	if(!ntk->checkCombLoop(true, dfsList))
		{ cerr << "[Error] The network for AigCutter should be acyclic!" << endl; return; }

	addConst0Cut();
	for(size_t i = 0, I = ntk->getInputNum(); i < I; ++i)
		addUnitCut(ntk->getInputID(i), true);
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
		addUnitCut(ntk->getLatchID(i), true);
	for(AigAnd* a: dfsList)
		genCutForAndGate(a);
}

void
AigCutter::addConst0Cut()
{
	cutIdx[0] = 0;
	if(maxCutNum > 1)
		cutIdx[1] = MAX_UNSIGNED;
	AigCut& unitCut = getCut(0);
	if(maxLeaves == 4)
		unitCut.truthTable = 0;
	// These three values are set to 0 in case we need it someday
	unitCut.numLeaves = 0;
	unitCut.absValue  = 0;
	unitCut.fanOutSum = 0;
}

void
AigCutter::addUnitCut(AigGateID id, bool isCI)
{
	if(isCI) assert(ntk->getGate(id)->isCI());
	unsigned idx = getIdxBaseNum(id);
	unsigned* cut = getIdxBasePtr(id);
	for(unsigned i = 0; i < maxCutNum; ++i)
		cut[i] = idx + i;
	AigCut& unitCut = getCut(idx);
	if(maxLeaves == 4)
		unitCut.truthTable = 0xAAAA;
    unitCut.numLeaves = 1;
    unitCut.leaves[0] = id;
	unitCut.calAbsValue();
	unitCut.calFanOutNum(fanOutNum);
	if(isCI && maxCutNum > 1)
		cut[1] = MAX_UNSIGNED;
}

void
AigCutter::genCutForAndGate(AigAnd* a)
{
	addUnitCut(a->getGateID());
	unsigned* cut = getIdxBasePtr(a->getGateID());
	unsigned* cutIn0 = getIdxBasePtr(a->getFanIn0ID());
	unsigned* cutIn1 = getIdxBasePtr(a->getFanIn1ID());
	unsigned idx = 1;
	for(size_t i = 0; i < maxCutNum && cutIn0[i] != MAX_UNSIGNED; ++i)
		for(size_t j = 0; j < maxCutNum && cutIn1[j] != MAX_UNSIGNED; ++j)
		{
			AigCut& cut0 = getCut(cutIn0[i]);
			AigCut& cut1 = getCut(cutIn1[j]);

			if(!checkNumOnes(cut0, cut1)                                     ||
			   !mergeTwoCuts(cut0, a->isFanIn0Inv(), cut1, a->isFanIn1Inv()) ||
			   !checkDominance(cut, idx)) continue;
			if(tmpCut->numLeaves <= 1)
				{ getCut(cut[0]) = *tmpCut; idx = 1; goto END; }
			checkInsertion(cut, idx);
		}
END:
	if(idx < maxCutNum)
		cut[idx] = MAX_UNSIGNED;

cout << a->getGateID() << ": " << idx << " " << cut[0] << " " << PrintHex(getCut(cut[0]).truthTable);
if(getCut(cut[0]).numLeaves != 0)
{ assert(getCut(cut[0]).numLeaves == 1); cout << ", " << getCut(cut[0]).leaves[0]; }
cout << endl;
}

bool
AigCutter::checkNumOnes(const AigCut& c1, const AigCut& c2)
{
	tmpCut->absValue = c1.absValue | c2.absValue;
	return tmpCut->numOnes() <= maxLeaves;
}

bool
AigCutter::mergeTwoCuts(const AigCut& c1, bool inv1, const AigCut& c2, bool inv2)
{
	tmpCut->numLeaves = 0;
	unsigned i = 0, j = 0;
/* style 1
	while(i < c1.numLeaves && j < c2.numLeaves)
		if(tmpCut->numLeaves == maxLeaves)
			return false;
		else if(c1.leaves[i] == c2.leaves[j])
			{ tmpCut->leaves[tmpCut->numLeaves++] = c1.leaves[i]; ++i; ++j; }
		else if(c1.leaves[i] < c2.leaves[j])
			tmpCut->leaves[tmpCut->numLeaves++] = c1.leaves[i++];
		else // c1.leaves[i] > c2.leaves[j]
			tmpCut->leaves[tmpCut->numLeaves++] = c2.leaves[j++];

	while(i < c1.numLeaves)
		if(tmpCut->numLeaves == maxLeaves)
			return false;
		else
			tmpCut->leaves[tmpCut->numLeaves++] = c1.leaves[i++];
	while(j < c2.numLeaves)
		if(tmpCut->numLeaves == maxLeaves)
			return false;
		else
			tmpCut->leaves[tmpCut->numLeaves++] = c2.leaves[j++];
*/
/* style 2 */
	for(; tmpCut->numLeaves < maxLeaves; ++tmpCut->numLeaves)
		switch(unsigned(i < c1.numLeaves) << 1 | unsigned(j < c2.numLeaves))
		{
			case BOTH:
				if(int diff = c1.leaves[i] - c2.leaves[j];
				   diff == 0)
					{ tmpCut->leaves[tmpCut->numLeaves] = c1.leaves[i]; ++i; ++j; }
				else
					tmpCut->leaves[tmpCut->numLeaves] = diff < 0 ? c1.leaves[i++] : c2.leaves[j++];
				break;

			case ONLY_1:
				tmpCut->leaves[tmpCut->numLeaves] = c1.leaves[i++];
				break;

			case ONLY_0:
				tmpCut->leaves[tmpCut->numLeaves] = c2.leaves[j++];
				break;

			case NONE:
				goto OK;
		}
	if(i < c1.numLeaves || j < c2.numLeaves)
		return false;
OK: {}
/**/
	calTruthTable(c1, inv1, c2, inv2);
	checkRedundant();
	tmpCut->calFanOutNum(fanOutNum);
	return true;
}

bool
AigCutter::checkDominance(unsigned* base, unsigned& size)
{
	bool dominating = false;
	unsigned curSize = 0;
	for(unsigned i = 0; i < size; ++i)
		if(!dominating && getCut(base[i]).dominate(*tmpCut))
			return false;
		else if(!tmpCut->dominate(getCut(base[i])))
		{
			if(i != curSize)
				xorSwap(base[i], base[curSize]);
			curSize += 1;
		}
	size = curSize;
	return true;
}

void
AigCutter::calTruthTable(const AigCut& c1, bool inv1, const AigCut& c2, bool inv2)
{
	auto shiftVarTruth = [this](const AigCut& c) -> unsigned
	{
		unsigned truth = c.truthTable;
		for(unsigned i = c.numLeaves - 1, j = tmpCut->numLeaves - 1; i != MAX_UNSIGNED; --i)
		{
			for(; c.leaves[i] != tmpCut->leaves[j]; --j)
				assert(j != MAX_UNSIGNED);
			if(i != j)
				truth = swapVarTruthTable(truth, i, j);
		}
		assert(truth < (1 << (1 << 4)));
		return truth;
	};

	if(maxLeaves != 4)
		return;

	unsigned truth1 = shiftVarTruth(c1);
	unsigned truth2 = shiftVarTruth(c2);
	if(inv1) truth1 = ~truth1;
	if(inv2) truth2 = ~truth2;
	tmpCut->truthTable = truth1 & truth2;
}

void
AigCutter::checkRedundant()
{
	if(maxLeaves != 4)
		return;

	unsigned s = 0;
	for(unsigned i = 0; i < tmpCut->numLeaves; ++i)
		if(!tmpCut->isRedundant(i))
		{
			if(i != s)
			{
				tmpCut->leaves[s] = tmpCut->leaves[i];
				tmpCut->truthTable = swapVarTruthTable(tmpCut->truthTable, s, i);
			}
			s += 1;
		}
	if(tmpCut->numLeaves != s)
		{ tmpCut->numLeaves = s; tmpCut->calAbsValue(); }
	for(unsigned i = tmpCut->numLeaves; i < maxLeaves; ++i)
		assert(tmpCut->isRedundant(i));
}

void
AigCutter::checkInsertion(unsigned* base, unsigned& idx)
{
	assert(tmpCut->numLeaves > 1);
	assert(idx <= maxCutNum);
	if(idx < maxCutNum)
		getCut(base[idx++]) = *tmpCut;
	else if(getCut(base[idx-1]) < *tmpCut)
		getCut(base[idx-1]) = *tmpCut;
	else return;

	unsigned tmp = base[idx-1];
	unsigned i = idx - 1;
	for(; i > 0 && getCut(base[i-1]) < getCut(tmp); --i)
		base[i] = base[i-1];
	base[i] = tmp;
}

unsigned
AigCutter::swapVarTruthTable(unsigned truthTable, unsigned idx1, unsigned idx2)
{
	assert(truthTable < (1 << (1 << 4)));
	switch(idx1)
	{
		case 0:
			switch(idx2)
			{
				case 1: return (truthTable & 0x9999) | ((truthTable & 0x4444) >> 1) | ((truthTable & 0x2222) << 1);
				case 2: return (truthTable & 0xA5A5) | ((truthTable & 0x5050) >> 3) | ((truthTable & 0x0A0A) << 3);
				case 3: return (truthTable & 0xAA55) | ((truthTable & 0x5500) >> 7) | ((truthTable & 0x00AA) << 7);
				default: assert(false);
			} break;

		case 1:
			switch(idx2)
			{
				case 2: return (truthTable & 0xC3C3) | ((truthTable & 0x3030) >> 2) | ((truthTable & 0x0C0C) << 2);
				case 3: return (truthTable & 0xCC33) | ((truthTable & 0x3300) >> 6) | ((truthTable & 0x00CC) << 6);
				default: assert(false);
			} break;

		case 2:
			switch(idx2)
			{
				case 3: return (truthTable & 0xF00F) | ((truthTable & 0x0F00) >> 4) | ((truthTable & 0x00F0) << 4);
				default: assert(false);
			} break;

		default: assert(false);
	}
}

}
