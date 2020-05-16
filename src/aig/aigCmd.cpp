/*========================================================================\
|: [Filename] aigCmd.cpp                                                 :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Define and implement the commands about AIG network        :|
<------------------------------------------------------------------------*/

#include "cmdMgr.h"
#include "aigParser.h"
#include "aigWriter.h"

namespace _54ff
{

CmdClass(ReadAig,  CMD_TYPE_IO, 0);
CmdClass(ReadAag,  CMD_TYPE_IO, 0);
CmdClass(WriteAig, CMD_TYPE_IO, 0);
CmdClass(WriteAag, CMD_TYPE_IO, 0);

CmdClass(PrintGate,    CMD_TYPE_DISPLAY, 2, "-Cone",  2,
                                            "-Bound", 2);
CmdClass(PrintNetwork, CMD_TYPE_DISPLAY, 9, "-Summary",   2,
                                            "-All",       2,
                                            "-Netlist",   2,
                                            "-PI",        3,
                                            "-LAtch",     3,
                                            "-PO",        3,
                                            "-Recycle",   2,
                                            "-LEvel",     3,
                                            "-Influence", 2);

CmdClass(SimpNetwork, CMD_TYPE_SYNTHESIS, 11, "-COMpress",   4,
                                              "-One",        2,
                                              "-Two",        2,
                                              "-CONE",       5,
                                              "-Reachable",  2,
                                              "-Fraig",      2,
                                              "-Balance",    2,
                                              "-CONStant",   5,
                                              "-Verbose",    2,
                                              "-Pdr",        2,
                                              "-Limit",      2);

CmdClass(SimuNetwork, CMD_TYPE_VERIFICATION, 3, "-All",    2,
                                                "-Output", 2,
                                                "-Latch",  2);

CmdClass(PrintConeM, CMD_TYPE_EXPERIMENT, 0);
CmdClass(PrintConeS, CMD_TYPE_EXPERIMENT, 1, "-Depth", 2);
CmdClass(AddCube,    CMD_TYPE_EXPERIMENT, 2, "-Number", 2,
                                             "-Human",  2);

CmdClass(TestNetwork, CMD_TYPE_HIDDEN, 5, "-COPYOld", 6,
                                          "-COPYNew", 6,
                                          "-MOVEOld", 6,
                                          "-MOVENew", 6,
                                          "-And",     2);
CmdClass(SizeNetwork, CMD_TYPE_HIDDEN, 0);

struct AigRegistrar : public CmdRegistrar
{
	AigRegistrar()
	{
		setFile();
		setLine(); cmdMgr->regCmd< ReadAigCmd>("REAd AIg",  3, 2);
		setLine(); cmdMgr->regCmd< ReadAagCmd>("REAd AAg",  3, 2);
		setLine(); cmdMgr->regCmd<WriteAigCmd>("WRIte AIg", 3, 2);
		setLine(); cmdMgr->regCmd<WriteAagCmd>("WRIte AAg", 3, 2);

		setLine(); cmdMgr->regCmd<PrintGateCmd   >("PRInt GAte",    3, 2);
		setLine(); cmdMgr->regCmd<PrintNetworkCmd>("PRInt NEtwork", 3, 2);

		setLine(); cmdMgr->regCmd<SimpNetworkCmd>("SIMPlify NEtwork", 4, 2);

		setLine(); cmdMgr->regCmd<SimuNetworkCmd>("SIMUlate NEtwork", 4, 2);

		setLine(); cmdMgr->regCmd<PrintConeMCmd  >("PRInt COne Multiple", 3, 2, 1);
		setLine(); cmdMgr->regCmd<PrintConeSCmd  >("PRInt COne Single",   3, 2, 1);
		setLine(); cmdMgr->regCmd<AddCubeCmd     >("ADD CUbe",            3, 2);

		setLine(); cmdMgr->regCmd<TestNetworkCmd>("TESt NEtwork", 3, 2);
		setLine(); cmdMgr->regCmd<SizeNetworkCmd>("SIZe NEtwork", 3, 2);
	}
} static aigRegistrar;

/*========================================================================
	REAd AIg <(string fileName)>
========================================================================*/

CmdExecStatus
ReadAigCmd::exec(char* options)const
{
	PureStrList tokens = breakToTokens(options);
	if(tokens.size() < 1)
		return errorOption(CMD_OPT_MISSING);
	if(tokens.size() > 1)
		return errorOption(CMD_OPT_EXTRA, tokens[1]);
	WrapStr s(replaceHomeDir(tokens[0]), false);
	const char* aigFileName = (const char*)s ? (const char*)s : tokens[0];
	AigNtk* newNtk = aigParser[BINARY]->parseAig(aigFileName);
	if(newNtk != 0)
	{
		if(checkNtk(false))
			{ cout << "[Note] The original network has been replaced!" << endl; delete aigNtk; }
		setNtk(newNtk);
		return CMD_EXEC_DONE;
	}
	else return CMD_EXEC_ERROR_INT;
}

const char*
ReadAigCmd::getUsageStr()const
{
	return "<(string fileName)>\n";
}

const char*
ReadAigCmd::getHelpStr()const
{
	return "Read AIGER file in binary format\n";
}

/*========================================================================
	REAd AAg <(string fileName)>
========================================================================*/

CmdExecStatus
ReadAagCmd::exec(char* options)const
{
	PureStrList tokens = breakToTokens(options);
	if(tokens.size() < 1)
		return errorOption(CMD_OPT_MISSING);
	if(tokens.size() > 1)
		return errorOption(CMD_OPT_EXTRA, tokens[1]);
	WrapStr s(replaceHomeDir(tokens[0]), false);
	const char* aagFileName = (const char*)s ? (const char*)s : tokens[0];
	AigNtk* newNtk = aigParser[ASCII]->parseAig(aagFileName);
	if(newNtk != 0)
	{
		if(checkNtk(false))
			{ cout << "[Note] The original network has been replaced!" << endl; delete aigNtk; }
		setNtk(newNtk);
		return CMD_EXEC_DONE;
	}
	else return CMD_EXEC_ERROR_INT;
}

const char*
ReadAagCmd::getUsageStr()const
{
	return "<(string fileName)>\n";
}

const char*
ReadAagCmd::getHelpStr()const
{
	return "Read AIGER file in ascii format\n";
}

/*========================================================================
	WRIte AIg <(string fileName)>
========================================================================*/

CmdExecStatus
WriteAigCmd::exec(char* options)const
{
	PureStrList tokens = breakToTokens(options);
	if(tokens.size() < 1)
		return errorOption(CMD_OPT_MISSING);
	if(tokens.size() > 1)
		return errorOption(CMD_OPT_EXTRA, tokens[1]);
	if(!checkNtk()) return CMD_EXEC_ERROR_INT;
	WrapStr s(replaceHomeDir(tokens[0]), false);
	const char* aigFileName = (const char*)s ? (const char*)s : tokens[0];
	return aigWriter[BINARY]->writeAig(aigFileName, aigNtk) ? CMD_EXEC_DONE : CMD_EXEC_ERROR_INT;
}

const char*
WriteAigCmd::getUsageStr()const
{
	return "<(string fileName)>\n";
}

const char*
WriteAigCmd::getHelpStr()const
{
	return "Write AIGER file in binary format\n";
}

/*========================================================================
	WRIte AAg <(string fileName)>
========================================================================*/

CmdExecStatus
WriteAagCmd::exec(char* options)const
{
	PureStrList tokens = breakToTokens(options);
	if(tokens.size() < 1)
		return errorOption(CMD_OPT_MISSING);
	if(tokens.size() > 1)
		return errorOption(CMD_OPT_EXTRA, tokens[1]);
	if(!checkNtk()) return CMD_EXEC_ERROR_INT;
	WrapStr s(replaceHomeDir(tokens[0]), false);
	const char* aagFileName = (const char*)s ? (const char*)s : tokens[0];
	return aigWriter[ASCII]->writeAig(aagFileName, aigNtk) ? CMD_EXEC_DONE : CMD_EXEC_ERROR_INT;
}

const char*
WriteAagCmd::getUsageStr()const
{
	return "<(string fileName)>\n";
}

const char*
WriteAagCmd::getHelpStr()const
{
	return "Write AIGER file in ascii format\n";
}

/*========================================================================
	PRInt Gate <<(unsigned gateId)> [<-Cone (unsigned level)> [-Bound]]>
--------------------------------------------------------------------------
	0: -Cone,  2
	1: -Bound, 2
========================================================================*/

CmdExecStatus
PrintGateCmd::exec(char* options)const
{
	PureStrList tokens = breakToTokens(options);
	if(tokens.size() == 0)
		return errorOption(CMD_OPT_MISSING);
	AigGateID id;
	if(!myStrToUInt(tokens[0], id))
		return errorOption(CMD_OPT_INVALID_UINT, tokens[0]);
	bool cone = false, bound = false;
	unsigned level;
	if(tokens.size() > 1)
	{
		if(!optMatch<0>(tokens[1]))
			return errorOption(CMD_OPT_ILLEGAL, tokens[1]);
		cone = true;
		if(tokens.size() == 2)
			return errorOption(CMD_OPT_MISSING);
		if(!myStrToUInt(tokens[2], level))
			return errorOption(CMD_OPT_INVALID_UINT, tokens[2]);
		if(tokens.size() > 3)
		{
			if(!optMatch<1>(tokens[3]))
				return errorOption(CMD_OPT_ILLEGAL, tokens[3]);
			bound = true;
			if(tokens.size() > 4)
				return errorOption(CMD_OPT_EXTRA, tokens[4]);
		}
	}
	if(!checkNtk()) return CMD_EXEC_ERROR_INT;
	if(AigGate* g = checkGate(id); g == 0)
		return CMD_EXEC_ERROR_INT;
	else cone ? g->printFanInCone(level, bound) : g->printStat();
	return CMD_EXEC_DONE;
}

const char*
PrintGateCmd::getUsageStr()const
{
	return "<<(unsigned gateId)> [<-Cone (unsigned level)> [-Bound]]>\n";
}

const char*
PrintGateCmd::getHelpStr()const
{
	return "Print information about AIG gate\n";
}

/*========================================================================
	PRInt Network [-Summary | -All | -Netlist |
	               -PI | -LAtch | -PO | -Recycle | -LEvel | -Influence]
--------------------------------------------------------------------------
	0: -Summary,   2
	1: -All,       2
	2: -Netlist,   2
	3: -PI,        3
	4: -LAtch,     3
	5: -PO,        3
	6: -Recycle,   2
	7: -LEvel,     3
	8: -Influence, 2
========================================================================*/

CmdExecStatus
PrintNetworkCmd::exec(char* options)const
{
	enum { DEFAULT, SUMMARY, ALL, NETLIST,
	       PI, LATCH, PO, RECYCLE, LEVEL, INFLUENCE } type = DEFAULT;
	for(const char* token: breakToTokens(options))
		if(optMatch<0>(token))
			{ if(type != DEFAULT) return errorOption(CMD_OPT_EXTRA, token); type = SUMMARY; }
		else if(optMatch<1>(token))
			{ if(type != DEFAULT) return errorOption(CMD_OPT_EXTRA, token); type = ALL; }
		else if(optMatch<2>(token))
			{ if(type != DEFAULT) return errorOption(CMD_OPT_EXTRA, token); type = NETLIST; }
		else if(optMatch<3>(token))
			{ if(type != DEFAULT) return errorOption(CMD_OPT_EXTRA, token); type = PI; }
		else if(optMatch<4>(token))
			{ if(type != DEFAULT) return errorOption(CMD_OPT_EXTRA, token); type = LATCH; }
		else if(optMatch<5>(token))
			{ if(type != DEFAULT) return errorOption(CMD_OPT_EXTRA, token); type = PO; }
		else if(optMatch<6>(token))
			{ if(type != DEFAULT) return errorOption(CMD_OPT_EXTRA, token); type = RECYCLE; }
		else if(optMatch<7>(token))
			{ if(type != DEFAULT) return errorOption(CMD_OPT_EXTRA, token); type = LEVEL; }
		else if(optMatch<8>(token))
			{ if(type != DEFAULT) return errorOption(CMD_OPT_EXTRA, token); type = INFLUENCE; }
		else return errorOption(CMD_OPT_ILLEGAL, token);
	if(!checkNtk()) return CMD_EXEC_ERROR_INT;
	switch(type)
	{
		case DEFAULT :
		case SUMMARY : aigNtk->printSummary();  break;
		case ALL     : aigNtk->printAll();      break;
		case NETLIST : aigNtk->printNetlist();  break;
		case PI      : aigNtk->printPIs();      break;
		case LATCH   : aigNtk->printLatches();  break;
		case PO      : aigNtk->printPOs();      break;
		case RECYCLE : aigNtk->printRecycled(); break;

		case LEVEL     : if(!aigNtk->printLevel())     return CMD_EXEC_ERROR_INT; break;
		case INFLUENCE : if(!aigNtk->printInfluence()) return CMD_EXEC_ERROR_INT; break;
	}
	return CMD_EXEC_DONE;
}

const char*
PrintNetworkCmd::getUsageStr()const
{
	return "[-Summary | -All | -Netlist |\n"
	       " -PI | -LAtch | -PO | -Recycle | -LEvel | -Influence]\n";
}

const char*
PrintNetworkCmd::getHelpStr()const
{
	return "Print information about AIG network\n";
}

/*========================================================================
	SIMPlify NEtwork <-One | -Two | -Fraig [-Limit (unsigned confLimit)] |
	                  -COMpress | -CONE | -Reachable |
	                  -Balance |
	                  -CONStant [-Pdr [-Limit (unsigned satLimit)]]>
	                 [-Verbose]
--------------------------------------------------------------------------
	0:  -COMpress,   4
	1:  -One,        2
	2:  -Two,        2
	3:  -CONE,       5
	4:  -Reachable,  2
	5:  -Fraig,      2
	6:  -Balance,    2
	7:  -CONStant,   5
	8:  -Verbose,    2
	9:  -Pdr,        2
	10: -Limit,      2
========================================================================*/

CmdExecStatus
SimpNetworkCmd::exec(char* options)const
{
	enum { NONE, COMPRESS, ONE, TWO, CONE, REACHABLE,
	       FRAIG, BALANCE, CONSTANT_MONO, CONSTANT_PDR } type = NONE;
	bool verbose = false;
	size_t limit = 0;
	bool customLimit = false;

	PureStrList tokens = breakToTokens(options);
	for(size_t i = 0, n = tokens.size(); i < n; ++i)
		if(optMatch<0>(tokens[i]))
			{ if(type != NONE) return errorOption(CMD_OPT_EXTRA, tokens[i]); type = COMPRESS; }
		else if(optMatch<1>(tokens[i]))
			{ if(type != NONE) return errorOption(CMD_OPT_EXTRA, tokens[i]); type = ONE; }
		else if(optMatch<2>(tokens[i]))
			{ if(type != NONE) return errorOption(CMD_OPT_EXTRA, tokens[i]); type = TWO; }
		else if(optMatch<3>(tokens[i]))
			{ if(type != NONE) return errorOption(CMD_OPT_EXTRA, tokens[i]); type = CONE; }
		else if(optMatch<4>(tokens[i]))
			{ if(type != NONE) return errorOption(CMD_OPT_EXTRA, tokens[i]); type = REACHABLE; }
		else if(optMatch<5>(tokens[i]))
			{ if(type != NONE) return errorOption(CMD_OPT_EXTRA, tokens[i]); type = FRAIG; }
		else if(optMatch<6>(tokens[i]))
			{ if(type != NONE) return errorOption(CMD_OPT_EXTRA, tokens[i]); type = BALANCE; }
		else if(optMatch<7>(tokens[i]))
			{ if(type != NONE) return errorOption(CMD_OPT_EXTRA, tokens[i]); type = CONSTANT_MONO; }
		else if(optMatch<8>(tokens[i]))
		{
			if(type == NONE)
				return errorOption(CMD_OPT_ILLEGAL, tokens[i]);
			if(verbose)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			verbose = true;
		}
		else if(optMatch<9>(tokens[i]))
		{
			if(type != CONSTANT_MONO && type != CONSTANT_PDR)
				return errorOption(CMD_OPT_ILLEGAL, tokens[i]);
			if(type == CONSTANT_PDR)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			type = CONSTANT_PDR;
		}
		else if(optMatch<10>(tokens[i]))
		{
			if(customLimit)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			if(type != CONSTANT_PDR && type != FRAIG)
				return errorOption(CMD_OPT_ILLEGAL, tokens[i]);
			if(++i == n)
				return errorOption(CMD_OPT_MISSING);
			if(!myStrToUInt(tokens[i], limit))
				return errorOption(CMD_OPT_INVALID_UINT, tokens[i]);
			customLimit = true;
		}
		else return errorOption(CMD_OPT_ILLEGAL, tokens[i]);
	if(type == NONE) return errorOption(CMD_OPT_MISSING);
	if(!checkNtk()) return CMD_EXEC_ERROR_INT;
	bool success = false;
	simpMsg.setActive(verbose);
	switch(type)
	{
		case NONE: assert(false); break;
		case ONE   : success = aigNtk->oneLvlStrucSimp(); break;
		case TWO   : success = aigNtk->twoLvlStrucSimp(); break;
		case FRAIG : success = aigNtk->fraig(limit);      break;

		case COMPRESS  : success = aigNtk->compress();     break;
		case CONE      : success = aigNtk->collectCOI();   break;
		case REACHABLE : success = aigNtk->calReachable(); break;

		case BALANCE : success = aigNtk->balance(); break;

		case CONSTANT_MONO : success = aigNtk->rmConstLatch(false, limit); break;
		case CONSTANT_PDR  : success = aigNtk->rmConstLatch(true,  limit); break;
	}
	return success ? CMD_EXEC_DONE : CMD_EXEC_ERROR_INT;
}

const char*
SimpNetworkCmd::getUsageStr()const
{
	return "<-One | -Two | -Fraig [-Limit (unsigned confLimit)] |\n"
	       " -COMpress | -CONE | -Reachable |\n"
	       " -Balance |\n"
	       " -CONStant [-Pdr [-Limit (unsigned satLimit)]]>\n"
	       "[-Verbose]\n";
}

const char*
SimpNetworkCmd::getHelpStr()const
{
	return "Simplify the AIG network\n";
}

/*========================================================================
	SIMUlation NEtwork <<(string patternFile)>
	                    [-All | -Latch] [-Output (string outputFile)]>
--------------------------------------------------------------------------
	0: -All,    2
	1: -Output, 2
	2: -Latch,  2
========================================================================*/

CmdExecStatus
SimuNetworkCmd::exec(char* options)const
{
	PureStrList tokens = breakToTokens(options);
	if(tokens.size() < 1)
		return errorOption(CMD_OPT_MISSING);
	AigSimPrintType printType = AIG_SIM_PRINT_PO;
	const char* outputFileName = 0;
	for(size_t i = 1; i < tokens.size(); ++i)
		if(optMatch<0>(tokens[i]))
		{
			if(printType != AIG_SIM_PRINT_PO)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			printType = AIG_SIM_PRINT_ALL;
		}
		else if(optMatch<1>(tokens[i]))
		{
			if(outputFileName != 0)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			if(++i == tokens.size())
				return errorOption(CMD_OPT_MISSING);
			outputFileName = tokens[i];
		}
		else if(optMatch<2>(tokens[i]))
		{
			if(printType != AIG_SIM_PRINT_PO)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			printType = AIG_SIM_PRINT_LATCH;
		}
		else return errorOption(CMD_OPT_ILLEGAL, tokens[i]);
	if(!checkNtk()) return CMD_EXEC_ERROR_INT;
	WrapStr s1(replaceHomeDir(tokens[0]), false);
	const char* patternFileName = (const char*)s1 ? (const char*)s1 : tokens[0];
	WrapStr s2;
	if(outputFileName != 0) s2.setStr(replaceHomeDir(outputFileName), false);
	if((const char*)s2) outputFileName = (const char*)s2;
	return aigNtk->simulate(patternFileName, printType, outputFileName) ? CMD_EXEC_DONE : CMD_EXEC_ERROR_INT;
}

const char*
SimuNetworkCmd::getUsageStr()const
{
	return "<<(string patternFile)>\n"
	       " [-All | -Latch] [-Output (string outputFile)]>\n";
}

const char*
SimuNetworkCmd::getHelpStr()const
{
	return "Perform 3-value simulation on the AIG network based on the given pattern\n";
}

/*========================================================================
	PRInt COne Multiple <(unsigned latchId)...>
========================================================================*/

CmdExecStatus
PrintConeMCmd::exec(char* options)const
{
	PureStrList tokens = breakToTokens(options);
	if(tokens.empty())
		return errorOption(CMD_OPT_MISSING);
	vector<AigGateID> latchIds(tokens.size());
	for(size_t i = 0, n = tokens.size(); i < n; ++i)
		if(!myStrToUInt(tokens[i], latchIds[i]))
			return errorOption(CMD_OPT_INVALID_UINT, tokens[i]);
	if(!checkNtk()) return CMD_EXEC_ERROR_INT;
	AigGate::setGlobalRef();
	for(AigGateID latchId: latchIds)
		if(AigGate* g = checkGate(latchId); g == 0)
			return CMD_EXEC_ERROR_INT;
		else if(g->getGateType() != AIG_LATCH)
			{ cerr << "[Error] Gate with ID " << latchId << " is not a latch!" << endl; return CMD_EXEC_ERROR_INT; }
		else if(g->isGlobalRef())
			{ cerr << "[Error] Gate with ID " << latchId << " is duplicated!" << endl; return CMD_EXEC_ERROR_INT; }
		else g->setToGlobalRef();
	Array<bool> latchInvolved(aigNtk->getLatchNum());
	for(size_t i = 0, L = aigNtk->getLatchNum(); i < L; ++i)
		latchInvolved[i] = aigNtk->getLatch(i)->isGlobalRef();
	aigNtk->printCone(latchInvolved, MAX_SIZE_T);
	return CMD_EXEC_DONE;
}

const char*
PrintConeMCmd::getUsageStr()const
{
	return "<(unsigned latchId)...>\n";
}

const char*
PrintConeMCmd::getHelpStr()const
{
	return "Print the latches within cone for multiple targets in AIG network\n";
}

/*========================================================================
	PRInt COne Single <<(unsigned COId)> [-Depth (unsigned maxDepth)]>
--------------------------------------------------------------------------
	0: -Depth, 2
========================================================================*/

CmdExecStatus
PrintConeSCmd::exec(char* options)const
{
	PureStrList tokens = breakToTokens(options);
	if(tokens.size() == 0)
		return errorOption(CMD_OPT_MISSING);
	size_t COId;
	if(!myStrToUInt(tokens[0], COId))
		return errorOption(CMD_OPT_INVALID_UINT, tokens[0]);
	else if(!checkNtk())
		return CMD_EXEC_ERROR_INT;
	else if(AigGate* g = checkGate(COId); g == 0)
		return CMD_EXEC_ERROR_INT;
	else if(!g->isCO())
		{ cerr << "[Error] Gate with ID " << COId << " is not a CO!" << endl; return CMD_EXEC_ERROR_INT; }

	size_t maxDepth = MAX_SIZE_T;
	if(tokens.size() > 1)
	{
		if(!optMatch<0>(tokens[1]))
			return errorOption(CMD_OPT_ILLEGAL, tokens[1]);
		if(tokens.size() == 2)
			return errorOption(CMD_OPT_MISSING);
		if(!myStrToUInt(tokens[2], maxDepth))
			return errorOption(CMD_OPT_INVALID_UINT, tokens[2]);
		if(tokens.size() > 3)
			return errorOption(CMD_OPT_EXTRA, tokens[4]);
	}
	aigNtk->printCone(COId, maxDepth);
	return CMD_EXEC_DONE;
}

const char*
PrintConeSCmd::getUsageStr()const
{
	return "<<(unsigned COId)> [-Depth (unsigned maxDepth)]>\n";
}

const char*
PrintConeSCmd::getHelpStr()const
{
	return "Print the latches within cone for single target in AIG network\n";
}

/*========================================================================
	ADD CUbe <-Number (unsigned latchLit)... |
	          -Human  (string   latchLit)...>
--------------------------------------------------------------------------
	0: -Number, 2
	1: -Human,  2
========================================================================*/

CmdExecStatus
AddCubeCmd::exec(char* options)const
{
	PureStrList tokens = breakToTokens(options);
	if(tokens.empty())
		return errorOption(CMD_OPT_MISSING);
	bool humanReadable;
	if(optMatch<0>(tokens[0]))
		humanReadable = false;
	else if(optMatch<1>(tokens[0]))
		humanReadable = true;
	else return errorOption(CMD_OPT_ILLEGAL, tokens[0]);

	if(tokens.size() == 1)
		return errorOption(CMD_OPT_MISSING);
	vector<AigGateLit> latchLits(tokens.size() - 1);
	if(humanReadable)
	{
		for(size_t i = 1, n = tokens.size(); i < n; ++i)
		{
			bool isInv = false;
			if(tokens[i][0] == '!')
				{ isInv = true; tokens[i] += 1; }
			if(!myStrToUInt(tokens[i], latchLits[i-1]))
				return errorOption(CMD_OPT_INVALID_UINT, tokens[i]);
			latchLits[i-1] = makeToLit(AigGateID(latchLits[i-1]), isInv);
		}
	}
	else
	{
		for(size_t i = 1, n = tokens.size(); i < n; ++i)
			if(!myStrToUInt(tokens[i], latchLits[i-1]))
				return errorOption(CMD_OPT_INVALID_UINT, tokens[i]);
	}

	if(!checkNtk()) return CMD_EXEC_ERROR_INT;
	vector<AigGateV> gateList;
	gateList.reserve(latchLits.size());
	for(AigGateLit lit: latchLits)
		if(AigGate* g = checkGate(getGateID(lit)); g == 0)
			return CMD_EXEC_ERROR_INT;
		else if(g->getGateType() == AIG_PO)
			{ cerr << "[Error] Gate with ID " << getGateID(lit) << " is a PO!" << endl; return CMD_EXEC_ERROR_INT; }
		else gateList.emplace_back(g, isInv(lit));
	cout << "The new PO is no " << aigNtk->getOutputNum()
	     << " with ID "         << aigNtk->createOutput(aigNtk->createAnd(gateList))->getGateID() << endl;
	return CMD_EXEC_DONE;
}

const char*
AddCubeCmd::getUsageStr()const
{
	return "<-Number (unsigned latchLit)... |\n"
	       " -Human  (string   latchLit)...>\n";
}

const char*
AddCubeCmd::getHelpStr()const
{
	return "Create a new PO to represent a cube in AIG network\n";
}

/*========================================================================
	TESt NEtwork <-COPYOld | -COPYNew | -MOVEOld | -MOVENew | -And>
--------------------------------------------------------------------------
	0: -COPYOld, 6
	1: -COPYNew, 6
	2: -MOVEOld, 6
	3: -MOVENew, 6
	4: -And,     2
========================================================================*/

CmdExecStatus
TestNetworkCmd::exec(char* options)const
{
	enum { NONE, COPYOLD, COPYNEW, MOVEOLD, MOVENEW, AND } type = NONE;
	for(const char* token: breakToTokens(options))
		if(optMatch<0>(token))
			{ if(type != NONE) return errorOption(CMD_OPT_EXTRA, token); type = COPYOLD; }
		else if(optMatch<1>(token))
			{ if(type != NONE) return errorOption(CMD_OPT_EXTRA, token); type = COPYNEW; }
		else if(optMatch<2>(token))
			{ if(type != NONE) return errorOption(CMD_OPT_EXTRA, token); type = MOVEOLD; }
		else if(optMatch<3>(token))
			{ if(type != NONE) return errorOption(CMD_OPT_EXTRA, token); type = MOVENEW; }
		else if(optMatch<4>(token))
			{ if(type != NONE) return errorOption(CMD_OPT_EXTRA, token); type = AND; }
		else return errorOption(CMD_OPT_ILLEGAL, token);
	if(type == NONE) return errorOption(CMD_OPT_MISSING);
	if(!checkNtk()) return CMD_EXEC_ERROR_INT;
	switch(type)
	{
		case NONE: assert(false); break;
		case COPYOLD:
			cout << "[Note] Copy aigNtk to a new network and delete the new one!" << endl;
			delete aigNtk->copyNtk(); break;

		case COPYNEW:
			cout << "[Note] Copy aigNtk to a new network and delete the original one!" << endl;
			{ AigNtk* newNtk = aigNtk->copyNtk(); delete aigNtk; setNtk(newNtk); } break;

		case MOVEOLD:
			cout << "[Note] Move aigNtk to a new network and delete the new one!" << endl;
			delete aigNtk->moveNtk(); break;

		case MOVENEW:
			cout << "[Note] Move aigNtk to a new network and delete the original one!" << endl;
			{ AigNtk* newNtk = aigNtk->moveNtk(); delete aigNtk; setNtk(newNtk); } break;

		case AND:
			cout << aigNtk->getAndNum() << endl; break;
	}
	return CMD_EXEC_DONE;
}

const char*
TestNetworkCmd::getUsageStr()const
{
	return "<-COPYOld | -COPYNew | -MOVEOld | -MOVENew | -And>\n";
}

const char*
TestNetworkCmd::getHelpStr()const
{
	return "Test the functionality of AigNtk\n";
}

/*========================================================================
	SIZe NEtwork
========================================================================*/

CmdExecStatus
SizeNetworkCmd::exec(char* options)const
{
	if(PureStrList tokens = breakToTokens(options);
	   tokens.size() > 0)
		return errorOption(CMD_OPT_ILLEGAL, tokens[0]);
	cout << "====== Size Statistics ======" << endl
	     << left << setw(16) << "AigNtk"          << ": " << right << setw(4) << sizeof(AigNtk)          << " bytes" << endl
	     << left << setw(16) << "AigGateV"        << ": " << right << setw(4) << sizeof(AigGateV)        << " bytes" << endl
	     << left << setw(16) << "AigPi"           << ": " << right << setw(4) << sizeof(AigPi)           << " bytes" << endl
	     << left << setw(16) << "AigLatch"        << ": " << right << setw(4) << sizeof(AigLatch)        << " bytes" << endl
	     << left << setw(16) << "AigPo"           << ": " << right << setw(4) << sizeof(AigPo)           << " bytes" << endl
	     << left << setw(16) << "AigAnd"          << ": " << right << setw(4) << sizeof(AigAnd)          << " bytes" << endl
	     << left << setw(16) << "AigConst0"       << ": " << right << setw(4) << sizeof(AigConst0)       << " bytes" << endl
	     << left << setw(16) << "AigAsciiParser"  << ": " << right << setw(4) << sizeof(AigAsciiParser)  << " bytes" << endl
	     << left << setw(16) << "AigBinaryParser" << ": " << right << setw(4) << sizeof(AigBinaryParser) << " bytes" << endl
	     << left << setw(16) << "AigAsciiWriter"  << ": " << right << setw(4) << sizeof(AigAsciiWriter)  << " bytes" << endl
	     << left << setw(16) << "AigBinaryWriter" << ": " << right << setw(4) << sizeof(AigBinaryWriter) << " bytes" << endl;
	return CMD_EXEC_DONE;
}

const char*
SizeNetworkCmd::getUsageStr()const
{
	return "\n";
}

const char*
SizeNetworkCmd::getHelpStr()const
{
	return "Get the size of classes about network\n";
}

}
