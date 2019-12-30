/*========================================================================\
|: [Filename] cirSolverGlu.cpp                                           :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Implement the CirSolverGlu wrapper class                   :|
<------------------------------------------------------------------------*/

#include "cirSolverGlu.h"

namespace _54ff
{

CirSolver* getSolverGlu(AigNtk* ntk) { return (new CirSolverGlu(ntk)); }

void
CirSolverGlu::addClause(const vector<Lit>& litList)
{
	G::vec<G::Lit> tmp;
	tmp.capacity(litList.size());
	for(const Lit& lit: litList)
		tmp.push_(toLit(lit));
	solver->addClause_(tmp);
}

bool
CirSolverGlu::inConflict(Var v)const
{
	for(int i = 0; i < solver->conflict.size(); ++i)
		if(G::var(solver->conflict[i]) == v)
			return true;
	return false;
}

}