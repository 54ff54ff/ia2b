/*========================================================================\
|: [Filename] bmcChecker.cpp                                             :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Implement the BMC, IND, ITP checkers                       :|
<------------------------------------------------------------------------*/

#include "bmcChecker.h"
#include "cirSolver.h"
#include "cirSolver114.h"
#include "condStream.h"

namespace _54ff
{

BmcChecker::BmcChecker(AigNtk* ntkToCheck, size_t outputIdx, bool _trace, size_t timeout,
	                   size_t maxD, BmcCheckType t)
: SafetyNCChecker (ntkToCheck, outputIdx, _trace, timeout)
, maxDepth        (maxD)
, type            (t)
{
	sfcMsg << "Max depth  : " << maxDepth << endl
	       << "Method     : Bounded model checking" << endl
	       << "Detail     : ";
	switch(type)
	{
		case BMC_ASSERT    : sfcMsg << "Assert already proved property";     break;
		case BMC_ONLY_LAST : sfcMsg << "Only the last property is involved"; break;
	}
	sfcMsg << endl;
}

void
BmcChecker::check()
{
	AigGateV init = buildInit();
	SolverPtr<CirSolver> solver(ntk);
	solver->convertToCNF(init.getGateID(), 0);
	solver->addClause(Lit(solver->getVarInt(init.getGateID(), 0), init.isInv()));
	for(size_t i = 0; i <= maxDepth; ++i)
	{
		cout << "\rTimeFrame = " << i << flush;
		solver->convertToCNF(property, i);
		solver->clearAssump();
		solver->addAssump(property, i, false);
		if(solver->solve())
		{
			cout << "\rObserve a counter example at depth " << i << endl;
			if(trace) solver->reportTrace(i);
			return;
		}
		if(type == BMC_ASSERT)
		//The property asserted here is not necessary "p"
		//If there is any PI in the cone of "p"
		//The asserted is actually between "p" and tautology
			solver->addClause(Lit(solver->getVarInt(property, i), true));
	}
	cout << "\rNo counter example up to depth " << maxDepth << endl;
}

IndChecker::IndChecker(AigNtk* ntkToCheck, size_t outputIdx, bool _trace, size_t timeout,
	                   size_t maxD, IndCheckType t)
: SafetyNCChecker (ntkToCheck, outputIdx, _trace, timeout)
, maxDepth        (maxD)
, type            (t)
{
	sfcMsg << "Max depth  : " << maxDepth << endl
	       << "Method     : K-induction" << endl
	       << "Detail     : ";
	switch(type)
	{
		case IND_SIMPLE_NO   : sfcMsg << "No simple constraint";            break;
		case IND_SIMPLE_NEED : sfcMsg << "Add simple constraint if needed"; break;
		case IND_SIMPLE_ALL  : sfcMsg << "Add all simple constraints";      break;
	}
	sfcMsg << endl;
}

void
IndChecker::check()
{
	AigGateV init = buildInit();
	SolverPtr<CirSolver> solver(ntk);
	solver->convertToCNF(init.getGateID(), 0);
	solver->convertToCNF(property, 0);
	const size_t L = ntk->getLatchNum();
	//for simple constraint, we convert every latch at every timeframe
	//if no this constraint, simply convert the cone of the property
	//this is more efficient, but converting it totally is more convenient
	//and maybe the inconvenience will introduce extra overhead
	const Var simpConstraint = solver->newVar(); //reserve for simple constraint
	cout << "Timeframe = 0," << flush;
	for(size_t i = 0; true;)
	{
		cout << " base" << flush;
		solver->clearAssump();
		solver->addAssump(init, 0);
		solver->addAssump(property, i, false);
		if(solver->solve())
		{
			cout << "\rObserve a counter example at depth " << i << endl;
			if(trace) solver->reportTrace(i);
			return;
		}
		if(++i > maxDepth)
			{ cout << "\rCannot determinie the property up to depth " << maxDepth << endl; return; }
		cout << CleanStrOnTerminal(" -> base");
		cout << "\rTimeframe = " << i << ", ind" << flush;
		solver->convertToCNF(property, i);
		for(size_t l = 0; l < L; ++l)
			solver->convertToCNF(ntk->getLatchID(l), i);
		solver->clearAssump();
		solver->addAssump(property, i, false);
		for(size_t j = 0; j < i; ++j)
			solver->addAssump(property, j, true);
		switch(type)
		{
			case IND_SIMPLE_NO:
				if(!solver->solve())
					{ cout << "\rProperty proved at depth " << i << endl; return; }
				break;

			case IND_SIMPLE_NEED:
				solver->addAssump(simpConstraint, false);
				while(true)
				{
					if(!solver->solve())
						{ cout << "\rProperty proved at depth " << i << endl; return; }
					for(size_t t1 = 0; t1 < i; ++t1)
						for(size_t t2 = t1 + 1; t2 <= i; ++t2)
						{
							bool same = true;
							for(size_t l = 0; l < L; ++l)
								if(AigGateID id = ntk->getLatchID(l);
								   solver->getValue(id, t1) != solver->getValue(id, t2))
									{ same = false; break; }
							if(same)
							{
								vector<Lit> miterList;
								miterList.reserve(L);
								for(size_t l = 0; l < L; ++l)
								{
									AigGateID id = ntk->getLatchID(l);
									Var miter = solver->newVar();
									miterList.emplace_back(miter, false);
									solver->convertXor(miter,                     false,
									                   solver->getVarInt(id, t1), false,
									                   solver->getVarInt(id, t2), false);
								}
								solver->convertOr(simpConstraint, false, miterList);
								goto NEXT;
							}
						}
					break;
					NEXT: {}
				} break;

			case IND_SIMPLE_ALL:
			{
				vector<Lit> miterList;
				miterList.reserve(L);
				for(size_t j = 0; j < i; ++j)
				{
					for(size_t l = 0; l < L; ++l)
					{
						AigGateID id = ntk->getLatchID(l);
						Var miter = solver->newVar();
						miterList.emplace_back(miter, false);
						solver->convertXor(miter,                    false,
						                   solver->getVarInt(id, j), false,
						                   solver->getVarInt(id, i), false);
					}
					solver->convertOr(simpConstraint, false, miterList);
					miterList.clear();
				}
				solver->addAssump(simpConstraint, false);
				if(!solver->solve())
					{ cout << "\rProperty proved at depth " << i << endl; return; }
			} break;
		}
		cout << " ->";
	}
}

ItpChecker::ItpChecker(AigNtk* ntkToCheck, size_t outputIdx, bool _trace, size_t timeout,
	                   size_t maxD, ItpCheckType t)
: SafetyNCChecker (ntkToCheck, outputIdx, _trace, timeout)
, maxDepth        (maxD)
, type            (t)
{
	sfcMsg << "Max depth  : " << maxDepth << endl
	       << "Method     : Interpolation" << endl
	       << "Detail     : ";
	switch(type)
	{
		case ITP_ALL_NOT_P : sfcMsg << "Disjunct all the property in the offset"; break;
		case ITP_ASSERT    : sfcMsg << "Assert already proved property";          break;
		case ITP_ONLY_LAST : sfcMsg << "Only the last property is involved";      break;
	}
	sfcMsg << endl;
}

void
ItpChecker::check()
{
	SolverPtr<CirSolver114Proof> solver(ntk);
	M1::Proof& p = solver->getProof();

	/*
	1. Check counter example at timeframe 0
	*/
	AigGateV init = buildInit();
	solver->convertToCNF(init.getGateID(), 0);
	solver->convertToCNF(property, 0);
	solver->addAssump(init, 0);
	solver->addAssump(property, 0, false);
	cout << "Timeframe = 0" << flush;
	if(solver->solve())
	{
		cout << "\rObserve a counter example at depth 0" << endl;
		if(trace) solver->reportTrace(0);
		return;
	}

	/*
	2.(1) Convert all the latch at timeframe 1
	  (2) Mark all of them as common variables,
	  (3) and mark the rest variables as local to A
	  (4) Mark all of the clauses up to now to onset
	*/
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
	{
		solver->convertToCNF(ntk->getLatchID(i), 1);                    //2-1
		solver->addCommon(ntk->getLatchID(i), 1, ntk->getLatchNorm(i)); //2-2
	}
	//2-3
	for(int i = 0, V = solver->getVarNum(); i < V; ++i)
		p.setToLocal(i);
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
		p.unsetLocal(solver->getVarInt(ntk->getLatchID(i), 1));
	//2-4
	for(int i = 0, C = p.now(); i < C; ++i)
		p.setToOnSet(i);

	/*
	3. Solve the bmc
	   (1) If SAT
	       -> j == 0: Counter example
	       -> Otherwise: Spurious CEX
	   (2) If UNSAT
	       Calculate the interpolant and use it to solve the next round
	*/
	Var target = var_Undef;
	if(type == ITP_ALL_NOT_P)
		solver->addClause(Lit(target = solver->newVar(), true));
	for(size_t i = 1; i <= maxDepth; ++i)
	{
		solver->convertToCNF(property, i);
		if(type == ITP_ALL_NOT_P)
		{
			Var tmp = solver->newVar();
			solver->convertOr(tmp, false, target, false, solver->getVarInt(property, i), false);
			target = tmp;
		}
		else target = solver->getVarInt(property, i);
		AigGateV curReach = init;
		AigGateV curReachAll = curReach;
		SolverPtr<CirSolver> eqChecker(ntk);
		for(size_t j = 1; true; ++j)
		{
			cout << "\rTimeframe = " << i << ", Iteration = " << j << flush;
			solver->clearAssump();
			solver->addAssump(curReach, 0);
			solver->addAssump(target, false);
			if(solver->solve())
			{
				if(j == 1)
				{
					cout << "\rObserve a counter example at depth " << i << endl;
					if(trace) solver->reportTrace(i);
					return;
				}
				else
					{ cout << CleanIntOnTerminal(j); break; }
			}
			AigGateV overApprox = solver->buildItp();
			eqChecker->convertToCNF(curReachAll.getGateID(), 0);
			eqChecker->convertToCNF(overApprox.getGateID(), 0);
			eqChecker->addAssump(~curReachAll, 0);
			eqChecker->addAssump(overApprox, 0);
			bool diff = eqChecker->solve();
			eqChecker->clearAssump();
			cout << CleanIntOnTerminal(j);
			if(!diff)
				{ cout << "\rProperty proved at depth " << i << endl; return; }
			curReachAll = ntk->createOrConstProp(curReachAll, curReach = overApprox);
			int c = p.getClsNum(), v = solver->getVarNum();
			solver->convertToCNF(curReach.getGateID(), 0);
			for(int V = solver->getVarNum(); v < V; ++v)
				p.setToLocal(v);
			for(int C = p.getClsNum(); c < C; ++c)
				p.setToOnSet(c);
		}
		if(type == ITP_ASSERT)
			solver->addClause(Lit(solver->getVarInt(property, i), true));
	}
	cout << "\rCannot determinie the property up to depth " << maxDepth << endl;
}

}
