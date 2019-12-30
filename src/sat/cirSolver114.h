/*========================================================================\
|: [Filename] cirSolver114.h                                             :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Define the wrapper class to adopt MiniSAT 1.14             :|
<------------------------------------------------------------------------*/

#ifndef HEHE_CIRSOLVER114_H
#define HEHE_CIRSOLVER114_H

#include "cirSolver.h"
#include "Solver114.h"

namespace M1 = Minisat114;

namespace _54ff
{

static inline M1::Lit toLit(Lit L) { return M1::toLit(L.value()); }
static inline lbool toLBool(M1::lbool lb)
{
	constexpr int mapping[3] = { 1, 2, 0 };
	return lbool(mapping[1+M1::toInt(lb)]);
}

class CirSolver114 : public CirSolver
{
public:
	CirSolver114(AigNtk* n)
	: CirSolver  (n)
	, solver     (new M1::Solver) {}
	~CirSolver114() { delete solver; }

	/*====================================*/

	void resetSolverInt() { delete solver; solver = new M1::Solver; }

	/*====================================*/

	void addClause(Lit p)               { solver->addUnit   (toLit(p)); }
	void addClause(Lit p, Lit q)        { solver->addBinary (toLit(p), toLit(q)); }
	void addClause(Lit p, Lit q, Lit r) { solver->addTernary(toLit(p), toLit(q), toLit(r)); }
	void addClause(const vector<Lit>&);
	void addConflict()                  { solver->addClause(solver->conflict); }

	/*====================================*/

	Var newVar() { return solver->newVar(); }

	/*====================================*/

	lbool getValue(Var v)const { return toLBool(solver->model[v]); }
	bool inConflict(Var v)const;

	unsigned getVarNum()const { return solver->nVars(); }
	unsigned getClsNum()const { return solver->nClauses(); }
	size_t getConflictNum()const { return solver->stats.conflicts; }
	size_t getDecisionNum()const { return solver->stats.decisions; }

	/*====================================*/

	void addAssump(Lit L) { assump.push(toLit(L)); }
	void clearAssump() { assump.clear(); }
	bool solve() { return solver->solve(assump); }

	void setConfLimit(size_t) { /* Dummy */ }
	void setDeciLimit(size_t) { /* Dummy */ }
	lbool solveLimited() { return solve() ? l_True : l_False; }

protected:
	M1::Solver*       solver;
	M1::vec<M1::Lit>  assump;
};

class CirSolver114Proof : public CirSolver114
{
public:
	CirSolver114Proof(AigNtk* n)
	: CirSolver114(n) { solver->proof = new M1::Proof; }
	~CirSolver114Proof() { delete solver->proof; }

	M1::Proof& getProof() { return *(solver->proof); }
	void addCommon(AigGateID id, size_t level, AigGate* g)
		{ commonVarToGate[getVarInt(id, level)] = AigGateV(g, false); }
	void clearCommon() { commonVarToGate.clear(); }

	using CirSolver::addAssump;

	AigGateV buildItp();

protected:
	unordered_map<Var, AigGateV>  commonVarToGate;
};

}

#endif