/*========================================================================\
|: [Filename] cirSolver114.cpp                                           :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Implement the CirSolver114 wrapper class                   :|
<------------------------------------------------------------------------*/

#include "cirSolver114.h"

namespace _54ff
{

CirSolver* getSolver114(AigNtk* ntk) { return (new CirSolver114(ntk)); }

void
CirSolver114::addClause(const vector<Lit>& litList)
{
	M1::vec<M1::Lit> tmp;
	tmp.capacity(litList.size());
	for(const Lit& lit: litList)
		tmp.push(toLit(lit));
	solver->addClause(tmp);
}

bool
CirSolver114::inConflict(Var v)const
{
	for(int i = 0; i < solver->conflict.size(); ++i)
		if(M1::var(solver->conflict[i]) == v)
			return true;
	return false;
}

AigGateV
CirSolver114Proof::buildItp()
{
	M1::Proof& p = getProof();
	/* Ensure the buffer of the temporary file is cleaned */
	p.finish();

	unsigned tmp, tmp2;

	/* Format
	case 1. begin & 1 == 0 -> root
	        lit 0 = begin >> 1
	        lit 1 = lit 0 + next
	          ...
	        ended by 0
	case 2. begin & 1 == 1 and then a zero
	        -> deleted, just ignore it
	case 3. begin & 1 == 1 and not followed by zero
	        -> derived, learnt, formed by resolution
	        clause 0 = thisClsId - begin >> 1
	        var    1 = next - 1
	        clause 1 = thisClsId - clause 0
	          ...
	        ended by 0
	*/

	/*
	1. Backward traverse from the conflict clause
	   Mark all the clauses in the UNSAT core
	*/
	p.setToCore(solver->conflict_id);
	for(int curClsId = solver->conflict_id; curClsId >= 0; --curClsId)
		if(p.inUnsatCore(curClsId))
			if(p.seekPos(curClsId); ((tmp = p.getUInt()) & 1) == 1)
			{
				tmp = (unsigned)curClsId - (tmp >> 1);
				do
				{
					assert(int(tmp) >= 0);
					p.setToCore(tmp);
					if(p.getUInt() == 0) break;
					tmp = (unsigned)curClsId - p.getUInt();
				} while(true);
			}

	/*
	2. Compute the interolant using McMillan's algorithm
	   *root
	   Test where the clause locates at
	   A(Onset) : disjunction of common variables
	   B(Offset): constant true
	   *derived C = C1 resolve C2
	   Test the locality of the pivot variable
	   Local to A: itp(C) = itp(C1) or  itp(C2)
	   Otherwise : itp(C) = itp(C1) and itp(C2)
	*/
	unordered_map<unsigned, AigGateV> clsToGateV;
	vector<AigGateV> commonVar;
	for(int curClsId = 0; curClsId <= solver->conflict_id; ++curClsId)
		if(p.inUnsatCore(curClsId))
		{
			p.seekPos(curClsId);
			if(tmp = p.getUInt(); (tmp & 1) == 0)
				if(p.inOnSet(curClsId))
				{
					tmp >>= 1;
					do
					{
						if(auto result = commonVarToGate.find(tmp >> 1); result != commonVarToGate.end())
							commonVar.push_back((tmp & 1) == 0 ? result->second : ~(result->second));
						if(tmp2 = p.getUInt(); tmp2 != 0)
							tmp += tmp2;
						else break;
					} while(true);
					switch(commonVar.size())
					{
						case 0 : clsToGateV[curClsId] = ntk->getConst0V();          break;
						case 1 : clsToGateV[curClsId] = commonVar[0];               break;
						default: clsToGateV[curClsId] = ntk->createAnd_(commonVar); break;
					}
					commonVar.clear();
				}
				else clsToGateV[curClsId] = ntk->getConst1V();
			else
			{
				tmp = (unsigned)curClsId - (tmp >> 1);
				assert(int(tmp) >= 0);
				assert(clsToGateV.find(tmp) != clsToGateV.end());
				AigGateV tmpV(clsToGateV[tmp]);
				while(true)
				{
					if(tmp2 = p.getUInt(); tmp2-- == 0)
						break;
					tmp = (unsigned)curClsId - p.getUInt();
					assert(clsToGateV.find(tmp) != clsToGateV.end());
					tmpV = p.localToA(tmp2) ? ntk->createOrConstProp (tmpV, clsToGateV[tmp]):
					                          ntk->createAndConstProp(tmpV, clsToGateV[tmp]);
				}
				clsToGateV[curClsId] = tmpV;
			}
			p.unsetCore(curClsId);
		}

	/* Reset the temporary file to write the resolution proof of the next round */
	p.goToEnd();

	/*
	3. The interpolant is the one of the conflict clause
	*/
	return clsToGateV[solver->conflict_id];
}

}