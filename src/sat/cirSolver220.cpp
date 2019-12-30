/*========================================================================\
|: [Filename] cirSolver220.cpp                                           :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Implement the CirSolver220 wrapper class                   :|
<------------------------------------------------------------------------*/

#include "cirSolver220.h"

namespace _54ff
{

CirSolver* getSolver220(AigNtk* ntk) { return (new CirSolver220(ntk)); }

void
CirSolver220::addClause(const vector<Lit>& litList)
{
	M2::vec<M2::Lit> tmp;
	tmp.capacity(litList.size());
	for(const Lit& lit: litList)
		tmp.push_(toLit(lit));
	solver->addClause_(tmp);
}

bool
CirSolver220::inConflict(Var v)const
{
	for(int i = 0; i < solver->conflict.size(); ++i)
		if(M2::var(solver->conflict[i]) == v)
			return true;
	return false;
}

}