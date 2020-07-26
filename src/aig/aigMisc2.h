/*========================================================================\
|: [Filename] aigMisc2.h                                                 :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Define various wrapper classes to perform operations on    :|
:|            AIG network                                                |:
|:            5. AigConster to calculate the set of state variables      :|
:|               reachable at only negative polarity                     |:
|:            6. AigCutter to calculate K-feasible cut                   :|
<------------------------------------------------------------------------*/

#ifndef HEHE_AIGMISC2_H
#define HEHE_AIGMISC2_H

#include "aigNtk.h"

namespace _54ff
{

class AigConster
{
public:
	AigConster(AigNtk* n): ntk(n) {}

	bool doSimpMono();
	bool doSimpPdr(size_t);

private:
	void replaceWithZero(const vector<AigGateID>&);

private:
	AigNtk*  ntk;
};

/**************************************/
/**************************************/
/**************************************/
/**************************************/
/**************************************/
/**************************************/

constexpr unsigned varPosMask[4] = { 0xAAAA, 0xCCCC, 0xF0F0, 0xFF00 };

struct AigCut
{
	static size_t calSize(unsigned K) { return sizeof(AigCut) + sizeof(AigGateID) * K; }

	bool operator<(const AigCut&)const;
	AigCut& operator=(const AigCut&);

	bool isConst0()const { return truthTable == 0; }
	bool isConst1()const { return truthTable == 0xFFFF; }

	bool dominate(const AigCut&)const;
	unsigned numOnes()const;
	bool isRedundant(unsigned)const;

	void calAbsValue();
	void calFanOutNum(const Array<unsigned>&);

	// Modify from Dar_Cut_t of ABC
	unsigned   absValue;
	unsigned   truthTable : 16; // Only active when K == 4
	unsigned   fanOutSum  : 11;
	unsigned   numLeaves  :  5;
	AigGateID  leaves[0];
};

class AigCutter
{
public:
	AigCutter(AigNtk*, unsigned, unsigned);
	~AigCutter() { operator delete(allCuts); }

private:
	void prepare();
	void buildCut();
	void addConst0Cut();
	void addUnitCut(AigGateID, bool = false);
	void genCutForAndGate(AigAnd*);
	bool checkNumOnes(const AigCut&, const AigCut&);
	bool mergeTwoCuts(const AigCut&, bool, const AigCut&, bool);
	bool checkDominance(unsigned*, unsigned&);
	void calTruthTable(const AigCut&, bool, const AigCut&, bool);
	void checkRedundant();
	void checkInsertion(unsigned*, unsigned&);

	static unsigned swapVarTruthTable(unsigned, unsigned, unsigned);

	unsigned  getIdxBaseNum(AigGateID id)const { return id * maxCutNum; }
	unsigned* getIdxBasePtr(AigGateID id)const { return cutIdx + getIdxBaseNum(id); }
	AigCut& getCut(unsigned idx) { return *(AigCut*)((char*)allCuts + idx * AigCut::calSize(maxLeaves)); }

private:
	AigNtk*          ntk;
	unsigned         maxLeaves; // K
	unsigned         maxCutNum; // L
	AigCut*          allCuts;
	AigCut*          tmpCut;
	unsigned*        cutIdx;
	Array<unsigned>  fanOutNum;
};

}

#endif