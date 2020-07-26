/*========================================================================\
|: [Filename] aigMisc1.cpp                                                :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Implement the warppaer classes including                   :|
:|            1. AigLeveler                                              |:
|:            2. AigFanouter                                             :|
:|            3. AigInfluencer                                           |:
|:            4. AigSimulator                                            :|
<------------------------------------------------------------------------*/

#include "aigMisc1.h"

namespace _54ff
{

bool
AigLeveler::init(const AigNtk* n, bool isLatchCO)
{
	assert(!isON());
	vector<AigAnd*> dfsList;
	if(!n->checkCombLoop(true, dfsList))
		return false;
	ntk = n;
	allLevel.init(ntk->getMaxGateNum());
	for(size_t i = 0, I = ntk->getInputNum(); i < I; ++i)
		allLevel[n->getInputID(i)] = 0;
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
		allLevel[n->getLatchID(i)] = 0;
	for(AigAnd* a: dfsList)
	{
		unsigned& level = allLevel[a->getGateID()];
		level = allLevel[a->getFanIn0ID()];
		if(unsigned l = allLevel[a->getFanIn1ID()];
		   l > level) level = l;
		level += 1;
	}
	if(isLatchCO)
		for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
			allLevel[ntk->getLatchID(i)] = allLevel[ntk->getLatch(i)->getFanIn0ID()];
	for(size_t i = 0, O = ntk->getOutputNum(); i < O; ++i)
		allLevel[ntk->getOutputID(i)] = allLevel[ntk->getOutput(i)->getFanIn0ID()];
	return true;
}

unsigned
AigLeveler::getMaxLevel()const
{
	assert(isON());
	assert(ntk->getLatchNum()  != 0 ||
	       ntk->getOutputNum() != 0);
	unsigned maxLevel = 0;
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
	{
		const unsigned l = allLevel[ntk->getLatch(i)->getFanIn0ID()];
		if(maxLevel < l) maxLevel = l;
	}
	for(size_t i = 0, O = ntk->getOutputNum(); i < O; ++i)
	{
		const unsigned l = allLevel[ntk->getOutput(i)->getFanIn0ID()];
		if(maxLevel < l) maxLevel = l;
	}
	return maxLevel;
}

unsigned
AigLeveler::getMinLevel()const
{
	assert(isON());
	assert(ntk->getLatchNum()  != 0 ||
	       ntk->getOutputNum() != 0);
	unsigned minLevel = numeric_limits<unsigned>::max();
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
	{
		const unsigned l = allLevel[ntk->getLatch(i)->getFanIn0ID()];
		if(minLevel > l) minLevel = l;
	}
	for(size_t i = 0, O = ntk->getOutputNum(); i < O; ++i)
	{
		const unsigned l = allLevel[ntk->getOutput(i)->getFanIn0ID()];
		if(minLevel > l) minLevel = l;
	}
	return minLevel;
}

double
AigLeveler::getAveLevel()const
{
	assert(isON());
	assert(ntk->getLatchNum()  != 0 ||
	       ntk->getOutputNum() != 0);
	unsigned levelSum = 0;
	const size_t L = ntk->getLatchNum(); 
	for(size_t i = 0; i < L; ++i)
		levelSum += allLevel[ntk->getLatch(i)->getFanIn0ID()];
	const size_t O = ntk->getOutputNum();
	for(size_t i = 0; i < O; ++i)
		levelSum += allLevel[ntk->getOutput(i)->getFanIn0ID()];
	return double(levelSum) / (L + O);
}

auto
AigLeveler::getAllLevel()const -> AllLevel
{
	assert(isON());
	assert(ntk->getLatchNum()  != 0 ||
	       ntk->getOutputNum() != 0);
	unsigned maxLevel = 0;
	unsigned minLevel = numeric_limits<unsigned>::max();
	unsigned levelSum = 0;
	const size_t L = ntk->getLatchNum(); 
	for(size_t i = 0; i < L; ++i)
	{
		const unsigned l = allLevel[ntk->getLatch(i)->getFanIn0ID()];
		if(maxLevel < l) maxLevel = l;
		if(minLevel > l) minLevel = l;
		levelSum += l;
	}
	const size_t O = ntk->getOutputNum();
	for(size_t i = 0; i < O; ++i)
	{
		const unsigned l = allLevel[ntk->getOutput(i)->getFanIn0ID()];
		if(maxLevel < l) maxLevel = l;
		if(minLevel > l) minLevel = l;
		levelSum += l;
	}
	return { maxLevel, minLevel, double(levelSum) / (L + O) };
}

/**************************************/
/**************************************/
/**************************************/
/**************************************/
/**************************************/
/**************************************/

void
AigFanouter::buildFanout()
{
	const size_t M = ntk->getMaxGateNum();

	Array<unsigned>  fanOutNum(M);
	Array<unsigned>& fanOutIdx = fanOutNum;
	for(size_t i = 0; i < M; ++i)
		fanOutNum[i] = 0;
	for(size_t i = 1; i < M; ++i)
		if(AigGate* g = ntk->getGate(i); g != 0)
			switch(g->getGateType())
			{
				case AIG_LATCH:
				case AIG_PO:
					fanOutNum[g->getFanIn0ID()] += 1; break;

				case AIG_AND:
					fanOutNum[g->getFanIn0ID()] += 1;
					fanOutNum[g->getFanIn1ID()] += 1; break;

				default:
					assert(g->isPI());
			}

	idxBegin.init(M+1);
	idxBegin[0] = 0;
	for(size_t i = 1; i <= M; ++i)
		idxBegin[i] = idxBegin[i-1] + fanOutNum[i-1],
		fanOutIdx[i-1] = 0;

	const size_t totalFanOutNum = ntk->getAndNum() * 2 +
	                              ntk->getLatchNum() + ntk->getOutputNum();
	assert(idxBegin[M] == totalFanOutNum);
	fanOut.init(totalFanOutNum);
	auto setFanOutID = [this, &fanOutIdx](AigGateID id, AigGateID fanOutID)
		{ fanOut[idxBegin[id]+(fanOutIdx[id]++)] = fanOutID; };
	for(size_t i = 1; i < M; ++i)
		if(AigGate* g = ntk->getGate(i); g != 0)
			switch(g->getGateType())
			{
				case AIG_LATCH:
				case AIG_PO:
					setFanOutID(g->getFanIn0ID(), i); break;

				case AIG_AND:
					setFanOutID(g->getFanIn0ID(), i);
					setFanOutID(g->getFanIn1ID(), i); break;

				default:
					assert(g->isPI());
			}
	for(size_t i = 0; i < M; ++i)
		assert(fanOutIdx[i] == idxBegin[i+1] - idxBegin[i]);
}

/**************************************/
/**************************************/
/**************************************/
/**************************************/
/**************************************/
/**************************************/

bool
AigInfluencer::init(const AigNtk* n)
{
	vector<AigAnd*> dfsList;
	if(!n->checkCombLoop(true, dfsList))
		return false;
	ntk = n;

	allInflu.init(ntk->getMaxGateNum());
	allInflu[0] = INFLU_CONST;
	for(size_t i = 0, I = ntk->getInputNum(); i < I; ++i)
		allInflu[ntk->getInputID(i)] = INFLU_PI;
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
		allInflu[ntk->getLatchID(i)] = INFLU_LATCH;
	for(AigAnd* a: dfsList)
		allInflu[a->getGateID()] = allInflu[a->getFanIn0ID()] |
		                           allInflu[a->getFanIn1ID()];
	for(size_t i = 0, O = ntk->getOutputNum(); i < O; ++i)
		allInflu[ntk->getOutputID(i)] = allInflu[ntk->getOutputNorm(i)->getFanIn0ID()];
	return true;
}

void
AigInfluencer::convertToOrder()
{
	constexpr unsigned order[INFLU_TOTAL] = { 2, 2, 0, 1 };
	for(size_t i = 0, M = ntk->getMaxGateNum(); i < M; ++i)
		allInflu[i] = order[allInflu[i]];
}

/**************************************/
/**************************************/
/**************************************/
/**************************************/
/**************************************/
/**************************************/

void
AigSimulator::setAllToDCorNone()
{
	for(size_t i = 0, M = ntk->getMaxGateNum(); i < M; ++i)
		simValue[i] = ntk->getGate(i) == 0 ? ThreeValue_None : ThreeValue_DC;
}

void
AigSimulator::setInitState()
{
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
		setLatchValue(i, ThreeValue_False);
}

void
AigSimulator::newEvent(AigGateID id)
{
	assert(!hasEvent[id]);
	eventList[level.getLevel(id)].push_back(id);
	hasEvent[id] = true;
}

void
AigSimulator::setValueEvent(AigGateID id, ThreeValue v)
{
	assert(readyForEvent());
	if(simValue[id] != v)
	{
		simValue[id] = v;
		for(size_t i = 0, n = fanOut.getFanOutNum(id); i < n; ++i)
			newEventCheck(fanOut.getFanOutID(id, i));
	}
}

void
AigSimulator::setValueEventRef(AigGateID id, ThreeValue v)
{
	assert(readyForEvent());
	if(simValue[id] != v)
	{
		simValue[id] = v;
		for(size_t i = 0, n = fanOut.getFanOutNum(id); i < n; ++i)
			if(AigGateID fanOutId = fanOut.getFanOutID(id, i);
			   ntk->getGate(fanOutId)->isGlobalRef())
				newEventCheck(fanOutId);
	}
}

ThreeValue
AigSimulator::simOneAndValue(AigAnd* a)const
{
	return simAnd(a->isFanIn0Inv() ? simNot(simValue[a->getFanIn0ID()]):
	                                        simValue[a->getFanIn0ID()],
	              a->isFanIn1Inv() ? simNot(simValue[a->getFanIn1ID()]):
	                                        simValue[a->getFanIn1ID()]);
}

ThreeValue
AigSimulator::simOneCOValue(AigGate* g)const
{
	return g->isFanIn0Inv() ? simNot(simValue[g->getFanIn0ID()]):
	                                 simValue[g->getFanIn0ID()];
}

void
AigSimulator::simAllLatch()
{
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
		simLatch(i);
}

void
AigSimulator::simAllOutput()
{
	for(size_t i = 0, O = ntk->getOutputNum(); i < O; ++i)
		simOutput(i);
}

void
AigSimulator::simByEvent()
{
	for(size_t l = 0, maxLevel = level.getMaxLevel(); l <= maxLevel; ++l)
	{
		for(AigGateID eventId: eventList[l])
		{
			switch(AigGate* g = ntk->getGate(eventId);
			       g->getGateType())
			{
				case AIG_LATCH:
				case AIG_PO:
					setValueEvent(eventId, simOneCOValue(g));
					break;

				case AIG_AND:
					setValueEvent(eventId, simOneCOValue((AigAnd*)g));
					break;

				default:
					assert(false);
			}
			hasEvent[eventId] = false;
		}
		eventList[l].clear();
	}
}

void
AigSimulator::simAllLatchSynch()
{
	const size_t L = ntk->getLatchNum();
	for(size_t i = 0; i < L; ++i)
		nextState[i] = simOneCOValue(ntk->getLatchNorm(i));
	for(size_t i = 0; i < L; ++i)
		simValue[ntk->getLatchID(i)] = nextState[i];
}

void
AigSimulator::printAllGate(ostream& os)const
{
	for(size_t i = 0, M = ntk->getMaxGateNum(); i < M; ++i)
		os << getSymbol(getValue(i));
}

void
AigSimulator::printAllInput(ostream& os)const
{
	for(size_t i = 0, I = ntk->getInputNum(); i < I; ++i)
		os << getSymbol(getInputValue(i));
}

void
AigSimulator::printAllLatch(ostream& os)const
{
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
		os << getSymbol(getLatchValue(i));
}

void
AigSimulator::printAllOutput(ostream& os)const
{
	for(size_t i = 0, O = ntk->getOutputNum(); i < O; ++i)
		os << getSymbol(getOutputValue(i));
}

void
AigSimulator::genDfsList(const vector<AigGateID>& target)
{
	AigGate::setGlobalRef(); 
	for(AigGateID id: target)
		ntk->getGate(id)->genDfsList(dfsList);
}

void
AigSimulator::doConstProp(const vector<AigGateID>& target)
{
	size_t s = 0;
	for(size_t i = 0, n = dfsList.size(); i < n; ++i)
		if(getValue(dfsList[i]->getGateID()) == ThreeValue_DC)
			dfsList[s++] = dfsList[i];
	dfsList.resize(s);
	AigGate::setGlobalRef();
	for(AigAnd* a: dfsList)
		a->setToGlobalRef();
	AigGate::setGlobalRef();
	for(AigGateID id: target)
		if(AigGate* g = ntk->getGate(id);
		   g->isGlobalRef(1))
			g->setToGlobalRef();
	for(size_t i = s - 1; i != MAX_SIZE_T; --i)
	{
		if(dfsList[i]->isGlobalRef())
			continue;
		AigGateID id = dfsList[i]->getGateID();
		for(size_t j = 0, n = fanOut.getFanOutNum(id); j < n; ++j)
			if(fanOut.getFanOut(id, j)->isGlobalRef())
				goto KEEP;
		dfsList[i] = 0;
		continue;
		KEEP: dfsList[i]->setToGlobalRef();
	}
	size_t ss = 0;
	for(size_t i = 0; i < s; ++i)
		if(dfsList[i] != 0)
			dfsList[ss++] = dfsList[i];
	dfsList.resize(ss);
}

unsigned
AigSimulator::getMaxLevel(const vector<AigGateID>& target)const
{
	assert(level.isON());
	unsigned maxLevel = 0;
	for(AigGateID id: target)
		if(unsigned l = level.getLevel(id);
		   l > maxLevel) maxLevel = l;
	return maxLevel;
}

void
AigSimulator::initEventList()
{
	assert(level.isON());
	assert(eventList.empty());
	assert(hasEvent.empty());
	eventList.init(level.getMaxLevel()+1);
	const size_t M = ntk->getMaxGateNum();
	hasEvent.init(M);
	for(size_t i = 0; i < M; ++i)
		hasEvent[i] = false;
}

void
AigSimulator::markDfsCone(const vector<AigGateID>& target)
{
	AigGate::setGlobalRef();
	for(AigGateID id: target)
		ntk->getGate(id)->traverseFromCO();
}

void
AigSimulator::markDfsList()const
{
	AigGate::setGlobalRef();
	for(AigAnd* a: dfsList)
		a->setToGlobalRef();
}

void
AigSimulator::simByEventRef(unsigned maxLevel)
{
	assert(readyForEvent());
	for(unsigned l = 0; l <= maxLevel; ++l)
	{
		for(AigGateID eventId: eventList[l])
			setValueEventRef(eventId, simOneAndValue((AigAnd*)(ntk->getGate(eventId)))),
			hasEvent[eventId] = false;
		eventList[l].clear();
	}
}

void
AigSimulator::cleanEvent(unsigned maxLevel)
{
	for(unsigned l = 0; l <= maxLevel; ++l)
	{
		for(AigGateID eventId: eventList[l])
			hasEvent[eventId] = false;
		eventList[l].clear();
	}
}

void
AigSimulator::backwardTerSim(CirSolver* solver, const vector<AigGateID>& target,
                             vector<AigGateID>& genCube, const Array<double>& activity, bool reverse)
{
	assert(genCube.empty());
	for(size_t i = 0, maxLevel = level.getMaxLevel(); i <= maxLevel; ++i)
		assert(eventList[i].empty());
	for(size_t i = 0, M = ntk->getMaxGateNum(); i < M; ++i)
		assert(!hasEvent[i]);

	for(AigGateID id: target)
		newEventCheck(id);

	for(unsigned l = getMaxLevel(target); l > 0; --l)
	{
		for(AigGateID careId: eventList[l])
		{
			if(solver->getValueBool(careId, 0))
			{
				//Output = 1, Two fanins should be cared
				AigGate* g = ntk->getGate(careId);
				assert(solver->getValueBool(g->getFanIn0ID(), 0) ^ g->isFanIn0Inv());
				assert(solver->getValueBool(g->getFanIn1ID(), 0) ^ g->isFanIn1Inv());
				newEventCheck(g->getFanIn0ID());
				newEventCheck(g->getFanIn1ID());
			}
			else
			{
				//Output = 0
				AigGate* g = ntk->getGate(careId);
				AigGateID fanIn0Id = g->getFanIn0ID();
				bool isFanIn0False = solver->getValueBool(fanIn0Id, 0) == g->isFanIn0Inv();
				AigGateID fanIn1Id = g->getFanIn1ID();
				bool isFanIn1False = solver->getValueBool(fanIn1Id, 0) == g->isFanIn1Inv();
				assert(isFanIn0False || isFanIn1False);
				if(isFanIn0False && isFanIn1False)
				{
					if(!hasEvent[fanIn0Id] && !hasEvent[fanIn1Id])
					{
						constexpr bool staticChoice = false;
						if constexpr(staticChoice)
							newEventCheck(fanIn1Id);
						else
						{
							if(unsigned diff = influ.getValue(fanIn0Id) - influ.getValue(fanIn1Id);
							   diff != 0) newEventCheck(int(diff) > 0 ? fanIn0Id : fanIn1Id);
							else if(diff = level.getLevel(fanIn0Id) - level.getLevel(fanIn1Id);
							        diff != 0) newEventCheck(int(diff) < 0 ? fanIn0Id : fanIn1Id);
							else if(!activity.empty())
								newEventCheck(((activity[fanIn0Id] > activity[fanIn1Id]) ^ reverse) ? fanIn0Id : fanIn1Id);
							else newEventCheck(fanIn1Id);
						}
					}
				}
				else newEventCheck(isFanIn0False ? fanIn0Id : fanIn1Id);
			}
			hasEvent[careId] = false;
		}
		eventList[l].clear();
	}

	for(AigGateID cubeId: eventList[0])
	{
		switch(ntk->getGate(cubeId)->getGateType())
		{
			default: assert(false);
			case AIG_PI:
			case AIG_CONST0: break;
			case AIG_LATCH:
				genCube.push_back(makeToLit(cubeId, !solver->getValueBool(cubeId, 0)));
				break;
		}
		hasEvent[cubeId] = false;
	}
	eventList[0].clear();
}

ThreeValue
AigSimulator::simNot(ThreeValue v)
{
	/*
	ori | 0 | 1 | X       ori | 0 | 1 | 2
	----------------  ->  ----------------
	not | 1 | 0 | X       not | 1 | 0 | 2

	from right to left, from MSB to LSB
	b100001 = d33
	*/

	constexpr ThreeValue magicNum = 33;
	return (magicNum >> (v << 1)) & 3;
	// Equivalent to
	constexpr ThreeValue LUT[3] = { ThreeValue_True, ThreeValue_False, ThreeValue_DC };
	return LUT[v];
}

ThreeValue
AigSimulator::simAnd(ThreeValue v1, ThreeValue v2)
{
	/*
	   | 0 | 1 | X          | 0 | 1 | 2
	---------------      ---------------
	 0 | 0 | 0 | 0        0 | 0 | 0 | 0
	---------------  ->  ---------------
	 1 | 0 | 1 | X        1 | 0 | 1 | 2
	---------------      ---------------
	 X | 0 | X | X        2 | 0 | 2 | 2

	    ori | 0.0 | 0.1 | 0.2 | 0.3 | 1.0 | 1.1 | 1.2 | 1.3 | 2.0 | 2.1 | 2.2 | 2.3
	->  ----------------------------------------------------------------------------
	    and |  0  |  0  |  0  |  D  |  0  |  1  |  2  |  D  |  0  |  2  |  2  |  D

	from right to left, from MSB to LSB
	b001010000010010000000000 = d2630656
	*/

	constexpr ThreeValue magicNum = 2630656;
	return (magicNum >> (((v1 << 2) | v2) << 1)) & 3;
	// Equivalent to
	constexpr ThreeValue LUT[3][3] = {{ ThreeValue_False, ThreeValue_False, ThreeValue_False },
	                                  { ThreeValue_False, ThreeValue_True,  ThreeValue_DC    },
	                                  { ThreeValue_False, ThreeValue_DC,    ThreeValue_DC    }};
	return LUT[v1][v2];
}

char
AigSimulator::getSymbol(ThreeValue v)
{
	/*
	value  |  0  |  1  |  2  |  3
	-------------------------------
	symbol | '0' | '1' | 'X' | 'N'

	'0' = b00110000
	'1' = b00110001
	'X' = b01011000
	'N' = b01001110
	b01001110010110000011000100110000 = d1314402608
	*/

	constexpr unsigned magicNum = 1314402608;
	return char((magicNum >> (v << 3)) & 0xFF);
	// Equivalent to
	constexpr char LUT[4] = { '0', '1', 'X', 'N' };
	return LUT[v];
}

}