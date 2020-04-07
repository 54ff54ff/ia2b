/*========================================================================\
|: [Filename] aigMisc1.h                                                  :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Define various wrapper classes to perform operations on    :|
:|            AIG network                                                |:
|:            1. AigLeveler to calculate the circuit depth               :|
:|            2. AigFanouter to record the fanout information            |:
|:            3. AigInfluencer to record the support variable            :|
:|               distribution for each gate                              |:
|:            4. AigSimulator to perform three value (True, False,       :|
:|               Don't-care) simulation                                  |:
<------------------------------------------------------------------------*/

#ifndef HEHE_AIGMISC1_H
#define HEHE_AIGMISC1_H

#include "aigNtk.h"
#include "cirSolver.h"

namespace _54ff
{

class AigLeveler
{
public:
	AigLeveler(): ntk(0) {}

	bool init(const AigNtk*, bool);
	bool isON()const { return ntk != 0; }

	unsigned getLevel(AigGateID id)const { return allLevel[id]; }
	unsigned getMaxLevel()const;
	unsigned getMinLevel()const;
	double getAveLevel()const;
	using AllLevel = tuple<size_t, size_t, double>;
	AllLevel getAllLevel()const;

private:
	const AigNtk*    ntk;
	Array<unsigned>  allLevel;
};

/**************************************/
/**************************************/
/**************************************/
/**************************************/
/**************************************/
/**************************************/

class AigFanouter
{
public:
	AigFanouter(): ntk(0) {}
	AigFanouter(const AigNtk* n): ntk(n) { buildFanout(); }

	void init(const AigNtk* n) { assert(!isON()); ntk = n; buildFanout(); }
	bool isON()const { return ntk != 0; }

	size_t getFanOutNum(AigGateID id)const { return idxBegin[id+1] - idxBegin[id]; }
	AigGateID getFanOutID(AigGateID id, size_t i)const { return fanOut[idxBegin[id]+i]; }
	AigGate* getFanOut(AigGateID id, size_t i)const { return ntk->getGate(getFanOutID(id, i)); }

private:
	void buildFanout();

private:
	const AigNtk*     ntk;
	Array<unsigned>   idxBegin;
	Array<AigGateID>  fanOut;
};

/**************************************/
/**************************************/
/**************************************/
/**************************************/
/**************************************/
/**************************************/

enum
{
	INFLU_CONST = 0,
	INFLU_PI,
	INFLU_LATCH,
	INFLU_BOTH,
	INFLU_TOTAL
};

class AigInfluencer
{
public:
	AigInfluencer(): ntk(0) {}

	bool init(const AigNtk*);
	bool isON()const { return ntk != 0; }

	void convertToOrder();
	unsigned getValue(AigGateID id)const { return allInflu[id]; }

private:
	const AigNtk*    ntk;
	Array<unsigned>  allInflu;
};

/**************************************/
/**************************************/
/**************************************/
/**************************************/
/**************************************/
/**************************************/

using ThreeValue = unsigned;
constexpr ThreeValue ThreeValue_False = 0;
constexpr ThreeValue ThreeValue_True  = 1;
constexpr ThreeValue ThreeValue_DC    = 2;
constexpr ThreeValue ThreeValue_None  = 3;

class AigSimulator
{
public:
	AigSimulator(const AigNtk* n): ntk(n) {}

	/*====================================*/

	void initValue() { assert(simValue.empty()); simValue.init(ntk->getMaxGateNum()); }
	void initNextState() { assert(nextState.empty()); nextState.init(ntk->getLatchNum()); }
	void setValue(AigGateID id, ThreeValue v) { simValue[id] = v; }
	void setInputValue(size_t i, ThreeValue v) { setValue(ntk->getInputID(i), v); }
	void setLatchValue(size_t i, ThreeValue v) { setValue(ntk->getLatchID(i), v); }
	void setAllToDCorNone();
	void setConst0() { setValue(0, ThreeValue_False); }
	void setInitState();

	void newEvent(AigGateID);
	void newEventCheck(AigGateID id) { if(!hasEvent[id]) newEvent(id); }
	void setValueEvent(AigGateID, ThreeValue);
	void setValueEventRef(AigGateID, ThreeValue);
	void setInputValueEvent(size_t i, ThreeValue v) { setValueEvent(ntk->getInputID(i), v); }
	void setLatchValueEvent(size_t i, ThreeValue v) { setValueEvent(ntk->getLatchID(i), v); }
	void setConst0Event() { setValueEvent(0, ThreeValue_False); }

	void simOneAnd(AigAnd* a) { simValue[a->getGateID()] = simOneAndValue(a); }
	ThreeValue simOneAndValue(AigAnd*)const;
	void simDfsList() { for(AigAnd* a: dfsList) simOneAnd(a); }
	void simOneCO(AigGate* g) { simValue[g->getGateID()] = simOneCOValue(g); }
	ThreeValue simOneCOValue(AigGate*)const;
	void simOutput(size_t i) { simOneCO(ntk->getOutputNorm(i)); }
	void simAllOutput();
	void simByEvent();
	void simAllLatchSynch();

	// Still preserve these, but these will lead to error if any latch depends directly on one latch
	void simLatch (size_t i) { simOneCO(ntk->getLatchNorm (i)); }
	void simAllLatch ();

	ThreeValue getValue(AigGateID id)const { return simValue[id]; }
	ThreeValue getInputValue (size_t i)const { return getValue(ntk->getInputID(i)); }
	ThreeValue getLatchValue (size_t i)const { return getValue(ntk->getLatchID(i)); }
	ThreeValue getOutputValue(size_t i)const { return getValue(ntk->getOutputID(i)); }

	void printAllGate  (ostream&)const;
	void printAllInput (ostream&)const;
	void printAllLatch (ostream&)const;
	void printAllOutput(ostream&)const;

	/*====================================*/

	/* For ternary simulation */
	bool checkCombLoop() { return ntk->checkCombLoop(true, dfsList); }
	void reserveDfsList(size_t c) { dfsList.reserve(c); }
	void reserveAndNum() { reserveDfsList(ntk->getAndNum()); }
	void clearDfsList() { dfsList.clear(); }
	void genDfsList(const vector<AigGateID>&);
	void doConstProp(const vector<AigGateID>&);

	bool readyForEvent()const { return fanOut.isON() && level.isON(); }
	unsigned getMaxLevel(const vector<AigGateID>&)const;
	void initLevel(bool isLatchCO) { assert(!level.isON()); level.init(ntk, isLatchCO); }
	void initFanOut() { assert(!fanOut.isON()); fanOut.init(ntk); }
	void initEventList();
	void markDfsCone(const vector<AigGateID>&);
	void markDfsList()const;
	void simByEventRef(unsigned);
	void cleanEvent(unsigned);

	void initInflu() { assert(!influ.isON()); influ.init(ntk); }
	void influOrder() { assert(influ.isON()); influ.convertToOrder(); }
	void backwardTerSim(CirSolver*, const vector<AigGateID>&, vector<AigGateID>&, const Array<double>&, bool);

	/*====================================*/

	static ThreeValue simNot(ThreeValue);
	static ThreeValue simAnd(ThreeValue, ThreeValue);
	static char getSymbol(ThreeValue);

private:
	const AigNtk*             ntk;
	Array<ThreeValue>         simValue;
	Array<ThreeValue>         nextState;
	vector<AigAnd*>           dfsList;
	AigFanouter               fanOut;
	AigLeveler                level;
	Array<vector<AigGateID>>  eventList;
	Array<bool>               hasEvent;
	AigInfluencer             influ;
};

}

#endif