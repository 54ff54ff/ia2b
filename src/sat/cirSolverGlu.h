/*========================================================================\
|: [Filename] cirSolverGlu.h                                             :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Define the wrapper class to adopt Glucose                  :|
<------------------------------------------------------------------------*/

#ifndef HEHE_CIRSOLVERGLU_H
#define HEHE_CIRSOLVERGLU_H

#include "cirSolver.h"
#include "SolverGlu.h"

namespace G = Glucose;

namespace _54ff
{

static inline G::Lit toLit(Lit L) { return G::toLit(L.value()); }
static inline lbool toLBool(G::lbool lb) { return lbool(G::toInt(lb)); }

class CirSolverGlu : public CirSolver
{
public:
	CirSolverGlu(AigNtk* n)
	: CirSolver  (n)
	, solver     (new G::Solver) {}
	~CirSolverGlu() { delete solver; }

	/*====================================*/

	void resetSolverInt() { delete solver; solver = new G::Solver; }

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
	void setDeciLimit(size_t n) { solver->setDeciBudget(n); }
	lbool solveLimited() { return toLBool(solver->solveLimited(assump)); }

protected:
	G::Solver*      solver;
	G::vec<G::Lit>  assump;
};

}

#endif