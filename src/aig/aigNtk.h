/*========================================================================\
|: [Filename] aigNtk.h                                                   :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Define the aig network                                     :|
<------------------------------------------------------------------------*/

#ifndef HEHE_AIGNTK_H
#define HEHE_AIGNTK_H

#include <signal.h>
#include <functional>
#include <unordered_set>
#include "aigGate.h"
using namespace std;

namespace _54ff
{

enum encodeType { ASCII = 0, BINARY, TOTAL_PARSE_TYPE };
extern AigNtk* aigNtk;
inline void setNtk(AigNtk* ntk) { aigNtk = ntk; }
inline bool checkNtk(bool report = true)
{
	if(aigNtk == 0)
		{ if(report) cerr << "[Error] Empty network!" << endl; return false; }
	else return true;
}
AigGate* checkGate(AigGateID, bool = true);

class AigNtk
{
friend class AigParser;
friend class AigAsciiParser;
friend class AigBinaryParser;

public:
	AigNtk(const char* n = "hehe")
	: ntkName(n), gateList(1, new AigConst0()) {}
	~AigNtk();

	AigNtk(const AigNtk&) = delete;
	AigNtk(AigNtk&&) = delete;
	AigNtk& operator=(const AigNtk&) = delete;
	AigNtk& operator=(AigNtk&&) = delete;

	AigNtk* copyNtk()const;
	AigNtk* moveNtk();
	void swap(AigNtk*);

	/*====================================*/

	size_t getInputNum  ()const { return PIList.size(); }
	size_t getLatchNum  ()const { return latchList.size(); }
	size_t getOutputNum ()const { return POList.size(); }
	size_t getGateNum   ()const { return gateList.size() - recycleList.size(); }
	size_t getMaxGateNum()const { return gateList.size(); }
	AigGateID getMaxGateID()const { return getMaxGateNum() - 1; }
	size_t getAndNum    ()const { return getGateNum() - getInputNum() - getOutputNum() - getLatchNum() - getConst0Num(); }
	size_t getConst0Num ()const { return 1; }

	/*====================================*/

	AigGateID getInputID (size_t i)const { return PIList   [i]; }
	AigGateID getLatchID (size_t i)const { return latchList[i]; }
	AigGateID getOutputID(size_t i)const { return POList   [i]; }

	/*====================================*/

	AigGate* getGate(AigGateID id)const { return gateList[id]; }
	AigGate* getGateCheck(AigGateID id)const { return id > getMaxGateNum() ? 0 : getGate(id); }
	AigGate* getInputNorm (size_t i)const { return getGate(getInputID (i)); }
	AigGate* getLatchNorm (size_t i)const { return getGate(getLatchID (i)); }
	AigGate* getOutputNorm(size_t i)const { return getGate(getOutputID(i)); }
	AigPi*    getInput (size_t i)const { return (AigPi*)   getInputNorm (i); }
	AigLatch* getLatch (size_t i)const { return (AigLatch*)getLatchNorm (i); }
	AigPo*    getOutput(size_t i)const { return (AigPo*)   getOutputNorm(i); }

	AigGateV getConst0V()const { return AigGateV(getGate(0), false); }
	AigGateV getConst1V()const { return AigGateV(getGate(0), true); }

	/*====================================*/

	void printSummary()const;
	void printAll()const;
	void printNetlist()const;
	void printPIs()const;
	void printLatches()const;
	void printPOs()const;
	void printRecycled()const;
	bool printLevel()const;
	bool printInfluence()const;

	void printCone(AigGateID, size_t)const;
	void printCone(Array<bool>&, size_t, size_t = 0)const;

	/*====================================*/

	bool compress();
	bool oneLvlStrucSimp();
	bool twoLvlStrucSimp();
	bool collectCOI();
	bool calReachable();
	bool fraig();
	bool balance();
	bool rmConstLatch();

	bool simulate(const char*, bool, const char*)const;

	/*====================================*/

	AigGate* createInput();

	AigGate* createLatch (AigGateV);
	AigGate* createOutput(AigGateV);

	AigGateV createAnd  (AigGateV, AigGateV);
	AigGateV createAnd  (const vector<AigGateV>& gateList) { vector<AigGateV> tmp(gateList); return createAnd_(tmp); }
	AigGateV createAnd_ (vector<AigGateV>&);
	AigGateV createOr   (AigGateV in0, AigGateV in1) { return ~createAnd(~in0, ~in1); }
	AigGateV createOr   (const vector<AigGateV>& gateList) { vector<AigGateV> tmp(gateList); return createOr_(tmp); }
	AigGateV createOr_  (vector<AigGateV>& gateList) { for(AigGateV& gv: gateList) gv = ~gv; return ~createAnd(gateList); }
	AigGateV createXor  (AigGateV in0, AigGateV in1) { return createOr(createAnd(in0, ~in1), createAnd(~in0, in1)); }
	AigGateV createEqual(AigGateV in0, AigGateV in1) { return ~createXor(~in0, ~in1); }

	AigGateV createMux(AigGateV sel, AigGateV inT, AigGateV inF)
		{ return createOr(createAnd(sel, inT), createAnd(~sel, inF)); }

	AigGateV createAndConstProp(AigGateV, AigGateV);
	AigGateV createOrConstProp(AigGateV in0, AigGateV in1) { return ~createAndConstProp(~in0, ~in1); }

	//be careful to update PIList, latchList and POList
	void removeGate(AigGateID id) { delete getGate(id); setGate(id, 0); recycleList.push_back(id); }
	void removeGate(AigGate* gate) { removeGate(gate->getGateID()); }

	/*====================================*/

	bool checkCombLoop(bool repErr)const                           { return checkCombLoop(repErr, 0); }
	bool checkCombLoop(bool repErr, vector<AigAnd*>& dfsList)const { return checkCombLoop(repErr, &dfsList); }
	bool checkValidity(bool)const;
	bool noLatchInCone(size_t)const;

public:
	string  ntkName;

private:
	vector<AigGateID>  PIList;
	vector<AigGateID>  latchList;
	vector<AigGateID>  POList;
	vector<AigGate*>   gateList;
	vector<AigGateID>  recycleList;

	/*====================================*/

	void setGate(AigGateID id, AigGate* gate) { gateList[id] = gate; }
	AigGateID getValidID();
	void createInputInt (AigGateID id) { setGate(id, new AigPi   (id)); PIList   .push_back(id); }
	void createLatchInt (AigGateID id) { setGate(id, new AigLatch(id)); latchList.push_back(id); }
	void createOutputInt(AigGateID id) { setGate(id, new AigPo   (id)); POList   .push_back(id); }
	void createAndInt   (AigGateID id) { setGate(id, new AigAnd  (id)); }

	/*====================================*/

	bool checkCombLoop(bool repErr, vector<AigAnd*>*)const;
	bool checkValidID(bool)const;
	bool checkInputID (bool repErr)const { return checkValidIDSub(repErr, PIList    , AIG_PI   ); }
	bool checkLatchID (bool repErr)const { return checkValidIDSub(repErr, latchList , AIG_LATCH); }
	bool checkOutputID(bool repErr)const { return checkValidIDSub(repErr, POList    , AIG_PO   ); }
	bool checkValidIDSub(bool, const vector<AigGateID>&, AigGateType)const;
	bool checkRecycleID(bool)const;
	bool checkAllGateID(bool)const;
	bool checkValidFanIns(bool)const;
};

}

#endif
