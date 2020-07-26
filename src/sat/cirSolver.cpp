/*========================================================================\
|: [Filename] cirSolver.cpp                                              :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Implement the CirSolver wrapper class                      :|
<------------------------------------------------------------------------*/

#include "cirSolver.h"
#include "aigMisc2.h"
#include "satData.h"

namespace _54ff
{

const Lit lit_Undef = -2;
const Lit lit_Error = -1;

const lbool l_True  = { 0 };
const lbool l_False = { 1 };
const lbool l_Undef = { 2 };

void
CirSolver::convertToCNFInt(AigGateID id, size_t level)
{
	//trivial Tseitin transformation
	if(isConverted(id, level))
		return;
	switch(AigGate* g = ntk->getGate(id); g->getGateType())
	{
		case AIG_PI:
			setVar(id, level);
			break;

		case AIG_LATCH:
			if(level > 0)
			{
				AigGateID inId = g->getFanIn0ID();
				convertToCNFInt(inId, level-1);
				setVar(id, level);
				convertBuf(getVarInt(id,   level),   false,
				           getVarInt(inId, level-1), g->isFanIn0Inv());
			}
			else setVar(id, level);
			break;

		case AIG_PO:
			{
				AigGateID inId = g->getFanIn0ID();
				convertToCNFInt(inId, level);
				setVar(id, level);
				convertBuf(getVarInt(id,   level), false,
				           getVarInt(inId, level), g->isFanIn0Inv());
			} break;

		case AIG_AND:
			{
				AigGateID inId0 = g->getFanIn0ID();
				AigGateID inId1 = g->getFanIn1ID();
				convertToCNFInt(inId0, level);
				convertToCNFInt(inId1, level);
				setVar(id, level);
				convertAnd(getVarInt(id,    level), false,
				           getVarInt(inId0, level), g->isFanIn0Inv(),
				           getVarInt(inId1, level), g->isFanIn1Inv());
			} break;

		case AIG_CONST0:
			setVar(id, level);
			addClause(Lit(getVarInt(id, level), true));
			break;

		default: assert(false);
	}
}

void
CirSolver::convertAnd(Var f, bool invF, Var a, bool invA, Var b, bool invB)
{
	Lit F(f, invF);
	Lit A(a, invA);
	Lit B(b, invB);

	/* F = A & B
	1. F' + A
	2. F' + B
	3. F + A' + B'
	*/
	addClause(~F, A);
	addClause(~F, B);
	addClause(F, ~A, ~B);
}

void
CirSolver::convertXor(Var f, bool invF, Var a, bool invA, Var b, bool invB)
{
	Lit F(f, invF);
	Lit A(a, invA);
	Lit B(b, invB);

	/* F = A ^ B
	-> F = (A + B) & (A' + B')
	1. F' + A  + B
	2. F' + A' + B'
	-> F = (A & B') + (A' & B)
	3. F + A  + B'
	4. F + A' + B
	*/
	addClause(~F,  A,  B);
	addClause(~F, ~A, ~B);
	addClause( F,  A, ~B);
	addClause( F, ~A,  B);
}

void
CirSolver::convertBuf(Var f, bool invF, Var g, bool invG)
{
	Lit F(f, invF);
	Lit G(g, invG);

	/* F = G
	1. F  + G'
	2. F' + G
	*/
	addClause( F, ~G);
	addClause(~F,  G);
}

void
CirSolver::convertAnd(Var f, bool invF, const vector<Lit>& litList)
{
	vector<Lit>& tmp = const_cast<vector<Lit>&>(litList);

	/* F = L1 & L2 & ... & Ln
	1. (F' + L1)(F' + L2)...(F' + Ln)
	2. F + L1' + L2' + ... + Ln'
	*/
	Lit notF(f, true ^ invF);
	for(const Lit& lit: litList)
		addClause(notF, lit);
	for(Lit& lit: tmp) lit.flip();
	tmp.push_back(Lit(f, false ^ invF));
	addClause(tmp);
	tmp.pop_back();
	for(Lit& lit: tmp) lit.flip();
}

void
CirSolver::convertOr(Var f, bool invF, const vector<Lit>& litList)
{
	vector<Lit>& tmp = const_cast<vector<Lit>&>(litList);

	/* F = L1 | L2 | ... | Ln
	1. (F + L1')(F + L2')...(F + Ln')
	2. F' + L1 + L2 + ... + Ln
	*/
	Lit F(f, false ^ invF);
	for(const Lit& lit: litList)
		addClause(F, ~lit);
	tmp.push_back(Lit(f, true ^ invF));
	addClause(tmp);
	tmp.pop_back();
}

void
CirSolver::techMapToCNF()
{
	AigCutter fourCut(ntk, 4, 10);
}

void
CirSolver::reportLatch(size_t idx, size_t level)const
{
	cout << idx << ": ";
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
		cout << getValueChar(ntk->getLatchID(i), level);
	cout << endl;
}

void
CirSolver::reportPI(size_t idx, size_t level)const
{
	cout << idx << ": ";
	for(size_t i = 0, I = ntk->getInputNum(); i < I; ++i)
		cout << getValueChar(ntk->getInputID(i), level);
	cout << endl;
}

}