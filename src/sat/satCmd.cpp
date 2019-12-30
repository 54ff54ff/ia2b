/*========================================================================\
|: [Filename] satCmd.cpp                                                 :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Define and implement the commands about SAT solver         :|
<------------------------------------------------------------------------*/

#include "cmdMgr.h"
#include "cirSolver.h"

namespace _54ff
{

const string solverName[SOLVER_TYPE_TOTAL] =
{
	"MiniSAT 1.14",
	"MiniSAT 2.2.0",
	"Glucose 3.0"
};

SolverType curSolverType = SOLVER_TYPE_GLUCOSE;

extern CirSolver* getSolver114(AigNtk*);
extern CirSolver* getSolver220(AigNtk*);
extern CirSolver* getSolverGlu(AigNtk*);

CirSolver* getSolver(AigNtk* ntk)
{
	constexpr CirSolver* (*funcMap[SOLVER_TYPE_TOTAL])(AigNtk*) = { getSolver114, getSolver220, getSolverGlu };
	return funcMap[curSolverType](ntk);
}

CmdClass(SetSolver, CMD_TYPE_SYSTEM, 4, "-One", 2,
                                        "-Two", 2,
                                        "-GLu", 3,
                                        "-GEt", 3);

struct SatRegistrar : public CmdRegistrar
{
	SatRegistrar()
	{
		setFile();
		setLine(); cmdMgr->regCmd<SetSolverCmd>("SET SOlver", 3, 2);
	}
} static satRegistrar;

/*========================================================================
	SET SOlver <-One | -Two | -GLu | -GEt>
--------------------------------------------------------------------------
	0: -One, 2
	1: -Two, 2
	2: -GLu, 3
	3: -GEt, 3
========================================================================*/

CmdExecStatus
SetSolverCmd::exec(char* options)const
{
	enum { ONE = 0, TWO, GLU, GET, NONE } type = NONE;
	for(const char* token: breakToTokens(options))
		if(optMatch<0>(token))
		{
			if(type != NONE)
				return errorOption(CMD_OPT_EXTRA, token);
			type = ONE;
		}
		else if(optMatch<1>(token))
		{
			if(type != NONE)
				return errorOption(CMD_OPT_EXTRA, token);
			type = TWO;
		}
		else if(optMatch<2>(token))
		{
			if(type != NONE)
				return errorOption(CMD_OPT_EXTRA, token);
			type = GLU;
		}
		else if(optMatch<3>(token))
		{
			if(type != NONE)
				return errorOption(CMD_OPT_EXTRA, token);
			type = GET;
		}
		else return errorOption(CMD_OPT_ILLEGAL, token);

	switch(type)
	{
		case NONE: return errorOption(CMD_OPT_MISSING);
		case GET:
			cout << "Current SAT solver is " << solverName[curSolverType] << endl;
			break;

		default:
			if(curSolverType == SolverType(type))
				cout << "The SAT solver is already " << solverName[curSolverType] << endl;
			else
			{
				cout << "Change SAT solver to be " << solverName[curSolverType = SolverType(type)] << endl;
				if(curSolverType == SOLVER_TYPE_MINISAT114)
					cout << "[Note] Resource-constrained solving is not supported by "
					     << solverName[SOLVER_TYPE_MINISAT114] << endl;
			}
			break;
	}
	return CMD_EXEC_DONE;
}

const char*
SetSolverCmd::getUsageStr()const
{
	return "<-One | -Two | -GLu | -GEt>\n";
}

const char*
SetSolverCmd::getHelpStr()const
{
	return "Set the type of SAT solver\n";
}

}
