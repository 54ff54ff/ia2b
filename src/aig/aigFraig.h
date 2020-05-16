/*========================================================================\
|: [Filename] aigFraig.h                                                 :|
:| [Author]   Chung-Yang (Ric) Huang, Chiang Chun-Yi                     |:
|: [Synopsis] Define a wrapper class to reduce the AIG network to become :|
:|            FRAIG (Functionally Reduced And-Inverter Graph)            |:	
<------------------------------------------------------------------------*/

#ifndef HEHE_AIGFRAIG_H
#define HEHE_AIGFRAIG_H

#include "aigNtk.h"
#include "cirSolver.h"

namespace _54ff
{

class AigFraiger
{
using LitVec  = vector<unsigned>;
using FecHash = unordered_map<size_t, LitVec*>;
enum FraigType { FRAIG_OPTIMIZE = 0, FRAIG_REWRITE, FRAIG_STRASH, FRAIG_FRAIG, FRAIG_TOTAL };

public:
	bool funcSimp (AigNtk* ntkToSimp, size_t confL) { confLimit = confL; return simpNtk(ntkToSimp, false, false); }
	bool oneLvlStrucSimp(AigNtk* ntkToSimp) { return simpNtk(ntkToSimp, true, false); }
	bool twoLvlStrucSimp(AigNtk* ntkToSimp) { return simpNtk(ntkToSimp, true, true); }

private:
	class StrashValue
	{
	public:
		StrashValue(AigAnd* _a): a(_a) {}

		operator size_t()const { return a->getFanIn0().getValue() +
										a->getFanIn1().getValue(); }
		bool operator==(const StrashValue& sv)const
			{ return a->getFanIn0() == sv.a->getFanIn0() &&
			         a->getFanIn1() == sv.a->getFanIn1(); }

		void helpSetEq(AigAnd* same)const
			{ a->noEqGate() ? same->setEqGate(a, false,       getHeader(FRAIG_STRASH)):
			                  same->setEqGate(a->getEqGate(), getHeader(FRAIG_STRASH)); }

	private:
		AigAnd*  a;
	};

	struct StrashHash
		{ size_t operator()(const StrashValue& s)const { return size_t(s); } };

	using StrashSet = unordered_set<StrashValue, StrashHash>;

private:
	void randomSim();
	bool simpNtk(AigNtk*, bool, bool);

	size_t calMaxFail()const;
	void simAllAnd();
	void initFecGrp();
	bool updateFecGrpRand();
	void collectPattern();
	void updateFecGrpSat(size_t);
	void setRandPat();
	void setFeqTarget();
	bool doTwoLevelSimp(AigAnd*);

	static void resetCount() { for(size_t i = 0; i < FRAIG_TOTAL; ++i) fraigCount[i] = 0; }
	static const char* getHeader(FraigType t) { fraigCount[t] += 1; return fraigHeader[t]; }

private:
	AigNtk*           ntk;
	vector<AigAnd*>   dfsList;
	CirSolver*        solver;
	vector<LitVec*>   fecGroups;
	Array<AigGateID>  feqTarget;
	Array<size_t>     simValue;
	size_t            confLimit;

	static const char* fraigHeader[FRAIG_TOTAL];
	static unsigned fraigCount[FRAIG_TOTAL];
};

extern AigFraiger* aigFraiger;

}

#endif