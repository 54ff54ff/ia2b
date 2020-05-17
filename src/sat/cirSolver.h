/*========================================================================\
|: [Filename] cirSolver.h                                                :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Define the wrapper class of SAT solver to deal with the    :|
:|            problem about AIG network                                  |:
<------------------------------------------------------------------------*/

#ifndef HEHE_CIRSOLVER_H
#define HEHE_CIRSOLVER_H

#include "aigNtk.h"

namespace _54ff
{

using Var = int;
constexpr Var var_Undef = -1;

class Lit
{
public:
	Lit(Var _v, bool inv): v((_v << 1) | int(inv)) {}
	Lit(int _v): v(_v) {}

	int value()const { return v; }
	void flip() { v ^= 1; }
	Var var()const { return v >> 1; }
	bool inv()const { return v & 1; }

	friend Lit operator~(const Lit& lit) { return Lit(lit.v ^ 1); }

private:
	int  v;
};

extern const Lit lit_Undef, lit_Error;

class lbool
{
public:
	lbool(uint8_t _v): v(_v) {}

	bool operator==(const lbool& lb)const { return v == lb.v; }
	bool operator!=(const lbool& lb)const { return v != lb.v; }

private:
	uint8_t  v;
};

extern const lbool l_True, l_False, l_Undef;

class CirSolver
{
using VarLevelList = vector<vector<Var>>;

public:
	CirSolver(AigNtk* n)
	: ntk        (n)
	, idLvlToVar (ntk->getMaxGateNum()) {}
	virtual ~CirSolver() {}

	/*====================================*/

	virtual void resetSolverInt() = 0;
	void resetSolver() { idLvlToVar.clear(); clearAssump();
	                     idLvlToVar.resize(ntk->getMaxGateNum()); resetSolverInt(); }

	/*====================================*/

	void convertToCNF   (AigGateID id, size_t level) { checkVarList(); convertToCNFInt(id, level); }
	void convertToCNFInt(AigGateID, size_t);

	void convertAnd(Var, bool, Var, bool, Var, bool);
	void convertXor(Var, bool, Var, bool, Var, bool);
	void convertBuf(Var, bool, Var, bool);

	void convertOr   (Var f, bool invF, Var a, bool invA, Var b, bool invB) { convertAnd(f, !invF, a, !invA, b, !invB); }
	void convertEqual(Var f, bool invF, Var a, bool invA, Var b, bool invB) { convertXor(f, !invF, a, !invA, b, !invB); }
	void convertInv  (Var f, bool invF, Var g, bool invG)                   { convertBuf(f, !invF, g, !invG); }

	void convertAnd(Var, bool, const vector<Lit>&);
	void convertOr (Var, bool, const vector<Lit>&);

	virtual void addClause(Lit)                = 0;
	virtual void addClause(Lit, Lit)           = 0;
	virtual void addClause(Lit, Lit, Lit)      = 0;
	virtual void addClause(const vector<Lit>&) = 0;
	virtual void addConflict()                 = 0;

	/*====================================*/

	virtual Var newVar() = 0;

	Var  getVar     (AigGateID id, size_t level)const { return level >= idLvlToVar[id].size() ? var_Undef : getVarInt(id, level); }
	Var  getVarInt  (AigGateID id, size_t level)const { return idLvlToVar[id][level]; }
	bool isConverted(AigGateID id, size_t level)const { return getVar(id, level) != var_Undef; }
	void setVar     (AigGateID id, size_t level)
	{
		if(level >= idLvlToVar[id].size())
			idLvlToVar[id].resize(level+1, var_Undef);
		idLvlToVar[id][level] = newVar();
	}
	void checkVarList() { assert(ntk->getMaxGateNum() >= idLvlToVar.size());
	                      idLvlToVar.resize(ntk->getMaxGateNum()); }

	bool isVarValid(Var v)const { return v > var_Undef && v < int(getVarNum()); }

	/*====================================*/

	virtual lbool getValue(Var v)const  = 0;
	virtual bool inConflict(Var v)const = 0;

	virtual unsigned getVarNum()const    = 0;
	virtual unsigned getClsNum()const    = 0;
	virtual size_t getConflictNum()const = 0;
	virtual size_t getDecisionNum()const = 0;

	lbool getValue(AigGateID id, size_t level)const { assert(isConverted(id, level)); return getValue(getVarInt(id, level)); }
	bool getValueBool(AigGateID id, size_t level)const { return getValueBool(getVarInt(id, level)); }
	bool getValueBool(Var v)const { assert(getValue(v) != l_Undef); return getValue(v) == l_True; }
	char getValueChar(AigGateID id, size_t level)const { if(Var v = getVar(id, level); v != var_Undef) return getValueChar(v); else return 'X'; }
	char getValueChar(Var v)const { return getValue(v) == l_Undef ? '?' : (getValue(v) == l_True ? '1' : '0'); }
	bool inConflict(AigGateID id, size_t level)const { return inConflict(getVarInt(id, level)); }

	void reportLatch(size_t, size_t)const;
	void reportPI   (size_t, size_t)const;
	void reportTrace(size_t maxLevel) { for(size_t l = 0; l <= maxLevel; ++l) reportPI(l, l); }

	/*====================================*/

	virtual void addAssump(Lit L) = 0;
	virtual void clearAssump()    = 0;
	virtual bool solve()          = 0;

	virtual void setConfLimit(size_t) = 0;
	virtual void setDeciLimit(size_t) = 0;
	virtual void resetLimit()         = 0;
	virtual lbool solveLimited()      = 0;

	void addAssump(Var v, bool inv) { assert(isVarValid(v)); addAssump(Lit(v, inv)); }
	void addAssump(AigGateID id, size_t level, bool inv) { assert(isConverted(id, level)); addAssump(getVarInt(id, level), inv); }

protected:
	AigNtk*       ntk;
	VarLevelList  idLvlToVar;
};

enum SolverType
{
	SOLVER_TYPE_MINISAT114 = 0,
	SOLVER_TYPE_MINISAT220,
	SOLVER_TYPE_GLUCOSE,
	SOLVER_TYPE_TOTAL
};

extern const string solverName[SOLVER_TYPE_TOTAL];

extern SolverType curSolverType;
CirSolver* getSolver(AigNtk*);

template <class S>
class SolverPtr
{
public:
	template <class... Param>
	SolverPtr(Param&&... param): s(new S(forward<Param>(param)...)) {}
	~SolverPtr() { delete s; }

	S* operator->()const { return s; }
	operator S*()const { return s; }

private:
	S*  s;
};

template <>
class SolverPtr<CirSolver>
{
public:
	SolverPtr<CirSolver>(AigNtk* ntk): s(getSolver(ntk)) {}
	~SolverPtr<CirSolver>() { delete s; }

	CirSolver* operator->()const { return s; }
	operator CirSolver*()const { return s; }

private:
	CirSolver*  s;
};

}

#endif