/*========================================================================\
|: [Filename] aigBalance.h                                               :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Define a wrapper class to perform tree-height reduction on :|
:|            AIG network                                                |:
<------------------------------------------------------------------------*/

#ifndef HEHE_AIGBALANCE_H
#define HEHE_AIGBALANCE_H

#include <queue>
#include "aigNtk.h"
using namespace std;

namespace _54ff
{

class AigBalancer
{
public:
	bool balance(AigNtk*);

private:
	void checkFanOut();
	void createNewCIs();
	void doBalance();
	void createNewCOs();
	void setIdLitMap();
	void finalize();
	void doBalanceSub(AigGate*);
	vector<AigGateV> collectCluster(AigGate*);
	void collectClusterRec(AigGateV, vector<AigGateV>&);
	unsigned maxLevel(const AigGateV&, const AigGateV&);

private:
	class LvlCmp
	{
	public:
		LvlCmp(const Array<unsigned>& l): level(l) {}
		bool operator()(const AigGateV& gv1, const AigGateV& gv2)const
			{ return level[gv1.getGateID()] > level[gv2.getGateID()]; }

	private:
		const Array<unsigned>&  level;
	};

	using LevelHeap = priority_queue<AigGateV, vector<AigGateV>, LvlCmp>;

private:
	AigNtk*          ntk;
	AigNtk*          newNtk;
	Array<bool>      hasMultiFanOut;
	Array<unsigned>  oldIdToNewLit;
	Array<unsigned>  newIdToLevel;
};

extern AigBalancer* aigBalancer;

}

#endif