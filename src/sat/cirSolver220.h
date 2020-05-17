/*========================================================================\
|: [Filename] cirSolver220.h                                             :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Define the wrapper class to adopt MiniSAT 2.2.0            :|
<------------------------------------------------------------------------*/

#ifndef HEHE_CIRSOLVER220_H
#define HEHE_CIRSOLVER220_H

#include "cirSolver.h"
#include "Solver220.h"

namespace M2 = Minisat220;

namespace _54ff
{

static inline M2::Lit toLit(Lit L) { return M2::toLit(L.value()); }
static inline lbool toLBool(M2::lbool lb) { return lbool(M2::toInt(lb)); }

class CirSolver220 : public CirSolver
{
public:
	CirSolver220(AigNtk* n)
	: CirSolver  (n)
	, solver     (new M2::Solver) {}
	~CirSolver220() { delete solver; }

	/*====================================*/

	void resetSolverInt() { delete solver; solver = new M2::Solver; }

	/*====================================*/

	void addClause(Lit p)               { solver->addClause(toLit(p)); }
	void addClause(Lit p, Lit q)        { solver->addClause(toLit(p), toLit(q)); }
	void addClause(Lit p, Lit q, Lit r) { solver->addClause(toLit(p), toLit(q), toLit(r)); }
	void addClause(const vector<Lit>&);
	void addConflict()                  { solver->addClause(solver->conflict); }

	/*====================================*/

	Var newVar() { return solver->newVar(); }

	/*====================================*/

	lbool getValue(Var v)const { return toLBool(solver->model[v]); }
	bool inConflict(Var v)const;

	unsigned getVarNum()const { return solver->nVars(); }
	unsigned getClsNum()const { return solver->nClauses(); }
	size_t getConflictNum()const { return solver->conflicts; }
	size_t getDecisionNum()const { return solver->decisions; }

	/*====================================*/

	void addAssump(Lit L) { assump.push(toLit(L)); }
	void clearAssump() { assump.clear(); }
	bool solve() { return solver->solve(assump); }

	void setConfLimit(size_t n) { solver->setConfBudget(n); }
	void setDeciLimit(size_t n) { /*TODO*/ }
	void resetLimit()           { solver->budgetOff(); }
	lbool solveLimited() { return toLBool(solver->solveLimited(assump)); }

protected:
	M2::Solver*       solver;
	M2::vec<M2::Lit>  assump;
};

}

#endif