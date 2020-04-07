/*========================================================================\
|: [Filename] sfcCmd.cpp                                                 :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Define and implement the commands to check safety property :|
<------------------------------------------------------------------------*/

#include "cmdMgr.h"
#include "sfcChecker.h"
#include "bmcChecker.h"
#include "pdrChecker.h"
#include "pbcChecker.h"

namespace _54ff
{

CmdClass(BmcCheck, CMD_TYPE_VERIFICATION, 4, "-TRace",   3,
                                             "-Max",     2,
                                             "-Assert",  2,
                                             "-TImeout", 3);
CmdClass(IndCheck, CMD_TYPE_VERIFICATION, 5, "-TRace",   3,
                                             "-Max",     2,
                                             "-Need",    2,
                                             "-All",     2,
                                             "-TImeout", 3);
CmdClass(ItpCheck, CMD_TYPE_VERIFICATION, 5, "-TRace",   3,
                                             "-Max",     2,
                                             "-All",     2,
                                             "-Last",    2,
                                             "-TImeout", 3);
CmdClass(PdrCheck, CMD_TYPE_VERIFICATION, 29, "-TRace",     3,
                                              "-Max",       2,
                                              "-EVent",     3,
                                              "-Backward",  2,
                                              "-INTernal",  4,
                                              "-ACtivity",  3,
                                              "-Push",      2,
                                              "-Decay",     2,
                                              "-REVerse",   4,
                                              "-Queue",     2,
                                              "-EAger",     3,
                                              "-STat",      3,
                                              "-TImeout",   3,
                                              "-NOPush",    4,
                                              "-Verbose",   2,
                                              "-RECycle",   4,
                                              "-NOGen",     4,
                                              "-APPROXGen", 8,
                                              "-INFinite",  4,
                                              "-NEEDCone",  6,
                                              "-SElf",      3,
                                              "-ASsert",    3,
                                              "-CAlled",    3,
                                              "-NEEDFrame", 6,
                                              "-CHeck",     3,
                                              "-LOCALInf",  7,
                                              "-LOCALAll",  7,
                                              "-LOCALMix",  7,
                                              "-HALF",      5);
CmdClass(PbcCheck, CMD_TYPE_VERIFICATION, 9, "-TRace",     3,
                                             "-Max",       2,
                                             "-Stat",      2,
                                             "-TImeout",   3,
                                             "-Verbose",   2,
                                             "-At",        2,
                                             "-Upto",      2,
                                             "-Block",     2,
                                             "-Increase",  2);

struct SfcRegistrar : public CmdRegistrar
{
	SfcRegistrar()
	{
		setFile();
		setLine(); cmdMgr->regCmd<BmcCheckCmd>("CHEck SAfety Bmc", 3, 2, 1);
		setLine(); cmdMgr->regCmd<IndCheckCmd>("CHEck SAfety INd", 3, 2, 2);
		setLine(); cmdMgr->regCmd<ItpCheckCmd>("CHEck SAfety ITp", 3, 2, 2);
		setLine(); cmdMgr->regCmd<PdrCheckCmd>("CHEck SAfety PDr", 3, 2, 2);
		setLine(); cmdMgr->regCmd<PbcCheckCmd>("CHEck SAfety PBc", 3, 2, 2);
	}
} static sfcRegistrar;

/*========================================================================
	CHEck SAfety Bmc <(unsigned outputIdx)> [-TRace]
	                 [-TImeout (unsigned timeout)]
	                 [-Max (unsigned maxDepth)] [-Assert]
--------------------------------------------------------------------------
	0: -TRace,   3
	1: -Max,     2
	2: -Assert,  2
	3: -TImeout, 3
========================================================================*/

CmdExecStatus
BmcCheckCmd::exec(char* options)const
{
	PureStrList tokens = breakToTokens(options);
	if(tokens.size() == 0)
		return errorOption(CMD_OPT_MISSING);
	size_t outputIdx;
	if(!myStrToUInt(tokens[0], outputIdx))
		return errorOption(CMD_OPT_INVALID_UINT, tokens[0]);
	size_t maxDepth = 100;
	bool customMax = false;
	size_t timeout = 0;
	bool customTime = false;
	BmcCheckType bct = BMC_ONLY_LAST;
	bool trace = false;
	for(size_t i = 1, n = tokens.size(); i < n; ++i)
		if(optMatch<0>(tokens[i]))
		{
			if(trace)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			trace = true;
		}
		else if(optMatch<1>(tokens[i]))
		{
			if(customMax)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			if(++i == n)
				return errorOption(CMD_OPT_MISSING);
			if(!myStrToUInt(tokens[i], maxDepth))
				return errorOption(CMD_OPT_INVALID_UINT, tokens[i]);
			customMax = true;
		}
		else if(optMatch<2>(tokens[i]))
		{
			if(bct == BMC_ASSERT)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			bct = BMC_ASSERT;
		}
		else if(optMatch<3>(tokens[i]))
		{
			if(customTime)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			if(++i == n)
				return errorOption(CMD_OPT_MISSING);
			if(!myStrToUInt(tokens[i], timeout))
				return errorOption(CMD_OPT_INVALID_UINT, tokens[i]);
			customTime = true;
		}
		else return errorOption(CMD_OPT_ILLEGAL, tokens[i]);
	if(!checkNtk()) return CMD_EXEC_ERROR_INT;
	SafetyChecker* checker = getChecker<BmcChecker>(aigNtk, outputIdx, trace, timeout, maxDepth, bct);
	if(checker == 0) return CMD_EXEC_ERROR_INT;
	checker->Check(); delete checker; return CMD_EXEC_DONE;
}

const char*
BmcCheckCmd::getUsageStr()const
{
	return "<(unsigned outputIdx)> [-Trace]\n"
	       "[-TImeout (unsigned timeout)]\n"
	       "[-Max (unsigned maxDepth)] [-Assert]\n";
}

const char*
BmcCheckCmd::getHelpStr()const
{
	return "Check safety property of AIG network using bounded model checking\n";
}

/*========================================================================
	CHEck SAfety INd <(unsigned outputIdx)> [-TRace]
	                 [-TImeout (unsigned timeout)]
	                 [-Max (unsigned maxDepth)] [-Need | -All]
--------------------------------------------------------------------------
	0: -TRace,   3
	1: -Max,     2
	2: -Need,    2
	3: -All,     2
	4: -TImeout, 3
========================================================================*/

CmdExecStatus
IndCheckCmd::exec(char* options)const
{
	PureStrList tokens = breakToTokens(options);
	if(tokens.size() == 0)
		return errorOption(CMD_OPT_MISSING);
	size_t outputIdx;
	if(!myStrToUInt(tokens[0], outputIdx))
		return errorOption(CMD_OPT_INVALID_UINT, tokens[0]);
	size_t maxDepth = 100;
	bool customMax = false;
	size_t timeout = 0;
	bool customTime = false;
	IndCheckType ict = IND_SIMPLE_NO;
	bool trace = false;
	for(size_t i = 1, n = tokens.size(); i < n; ++i)
		if(optMatch<0>(tokens[i]))
		{
			if(trace)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			trace = true;
		}
		else if(optMatch<1>(tokens[i]))
		{
			if(customMax)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			if(++i == n)
				return errorOption(CMD_OPT_MISSING);
			if(!myStrToUInt(tokens[i], maxDepth))
				return errorOption(CMD_OPT_INVALID_UINT, tokens[i]);
			customMax = true;
		}
		else if(optMatch<2>(tokens[i]))
		{
			if(ict != IND_SIMPLE_NO)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			ict = IND_SIMPLE_NEED;
		}
		else if(optMatch<3>(tokens[i]))
		{
			if(ict != IND_SIMPLE_NO)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			ict = IND_SIMPLE_ALL;
		}
		else if(optMatch<4>(tokens[i]))
		{
			if(customTime)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			if(++i == n)
				return errorOption(CMD_OPT_MISSING);
			if(!myStrToUInt(tokens[i], timeout))
				return errorOption(CMD_OPT_INVALID_UINT, tokens[i]);
			customTime = true;
		}
		else return errorOption(CMD_OPT_ILLEGAL, tokens[i]);
	if(!checkNtk()) return CMD_EXEC_ERROR_INT;
	SafetyChecker* checker = getChecker<IndChecker>(aigNtk, outputIdx, trace, timeout, maxDepth, ict);
	if(checker == 0) return CMD_EXEC_ERROR_INT;
	checker->Check(); delete checker; return CMD_EXEC_DONE;
}

const char*
IndCheckCmd::getUsageStr()const
{
	return "<(unsigned outputIdx)> [-TRace]\n"
	       "[-TImeout (unsigned timeout)]\n"
	       "[-Max (unsigned maxDepth)] [-Need | -All]\n";
}

const char*
IndCheckCmd::getHelpStr()const
{
	return "Check safety property of AIG network using k-induction\n";
}

/*========================================================================
	CHEck SAfety ITp <(unsigned outputIdx)> [-TRace]
	                 [-TImeout (unsigned timeout)]
	                 [-Max (unsigned maxDepth)] [-All | -Last]
--------------------------------------------------------------------------
	0: -TRace,   3
	1: -Max,     2
	2: -All,     2
	3: -Last,    2
	4: -TImeout, 3
========================================================================*/

CmdExecStatus
ItpCheckCmd::exec(char* options)const
{
	PureStrList tokens = breakToTokens(options);
	if(tokens.size() == 0)
		return errorOption(CMD_OPT_MISSING);
	size_t outputIdx;
	if(!myStrToUInt(tokens[0], outputIdx))
		return errorOption(CMD_OPT_INVALID_UINT, tokens[0]);
	size_t maxDepth = 100;
	bool customMax = false;
	size_t timeout = 0;
	bool customTime = false;
	ItpCheckType ict = ITP_ASSERT;
	bool trace = false;
	for(size_t i = 1, n = tokens.size(); i < n; ++i)
		if(optMatch<0>(tokens[i]))
		{
			if(trace)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			trace = true;
		}
		else if(optMatch<1>(tokens[i]))
		{
			if(customMax)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			if(++i == n)
				return errorOption(CMD_OPT_MISSING);
			if(!myStrToUInt(tokens[i], maxDepth))
				return errorOption(CMD_OPT_INVALID_UINT, tokens[i]);
			customMax = true;
		}
		else if(optMatch<2>(tokens[i]))
		{
			if(ict != ITP_ASSERT)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			ict = ITP_ALL_NOT_P;
		}
		else if(optMatch<3>(tokens[i]))
		{
			if(ict != ITP_ASSERT)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			ict = ITP_ONLY_LAST;
		}
		else if(optMatch<4>(tokens[i]))
		{
			if(customTime)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			if(++i == n)
				return errorOption(CMD_OPT_MISSING);
			if(!myStrToUInt(tokens[i], timeout))
				return errorOption(CMD_OPT_INVALID_UINT, tokens[i]);
			customTime = true;
		}
		else return errorOption(CMD_OPT_ILLEGAL, tokens[i]);
	if(!checkNtk()) return CMD_EXEC_ERROR_INT;
	SafetyChecker* checker = getChecker<ItpChecker>(aigNtk, outputIdx, trace, timeout, maxDepth, ict);
	if(checker == 0) return CMD_EXEC_ERROR_INT;
	checker->Check(); delete checker; return CMD_EXEC_DONE;
}

const char*
ItpCheckCmd::getUsageStr()const
{
	return "<(unsigned outputIdx)> [-TRace]\n"
	       "[-TImeout (unsigned timeout)]\n"
	       "[-Max (unsigned maxDepth)] [-All | -Last]\n";
}

const char*
ItpCheckCmd::getHelpStr()const
{
	return "Check safety property of AIG network using interpolation-based method\n";
}

/*========================================================================
	CHEck SAfety PDr <(unsigned outputIdx)> [-TRace]
	                 [-TImeout (unsigned timeout)]
	                 [-Max (unsigned maxFrame)]
	                 [<-RECycle (unsigned recycleNum)> [-CAlled]]
	                 [-EVent | -Backward | -INTernal]
	                 [-NEEDCone] [-NEEDFrame]
	                 [<-ACtivity | -Decay> [-REVerse]] [-SElf]
	                 [-Push | -NOPush] [-Queue]
	                 [-APPROXGen | -NOGen]
	                 [-EAger] [-INFinite] [-ASsert]
	                 [-STat ("atsgprcx")] [-Verbose ("aogpbtcimfx")]
	                 [<<-LOCALInf | -LOCALAll | -LOCALMix> ((unsigned) backtrackNum matchNum) |
	                  -HALF ((unsigned) observeNum matchNum)> (unsigned satLimit)]
                     [-CHeck]
--------------------------------------------------------------------------
	0:  -TRace,     3
	1:  -Max,       2
	2:  -EVent,     3
	3:  -Backward,  2
	4:  -INTernal,  4
	5:  -ACtivity,  3
	6:  -Push,      2
	7:  -Decay,     2
	8:  -REVerse,   4
	9:  -Queue,     2
	10: -EAger,     3
	11: -STat,      3
	12: -TImeout,   3
	13: -NOPush,    4
	14: -Verbose,   2
	15: -RECycle,   4
	16: -NOGen,     4
	17: -APPROXGen, 8
	18: -INFinite,  4
	19: -NEEDCone,  6
	20: -SElf,      3
	21: -ASsert,    3
	22: -CAlled,    3
	23: -NEEDFrame, 6
	24: -CHeck,     3
	25: -LOCALInf,  7
	26: -LOCALAll,  7
	27: -LOCALMix,  7
	28: -HALF,      5
========================================================================*/

CmdExecStatus
PdrCheckCmd::exec(char* options)const
{
	PureStrList tokens = breakToTokens(options);
	if(tokens.size() == 0)
		return errorOption(CMD_OPT_MISSING);
	size_t outputIdx;
	if(!myStrToUInt(tokens[0], outputIdx))
		return errorOption(CMD_OPT_INVALID_UINT, tokens[0]);

	bool trace = false;
	size_t maxFrame = 100;
	bool customMax = false;

	size_t timeout = 0;
	bool customTime = false;

	size_t recycleNum = 0;
	bool customRecycle = false;

	PdrSimType psit = PDR_SIM_FORWARD_NORMAL;
	PdrOrdType port = PDR_ORD_INDEX;
	PdrOblType pobt = PDR_OBL_NORMAL;
	PdrDeqType pdt  = PDR_DEQ_STACK;
	PdrPrpType ppt  = PDR_PRP_NORMAL;
	PdrGenType pgt  = PDR_GEN_NORMAL;
	bool rInf      = false;
	bool cInNeedC  = false;
	bool cSelf     = false;
	bool assertF   = false;
	bool recycleBQ = false;
	bool cInNeedF  = false;

	size_t stats     = 0;
	size_t verbosity = 0;
	bool   checkII   = false;

	size_t satQL = 0;

	PdrStimuType pstt = PDR_STIMU_NONE;
	size_t stimuNum1, stimuNum2, stimuNum3;

	for(size_t i = 1, n = tokens.size(); i < n; ++i)
		if(optMatch<0>(tokens[i]))
		{
			if(trace)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			trace = true;
		}
		else if(optMatch<1>(tokens[i]))
		{
			if(customMax)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			if(++i == n)
				return errorOption(CMD_OPT_MISSING);
			if(!myStrToUInt(tokens[i], maxFrame))
				return errorOption(CMD_OPT_INVALID_UINT, tokens[i]);
			customMax = true;
		}
		else if(optMatch<2>(tokens[i]))
		{
			if(psit != PDR_SIM_FORWARD_NORMAL)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			psit = PDR_SIM_FORWARD_EVENT;
		}
		else if(optMatch<3>(tokens[i]))
		{
			if(psit != PDR_SIM_FORWARD_NORMAL)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			psit = PDR_SIM_BACKWARD_NORMAL;
		}
		else if(optMatch<4>(tokens[i]))
		{
			if(psit != PDR_SIM_FORWARD_NORMAL)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			psit = PDR_SIM_BACKWARD_INTERNAL;
		}
		else if(optMatch<5>(tokens[i]))
		{
			if(port != PDR_ORD_INDEX)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			port = PDR_ORD_ACTIVITY;
		}
		else if(optMatch<6>(tokens[i]))
		{
			if(pobt != PDR_OBL_NORMAL)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			pobt = PDR_OBL_PUSH;
		}
		else if(optMatch<7>(tokens[i]))
		{
			if(port != PDR_ORD_INDEX)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			port = PDR_ORD_DECAY;
		}
		else if(optMatch<8>(tokens[i]))
			switch(port)
			{
				case PDR_ORD_INDEX            : return errorOption(CMD_OPT_ILLEGAL, tokens[i]);
				case PDR_ORD_ACTIVITY_REVERSE :
				case PDR_ORD_DECAY_REVERSE    : return errorOption(CMD_OPT_EXTRA, tokens[i]);
				case PDR_ORD_ACTIVITY         : port = PDR_ORD_ACTIVITY_REVERSE; break;
				case PDR_ORD_DECAY            : port = PDR_ORD_DECAY_REVERSE;    break;
			}
		else if(optMatch<9>(tokens[i]))
		{
			if(pdt != PDR_DEQ_STACK)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			pdt = PDR_DEQ_QUEUE;
		}
		else if(optMatch<10>(tokens[i]))
		{
			if(ppt != PDR_PRP_NORMAL)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			ppt = PDR_PRP_EAGER;
		}
		else if(optMatch<11>(tokens[i]))
		{
			if(stats != 0)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			if(++i == n)
				return errorOption(CMD_OPT_MISSING);
			char buffer[2] = { 0, 0 };
			for(const char* tmp = tokens[i]; *tmp != 0; ++tmp)
			{
				size_t sMask;
				switch(*tmp)
				{
					case 'a' : sMask = getPdrStatMask(PDR_STAT_TOTAL) - 1;  break;
					case 't' : sMask = getPdrStatMask(PDR_STAT_TERSIM);     break;
					case 's' : sMask = getPdrStatMask(PDR_STAT_SAT);        break;
					case 'g' : sMask = getPdrStatMask(PDR_STAT_GENERALIZE); break;
					case 'p' : sMask = getPdrStatMask(PDR_STAT_PROPAGATE);  break;
					case 'r' : sMask = getPdrStatMask(PDR_STAT_RECYCLE);    break;
					case 'c' : sMask = getPdrStatMask(PDR_STAT_CUBE);       break;
					case 'x' : sMask = getPdrStatMask(PDR_STAT_STIMU);      break;
					default  :
						buffer[0] = *tmp;
						return errorOption(CMD_OPT_ILLEGAL, buffer);
				}
				if((stats & sMask) != 0)
					{ buffer[0] = *tmp; return errorOption(CMD_OPT_EXTRA, buffer); }
				stats |= sMask;
			}
		}
		else if(optMatch<12>(tokens[i]))
		{
			if(customTime)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			if(++i == n)
				return errorOption(CMD_OPT_MISSING);
			if(!myStrToUInt(tokens[i], timeout))
				return errorOption(CMD_OPT_INVALID_UINT, tokens[i]);
			customTime = true;
		}
		else if(optMatch<13>(tokens[i]))
		{
			if(pobt != PDR_OBL_NORMAL)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			pobt = PDR_OBL_IGNORE;
		}
		else if(optMatch<14>(tokens[i]))
		{
			if(verbosity != 0)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			if(++i == n)
				return errorOption(CMD_OPT_MISSING);
			char buffer[2] = { 0, 0 };
			for(const char* tmp = tokens[i]; *tmp != 0; ++tmp)
			{
				size_t vMask;
				switch(*tmp)
				{
					case 'a' : vMask = getPdrVbsMask(PDR_VERBOSE_TOTAL) - 1; break;
					case 'o' : vMask = getPdrVbsMask(PDR_VERBOSE_OBL);       break;
					case 'g' : vMask = getPdrVbsMask(PDR_VERBOSE_GEN);       break;
					case 'p' : vMask = getPdrVbsMask(PDR_VERBOSE_PROP);      break;
					case 'b' : vMask = getPdrVbsMask(PDR_VERBOSE_BLK);       break;
					case 't' : vMask = getPdrVbsMask(PDR_VERBOSE_TERSIM);    break;
					case 'c' : vMask = getPdrVbsMask(PDR_VERBOSE_CUBE);      break;
					case 'i' : vMask = getPdrVbsMask(PDR_VERBOSE_INF);       break;
					case 'm' : vMask = getPdrVbsMask(PDR_VERBOSE_MISC);      break;
					case 'f' : vMask = getPdrVbsMask(PDR_VERBOSE_FINAL);     break;
					case 'x' : vMask = getPdrVbsMask(PDR_VERBOSE_STIMU);     break;
					default  :
						buffer[0] = *tmp;
						return errorOption(CMD_OPT_ILLEGAL, buffer);
				}
				if((verbosity & vMask) != 0)
					{ buffer[0] = *tmp; return errorOption(CMD_OPT_EXTRA, buffer); }
				verbosity |= vMask;
			}
		}
		else if(optMatch<15>(tokens[i]))
		{
			if(customRecycle)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			if(++i == n)
				return errorOption(CMD_OPT_MISSING);
			if(!myStrToUInt(tokens[i], recycleNum))
				return errorOption(CMD_OPT_INVALID_UINT, tokens[i]);
			customRecycle = true;
		}
		else if(optMatch<16>(tokens[i]))
		{
			if(pgt != PDR_GEN_NORMAL)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			pgt = PDR_GEN_IGNORE;
		}
		else if(optMatch<17>(tokens[i]))
		{
			if(pgt != PDR_GEN_NORMAL)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			pgt = PDR_GEN_APPROX;
		}
		else if(optMatch<18>(tokens[i]))
		{
			if(rInf)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			rInf = true;
		}
		else if(optMatch<19>(tokens[i]))
		{
			if(cInNeedC)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			cInNeedC = true;
		}
		else if(optMatch<20>(tokens[i]))
		{
			if(cSelf)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			cSelf = true;
		}
		else if(optMatch<21>(tokens[i]))
		{
			if(assertF)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			assertF = true;
		}
		else if(optMatch<22>(tokens[i]))
		{
			if(recycleBQ)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			if(!customRecycle)
				return errorOption(CMD_OPT_ILLEGAL, tokens[i]);
			recycleBQ = true;
		}
		else if(optMatch<23>(tokens[i]))
		{
			if(cInNeedF)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			cInNeedF = true;
		}
		else if(optMatch<24>(tokens[i]))
		{
			if(checkII)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			checkII = true;
		}
		else if(optMatch<25>(tokens[i]))
		{
			if(pstt != PDR_STIMU_NONE)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			if(++i == n)
				return errorOption(CMD_OPT_MISSING);
			if(!myStrToUInt(tokens[i], stimuNum1))
				return errorOption(CMD_OPT_INVALID_UINT, tokens[i]);
			if(++i == n)
				return errorOption(CMD_OPT_MISSING);
			if(!myStrToUInt(tokens[i], stimuNum2))
				return errorOption(CMD_OPT_INVALID_UINT, tokens[i]);
			if(++i == n)
				return errorOption(CMD_OPT_MISSING);
			if(!myStrToUInt(tokens[i], stimuNum3))
				return errorOption(CMD_OPT_INVALID_UINT, tokens[i]);
			if(stimuNum2 == 0)
				{ cerr << "[Error] matchNum cannot be 0!" << endl; return CMD_EXEC_ERROR_EXT; }
			if(stimuNum1 < stimuNum2) {
				cerr << "[Error] backtrackNum (" << stimuNum1 << ") is smaller than "
				     << "matchNum (" << stimuNum2 << ")!" << endl; return CMD_EXEC_ERROR_EXT; }
			pstt = PDR_STIMU_LOCAL_INF;
		}
		else if(optMatch<26>(tokens[i]))
		{
			if(pstt != PDR_STIMU_NONE)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			if(++i == n)
				return errorOption(CMD_OPT_MISSING);
			if(!myStrToUInt(tokens[i], stimuNum1))
				return errorOption(CMD_OPT_INVALID_UINT, tokens[i]);
			if(++i == n)
				return errorOption(CMD_OPT_MISSING);
			if(!myStrToUInt(tokens[i], stimuNum2))
				return errorOption(CMD_OPT_INVALID_UINT, tokens[i]);
			if(++i == n)
				return errorOption(CMD_OPT_MISSING);
			if(!myStrToUInt(tokens[i], stimuNum3))
				return errorOption(CMD_OPT_INVALID_UINT, tokens[i]);
			if(stimuNum2 == 0)
				{ cerr << "[Error] matchNum cannot be 0!" << endl; return CMD_EXEC_ERROR_EXT; }
			if(stimuNum1 < stimuNum2) {
				cerr << "[Error] backtrackNum (" << stimuNum1 << ") is smaller than "
				     << "matchNum (" << stimuNum2 << ")!" << endl; return CMD_EXEC_ERROR_EXT; }
			pstt = PDR_STIMU_LOCAL_ALL;
		}
		else if(optMatch<27>(tokens[i]))
		{
			if(pstt != PDR_STIMU_NONE)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			if(++i == n)
				return errorOption(CMD_OPT_MISSING);
			if(!myStrToUInt(tokens[i], stimuNum1))
				return errorOption(CMD_OPT_INVALID_UINT, tokens[i]);
			if(++i == n)
				return errorOption(CMD_OPT_MISSING);
			if(!myStrToUInt(tokens[i], stimuNum2))
				return errorOption(CMD_OPT_INVALID_UINT, tokens[i]);
			if(++i == n)
				return errorOption(CMD_OPT_MISSING);
			if(!myStrToUInt(tokens[i], stimuNum3))
				return errorOption(CMD_OPT_INVALID_UINT, tokens[i]);
			if(stimuNum2 == 0)
				{ cerr << "[Error] matchNum cannot be 0!" << endl; return CMD_EXEC_ERROR_EXT; }
			if(stimuNum1 < stimuNum2) {
				cerr << "[Error] backtrackNum (" << stimuNum1 << ") is smaller than "
				     << "matchNum (" << stimuNum2 << ")!" << endl; return CMD_EXEC_ERROR_EXT; }
			pstt = PDR_STIMU_LOCAL_MIX;
		}
		else if(optMatch<28>(tokens[i]))
		{
			if(pstt != PDR_STIMU_NONE)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			if(++i == n)
				return errorOption(CMD_OPT_MISSING);
			if(!myStrToUInt(tokens[i], stimuNum1))
				return errorOption(CMD_OPT_INVALID_UINT, tokens[i]);
			if(++i == n)
				return errorOption(CMD_OPT_MISSING);
			if(!myStrToUInt(tokens[i], stimuNum2))
				return errorOption(CMD_OPT_INVALID_UINT, tokens[i]);
			if(++i == n)
				return errorOption(CMD_OPT_MISSING);
			if(!myStrToUInt(tokens[i], stimuNum3))
				return errorOption(CMD_OPT_INVALID_UINT, tokens[i]);
			if(stimuNum1 == 0)
				{ cerr << "[Error] observeNum cannot be 0!" << endl; return CMD_EXEC_ERROR_EXT; }
			if(stimuNum2 < 2)
				{ cerr << "[Error] matchNum cannot be less than 2!" << endl; return CMD_EXEC_ERROR_EXT; }
			pstt = PDR_STIMU_HALF;
		}
		else return errorOption(CMD_OPT_ILLEGAL, tokens[i]);
	if(!checkNtk()) return CMD_EXEC_ERROR_INT;
	SafetyChecker* checker = getChecker<PdrChecker>(aigNtk, outputIdx, trace, timeout, maxFrame, recycleNum, stats,
	                                                psit, port, pobt, pdt, ppt, pgt,
	                                                rInf, cInNeedC, cSelf, assertF, recycleBQ, cInNeedF,
	                                                satQL, verbosity, checkII,
	                                                pstt, stimuNum1, stimuNum2, stimuNum3);
	if(checker == 0) return CMD_EXEC_ERROR_INT;
	checker->Check(); delete checker; return CMD_EXEC_DONE;
}

const char*
PdrCheckCmd::getUsageStr()const
{
	return "<(unsigned outputIdx)> [-Trace]\n"
	       "[-TImeout (unsigned timeout)]\n"
           "[-Max (unsigned maxFrame)]\n"
	       "[<-RECycle (unsigned recycleNum)> [-CAlled]]\n"
           "[-EVent | -Backward | -INTernal]\n"
	       "[-NEEDCone] [-NEEDFrame]\n"
	       "[<-ACtivity | -Decay> [-REVerse]] [-SElf]\n"
	       "[-Push | -NOPush] [-Queue]\n"
	       "[-APPROXGen | -NOGen]\n"
	       "[-EAger] [-INFinite] [-ASsert]\n"
	       "[-STat (\"atsgprcx\")] [-Verbose (\"aogpbtcimfx\")]\n"
	       "[<<-LOCALInf | -LOCALAll | -LOCALMix> ((unsigned) backtrackNum matchNum) |\n"
	       " -HALF ((unsigned) observeNum matchNum)> (unsigned satLimit)]\n"
	       "[-CHeck]\n";
}

const char*
PdrCheckCmd::getHelpStr()const
{
	return "Check safety property of AIG network using property directed reachability\n";
}

/*========================================================================
	CHEck SAfety PBc <(unsigned outputIdx)> [-TRace]
	                 [-TImeout (unsigned timeout)]
	                 [-Max (unsigned maxFrame)]
	                 <<-At | -Upto> [-Block] | -Increase>
	                 [-Stat ("atbif")] [-Verbose]
--------------------------------------------------------------------------
	0: -TRace,     3
	1: -Max,       2
	2: -Stat,      2
	3: -TImeout,   3
	4: -Verbose,   2
	5: -At,        2
	6: -Upto,      2
	7: -Block,     2
	8: -Increase,  2
========================================================================*/

CmdExecStatus
PbcCheckCmd::exec(char* options)const
{
	PureStrList tokens = breakToTokens(options);
	if(tokens.size() == 0)
		return errorOption(CMD_OPT_MISSING);
	size_t outputIdx;
	if(!myStrToUInt(tokens[0], outputIdx))
		return errorOption(CMD_OPT_INVALID_UINT, tokens[0]);

	bool trace = false;
	size_t maxFrame = 100;
	bool customMax = false;

	size_t timeout = 0;
	bool customTime = false;

	PbcRchType prt = PBC_RCH_NONE;
	bool blockState = false;

	bool statON = false;
	Array<bool> stat(PBC_STAT_TOTAL);
	for(unsigned i = 0; i < PBC_STAT_TOTAL; ++i)
		stat[i] = false;
	bool verbose = false;

	for(size_t i = 1, n = tokens.size(); i < n; ++i)
		if(optMatch<0>(tokens[i]))
		{
			if(trace)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			trace = true;
		}
		else if(optMatch<1>(tokens[i]))
		{
			if(customMax)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			if(++i == n)
				return errorOption(CMD_OPT_MISSING);
			if(!myStrToUInt(tokens[i], maxFrame))
				return errorOption(CMD_OPT_INVALID_UINT, tokens[i]);
			customMax = true;
		}
		else if(optMatch<2>(tokens[i]))
		{
			if(statON)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			if(++i == n)
				return errorOption(CMD_OPT_MISSING);
			char buffer[2] = { 0, 0 };
			for(const char* tmp = tokens[i]; *tmp != 0; ++tmp)
			{
				PbcStatType pstt;
				switch(*tmp)
				{
					case 'a' : pstt = PBC_STAT_ALL;       break;
					case 't' : pstt = PBC_STAT_TERSIM;    break;
					case 'b' : pstt = PBC_STAT_BLOCK_SAT; break;
					case 'i' : pstt = PBC_STAT_INDUC_SAT; break;
					case 'f' : pstt = PBC_STAT_FIXED_SAT; break;
					default  : pstt = PBC_STAT_ERROR;     break;
				}
				switch(pstt)
				{
					case PBC_STAT_ALL:
						for(unsigned i = 0; i < PBC_STAT_TOTAL; ++i)
							if(stat[i])
								{ buffer[0] = *tmp; return errorOption(CMD_OPT_EXTRA, buffer); }
							else stat[i] = true;
						break;

					case PBC_STAT_ERROR:
						buffer[0] = *tmp;
						return errorOption(CMD_OPT_ILLEGAL, buffer);

					default:
						if(stat[pstt])
							{ buffer[0] = *tmp; return errorOption(CMD_OPT_EXTRA, buffer); }
						else stat[pstt] = true;
				}
			}
			statON = true;
		}
		else if(optMatch<3>(tokens[i]))
		{
			if(customTime)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			if(++i == n)
				return errorOption(CMD_OPT_MISSING);
			if(!myStrToUInt(tokens[i], timeout))
				return errorOption(CMD_OPT_INVALID_UINT, tokens[i]);
			customTime = true;
		}
		else if(optMatch<4>(tokens[i]))
		{
			if(verbose)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			verbose = true;
		}
		else if(optMatch<5>(tokens[i]))
		{
			if(prt != PBC_RCH_NONE)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			prt = PBC_RCH_AT;
		}
		else if(optMatch<6>(tokens[i]))
		{
			if(prt != PBC_RCH_NONE)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			prt = PBC_RCH_UPTO;
		}
		else if(optMatch<7>(tokens[i]))
		{
			if(prt != PBC_RCH_AT || prt != PBC_RCH_UPTO)
				return errorOption(CMD_OPT_ILLEGAL, tokens[i]);
			if(blockState)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			blockState = true;
		}
		else if(optMatch<8>(tokens[i]))
		{
			if(prt != PBC_RCH_NONE)
				return errorOption(CMD_OPT_EXTRA, tokens[i]);
			prt = PBC_RCH_INCREASE;
		}
		else return errorOption(CMD_OPT_ILLEGAL, tokens[i]);
	if(!checkNtk()) return CMD_EXEC_ERROR_INT;
	SafetyChecker* checker;
	switch(prt)
	{
		case PBC_RCH_NONE:
			return errorOption(CMD_OPT_MISSING);

		case PBC_RCH_AT:
			checker = getChecker<PbcAChecker>(aigNtk, outputIdx, trace, timeout, maxFrame,
	                                          stat, blockState, verbose); break;
		case PBC_RCH_UPTO:
			checker = getChecker<PbcUChecker>(aigNtk, outputIdx, trace, timeout, maxFrame,
	                                          stat, blockState, verbose); break;
		case PBC_RCH_INCREASE:
			checker = getChecker<PbcIChecker>(aigNtk, outputIdx, trace, timeout, maxFrame,
	                                          stat, verbose); break;
	}
	if(checker == 0) return CMD_EXEC_ERROR_INT;
	checker->Check(); delete checker; return CMD_EXEC_DONE;
}

const char*
PbcCheckCmd::getUsageStr()const
{
	return "<(unsigned outputIdx)> [-Trace]\n"
	       "[-TImeout (unsigned timeout)]\n"
           "[-Max (unsigned maxFrame)]\n"
	       "<<-At | -Upto> [-Block] | -Increase>\n"
	       "[-Stat (\"atbif\")] [-Verbose]\n";
}

const char*
PbcCheckCmd::getHelpStr()const
{
	return "Check safety property of AIG network using property directed bounded model checking\n";
}

}
