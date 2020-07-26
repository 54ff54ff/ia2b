/*========================================================================\
|: [Filename] pdrChecker.cpp                                             :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Implement the PDR checker                                  :|
<------------------------------------------------------------------------*/

#include <queue>
#include <sstream>
#include "pdrChecker.h"
#include "condStream.h"
#include "alg.h"
#include "cmdCharDef.h"
using namespace std;

namespace _54ff
{

const string pdrStatStr[PDR_STAT_TOTAL] =
{
	"Ternary Simulation",
	"SAT Query",
	"UNSAT Generalization",
	"Cube Propagation",
	"Solver Recycling",
	"Cube Subsumption",
	"Stimulation if performed"
};

const string pdrVerboseStr[PDR_VERBOSE_TOTAL] =
{
	"Obligation Checking",
	"UNSAT Generalization",
	"Cube Propagation",
	"Cube Blocking",
	"Ternary Simulation",
	"Cube Distribution",
	"Infinity Refinement if performed",
	"Miscellaneous Things",
	"Inductive Invariant if found",
	"Stimulation if performed"
};

void
PdrTerSimStat::printStat()const
{
	cout << RepeatChar('=', 72) << endl;
	if(getNum() == 0)
		cout << "No ternary simulation!" << endl;
	else
	{
		streamsize ss = cout.precision();
		cout << "Number of ternary simulation   = " << getNum()                     << endl
		     << fixed << setprecision(3)
		     << "Average literal count (TerSim) = " << double(getNum(1)) / getNum() << endl
		     << "Average removal count (TerSim) = " << double(getNum(2)) / getNum() << endl
		     << "Total runtime on TerSim        = " << getTotalTime() << " s"       << endl
		     << setprecision(ss)
		     << "Average runtime on TerSim      = " << getAveTime()   << " s"       << endl;
	}
}

void
PdrSatStat::printStat(bool useApprox)const
{
	cout << RepeatChar('=', 72) << endl;
	if(getNum(0) + getNum(1) + getNum(2) == 0)
		cout << "No SAT query!" << endl;
	else
	{
		cout << "The approximate SAT query is " << (useApprox ? "" : "not ") << "activated!" << endl;
		streamsize ss = cout.precision();
		cout << fixed << setprecision(3)
		     << "  SAT number      = " << getNum(1)                                 << endl
		     << "  SAT runtime     = " << getTotalTime(1)                   << " s" << endl
		     << "UNSAT number      = " << getNum(0)                                 << endl
		     << "UNSAT runtime     = " << getTotalTime(0)                   << " s" << endl
		     << "Abort number      = " << getNum(2)                                 << endl
		     << "Abort runtime     = " << getTotalTime(2)                   << " s" << endl
		     << "Total number      = " << getNum(1) + getNum(0) + getNum(2)         << endl
		     << "Total runtime     = " << getTotalTime(1) +
		                                  getTotalTime(0) +
		                                  getTotalTime(2)                   << " s" << endl
		     << "Last solving time = " << getTotalTime(3)                   << " s" << endl;
		cout << "Min   SAT decision = "; if(getNum(1) != 0) cout << minSAT_D;   else cout << "None"; cout << endl;
		cout << "Max UNSAT decision = "; if(getNum(0) != 0) cout << maxUNSAT_D; else cout << "None"; cout << endl;
		cout << "Max Abort decision = "; if(getNum(2) != 0) cout << maxAbort_D; else cout << "None"; cout << endl;
		cout << "Average number of decision per query = " << double(getNum(3)) / (getNum(1) + getNum(0) + getNum(2)) << endl
		     << "Average number of conflict per query = " << double(getNum(4)) / (getNum(1) + getNum(0) + getNum(2)) << endl
		     << setprecision(ss);
	}
}

void
PdrUnsatGenStat::printStat()const
{
	cout << RepeatChar('=', 72) << endl;
	if(getNum() == 0)
		cout << "No UNSAT generalization!" << endl;
	else
	{
		streamsize ss = cout.precision();
		cout << fixed << setprecision(3)
		     << "Number of UNSAT generalization   = " << getNum()                                  << endl
		     << "Runtime on remove stage          = " << getTotalTime(0)                   << " s" << endl
		     << "Runtime on push stage            = " << getTotalTime(1)                   << " s" << endl
		     << "Runtime on UNSAT Gen             = " << getTotalTime(0) + getTotalTime(1) << " s" << endl
		     << "Average remove count (UNSAT Gen) = " << double(getNum(1)) / getNum()              << endl
		     << setprecision(ss);
	}
}

void
PdrPropStat::printStat()const
{
	cout << RepeatChar('=', 72) << endl;
	if(getNum() == 0)
		cout << "No cube propagation!" << endl;
	else
	{
		streamsize ss = cout.precision();
		cout << fixed << setprecision(3)
		     << "Number of cube propagation  = " << getNum()               << endl
		     << "Runtime on cube propagation = " << getTotalTime() << " s" << endl
		     << "Number of cubes tried to be propagated            = " << getNum(1) << endl
		     << "Number of cubes propagated to further frame       = " << getNum(2) << endl
		     << "Number of cubes added to Inf by eager propagation = " << getNum(3) << endl
		     << "Number of literals removed in Inf                 = " << getNum(4) << endl
		     << setprecision(ss);
	}
}

void
PdrRecycleStat::printStat()const
{
	cout << RepeatChar('=', 72) << endl;
	if(getNum() == 0)
		cout << "No solver recycling!" << endl;
	else
	{
		streamsize ss = cout.precision();
		cout << fixed << setprecision(3)
		     << "Number of solver recycling  = " << getNum()               << endl
		     << "Max number at recycling     = " << maxRecNum              << endl
		     << "Runtime on solver recycling = " << getTotalTime() << " s" << endl
		     << setprecision(ss);
	}
}

void
PdrCubeStat::printStat()const
{
	streamsize ss = cout.precision();
	cout << RepeatChar('=', 72) << endl
	     << fixed << setprecision(3)
	     << "Number of subsumption checking  = " << getNum()                               << endl
	     << "Runtime on subsumption checking = " << getTotalTime() << " s (Approximation)" << endl
	     << "     Subsumption when adding proof obligation   = " << getNum(1)   << endl
	     << "Self subsumption when adding proof obligation   = " << getNum(2)   << endl
	     << "     Subsumption when blocking proof obligation = " << getNum(3)   << endl
	     << "     Subsumption when adding blocked cube       = " << getNum(4)   << endl
	     << "Self subsumption when adding blocked cube       = " << getNum(5)   << endl
	     << "Max number of proof obligation in one timeframe = " << maxBadNum   << endl
	     << "Max number of proof tree in one recBlockCube    = " << maxTreeSize << endl
	     << setprecision(1)
	     << "Total number of cubes added to frame Inf   = " << getNum(7)                     << endl;
	cout << "Average length of cubes added to frame Inf = ";
	if(getNum(7) != 0) cout << double(getNum(6)) / getNum(7); else cout << "None";      cout << endl;
	cout << "Maximum length of cubes added to frame Inf = ";
	if(getNum(7) != 0) cout << maxInfClsLen;                  else cout << "None";      cout << endl;
	cout << setprecision(ss);
}

void
PdrStimuStat::printStat()const
{
	cout << RepeatChar('=', 72) << endl;
	if(getNum(0) + getNum(1) + getNum(2) == 0)
		cout << "No stimulation!" << endl;
	else
	{
		streamsize ss = cout.precision();
		cout << fixed << setprecision(3)
		     << "Runtime on stimulation = " << getTotalTime() << " s" << endl
		     << "#PASS = " << getNum(0) << ", #FAIL = " << getNum(1) << ", #ABORT = " << getNum(2)
		     << ", #Total = " << getNum(0) + getNum(1) + getNum(2) << endl
		     << "Number of clauses added to frame Inf = " << getNum(3) << endl
		     << setprecision(ss);
	}
}

ostream& operator<<(ostream& os, const PdrCube& c)
{
	os << "Size = " << c.getSize()
	   << ", Lit =";
	for(unsigned i = 0; i < c.getSize(); ++i)
	{
		os << " ";
		if(isInv(c.getLit(i))) os << "!";
		os << getGateID(c.getLit(i));
	}
	return os;
}

PdrCube::PdrCube(const vector<AigGateLit>& litList, bool toSort, const PdrCube* prevCube, bool keepPrev)
{
	const unsigned s = litList.size();
	allocMem(s);
	initCount();
	setSize(s);
	initAbstract();
	for(unsigned i = 0; i < s; ++i)
		setLit(i, litList[i]), calAbstract(i);
	if(toSort)
		sort(uint32Ptr, uint32Ptr + s);
	for(unsigned i = 1; i < s; ++i)
		assert(getLit(i-1) < getLit(i));
	incCount();
	initMark();
	setPrevCube(keepPrev ? prevCube : 0);
	setBadDepth(prevCube == 0 ? 0 : prevCube->getBadDepth() + 1);
}

bool
PdrCube::operator<(const PdrCube& c)const
{
	for(unsigned i = 0; i < getSize(); ++i)
	{
		if(i == c.getSize())
			return false;
		if(getLit(i) != c.getLit(i))
			return getLit(i) < c.getLit(i);
	}
	assert(getSize() <= c.getSize());
	return getSize() < c.getSize();
}

bool
PdrCube::operator==(const PdrCube& c)const
{
	if(getAbs() != c.getAbs() ||
	   getSize() != c.getSize())
		return false;
	for(unsigned i = 0; i < getSize(); ++i)
		if(getLit(i) != c.getLit(i))
			return false;
	return true;
}

// If we always use the general function "selfSubsume", much more literals are needed to check
// Since the subsumption checking performs very very often, we need to lighten it as possible as we can

bool
PdrCube::subsumeTrivial(const PdrCube& c)const
{
	for(unsigned i = 0; i < getSize(); ++i)
	{
		bool match = false;
		for(unsigned j = 0; j < c.getSize(); ++j)
			if(getLit(i) == c.getLit(j))
				{ match = true; break; }
		if(!match) return false;
	}
	return true;
}

bool
PdrCube::subsumeComplexN(const PdrCube& c)const
{
	if((getAbs() & ~c.getAbs()) != 0 ||
	   getSize() > c.getSize())
		return false;
	for(unsigned i = 0, j = 0; i < getSize(); ++i, ++j)
	{
		for(; j < c.getSize(); ++j)
			if(getLit(i) == c.getLit(j))
				goto OK;
			else if(getLit(i) < c.getLit(j))
				return false;
		return false;
		OK: {}
	}
	return true;
}

bool
PdrCube::subsumeComplexB(const PdrCube& c)const
{
	if((getAbs() & ~c.getAbs()) != 0 ||
	   getSize() > c.getSize())
		return false;
	unsigned _lo = 0;
	for(unsigned i = 0; i < getSize(); ++i)
	{
		unsigned lo = _lo;
		unsigned hi = c.getSize();
		unsigned mid;
		while(lo < hi)
			if(int diff = c.getLit(mid=(lo+hi)/2) - getLit(i);
			   diff == 0) { _lo = mid + 1; goto OK; }
			else if(diff < 0)
				lo = mid + 1;
			else // diff > 0
				hi = mid;
		return false;
	OK: {}
	}
	return true;
}

/*
ERROR_GATELIT: Neither
UNDEF_GATELIT: Subsume
Other literal: Self Subsume, the literal can be removed from c
*/

AigGateLit
PdrCube::selfSubsumeTrivial(const PdrCube& c)const
{
	AigGateLit ret = UNDEF_GATELIT;
	for(unsigned i = 0; i < getSize(); ++i)
	{
		bool match = false;
		for(unsigned j = 0; j < c.getSize(); ++j)
			if(getLit(i) == c.getLit(j))
				{ match = true; break; }
			else if(getGateID(getLit(i)) == getGateID(c.getLit(j)) &&
			        ret == UNDEF_GATELIT)
				{ ret = c.getLit(j); match = true; break; }
		if(!match) return ERROR_GATELIT;
	}
	return ret;
}

AigGateLit
PdrCube::selfSubsumeComplex(const PdrCube& c)const
{
	if((getAbs() & ~c.getAbs()) != 0 ||
	   getSize() > c.getSize())
		return ERROR_GATELIT;
	AigGateLit ret = UNDEF_GATELIT;
	for(unsigned i = 0, j = 0; i < getSize(); ++i, ++j)
	{
		for(; j < c.getSize(); ++j)
			if(getLit(i) == c.getLit(j))
				goto OK;
			else if(getGateID(getLit(i)) <= getGateID(c.getLit(j)))
			{
				if(getGateID(getLit(i)) == getGateID(c.getLit(j)) &&
				   ret == UNDEF_GATELIT)
					{ ret = c.getLit(j); goto OK; }
				return ERROR_GATELIT;
			}
		return ERROR_GATELIT;
		OK: {}
	}
	return ret;
}

PdrChecker* getDefaultPdr(AigNtk* ntk)
{
	constexpr size_t fakeOutputIdx = 0;
	constexpr bool   noTrace       = false;
	constexpr size_t noTimeout     = 0;
	constexpr size_t noMaxFrame    = MAX_SIZE_T;
	constexpr size_t noStat        = 0;
	constexpr size_t noSatLimit    = 0;
	constexpr size_t noOblLimit    = 0;
	constexpr size_t vbsOff        = 0;
	constexpr bool   noCheckII     = false;

	constexpr PdrClsStimuType noClsStimu = PDR_CLS_STIMU_NONE;
	constexpr PdrOblStimuType noOblStimu = PDR_OBL_STIMU_NONE;
	constexpr PdrShareType    noShare    = PDR_SHARE_NONE;
	constexpr size_t          dummy      = 0;

	constexpr size_t     recycleNum         = 300;
	constexpr PdrSimType simType            = PDR_SIM_FORWARD_EVENT;
	constexpr PdrOrdType ordType            = PDR_ORD_INDEX;
	constexpr PdrOblType oblType            = PDR_OBL_PUSH;
	constexpr PdrDeqType deqType            = PDR_DEQ_STACK;
	constexpr PdrPrpType prpType            = PDR_PRP_NORMAL;
	constexpr PdrGenType genType            = PDR_GEN_NORMAL;
	constexpr bool       toRefineInf        = false;
	constexpr bool       convertInNeedCone  = true;
	constexpr bool       convertInNeedFrame = true;
	constexpr bool       checkSelf          = false;
	constexpr bool       assertFrame        = false;
	constexpr bool       recycleByQuery     = true;
	constexpr bool       lazyProp           = true;
	constexpr bool       sortByBadDepth     = false;

	return (new PdrChecker(ntk, fakeOutputIdx, noTrace, noTimeout, noMaxFrame, recycleNum, noStat,
	                       simType, ordType, oblType, deqType, prpType, genType,
	                       toRefineInf, convertInNeedCone, checkSelf, assertFrame, recycleByQuery, convertInNeedFrame, lazyProp, sortByBadDepth,
	                       noSatLimit, noOblLimit, vbsOff, noCheckII,
	                       noClsStimu, noShare, dummy, dummy, dummy,
	                       noOblStimu, noShare, dummy, dummy));
}

PdrChecker::PdrChecker(AigNtk* ntkToCheck, size_t outputIdx, bool _trace, size_t timeout, size_t maxF, size_t recycleN, size_t stats,
                       PdrSimType simT, PdrOrdType ordT, PdrOblType oblT, PdrDeqType deqT, PdrPrpType prpT, PdrGenType genT,
                       bool rInf, bool cInNeedC, bool cSelf, bool assertF, bool recycleBQ, bool cInNeedF, bool lazyP, bool sortByBD,
                       size_t satQL, size_t oblL, size_t _verbosity, bool checkII,
                       PdrClsStimuType clsStimuT, PdrShareType clsShareT, size_t clsStimuNum1, size_t clsStimuNum2, size_t clsStimuNum3,
                       PdrOblStimuType oblStimuT, PdrShareType oblShareT, size_t oblStimuNum1, size_t oblStimuNum2)
: SafetyBNChecker    (ntkToCheck, outputIdx, _trace, timeout)
, mainType           (PDR_MAIN_NORMAL)
, initType           (PDR_INIT_DEFAULT)
, curFrame           (0)
, maxFrame           (maxF)
, initState          ()
, targetCube         ()
, clsStimulator      (getClsStimulator(clsStimuT, clsShareT, isPdrStatON(stats, PDR_STAT_STIMU),
                                       clsStimuNum1, clsStimuNum2, clsStimuNum3))
, oblStimulator      (getOblStimulator(oblStimuT, oblShareT, isPdrStatON(stats, PDR_STAT_STIMU),
                                       oblStimuNum1, oblStimuNum2))
, satQueryTime       (0)
, unusedVarNum       (0)
, recycleNum         (recycleN)
, maxUNSAT_D         (0)
, solver             (ntk)
, terSimSup          (ntk)
, actInc             (1.0)

, terSimStat         (isPdrStatON(stats, PDR_STAT_TERSIM))
, satStat            (isPdrStatON(stats, PDR_STAT_SAT))
, unsatGenStat       (isPdrStatON(stats, PDR_STAT_GENERALIZE))
, propStat           (isPdrStatON(stats, PDR_STAT_PROPAGATE))
, recycleStat        (isPdrStatON(stats, PDR_STAT_RECYCLE))
, cubeStat           (isPdrStatON(stats, PDR_STAT_CUBE))

, simType            (simT)
, ordType            (ordT)
, oblType            (oblT)
, deqType            (deqT)
, prpType            (prpT)
, genType            (genT)
, toRefineInf        (rInf)
, convertInNeedCone  (cInNeedC)
, convertInNeedFrame (cInNeedF)
, checkSelf          (cSelf)
, assertFrame        (assertF)
, recycleByQuery     (recycleBQ)
, lazyProp           (lazyP)
, sortByBadDepth     (sortByBD)
, checkIndInv        (checkII)

, frameConverted     (0)
, minBlockFrame      (1)

, totalSatQuery      (0)
, satQueryLimit      (satQL)
, numObl             (0)
, numOblLimit        (oblL)
, verbosity          (_verbosity | getPdrVbsMask(PDR_VERBOSE_FRAME))
{
	sfcMsg << "Max Frame  : " << maxFrame << endl
	       << "Method     : Property directed reachability" << endl
	       << "Detail     : ";
	sfcMsg << "Number of total SAT query limit = ";
	if(satQueryLimit == 0) sfcMsg << "Infinity"; else sfcMsg << satQueryLimit;
	sfcMsg << endl;
	sfcMsg << "             Number of generated obligation limit = ";
	if(numOblLimit == 0) sfcMsg << "Infinity"; else sfcMsg << numOblLimit;
	sfcMsg << endl;
	sfcMsg << "             Number of " << (recycleByQuery ? "SAT query times" : "unused variable") << " to recycle the solver = ";
	if(recycleNum == 0) sfcMsg << "Infinity"; else sfcMsg << recycleNum;
	sfcMsg << endl;
	sfcMsg << "             ";
	switch(simType)
	{
		case PDR_SIM_FORWARD_NORMAL    : sfcMsg << "Forward ternary simulation, Normal mode";              break;
		case PDR_SIM_FORWARD_EVENT     : sfcMsg << "Forward ternary simulation, Event-driven mode";        break;
		case PDR_SIM_BACKWARD_NORMAL   : sfcMsg << "BackWard SAT generalization, Only latch variable";     break;
		case PDR_SIM_BACKWARD_INTERNAL : sfcMsg << "BackWard SAT generalization, Involve internal signal"; break;
	}
	sfcMsg << endl;
	sfcMsg << "             ";
	switch(ordType)
	{
		case PDR_ORD_INDEX            : sfcMsg << "Static order, Follow index";                     break;
		case PDR_ORD_ACTIVITY         : sfcMsg << "Dynamic order, Follow activity";                 break;
		case PDR_ORD_ACTIVITY_REVERSE : sfcMsg << "Dynamic order, Follow reverse activity";         break;
		case PDR_ORD_DECAY            : sfcMsg << "Dynamic order, Follow decayed activity";         break;
		case PDR_ORD_DECAY_REVERSE    : sfcMsg << "Dynamic order, Follow reverse decayed activity"; break;
	}
	sfcMsg << endl;
	sfcMsg << "             ";
	switch(oblType)
	{
		case PDR_OBL_NORMAL : sfcMsg << "Find new proof obligation at each new timeframe";     break;
		case PDR_OBL_PUSH   : sfcMsg << "Preserve all proof obligations during the procedure"; break;
		case PDR_OBL_IGNORE : sfcMsg << "Never push proof obligation to further frame";        break;
	}
	sfcMsg << endl;
	sfcMsg << "             ";
	switch(deqType)
	{
		case PDR_DEQ_STACK : sfcMsg << "Apply stack-like behavior when getting bad cubes"; break;
		case PDR_DEQ_QUEUE : sfcMsg << "Apply queue-like behavior when getting bad cubes"; break;
	}
	sfcMsg << endl;
	sfcMsg << "             ";
	switch(prpType)
	{
		case PDR_PRP_NORMAL : sfcMsg << "Propagate blocked cubes up to the currently maximum timeframe";  break;
		case PDR_PRP_EAGER  : sfcMsg << "Propagate blocked cubes out of the currently maximum timeframe"; break;
	}
	sfcMsg << endl;
	sfcMsg << "             ";
	switch(genType)
	{
		case PDR_GEN_NORMAL : sfcMsg << "Try to remove a literal once a time during UNSAT generalization";                    break;
		case PDR_GEN_APPROX : sfcMsg << "Try to remove a literal once a time during UNSAT generalization by approximate SAT"; break;
		case PDR_GEN_IGNORE : sfcMsg << "Do not try to remove any literal during UNSAT generalization";                       break;
	}
	sfcMsg << endl;
	if(toRefineInf)
		sfcMsg << "             Try to further refine timeframe Inf" << endl;
	if(convertInNeedCone)
		sfcMsg << "             Convert CNF formula of latch only if needed" << endl;
	if(convertInNeedFrame)
		sfcMsg << "             Convert the clauses of frame only if needed" << endl;
	if(checkSelf)
		sfcMsg << "             Check self subsumption on cubes as well" << endl;
	if(assertFrame)
		sfcMsg << "             Assert the activation variables for unused frames during SAT query" << endl;
	if(lazyProp)
		sfcMsg << "             Propagate blocked cubes from the currently minimum processing frame" << endl;
	if(sortByBadDepth)
		sfcMsg << "             Sort by the bad depth of proof obligation during queueing" << endl;
	size_t numActive = 0;
	for(unsigned i = 0; i < PDR_STAT_TOTAL; ++i)
		if(isPdrStatON(stats, PdrStatType(i)))
			sfcMsg << "             "
			       << (numActive++ == 0 ? "Toggle statistics output for "
			                            : "                             ")
			       << "- " << pdrStatStr[i] << endl;
	numActive = 0;
	for(unsigned i = 0; i < PDR_VERBOSE_TOTAL; ++i)
		if(isVerboseON(PdrVerboseType(i)))
			sfcMsg << "             "
			       << (numActive++ == 0 ? "Toggle verbose output for "
			                            : "                          ")
			       << "- " << pdrVerboseStr[i] << endl;
	if(checkIndInv)
		sfcMsg << "             ** Check the inductive invariant at the end if found" << endl;

	if(clsStimuT != PDR_CLS_STIMU_NONE)
	{
		sfcMsg << "Stimulate  : ";
		switch(clsStimuT)
		{
			case PDR_CLS_STIMU_LOCAL_INF  : sfcMsg << "Observe only part of the clauses, focus on inifinte frame" << endl
			                                       << "             "
			                                       << "backtrackNum = " << clsStimuNum1
			                                       << ", matchNum = "   << clsStimuNum2; break;
			case PDR_CLS_STIMU_LOCAL_ALL  : sfcMsg << "Observe only part of the clauses, focus on all frames" << endl
			                                       << "             "
			                                       << "backtrackNum = " << clsStimuNum1
			                                       << ", matchNum = "   << clsStimuNum2; break;
			case PDR_CLS_STIMU_LOCAL_GOLD : sfcMsg << "Observe only part of the clauses, use golden parameter" << endl
			                                       << "             "
			                                       << "(backtrackNum, matchNum) = (10, 2) for non-inf and (20, 1) for inf"; break;
			case PDR_CLS_STIMU_LOCAL_MIX  : sfcMsg << "Observe only part of the clauses, focus on mixed frames" << endl
			                                       << "             "
			                                       << "backtrackNum = " << clsStimuNum1
			                                       << ", matchNum = "   << clsStimuNum2; break;
			case PDR_CLS_STIMU_HALF       : sfcMsg << "Observe neighboring region of each clause, focus on infinite frame" << endl
			                                       << "             "
			                                       << "observeNum = "   << clsStimuNum1
			                                       << ", matchNum = "   << clsStimuNum2; break;
			case PDR_CLS_STIMU_NONE       : assert(false);
		}
		sfcMsg << ", satLimit = "; if(clsStimuNum3 == 0) sfcMsg << "Infinity"; else sfcMsg << clsStimuNum3; sfcMsg << endl;
		sfcMsg << "             ";
		switch(clsShareT)
		{
			case PDR_SHARE_NONE : sfcMsg << "Reuse nothing, start from the beginning"; break;
			case PDR_SHARE_INF  : sfcMsg << "Reuse the infinite frame";                break;
			case PDR_SHARE_ALL  : sfcMsg << "Reuse all the frames";                    break;
		}
		sfcMsg << endl;
	}
	else assert(clsShareT == PDR_SHARE_NONE);

	if(oblStimuT != PDR_OBL_STIMU_NONE)
	{
		sfcMsg << "Stimulate  : ";
		switch(oblStimuT)
		{
			case PDR_OBL_STIMU_ALL   : sfcMsg << "Consider all the proof obligations";                break;
			case PDR_OBL_STIMU_DEPTH : sfcMsg << "Consider the proof obligations by their bad depth"; break;
			case PDR_OBL_STIMU_NONE  : assert(false);
		}
		sfcMsg << ", oblThreshold = " << oblStimuNum1 << ", satLimit = ";
		if(oblStimuNum2 == 0) sfcMsg << "Infinity"; else sfcMsg << oblStimuNum2 << endl;
		sfcMsg << "             ";
		switch(oblShareT)
		{
			case PDR_SHARE_NONE : sfcMsg << "Reuse nothing, start from the beginning"; break;
			case PDR_SHARE_INF  : sfcMsg << "Reuse the infinite frame";                break;
			case PDR_SHARE_ALL  : sfcMsg << "Reuse all the frames";                    break;
		}
		sfcMsg << endl;
	}
	else assert(oblShareT == PDR_SHARE_NONE);

	/* Prepare for checking */
	genCube.reserve(ntk->getLatchNum());
	switch(simType)
	{
		case PDR_SIM_FORWARD_NORMAL:
			terSimSup.initValue();
			terSimSup.setConst0();
			terSimSup.initFanOut();
			terSimSup.reserveAndNum();
			break;

		case PDR_SIM_FORWARD_EVENT:
			terSimSup.initValue();
			terSimSup.setConst0();
			terSimSup.initFanOut();
			terSimSup.initLevel(true);
			terSimSup.initEventList();
			terSimSup.reserveAndNum();
			break;

		case PDR_SIM_BACKWARD_NORMAL:
			terSimSup.initLevel(false);
			terSimSup.initEventList();
			terSimSup.initInflu();
			terSimSup.influOrder();
			break;

		case PDR_SIM_BACKWARD_INTERNAL:
			throw CheckerErr("Backward ternary simulation with internal signal is not implemented yet!");
			break;
	}

	if(isOrdDynamic())
	{
		const size_t M = ntk->getMaxGateNum();
		activity.init(M);
		for(size_t i = 0; i < M; ++i)
			activity[i] = 0.0;
	}

	if(oblType == PDR_OBL_PUSH)
		badDequeVec.emplace_back();

	if(checkSelf)
		throw CheckerErr("Self subsumption checking is not implemented yet!");

	/* Convert all circuit for a timeframe */
	convertCNF();

	/* Prepare for the initial state and infinite frame */
	frame.emplace_back();
	newFrame();
// TODO, fix this part
addInitState();
	if(convertInNeedFrame)
		frameConverted = 1;
//	else addInitState();

	#ifdef UsePatternCheckSAT
	simValue.init(ntk->getMaxGateNum());
	simValue[0] =  0;
	patIdx      = 64;
	matchTry    =  0;
	matchFrame  =  0;
	matchSAT    =  0;
	matchTime   =  0;
	simResult   = false;
	#endif

	#ifdef CheckOblCommonPart
	numInf = 0;
	isDC.init(ntk->getMaxGateNum());
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
		isDC[ntk->getLatchID(i)] = true;
	#endif
}

PdrChecker::~PdrChecker()
{
	printStats();
	if(clsStimulator != 0)
		delete clsStimulator;
	if(oblStimulator != 0)
		delete oblStimulator;
}

auto
PdrChecker::getClsStimulator(PdrClsStimuType clsStimuT, PdrShareType shareT, bool statON,
                             size_t stimuNum1, size_t stimuNum2, size_t stimuNum3) -> PdrClsStimulator*
{
	switch(clsStimuT)
	{
		case PDR_CLS_STIMU_LOCAL_INF  : return (new PdrClsStimulatorLocalInfAll(this, shareT, statON, stimuNum1, stimuNum2, stimuNum3, true));
		case PDR_CLS_STIMU_LOCAL_ALL  : return (new PdrClsStimulatorLocalInfAll(this, shareT, statON, stimuNum1, stimuNum2, stimuNum3, false));
		case PDR_CLS_STIMU_LOCAL_GOLD : return (new PdrClsStimulatorLocalInfAll(this, shareT, statON, stimuNum3));
		case PDR_CLS_STIMU_LOCAL_MIX  : return (new PdrClsStimulatorLocalMix   (this, shareT, statON, stimuNum1, stimuNum2, stimuNum3));
		case PDR_CLS_STIMU_HALF       : return (new PdrClsStimulatorHalf       (this, shareT, statON, stimuNum1, stimuNum2, stimuNum3));

		default                       : assert(false);
		case PDR_CLS_STIMU_NONE       : return 0;
	}
}

auto
PdrChecker::getOblStimulator(PdrOblStimuType oblStimuT, PdrShareType shareT, bool statON,
                             size_t stimuNum1, size_t stimuNum2) -> PdrOblStimulator*
{
	switch(oblStimuT)
	{
		case PDR_OBL_STIMU_ALL   : return (new PdrOblStimulatorAll  (this, shareT, statON, stimuNum1, stimuNum2));
		case PDR_OBL_STIMU_DEPTH : return (new PdrOblStimulatorDepth(this, shareT, statON, stimuNum1, stimuNum2));

		default                  : assert(false);
		case PDR_OBL_STIMU_NONE  : return 0;
	}
}

void
PdrChecker::mergeInf(const vector<PdrCube>& indSet, bool pushAtBack, StatPtr<PdrStimuStat>& stimuStat)
{
	vector<PdrCube>  tmp = indSet;
	vector<PdrCube>& inf = frame.back();
	const size_t infIdx = frame.size() - 1;
	size_t s = 0;
	for(size_t i = 0, n = tmp.size(); i < n; ++i)
		if(!checkIsSubsumed(tmp[i], infIdx, infIdx))
		{
			checkSubsumeOthers(tmp[i], 1, infIdx);
			if(s < i)
				tmp[s++] = move(tmp[i]);
			else { assert(s == i); s += 1; }
		}

	if(isVerboseON(PDR_VERBOSE_STIMU))
		cout << ", #original clause = " << tmp.size() << ", #added clause = " << s << endl;
	if(stimuStat.isON())
		stimuStat->incInfClsNum(s);
	tmp.resize(s);

	for(const PdrCube& c: tmp)
		addBlockedCubeFrame(FRAME_INF, c);
	if(cubeStat.isON())
		for(const PdrCube& c: tmp)
			cubeStat->incInfCubeNum(),
			cubeStat->incInfLitNum(c.getSize()),
			cubeStat->checkMaxCLen(c.getSize());

	if(pushAtBack)
		for(PdrCube& c: tmp)
			inf.push_back(move(c));
	else
	{
		for(PdrCube& c: inf)
			tmp.push_back(move(c));
		inf.swap(tmp);
	}
}

void
PdrChecker::mergeFrames(const vector<vector<PdrCube>>& mergedFrame, bool pushAtBack, StatPtr<PdrStimuStat>& stimuStat)
{
	assert(mergedFrame.size() >= 2);
	assert(mergedFrame.size() <= frame.size());
	assert(mergedFrame[0].size() == 0);

	vector<vector<PdrCube>> tmpMerged = mergedFrame;
	while(tmpMerged.size() < frame.size())
		{ tmpMerged.push_back(move(tmpMerged.back())); tmpMerged[tmpMerged.size()-2].clear(); }
	const size_t infIdx = frame.size() - 1;
	for(size_t f = 1; f < infIdx; ++f)
	{
		vector<PdrCube>  tmp = mergedFrame[f];
		vector<PdrCube>& cur = frame[f];
		size_t s = 0;
		for(size_t i = 0, n = tmp.size(); i < n; ++i)
			if(!checkIsSubsumed(tmp[i], f, infIdx))
			{
				checkSubsumeOthers(tmp[i], 1, f);
				if(s < i)
					tmp[s++] = move(tmp[i]);
				else { assert(s == i); s += 1; }
			}

		if(isVerboseON(PDR_VERBOSE_STIMU))
			cout << "For frame " << f << ": #original clause = " << tmp.size() << ", #added clause = " << s << endl;
		tmp.resize(s);

		for(const PdrCube& c: tmp)
			addBlockedCubeFrame(f, c);

		if(pushAtBack)
			for(PdrCube& c: tmp)
				cur.push_back(move(c));
		else
		{
			for(PdrCube& c: cur)
				tmp.push_back(move(c));
			cur.swap(tmp);
		}
	}
	mergeInf(mergedFrame.back(), pushAtBack, stimuStat);
}

PdrChecker*
PdrChecker::cloneChecker()const
{
	constexpr size_t fakeOutputIdx = 0;
	constexpr bool   noTrace       = false;
	constexpr size_t noTimeout     = 0;
	constexpr size_t noMaxFrame    = MAX_SIZE_T;
	constexpr size_t noStat        = 0;
	constexpr size_t noSatLimit    = 0;
	constexpr size_t noOblLimit    = 0;
	constexpr size_t vbsOff        = 0;
	constexpr bool   noCheckII     = false;

	constexpr PdrClsStimuType noClsStimu = PDR_CLS_STIMU_NONE;
	constexpr PdrOblStimuType noOblStimu = PDR_OBL_STIMU_NONE;
	constexpr PdrShareType    noShare    = PDR_SHARE_NONE;
	constexpr size_t          dummy      = 0;

	PdrChecker* checker = new PdrChecker(ntk, fakeOutputIdx, noTrace, noTimeout, noMaxFrame, recycleNum, noStat,
	                                     simType, ordType, oblType, deqType, prpType, genType,
	                                     toRefineInf, convertInNeedCone, checkSelf, assertFrame, recycleByQuery, convertInNeedFrame, lazyProp, sortByBadDepth,
	                                     noSatLimit, noOblLimit, vbsOff, noCheckII,
	                                     noClsStimu, noShare, dummy, dummy, dummy,
	                                     noOblStimu, noShare, dummy, dummy);
	return checker;
}

void
PdrChecker::check()
{
	switch(checkInt())
	{
		case PDR_RESULT_SAT         : cout << "Observe a counter example at frame "         << curFrame << endl;
		                              printTrace();                                                              break;
		case PDR_RESULT_UNSAT       : cout << "Property proved at frame "                   << curFrame << endl;
		                              checkAndPrintIndInv();                                                     break;
		case PDR_RESULT_ABORT_FRAME : cout << "Cannot determinie the property up to frame " << maxFrame << endl; break;
		case PDR_RESULT_ABORT_RES   : cout << "Cannot determinie the property"                          << endl; break;

		default: assert(false);
	}
}

PdrResultType
PdrChecker::checkInt()
{
	switch(mainType)
	{
		case PDR_MAIN_NORMAL     : return checkIntNormal();
		case PDR_MAIN_ONE_BY_ONE : return checkIntOneByOne();
		default: assert(false);    return PDR_RESULT_ERROR;
	}
}

void
PdrChecker::startWithIndSet(const vector<PdrCube>& indSet)
{
	for(const vector<PdrCube>& f: frame)
		assert(f.empty());
	frame.back() = indSet;
	addBlockedCubeInf();
}

PdrResultType
PdrChecker::checkIntNormal()
{
	try
	{
		while(true)
		{
			if(oblType == PDR_OBL_PUSH)
				if(!recBlockCube(PdrTCube(FRAME_NULL, PdrCube())))
					return PDR_RESULT_SAT;

			if(PdrCube notPCube = getNotPCube();
			   notPCube.isNone())
			{
				//finish blocking
				if(curFrame == maxFrame)
					return PDR_RESULT_ABORT_FRAME;
				if(propBlockedCubes())
					return PDR_RESULT_UNSAT;
			}
			else if(!recBlockCube(PdrTCube(curFrame, notPCube)))
				return PDR_RESULT_SAT;
		}
	}
	catch(const CheckerBreak&) { return PDR_RESULT_ABORT_RES; }
}

PdrResultType
PdrChecker::checkIntOneByOne()
{
	for(size_t f = 0; true;)
	{
		assert(f <= maxFrame);
		curFrame = f;
		if(PdrCube notPCube = getNotPCube();
		   notPCube.isNone())
		{
			assert(f + 2 <= frame.size());
			if(f + 2 == frame.size())
			{
				if(f == maxFrame)
					return PDR_RESULT_ABORT_FRAME;
				if(propBlockedCubes())
					return PDR_RESULT_UNSAT;
			}
			else if(frame[curFrame].empty())
				return PDR_RESULT_UNSAT;
			f += 1;
		}
		else
		{
			setTargetCube(notPCube);
			if(PdrResultType r = checkIntNormal();
			   r != PDR_RESULT_UNSAT)
				return r;
			//TODO, try other schemes
			// 1. Just reuse the infinite frame
			// 2. Not to resue any result, treat them as separate problems
			resetTargetCube();
			size_t emptyFrame = 1;
			for(; !frame[emptyFrame].empty(); ++emptyFrame);
			pushToFrameInf(emptyFrame);
			if(oblType == PDR_OBL_PUSH)
			{
				//TODO
			}
			for(size_t i = 1; i < emptyFrame; ++i)
				frame[i].clear();
		}
	}
}

vector<AigGateID>
PdrChecker::genTarget(const PdrCube& c)const
{
	vector<AigGateID> target;
	target.reserve(c.getSize());
	for(unsigned i = 0; i < c.getSize(); ++i)
		target.push_back(ntk->getGate(getGateID(c.getLit(i)))->getFanIn0ID());
	return target;
}

bool
PdrChecker::recBlockCube(const PdrTCube& notPTCube)
{
	auto returnBy = [this](bool value) -> bool
	{
		if(cubeStat.isON())
			cubeStat->checkMaxTSize(oblTreeSize);
		printCurFrames(value ? "B " : "F ");
		return value;
	};

	assert(notPTCube.getFrame() == curFrame ||
	       (oblType == PDR_OBL_PUSH && notPTCube.getFrame() == FRAME_NULL));
	for(size_t i = 0, f = oblType == PDR_OBL_PUSH ? curFrame : curFrame + 1; i < f; ++i)
		assert(badDequeVec[i].empty());

	if(notPTCube.getFrame() != FRAME_NULL)
		checkThenPushObl(notPTCube.getFrame(), notPTCube.getCube());
	const size_t maxFrame = curFrame + 1;
	size_t checkFrame = curFrame;
	if(badDequeVec[checkFrame].empty())
		{ assert(oblType == PDR_OBL_PUSH); return true; }
	auto checkEmpty = [this, &checkFrame]()
	{
		if(deqType == PDR_DEQ_STACK) badDequeVec[checkFrame].pop_back();
		else                         badDequeVec[checkFrame].pop_front();
		if(badDequeVec[checkFrame].empty()) checkFrame += 1;
	};
	oblTreeSize = badDequeVec[curFrame].size();
	while(checkFrame < maxFrame)
	{
		assert(!badDequeVec[checkFrame].empty());
		if(checkFrame == 0)
			return returnBy(false);
		const PdrCube& badCube = deqType == PDR_DEQ_STACK ? badDequeVec[checkFrame].back()
		                                                  : badDequeVec[checkFrame].front();
		if(isVerboseON(PDR_VERBOSE_OBL))
			cout << RepeatChar('-', 36) << endl
			     << "Check: frame = " << checkFrame
			     << ", " << badCube << endl
			     << RepeatChar('-', 36) << endl;
		PdrTCube badTCube(checkFrame, badCube);
		if(lazyProp && minBlockFrame > checkFrame)
			minBlockFrame = checkFrame;
		if(size_t blockFrame = isBlocked(badTCube); blockFrame == FRAME_NULL)
		{
			assert(!isInitial(badCube));
			if(oblStimulator != 0)
				oblStimulator->checkCommonPart(badCube);
			PdrTCube newTCube = solveRelative(badTCube, EXTRACT);
			CheckBreakPdr(true);
			if(newTCube.getFrame() != FRAME_NULL)
			{
				//UNSAT, blocked
				newTCube = generalize(newTCube);
				assert(newTCube.getFrame() >= checkFrame);
				assert(newTCube.getFrame() < actVar.size() || newTCube.getFrame() == FRAME_INF);
				addBlockedCube(newTCube);
				if(oblType != PDR_OBL_IGNORE)
					if(newTCube.getFrame() != FRAME_INF)
						if(newTCube.getFrame() < curFrame || oblType == PDR_OBL_PUSH)
							checkThenPushObl(newTCube.getFrame()+1, badCube);
				checkEmpty();
			}
			else //SAT, not blocked
			{
				oblTreeSize += 1;
				checkThenPushObl(--checkFrame, newTCube.getCube());
				if(oblStimulator != 0)
					oblStimulator->stimulate();
			}
			checkRecycle();
		}
		else
		{
			if(oblType == PDR_OBL_PUSH && blockFrame != FRAME_INF)
				checkThenPushObl(blockFrame+1, badCube);
			checkEmpty();
		}
		CheckBreakPdr(true);
	}
	return returnBy(true);
}

size_t
PdrChecker::isBlocked(const PdrTCube& badTCube)const
{
	size_t f = badTCube.getFrame();
	const size_t maxF = frame.size();
	assert(f <= curFrame);
	if(cubeStat.isON())
		cubeStat->startTime();
	for(; f < maxF; ++f)
		for(const PdrCube& frameCube: frame[f])
			if(subsume(frameCube, badTCube.getCube()))
				goto DONE;
DONE:
	if(cubeStat.isON())
		cubeStat->finishTime();
	assert(f <= maxF);
	if(f != maxF && cubeStat.isON())
		cubeStat->incSubsumeBlockObl();
	switch(maxF - f)
	{
		case 0 : return isBlockedSAT(badTCube);
		case 1 : return FRAME_INF;
		default: return f;
	}
}

PdrTCube
PdrChecker::generalize(const PdrTCube& s)
{
	genCube.clear();
	const PdrCube& c = s.getCube();
	for(unsigned i = 0; i < c.getSize(); ++i)
		genCube.push_back(c.getLit(i));
	if(isOrdDynamic())
		sortGenCubeByAct();
	size_t newF = s.getFrame();
	for(size_t i = 0, n = genCube.size(); i < n; ++i)
		assert(solver->isConverted(getGateID(genCube[i]), 1));

	switch(initType)
	{
		case PDR_INIT_DEFAULT : generalizeDefault(newF); break;
		case PDR_INIT_CLAUSE  : generalizeClause (newF); break;
		case PDR_INIT_CUBE    : generalizeCube   (newF); break;
	}
	if(unsatGenStat.isON())
		unsatGenStat->incRemoveCount(size_t(c.getSize()) - genCube.size());

	PdrTCube newTCube(newF, PdrCube(genCube, isOrdDynamic()));
	if(isVerboseON(PDR_VERBOSE_GEN))
	{
		cout << RepeatChar('-', 36) << endl
		     << "UNSAT generalization: frame = ";
		if(newTCube.getFrame() == FRAME_INF) cout << "Inf"; else cout << newTCube.getFrame();
		cout << ", " << newTCube.getCube() << endl
		     << RepeatChar('-', 36) << endl;
	}
	return newTCube;
}

void
PdrChecker::generalizeDefault(size_t& newF)
{
	/* Calculate the number of positive literal */
	size_t numPos = 0;
	for(size_t i = 0, n = genCube.size(); i < n; ++i)
		if(!isInv(genCube[i]))
			numPos += 1;
	assert(numPos != 0);

	/* Remove a literal each time */
	if(unsatGenStat.isON())
		unsatGenStat->doOneTime(),
		unsatGenStat->startTime();

	if(genType != PDR_GEN_IGNORE)
	{
		for(size_t i = 0; i < genCube.size();)		
			if(isInv(genCube[i]) || numPos > 1)
			{
				#ifdef UsePatternCheckSAT
				checkPatForSat(newF - 1, i);
				#endif
				solver->clearAssump();
				const Var act = addCurNotState(genCube, i);
				activateFrame(newF-1); //it's fine even if newF is FRAME_INF
				addNextState(genCube, i);
				if(genType == PDR_GEN_APPROX ? !satSolveLimited() : !satSolve())
				{
					if(!isInv(genCube[i]))
						numPos -= 1;
					newF = findLowestActPlus1(newF-1);
					size_t s1 = 0;
					for(size_t j = 0; j < i; ++j)
						if(numPos == 1 && !isInv(genCube[j]))
							genCube[s1++] = genCube[j];
						else if(solver->inConflict(getGateID(genCube[j]), 1))
							genCube[s1++] = genCube[j];
						else if(!isInv(genCube[j]))
							numPos -= 1;
					size_t s2 = s1;
					for(size_t j = i + 1, n = genCube.size(); j < n; ++j)
						if(numPos == 1 && !isInv(genCube[j]))
							genCube[s2++] = genCube[j];
						else if(solver->inConflict(getGateID(genCube[j]), 1))
							genCube[s2++] = genCube[j];
						else if(!isInv(genCube[j]))
							numPos -= 1;
					genCube.resize(s2);
					i = s1;
					assert(!isInitial(genCube));
				}
				else i += 1;
				disableActVar(act);
				CheckBreakPdr(true);
			}
			else i += 1;
	}

	if(unsatGenStat.isON())
		unsatGenStat->finishRemoveTime();

	/* Try to propagate to further timeframe */
	if(unsatGenStat.isON())
		unsatGenStat->startTime();

	while(newF < actVar.size() - 1)
	{
		#ifdef UsePatternCheckSAT
		checkPatForSat(newF, MAX_SIZE_T);
		#endif
		solver->clearAssump();
		const Var act = addCurNotState(genCube);
		activateFrame(newF);
		addNextState(genCube);
		bool Break = true;
		if(!satSolve())
		{
			Break = false;
			newF = findLowestActPlus1(newF);
			size_t s = 0;
			for(size_t i = 0, n = genCube.size(); i < n; ++i)
				if(numPos == 1 && !isInv(genCube[i]))
					genCube[s++] = genCube[i];
				else if(solver->inConflict(getGateID(genCube[i]), 1))
					genCube[s++] = genCube[i];
				else if(!isInv(genCube[i]))
					numPos -= 1;
			genCube.resize(s);
			assert(!isInitial(genCube));
		}
		disableActVar(act);
		CheckBreakPdr(true);
		if(Break) break;
	}

	if(unsatGenStat.isON())
		unsatGenStat->finishPushTime();
}

void
PdrChecker::generalizeClause(size_t& newF)
{
	/* Remove a literal each time */
	if(unsatGenStat.isON())
		unsatGenStat->doOneTime(),
		unsatGenStat->startTime();

	if(genType != PDR_GEN_IGNORE)
	{
		for(size_t i = 0; i < genCube.size();)
			if(getInitValue(getGateID(genCube[i])) == ThreeValue_DC)
			{
				solver->clearAssump();
				const Var act = addCurNotState(genCube, i);
				activateFrame(newF-1); //it's fine even if newF is FRAME_INF
				addNextState(genCube, i);
				if(genType == PDR_GEN_APPROX ? !satSolveLimited() : !satSolve())
				{
					newF = findLowestActPlus1(newF-1);
					size_t s1 = 0;
					for(size_t j = 0; j < i; ++j)
						if(solver->inConflict(getGateID(genCube[j]), 1) ||
						   getInitValue(getGateID(genCube[j])) != ThreeValue_DC)
							genCube[s1++] = genCube[j];
					size_t s2 = s1;
					for(size_t j = i + 1, n = genCube.size(); j < n; ++j)
						if(solver->inConflict(getGateID(genCube[j]), 1) ||
						   getInitValue(getGateID(genCube[j])) != ThreeValue_DC)
							genCube[s2++] = genCube[j];
					genCube.resize(s2);
					i = s1;
					assert(!isInitial(genCube));
				}
				else i += 1;
				disableActVar(act);
				CheckBreakPdr(true);
			}
			else
			{
				assert(diffPolar(genCube[i]));
				i += 1;
			}
	}

	if(unsatGenStat.isON())
		unsatGenStat->finishRemoveTime();

	/* Try to propagate to further timeframe */
	if(unsatGenStat.isON())
		unsatGenStat->startTime();

	while(newF < actVar.size() - 1)
	{
		solver->clearAssump();
		const Var act = addCurNotState(genCube);
		activateFrame(newF);
		addNextState(genCube);
		bool Break = true;
		if(!satSolve())
		{
			Break = false;
			newF = findLowestActPlus1(newF);
			size_t s = 0;
			for(size_t i = 0, n = genCube.size(); i < n; ++i)
				if(solver->inConflict(getGateID(genCube[i]), 1) ||
				   getInitValue(getGateID(genCube[i])) != ThreeValue_DC)
					genCube[s++] = genCube[i];
			genCube.resize(s);
			assert(!isInitial(genCube));
		}
		disableActVar(act);
		CheckBreakPdr(true);
		if(Break) break;
	}

	if(unsatGenStat.isON())
		unsatGenStat->finishPushTime();
}

void
PdrChecker::generalizeCube(size_t& newF)
{
	/* Calculate the number of positive literal */
	size_t numDiffPolar = 0;
	for(size_t i = 0, n = genCube.size(); i < n; ++i)
		if(diffPolar(genCube[i]))
			numDiffPolar += 1;
	assert(numDiffPolar != 0);

	/* Remove a literal each time */
	if(unsatGenStat.isON())
		unsatGenStat->doOneTime(),
		unsatGenStat->startTime();

	if(genType != PDR_GEN_IGNORE)
	{
		for(size_t i = 0; i < genCube.size();)		
			if(!diffPolar(genCube[i]) || numDiffPolar > 1)
			{
				solver->clearAssump();
				const Var act = addCurNotState(genCube, i);
				activateFrame(newF-1); //it's fine even if newF is FRAME_INF
				addNextState(genCube, i);
				if(genType == PDR_GEN_APPROX ? !satSolveLimited() : !satSolve())
				{
					if(diffPolar(genCube[i]))
						numDiffPolar -= 1;
					newF = findLowestActPlus1(newF-1);
					size_t s1 = 0;
					for(size_t j = 0; j < i; ++j)
						if(numDiffPolar == 1 && diffPolar(genCube[j]))
							genCube[s1++] = genCube[j];
						else if(solver->inConflict(getGateID(genCube[j]), 1))
							genCube[s1++] = genCube[j];
						else if(diffPolar(genCube[j]))
							numDiffPolar -= 1;
					size_t s2 = s1;
					for(size_t j = i + 1, n = genCube.size(); j < n; ++j)
						if(numDiffPolar == 1 && diffPolar(genCube[j]))
							genCube[s2++] = genCube[j];
						else if(solver->inConflict(getGateID(genCube[j]), 1))
							genCube[s2++] = genCube[j];
						else if(diffPolar(genCube[j]))
							numDiffPolar -= 1;
					genCube.resize(s2);
					i = s1;
					assert(!isInitial(genCube));
				}
				else i += 1;
				disableActVar(act);
				CheckBreakPdr(true);
			}
			else i += 1;
	}

	if(unsatGenStat.isON())
		unsatGenStat->finishRemoveTime();

	/* Try to propagate to further timeframe */
	if(unsatGenStat.isON())
		unsatGenStat->startTime();

	while(newF < actVar.size() - 1)
	{
		solver->clearAssump();
		const Var act = addCurNotState(genCube);
		activateFrame(newF);
		addNextState(genCube);
		bool Break = true;
		if(!satSolve())
		{
			Break = false;
			newF = findLowestActPlus1(newF);
			size_t s = 0;
			for(size_t i = 0, n = genCube.size(); i < n; ++i)
				if(numDiffPolar == 1 && diffPolar(genCube[i]))
					genCube[s++] = genCube[i];
				else if(solver->inConflict(getGateID(genCube[i]), 1))
					genCube[s++] = genCube[i];
				else if(diffPolar(genCube[i]))
					numDiffPolar -= 1;
			genCube.resize(s);
			assert(!isInitial(genCube));
		}
		disableActVar(act);
		CheckBreakPdr(true);
		if(Break) break;
	}

	if(unsatGenStat.isON())
		unsatGenStat->finishPushTime();
}

bool
PdrChecker::propBlockedCubes()
{
	auto returnBy = [this](bool value) -> bool
	{
		printCurFrames(value ? "F " : "P ");
		if(lazyProp) minBlockFrame = actVar.size();
		return value;
	};

	// TODO, printCurFrames during propBlockedCubes, one more frame will be printed
	if(++curFrame == 1)
		{ newFrame(); return false; }

	if(clsStimulator != 0)
		clsStimulator->stimulateAtEndOfFrame();

	if(propStat.isON())
		propStat->doOneTime(),
		propStat->startTime();

	assert(minBlockFrame != 0);
	size_t f = minBlockFrame == 1 ? 1 : minBlockFrame - 1;
	for(; true; ++f)
	{
		assert(f != frame.size() - 1);
		if(f >= curFrame && prpType == PDR_PRP_NORMAL)
			break;
		// TODO, line 1185 is kind of weird
		if(f >= curFrame && frame[f].empty())
			{ assert(prpType == PDR_PRP_EAGER); break; }
		if(f + 1 == frame.size() - 1)
			newFrame();

		vector<PdrCube> tmp(frame[f]);
		for(const PdrCube& c: tmp)
			if(!c.getMarkA())
			{
				if(propStat.isON())
					propStat->incPropCubeCount();
				if(PdrTCube s = solveRelative(PdrTCube(f+1, c), NOIND);
				   s.getFrame() != FRAME_NULL)
				{
					if(propStat.isON())
						propStat->incPropSuccessCount();
					if(isVerboseON(PDR_VERBOSE_PROP))
					{
						cout << RepeatChar('-', 36) << endl
						     << "Propagate cube from frame " << f << " to ";
						if(s.getFrame() == FRAME_INF) cout << "Inf"; else cout << s.getFrame();
						cout << ":"                       << endl
						     << "Before: " << c           << endl
						     << "After:  " << s.getCube() << endl
						     << RepeatChar('-', 36)       << endl;
					}
					addBlockedCube(s, c.getSize() == s.getCube().getSize() ? f : 1);
				}
				CheckBreakPdr(false);
				checkRecycle();
			}
		if(frame[f].empty())
		{
			if(f < curFrame)
			{
				if(f != curFrame - 1)
					curFrame -= 1;
				return returnBy(true);
			}
			assert(prpType == PDR_PRP_EAGER);
			pushToFrameInf(f);
			break;
		}
	}

	if(propStat.isON())
		propStat->finishTime();

	if(toRefineInf)
		refineInf();

	return returnBy(false);
}

PdrCube
PdrChecker::getNotPCube()const
{
	#ifdef UsePatternCheckSAT
	checkPatForSat(curFrame);
	#endif
	if(!targetCube.isNone())
	{
		solver->clearAssump();
		addCurState(targetCube);
		activateFrame(curFrame);
		return satSolve() ? targetCube : PdrCube();
	}
	else
	{
		if(convertInNeedCone)
			solver->convertToCNF(property, 0);
		solver->clearAssump();
		solver->addAssump(property, 0, false);
		activateFrame(curFrame);
		return satSolve() ? terSim(vector<AigGateID>(1, ntk->getGate(property)->getFanIn0ID())) : PdrCube();
	}
}

size_t
PdrChecker::isBlockedSAT(const PdrTCube& badTCube)const
{
	constexpr bool checkBlockedUseSAT = false;
	if constexpr(checkBlockedUseSAT)
	{
		solver->clearAssump();
		activateFrame(badTCube.getFrame());
		addCurState(badTCube.getCube());
		//TODO, get block frame
		return !satSolve();
	}
	return FRAME_NULL;
}

bool
PdrChecker::isInitial(const PdrCube& cube)const
{
	switch(initType)
	{
		case PDR_INIT_DEFAULT:
			for(unsigned i = 0; i < cube.getSize(); ++i)
				if(!isInv(cube.getLit(i)))
					return false;
			return true;

		case PDR_INIT_CLAUSE:
		{
			size_t numDiffLit = 0;
			for(unsigned i = 0; i < cube.getSize(); ++i)
				if(ThreeValue v = getInitValue(getGateID(cube.getLit(i))); v != ThreeValue_DC)
				{
					if(v ^ isInvNum(cube.getLit(i)))
						return true;
					numDiffLit += 1;
				}
			assert(numDiffLit <= numLitInit);
			return numDiffLit < numLitInit;
		}

		case PDR_INIT_CUBE:
			for(unsigned i = 0; i < cube.getSize(); ++i)
				if(diffPolar(cube.getLit(i)))
					return false;
			return true;

		default:
			assert(false);
			return false;
	}
}

PdrTCube
PdrChecker::solveRelative(const PdrTCube& s, SolveType type)const
{
	#ifdef UsePatternCheckSAT
	checkPatForSat(s.getFrame() - 1, s.getCube());
	#endif
	solver->clearAssump();
	struct DisableAct {
		~DisableAct() { if(act != var_Undef) c->disableActVar(act); }
		const PdrChecker* c; Var act;
	} disableAct = { this, type != NOIND ? addCurNotState(s.getCube()) : var_Undef };
	activateFrame(s.getFrame()-1);
	if(convertInNeedCone) convertCNF(s.getCube());
	addNextState(s.getCube());
	return satSolve() ? PdrTCube(FRAME_NULL, type == EXTRACT ? terSim(genTarget(s.getCube()), &(s.getCube())) : PdrCube())
	                  : PdrTCube(findLowestActPlus1(s.getFrame()-1), unsatGen(s.getCube()));
}

void
PdrChecker::newFrame()
{
	frame.push_back(move(frame.back()));
	frame[frame.size()-2].clear();
	actVar.push_back(solver->newVar());
	badDequeVec.emplace_back();
	assert(actVar.size() == frame.size() - 1);
	assert(actVar.size() == badDequeVec.size() ||
	       (oblType == PDR_OBL_PUSH && actVar.size() + 1 == badDequeVec.size()));
}

void
PdrChecker::addBlockedCube(const PdrTCube& blockTCube, size_t initF)
{
	size_t k = frame.size() - 1;
	const size_t blockFrame = blockTCube.getFrame();
	assert(blockFrame != FRAME_NULL);
	assert(blockFrame == FRAME_INF || blockFrame < actVar.size());
	if(k > blockFrame)
		k = blockFrame;
	else assert(blockFrame == FRAME_INF);
	assert(initF <= k);

	for(size_t f = 0; f < frame.size(); ++f)
		for(size_t i = 0; i < frame[f].size(); ++i)
			assert(!frame[f][i].isNone());

	const PdrCube& blockCube = blockTCube.getCube();
	checkSubsumeOthers(blockCube, initF, k);

	for(size_t f = 0; f < frame.size(); ++f)
		for(size_t i = 0; i < frame[f].size(); ++i)
			assert(!frame[f][i].isNone());

	if(isOrdDynamic())
	{
		for(unsigned i = 0; i < blockCube.getSize(); ++i)
			if((activity[getGateID(blockCube.getLit(i))] += actInc) > 1e100)
			{
				for(size_t i = 0, M = ntk->getMaxGateNum(); i < M; ++i)
					activity[i] *= 1e-100;
				actInc *= 1e-100;
			}
		if(isOrdDecay())
		{
			constexpr double actDecay = 1.05;
			actInc *= actDecay;
		}
	}

	frame[k].push_back(blockCube);
	// Actually it must hold currently, but we do not exclude the possibility for future
	if(blockFrame >= frameConverted)
		addBlockedCubeFrame(blockFrame, blockCube);

	if(isVerboseON(PDR_VERBOSE_BLK))
	{
		cout << RepeatChar('-', 36) << endl
		     << "Add blocked Cube: frame = ";
		if(blockFrame == FRAME_INF) cout << "Inf"; else cout << blockFrame;
		cout << ", " << blockCube << endl
		     << RepeatChar('-', 36) << endl;
	}

	if(cubeStat.isON())
		if(blockFrame == FRAME_INF)
		{
			cubeStat->incInfCubeNum();
			cubeStat->incInfLitNum(blockCube.getSize());
			cubeStat->checkMaxCLen(blockCube.getSize());
		}

	#ifdef CheckOblCommonPart
	checkIsDC(blockCube);
	checkInfCommonPart(blockTCube);
	#endif

	if(clsStimulator != 0)
		clsStimulator->stimulateWithOneCube(blockTCube);
}

void
PdrChecker::addBlockedCubeFrame(size_t blockFrame, const PdrCube& blockCube)const
{
	assert(blockFrame > 0);
	assert(blockFrame < actVar.size() || blockFrame == FRAME_INF);

	if(blockFrame != FRAME_INF)
		litList.emplace_back(actVar[blockFrame], true);
	for(unsigned i = 0; i < blockCube.getSize(); ++i)
		litList.emplace_back(solver->getVarInt(getGateID(blockCube.getLit(i)), 0),
		                     !isInv(blockCube.getLit(i)));
	solver->addClause(litList);
	litList.clear();
	const_cast<PdrCube&>(blockCube).setMarkA(false);
}

void
PdrChecker::addBlockedCubeFrame(size_t blockFrame)const
{
	assert(blockFrame < actVar.size());
	if(blockFrame == 0)
		addInitState();
	else
		for(const PdrCube& c: frame[blockFrame])
			addBlockedCubeFrame(blockFrame, c);
}

void
PdrChecker::addBlockedCubeInf()const
{
	for(const PdrCube& c: frame.back())
		addBlockedCubeFrame(FRAME_INF, c);
}

PdrCube
PdrChecker::terSim(const vector<AigGateID>& target, const PdrCube* prevCube)const
{
	if(terSimStat.isON())
	{
		terSimStat->doOneTime();
		terSimStat->startTime();
	}
	genCube.clear();
	switch(simType)
	{
		case PDR_SIM_FORWARD_NORMAL    : terSimForwardNormal   (target); break;
		case PDR_SIM_FORWARD_EVENT     : terSimForwardEvent    (target); break;
		case PDR_SIM_BACKWARD_NORMAL   : terSimBackwardNormal  (target); break;
		case PDR_SIM_BACKWARD_INTERNAL : terSimBackwardInternal(target); break;
	}
	PdrCube c(genCube, true, prevCube, trace);
	numObl += 1;
	if(terSimStat.isON())
	{
		terSimStat->finishTime();
		terSimStat->incLitCount(c.getSize());
		AigGate::setGlobalRef();
		terSimSup.markDfsCone(target);
		size_t latchInCone = 0;
		for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
			if(ntk->getLatchNorm(i)->isGlobalRef())
				latchInCone += 1;
		terSimStat->incRemoveCount(latchInCone - size_t(c.getSize()));
	}
	if(isVerboseON(PDR_VERBOSE_TERSIM))
		cout << RepeatChar('-', 36) << endl
		     << "Generate proof obligation: " << c << endl
		     << RepeatChar('-', 36) << endl;
	return c;
}

void
PdrChecker::terSimForwardNormal(const vector<AigGateID>& target)const
{
	genSimCand(target);

	/* Constant Propagation */
	for(size_t i = 0, I = ntk->getInputNum(); i < I; ++i)
		if(AigGateID id = ntk->getInputID(i); ntk->getGate(id)->isGlobalRef())
			terSimSup.setValue(id, solver->getValueBool(id, 0));
	for(AigGateLit lit: genCube)
		terSimSup.setValue(getGateID(lit), ThreeValue_DC);
	terSimSup.simDfsList();
	terSimSup.doConstProp(target);

	/* Ternary Simulation */
	for(AigGateLit lit: genCube)
		terSimSup.setValue(getGateID(lit), !isInv(lit));
	size_t s = 0;
	for(size_t i = 0, n = genCube.size(); i < n; ++i)
	{
		terSimSup.setValue(getGateID(genCube[i]), ThreeValue_DC);
		terSimSup.simDfsList();
		bool necessary = false;
		for(AigGateID t: target)
			if(terSimSup.getValue(t) == ThreeValue_DC)
				{ necessary = true; break; }
		if(necessary)
		{
			terSimSup.setValue(getGateID(genCube[i]), !isInv(genCube[i]));
			genCube[s++] = genCube[i];
		}
	}
	genCube.resize(s);
}

void
PdrChecker::terSimForwardEvent(const vector<AigGateID>& target)const
{
	genSimCand(target);

	/* Do First Simulation */
	for(size_t i = 0, I = ntk->getInputNum(); i < I; ++i)
		if(AigGateID id = ntk->getInputID(i); ntk->getGate(id)->isGlobalRef())
			terSimSup.setValue(id, solver->getValueBool(id, 0));
	for(AigGateLit lit: genCube)
		terSimSup.setValue(getGateID(lit), !isInv(lit));
	terSimSup.simDfsList();
	terSimSup.markDfsList();

	/* Ternary Simulation */
	const unsigned maxLevel = terSimSup.getMaxLevel(target);
	size_t s = 0;
	for(size_t i = 0, n = genCube.size(); i < n; ++i)
	{
		terSimSup.setValueEventRef(getGateID(genCube[i]), ThreeValue_DC);
		terSimSup.simByEventRef(maxLevel);
		bool necessary = false;
		for(AigGateID t: target)
			if(terSimSup.getValue(t) == ThreeValue_DC)
				{ necessary = true; break; }
		if(necessary)
		{
			terSimSup.setValueEventRef(getGateID(genCube[i]), !isInv(genCube[i]));
			genCube[s++] = genCube[i];
		}
	}
	terSimSup.cleanEvent(maxLevel);
	genCube.resize(s);
}

void
PdrChecker::terSimBackwardNormal(const vector<AigGateID>& target)const
{
	terSimSup.backwardTerSim(solver, target, genCube, activity, isOrdReverse());
}

void
PdrChecker::terSimBackwardInternal(const vector<AigGateID>&)const
{
	//TODO
}

void
PdrChecker::satGenBySAT(const vector<AigGateLit>& target)const
{
	const Var act = solver->newVar();
	litList.emplace_back(act, true);
	for(AigGateLit lit: target)
		litList.emplace_back(solver->getVarInt(getGateID(lit), 1), !isInv(lit));
	solver->addClause(litList);
	litList.clear();
//	terSimSup.markDfsCone(target);
	unsigned numPI = 0;
	for(size_t i = 0, I = ntk->getInputNum(); i < I; ++i)
		if(ntk->getInputNorm(i)->isGlobalRef())
			numPI += 1;
	Array<AigGateLit> piValue(numPI);
	for(size_t i = 0, idx = 0, I = ntk->getInputNum(); i < I; ++i)
		if(ntk->getInputNorm(i)->isGlobalRef())
			piValue[idx++] = makeToLit(ntk->getInputID(i), !solver->getValueBool(ntk->getInputID(i), 0));

	solver->clearAssump();
	solver->addAssump(act, false);
	for(unsigned i = 0; i < numPI; ++i)
		solver->addAssump(piValue[i], 0);
	for(AigGateLit lit: genCube)
		solver->addAssump(lit, 0);
	bool result = satSolve();
	assert(!result);
	if(result)
		{ cerr << "[Error] The previous terSim is wrong!" << endl; return; }
	size_t s = 0, n = genCube.size();
	for(size_t i = 0; i < n; ++i)
		if(solver->inConflict(getGateID(genCube[i])))
			genCube[s++] = genCube[i];
	// TODO

	disableActVar(act);
}

void
PdrChecker::genSimCand(const vector<AigGateID>& target)const
{
	terSimSup.clearDfsList();
	terSimSup.genDfsList(target);
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
		if(AigGateID id = ntk->getLatchID(i); ntk->getGate(id)->isGlobalRef())
			genCube.push_back(makeToLit(id, !solver->getValueBool(id, 0)));
	if(isOrdDynamic())
		sortGenCubeByAct();
}

void
PdrChecker::sortGenCubeByAct()const
{
	auto cmpByActNormal = [this](AigGateLit lit1, AigGateLit lit2)
		{ return activity[getGateID(lit1)] < activity[getGateID(lit2)]; };
	auto cmpByActReverse = [this](AigGateLit lit1, AigGateLit lit2)
		{ return activity[getGateID(lit1)] > activity[getGateID(lit2)]; };
	if(isOrdReverse()) sort(genCube.begin(), genCube.end(), cmpByActReverse);
	else               sort(genCube.begin(), genCube.end(), cmpByActNormal);
}

Var
PdrChecker::addCurNotState(const PdrCube& c)const
{
	const Var act = solver->newVar();
	litList.emplace_back(act, true);
	for(unsigned i = 0; i < c.getSize(); ++i)
		litList.emplace_back(solver->getVarInt(getGateID(c.getLit(i)), 0),
		                     !isInv(c.getLit(i)));
	solver->addClause(litList);
	litList.clear();
	solver->addAssump(act, false);
	return act;
}

void
PdrChecker::addState(const PdrCube& c, size_t level)const
{
	for(unsigned i = 0; i < c.getSize(); ++i)
		solver->addAssump(c.getLit(i), level);
}

void
PdrChecker::activateFrame(size_t f)const
{
	while(frameConverted > f)
		addBlockedCubeFrame(--frameConverted);
	if(assertFrame)
		if(f < frame.size() - 1)
			for(size_t ff = 0; ff < f; ++ff)
				solver->addAssump(actVar[ff], true);
	for(size_t maxF = actVar.size(); f < maxF; ++f)
		solver->addAssump(actVar[f], false);
}

PdrCube
PdrChecker::unsatGen(const PdrCube& c)const
{
	assert(!isInitial(c));
	genCube.clear();
	switch(initType)
	{
		case PDR_INIT_DEFAULT:
			for(unsigned i = 0; i < c.getSize(); ++i)
				if(solver->inConflict(getGateID(c.getLit(i)), 1))
					genCube.push_back(c.getLit(i));
			if(isInitial(genCube))
			{
				for(unsigned i = c.getSize() - 1; i != MAX_UNSIGNED; --i)
					if(!isInv(c.getLit(i)))
					{
						size_t j = genCube.size();
						genCube.emplace_back();
						for(; j > 0 && c.getLit(i) < genCube[j-1]; --j)
							genCube[j] = genCube[j-1];
						genCube[j] = c.getLit(i);
						break;
					}
			}
			break;

		case PDR_INIT_CLAUSE:
			for(unsigned i = 0; i < c.getSize(); ++i)
				if(solver->inConflict(getGateID(c.getLit(i)), 1) || diffPolar(c.getLit(i)))
					genCube.push_back(c.getLit(i));
			break;

		case PDR_INIT_CUBE:
			for(unsigned i = 0; i < c.getSize(); ++i)
				if(solver->inConflict(getGateID(c.getLit(i)), 1))
					genCube.push_back(c.getLit(i));
			if(isInitial(genCube))
			{
				for(unsigned i = c.getSize() - 1; i != MAX_UNSIGNED; --i)
					if(diffPolar(c.getLit(i)))
					{
						size_t j = genCube.size();
						genCube.emplace_back();
						for(; j > 0 && c.getLit(i) < genCube[j-1]; --j)
							genCube[j] = genCube[j-1];
						genCube[j] = c.getLit(i);
						break;
					}
			}
			break;
	}
	assert(!isInitial(genCube));
	return PdrCube(genCube);
}

size_t
PdrChecker::findLowestActPlus1(size_t f)const
{
	const size_t maxF = actVar.size();
	for(; f < maxF; ++f)
		if(solver->inConflict(actVar[f]))
			return ++f == maxF ? f - 1 : f;
	return FRAME_INF;
}

bool
PdrChecker::isInitial(const vector<AigGateLit>& cubeVec, size_t ignoreIdx)const
{
	switch(initType)
	{
		case PDR_INIT_DEFAULT:
			for(size_t i = 0, n = cubeVec.size(); i < n; ++i)
				if(!isInv(cubeVec[i]) && i != ignoreIdx)
					return false;
			return true;

		case PDR_INIT_CLAUSE:
		{
			size_t numDiffLit = 0;
			for(size_t i = 0, n = cubeVec.size(); i < n; ++i)
				if(ThreeValue v = getInitValue(getGateID(cubeVec[i])); v != ThreeValue_DC)
				{
					if((v ^ isInvNum(cubeVec[i])) && i != ignoreIdx)
						return true;
					numDiffLit += 1;
				}
			assert(numDiffLit <= numLitInit);
			return numDiffLit < numLitInit;
		}

		case PDR_INIT_CUBE:
			for(size_t i = 0, n = cubeVec.size(); i < n; ++i)
				if(diffPolar(cubeVec[i]) && i != ignoreIdx)
					return false;
			return true;

		default:
			assert(false);
			return false;
	}
}

Var
PdrChecker::addCurNotState(const vector<AigGateLit>& cubeVec, size_t ignoreIdx)const
{
	const Var act = solver->newVar();
	litList.emplace_back(act, true);
	for(size_t i = 0, n = cubeVec.size(); i < n; ++i)
		if(i != ignoreIdx)
			litList.emplace_back(solver->getVarInt(getGateID(cubeVec[i]), 0),
			                     !isInv(cubeVec[i]));
	solver->addClause(litList);
	litList.clear();
	solver->addAssump(act, false);
	return act;
}

void
PdrChecker::addNextState(const vector<AigGateLit>& cubeVec, size_t ignoreIdx)const
{
	for(size_t i = 0, n = cubeVec.size(); i < n; ++i)
		if(i != ignoreIdx)
			solver->addAssump(cubeVec[i], 1);
}

void
PdrChecker::printCurFrames(const char* prefix)const
{
	if(isVerboseON(PDR_VERBOSE_CUBE))
		cout << "/" << RepeatChar('*', 72) << "\\" << endl
		     << "|" << RepeatChar('*', 72) << "|"  << endl;

	if(isVerboseON(PDR_VERBOSE_FRAME))
	{
		cout << prefix << curFrame << ":";
		size_t f = 0;
		for(; f <= curFrame; ++f)
			cout << " " << frame[f].size();
		if(f + 1 < frame.size())
		{
			cout << " |";
			for(; f < frame.size() - 1; ++f)
				cout << " " << frame[f].size();
			cout << " |";
		}
		cout << " " << frame.back().size() << endl;
	}

	if(isVerboseON(PDR_VERBOSE_CUBE))
	{
		for(size_t f = 1, n = frame.size(); f < n; ++f)
		{
			cout << RepeatChar('*', 6) << endl;
			cout << "Frame "; if(f == n - 1) cout << "Inf"; else cout << f; cout << endl;
			cout << RepeatChar('*', 6) << endl;
			if(frame[f].empty())
				cout << "None" << endl;
			else
				for(size_t i = 0, nn = frame[f].size(); i < nn; ++i)
					cout << "(" << (i + 1) << ") " << frame[f][i] << endl;
		}
		cout << "|"  << RepeatChar('*', 72) << "|" << endl
		     << "\\" << RepeatChar('*', 72) << "/" << endl;
	}
}

void
PdrChecker::printRemainObl()const
{
	if(isVerboseON(PDR_VERBOSE_FRAME))
	{
		cout << "Remaining proof obligation:";
		for(const deque<PdrCube>& f: badDequeVec)
			cout << " " << f.size();
		cout << endl;
	}
}

void
PdrChecker::checkBreak(const char* funcName, bool blockNow)const
{
	if(checkIsStopped())
	{
		cout << "\r";
		printCurFrames("Z ");
		printStats();
		const string wait = "Press any key to continue...";
		cout << "\r" << wait;
		returnExactKey();
		setToOld();
		cout << CleanStrOnTerminal(wait.c_str()) << "\r" << flush;
		resetStop();
	}
	if(checkBreakCond())
		sfcMsg << " during " << funcName << endl;
	else if(blockNow)
	{
		if(satQueryLimit != 0 && totalSatQuery >= satQueryLimit)
			sfcMsg << "Out of SAT query limit!" << endl;
		else if(numOblLimit != 0 && numObl >= numOblLimit)
			sfcMsg << "Out of generated obligation limit!" << endl;
		else return;
	}
	else return;
	
	printCurFrames("U ");
	if(blockNow) printRemainObl();
	throw CheckerBreak();
}

bool
PdrChecker::satSolve()const
{
	satQueryTime += 1;
	totalSatQuery += 1;
	size_t d = solver->getDecisionNum();
	size_t c = solver->getConflictNum();
	if(satStat.isON())
		satStat->startTime();
	bool isSAT = solver->solve();
	if(satStat.isON())
		satStat->countOne  (isSAT),
		satStat->finishTime(isSAT),
		satStat->setLastTime();
	d = solver->getDecisionNum() - d;
	c = solver->getConflictNum() - c;
	if(satStat.isON())
	{
		isSAT ? satStat->setSD(d) : satStat->setUD(d);
		satStat->incDeciNum(d);
		satStat->incConfNum(c);
	}
	if(!isSAT) checkMaxD(d);
	#ifdef UsePatternCheckSAT
	assert(!simResult || isSAT);
	if(isSAT) collectPattern();
	#endif
	return isSAT;
}

bool
PdrChecker::satSolveLimited()const
{
	satQueryTime += 1;
	totalSatQuery += 1;
	size_t d = solver->getDecisionNum();
	size_t c = solver->getConflictNum();
	if(satStat.isON())
		satStat->startTime();
	size_t dd = maxUNSAT_D / 2;
	constexpr size_t minDiff = 36;
	if(dd < minDiff) dd = minDiff;
	solver->setDeciLimit(maxUNSAT_D + dd);
	lbool result = solver->solveLimited();
	if(satStat.isON())
	{
		satStat->countOne  (result == l_Undef ? 2 : (result == l_True ? 1 : 0));
		satStat->finishTime(result == l_Undef ? 2 : (result == l_True ? 1 : 0));
		if(result != l_Undef) satStat->setLastTime();
	}
	d = solver->getDecisionNum() - d;
	c = solver->getConflictNum() - c;
	if(satStat.isON())
	{
		result == l_Undef ? satStat->setAD(d) : (result == l_True ? satStat->setSD(d) : satStat->setUD(d));
		satStat->incDeciNum(d);
		satStat->incConfNum(c);
	}
	if(result == l_False) checkMaxD(d);
	#ifdef UsePatternCheckSAT
	assert(!simResult || result != l_False);
	if(result == l_True) collectPattern();
	#endif
	return result != l_False;
}

void
PdrChecker::disableActVar(Var act)const
{
	solver->addClause(Lit(act, true));
	unusedVarNum += 1;
}

void
PdrChecker::checkRecycle()
{
	if(recycleNum != 0 && getCurRecycleNum() >= recycleNum)
	{
		if(recycleStat.isON())
			recycleStat->doOneTime(),
			recycleStat->checkMaxRecNum(getCurRecycleNum()),
			recycleStat->startTime();
		unusedVarNum = 0;
		satQueryTime = 0;
		solver->resetSolver();
		convertCNF();
		for(Var& act: actVar)
			act = solver->newVar();
		if(!convertInNeedFrame)
			for(size_t f = 0; f < actVar.size(); ++f)
				addBlockedCubeFrame(f);
		else frameConverted = actVar.size();
		addBlockedCubeInf();
		if(recycleStat.isON())
			recycleStat->finishTime();
	}
}

void
PdrChecker::convertCNF()const
{
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
		solver->convertToCNF(ntk->getLatchID(i), 0);
	if(!convertInNeedCone)
	{
		for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
			solver->convertToCNF(ntk->getLatchID(i), 1);
		solver->convertToCNF(property, 0);
	}
}


void
PdrChecker::convertCNF(const PdrCube& c)const
{
	for(unsigned i = 0; i < c.getSize(); ++i)
		solver->convertToCNF(getGateID(c.getLit(i)), 1);
}

void
PdrChecker::addInitState()const
{
	Lit actInit(actVar[0], true);
	switch(initType)
	{
		case PDR_INIT_DEFAULT:
			for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
				solver->addClause(actInit,
				                  Lit(solver->getVarInt(ntk->getLatchID(i), 0), true));
			break;

		case PDR_INIT_CLAUSE:
			litList.push_back(actInit);
			for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
				if(ThreeValue v = getInitValue(ntk->getLatchID(i));
				   v != ThreeValue_DC)
					litList.emplace_back(solver->getVarInt(ntk->getLatchID(i), 0),
					                     v == ThreeValue_False);
			solver->addClause(litList);
			litList.clear();
			break;

		case PDR_INIT_CUBE:
			for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
				if(ThreeValue v = getInitValue(ntk->getLatchID(i));
				   v != ThreeValue_DC)
					solver->addClause(actInit,
					                  Lit(solver->getVarInt(ntk->getLatchID(i), 0),
					                      v == ThreeValue_False));
			break;
	}
}

void
PdrChecker::refineInf()
{
	vector<PdrCube> tmp(frame.back());
	for(const PdrCube& c: tmp)
		if(!c.getMarkA())
		{
			if(convertInNeedCone) convertCNF(c);
			const PdrTCube newTCube = generalize(PdrTCube(FRAME_INF, c));
			assert(newTCube.getFrame() == FRAME_INF);
			const PdrCube& newCube = newTCube.getCube();
			if(newCube.getSize() != c.getSize())
			{
				assert(newCube.getSize() < c.getSize());
				if(isVerboseON(PDR_VERBOSE_INF))
					cout << RepeatChar('-', 36) << endl
					     << "Further decrease " << (c.getSize() - newCube.getSize()) << " literals!" << endl
					     << RepeatChar('-', 36) << endl;
				if(propStat.isON())
					propStat->incInfLitCount(c.getSize() - newCube.getSize());
				addBlockedCube(newTCube);
			}
			checkRecycle();
		}
}

void
PdrChecker::checkThenPushObl(size_t f, const PdrCube& c)
{
	bool keep = true;
	if(checkSelf)
	{
		size_t s = 0;
		for(size_t i = 0, n = badDequeVec[f].size(); i < n; ++i)
			// TODO, get runtime of subsumption
			if(AigGateLit ret = selfSubsume(c, badDequeVec[f][i]);
			   ret == ERROR_GATELIT)
			{
				// TODO, get runtime of subsumption
				if(ret = selfSubsume(badDequeVec[f][i], c);
				   ret == UNDEF_GATELIT)
				{
cout << "Subsume 2" << endl;
					assert(s == i);
					if(cubeStat.isON())
						cubeStat->incSubsumeAddObl();
					s = n;
					keep = false;
					break;
				}
				else if(ret != ERROR_GATELIT)
				{}

//cout << "None" << endl;
				if(s < i)
					badDequeVec[f][s++] = move(badDequeVec[f][i]);
				else { assert(s == i); s += 1; }
			}
			else if(ret == UNDEF_GATELIT)
			{
cout << "Subsume" << endl;
				if(cubeStat.isON())
					cubeStat->incSubsumeAddObl();
			}
			else
			{
cout << "Self Subsume " << ret << endl;
				if(s < i)
					badDequeVec[f][s++] = move(badDequeVec[f][i]);
				else { assert(s == i); s += 1; }
			}
		badDequeVec[f].resize(s);
	}
	if(keep)
	{
		if(sortByBadDepth)
		{
			size_t s = badDequeVec[f].size();
			badDequeVec[f].emplace_back();
			for(; s > 0 && badDequeVec[f][s-1].getBadDepth() > c.getBadDepth(); --s)
				badDequeVec[f][s] = move(badDequeVec[f][s-1]);
			assert(badDequeVec[f][s].isNone());
			badDequeVec[f][s] = c;
			for(size_t i = 1, s = badDequeVec[f].size(); i < s; ++i)
				assert(badDequeVec[f][i-1].getBadDepth() <= badDequeVec[f][i].getBadDepth());
		}
		else badDequeVec[f].push_back(c);
	}
	if(cubeStat.isON())
		cubeStat->checkMaxBNum(badDequeVec[f].size());
}

void
PdrChecker::checkAndPrintIndInv()
{
	if(!checkIndInv && !isVerboseON(PDR_VERBOSE_FINAL))
		return;

	vector<PdrCube> indInv = getCurIndSet(true);
	if(checkIndInv)
	{
		cout << RepeatChar('=', 36) << endl
		     << "Check the inductive invariant:" << endl;
		SolverPtr<CirSolver> checkSolver(ntk);

		Progresser initP("1. Check if the initial state is in the set : ", indInv.size());
		for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
			checkSolver->convertToCNF(ntk->getLatchID(i), 0);
		Lit initAct(checkSolver->newVar(), true);
		switch(initType)
		{
			case PDR_INIT_DEFAULT:
				for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
					checkSolver->addClause(initAct,
					                       Lit(checkSolver->getVarInt(ntk->getLatchID(i), 0), true));
				break;

			case PDR_INIT_CLAUSE:
				litList.push_back(initAct);
				for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
					if(ThreeValue v = getInitValue(ntk->getLatchID(i));
					   v != ThreeValue_DC)
						litList.emplace_back(checkSolver->getVarInt(ntk->getLatchID(i), 0),
						                     v == ThreeValue_False);
				checkSolver->addClause(litList);
				litList.clear();
				break;

			case PDR_INIT_CUBE:
				for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
					if(ThreeValue v = getInitValue(ntk->getLatchID(i));
					   v != ThreeValue_DC)
						checkSolver->addClause(initAct,
						                       Lit(checkSolver->getVarInt(ntk->getLatchID(i), 0),
						                           v == ThreeValue_False));
				break;
		}
		initP.printLine();
		vector<PdrCube> notInit;
		for(const PdrCube& c: indInv)
		{
			checkSolver->clearAssump();
			checkSolver->addAssump(~initAct);
			for(AigGateLit lit: c)
				checkSolver->addAssump(lit, 0);
			if(checkSolver->solve())
				notInit.push_back(c);
			initP.count();
		}
		initP.cleanCurLine();
		cout << "\r1. Check if the initial state is in the set : "
		     << (notInit.empty() ? "PASS" : "FAIL") << endl;
		if(!notInit.empty())
		{
			cout << "Failed clause:" << endl;
			for(const PdrCube& c: notInit)
				cout << c << endl;
		}
		checkSolver->addClause(initAct);

		cout << "2. Check if the property holds for the set  : " << flush;
		for(const PdrCube& c: indInv)
		{
			for(unsigned i = 0; i < c.getSize(); ++i)
				litList.emplace_back(checkSolver->getVarInt(getGateID(c.getLit(i)), 0),
				                     !isInv(c.getLit(i)));
			checkSolver->addClause(litList);
			litList.clear();
		}
		checkSolver->clearAssump();
		if(targetCube.isNone())
			checkSolver->convertToCNF(property, 0),
			checkSolver->addAssump(property, 0, false);
		else
			for(AigGateLit lit: targetCube)
				checkSolver->addAssump(lit, 0);
		cout << (checkSolver->solve() ? "FAIL" : "PASS") << endl;

		Progresser indP("3. Check if the set itself is inductive: ", indInv.size());
		indP.printLine();
		vector<PdrCube> notInd;
		for(const PdrCube& c: indInv)
		{
			checkSolver->clearAssump();
			for(AigGateLit lit: c)
				checkSolver->convertToCNF(getGateID(lit), 1),
				checkSolver->addAssump(lit, 1);
			if(checkSolver->solve())
				notInd.push_back(c);
			indP.count();
		}
		indP.cleanCurLine();
		cout << "\r3. Check if the set itself is inductive     : "
		     << (notInd.empty() ? "PASS" : "FAIL") << endl;
		if(!notInd.empty())
		{
			cout << "Failed clause:" << endl;
			for(const PdrCube& c: notInd)
				cout << c << endl;
		}
	}

	if(isVerboseON(PDR_VERBOSE_FINAL))
	{
		cout << "/" << RepeatChar('*', 72) << "\\" << endl
		     << "|" << RepeatChar('*', 72) << "|"  << endl;
		if(indInv.empty())
			cout << "Tautology" << endl;
		else
			for(size_t i = 0, n = indInv.size(); i < n; ++i)
				cout << "(" << i + 1 << ") " << indInv[i] << endl;
		cout << "|"  << RepeatChar('*', 72) << "|" << endl
		     << "\\" << RepeatChar('*', 72) << "/" << endl;
	}
}

bool
PdrChecker::subsume(const PdrCube& c1, const PdrCube& c2)const
{
// Too many calls to clock(), introduce too much overhead
// Use approximation at higher hierarchy
//	if(cubeStat.isON())
//		cubeStat->doOneTime(),
//		cubeStat->startTime();
	if(cubeStat.isON())
		cubeStat->doOneTime();
	bool ret = c1.subsume(c2);
//	if(cubeStat.isON())
//		cubeStat->finishTime();
	return ret;
}

AigGateLit
PdrChecker::selfSubsume(const PdrCube& c1, const PdrCube& c2)const
{
// Too many calls to clock(), introduce too much overhead
// Use approximation at higher hierarchy
//	if(cubeStat.isON())
//		cubeStat->doOneTime(),
//		cubeStat->startTime();
	if(cubeStat.isON())
		cubeStat->doOneTime();
	AigGateLit ret = c1.selfSubsume(c2);
//	if(cubeStat.isON())
//		cubeStat->finishTime();
	return ret;
}

vector<PdrCube>
PdrChecker::getCurIndSet(bool toSort)
{
	collectInd();
	vector<PdrCube> indSet(getInfFrame());
	if(toSort)
		sort(indSet.begin(), indSet.end());
	return indSet;
}

void
PdrChecker::pushToFrameInf(size_t emptyFrame)
{
	const size_t inf = frame.size() - 1;
	assert(emptyFrame < inf);
	for(size_t ff = inf - 1; ff > emptyFrame; --ff)
	{
		vector<PdrCube> tmp(frame[ff]);
		// If purely adding cube to frame Inf, nothing goes wrong since no cube will be subsumed
		// But if combining with stimulation, some cubes may be subsumed and removed
		// So we cannot just iterate through the frame since it may be changed
		for(const PdrCube& c: tmp)
			if(!c.getMarkA())
			{
				if(isVerboseON(PDR_VERBOSE_PROP))
				{
					cout << RepeatChar('-', 36) << endl
					     << "Propagate cube by induction from frame " << ff << " to Inf"
					     << ":" << c            << endl
					     << RepeatChar('-', 36) << endl;
				}
				addBlockedCube(PdrTCube(FRAME_INF, c), inf);
				if(propStat.isON())
					propStat->incInfCubeCount();
			}
		frame[ff].clear();
	}
	for(; emptyFrame < inf; ++emptyFrame)
		assert(frame[emptyFrame].empty());
}

bool
PdrChecker::checkIsSubsumed(const PdrCube& cube, size_t startFrame, size_t endFrame)const
{
	if(cubeStat.isON())
		cubeStat->startTime();
	bool ret = false;
	for(size_t f = startFrame; f <= endFrame; ++f)
		for(size_t i = 0, n = frame[f].size(); i < n; ++i)
			if(subsume(frame[f][i], cube))
				{ ret = true; break; }
	if(cubeStat.isON())
		cubeStat->finishTime();
	return ret;
}

void
PdrChecker::checkSubsumeOthers(const PdrCube& cube, size_t startFrame, size_t endFrame)
{
	if(cubeStat.isON())
		cubeStat->startTime();
	for(size_t f = startFrame; f <= endFrame; ++f)
	{
		size_t s = 0;
		for(size_t i = 0, n = frame[f].size(); i < n; ++i)
		{
			assert(!frame[f][i].getMarkA());
			if(!subsume(cube, frame[f][i]))
			{
				if(s < i)
					frame[f][s++] = move(frame[f][i]);
				else { assert(s == i); s += 1; }
			}
			else
			{
				frame[f][i].setMarkA(true);
				if(cubeStat.isON())
					cubeStat->incSubsumeBlockCube();
			}
		}
		frame[f].resize(s);
	}
	if(cubeStat.isON())
		cubeStat->finishTime();
}

void
PdrChecker::printTrace()const
{
	assert(badDequeVec[0].size() == 1);
	assert(isInitial(badDequeVec[0][0]));

	if(trace)
	{
		// TODO, more testing
		vector<PdrCube> cexTrace;
		cexTrace.push_back(badDequeVec[0][0]);

		while(true)
		{
			const PdrCube& prev = cexTrace.back().getPrevCube();
			if(prev.isNone()) break;
			cexTrace.push_back(prev);
		}

		SolverPtr<CirSolver> traceSolver(ntk);
		const size_t L = ntk->getLatchNum();
		Array<bool> curState(L);
		for(size_t i = 0; i < L; ++i)
			traceSolver->convertToCNF(ntk->getLatchID(i), 0),
			traceSolver->convertToCNF(ntk->getLatchID(i), 1);
		switch(initType)
		{
			case PDR_INIT_DEFAULT:
				for(size_t i = 0; i < L; ++i)
					curState[i] = true;
				break;

			case PDR_INIT_CLAUSE:
				// TODO
				break;

			case PDR_INIT_CUBE:
				// TODO
				break;
		}
		for(size_t c = 1, n = cexTrace.size(); c < n; ++c)
		{
			for(size_t i = 0; i < L; ++i)
				traceSolver->addAssump(ntk->getLatchID(i), 0, curState[i]);
			for(AigGateLit lit: cexTrace[c])
				traceSolver->addAssump(lit, 1);
			bool result = traceSolver->solve();
			assert(result);
			if(!result)
				{ cerr << "[Error] The trace cannot be connected! The proof goes wrong!" << endl; return; }
			traceSolver->reportPI(c-1, 0);
			traceSolver->clearAssump();
			for(size_t i = 0; i < L; ++i)
				curState[i] = !(traceSolver->getValueBool(ntk->getLatchID(i), 1));
		}

		for(size_t i = 0; i < L; ++i)
			traceSolver->addAssump(ntk->getLatchID(i), 0, curState[i]);
		if(targetCube.isNone())
			traceSolver->convertToCNF(property, 0),
			traceSolver->addAssump(property, 0, false);
		else
			for(AigGateLit lit: targetCube)
				traceSolver->addAssump(lit, 0);
		bool result = traceSolver->solve();
		assert(result);
		if(!result)
			{ cerr << "[Error] The trace cannot be connected! The proof goes wrong!" << endl; return; }
		traceSolver->reportPI(cexTrace.size()-1, 0);
	}
}

void
PdrChecker::collectInd()
{
	const size_t infFrame = frame.size() - 1;
	size_t f = 1;
	for(; f < infFrame && !frame[f].empty(); ++f);
	for(; f < infFrame &&  frame[f].empty(); ++f);
	for(size_t ff = infFrame - 1; ff >= f; --ff)
		for(const PdrCube& c: frame[ff])
		{
			checkSubsumeOthers(c, infFrame, infFrame);
			frame[infFrame].push_back(c);
		}
}

void
PdrChecker::newInitState()
{
	assert(ntk->getLatchNum() != 0);
	AigGateID minId = ntk->getLatchID(0),
	          maxId = minId;
	for(size_t i = 1, L = ntk->getLatchNum(); i < L; ++i)
		if(AigGateID latchId = ntk->getLatchID(i);
		   latchId > maxId)
			maxId = latchId;
		else if(latchId < minId)
			minId = latchId;
	assert(initState.empty());
	initState.init(maxId - minId + 1);
	initIdBase = minId;
}

void
PdrChecker::setAllToDC()
{
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
		setInitValue(ntk->getLatchID(i), ThreeValue_DC);
}

bool
PdrChecker::diffPolar(AigGateLit lit)const
{
	/*
	   | 0 | 1 | X        | 0 | 1 | 2
	---------------    ---------------
	 0 | 0 | 1 | 0  ->  1 | 0 | 1 | 0
	---------------    ---------------
	 1 | 1 | 0 | 0      0 | 1 | 0 | 0

	    ori | 0.0 | 0.1 | 1.0 | 1.1 | | 2.0 | 2.1
	->  ------------------------------------------
	    and |  1  |  0  |  0  |  1  | |  0  |  0 

	from right to left, from MSB to LSB
	b001001 = d9
	*/
	constexpr ThreeValue magicNum = 9;
	return bool((magicNum >> ((getInitValue(getGateID(lit)) << 1) | isInvNum(lit))) & 1);
	// Equivalent to
	constexpr bool LUT[2][3] = {{ true,  false, false },
	                            { false, true,  false }};
	return LUT[isInvNum(lit)][getInitValue(getGateID(lit))];
}

bool
PdrChecker::isInitNumMatch()const
{
	assert(!initState.empty());
	size_t num = 0;
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
		if(getInitValue(ntk->getLatchID(i)) != ThreeValue_DC)
			num += 1;
	return num == numLitInit;
}

void
PdrChecker::printStats()const
{
	if(terSimStat.isON())
		terSimStat->printStat();
	if(satStat.isON())
		satStat->printStat(genType == PDR_GEN_APPROX);
	if(unsatGenStat.isON())
		unsatGenStat->printStat();
	if(propStat.isON())
		propStat->printStat();
	if(recycleStat.isON())
		recycleStat->printStat();
	if(cubeStat.isON())
		cubeStat->printStat();

	#ifdef UsePatternCheckSAT
	sfcMsg << RepeatChar('=', 36) << endl
	       << "matchTry   = " << matchTry   << endl
	       << "matchFrame = " << matchFrame << endl
	       << "matchSAT   = " << matchSAT   << endl
	       << "matchTime  = " << (double(matchTime) / CLOCKS_PER_SEC) << endl;
	#endif

	if(clsStimulator != 0)
		clsStimulator->printStats();
	if(oblStimulator != 0)
		oblStimulator->printStats();

	#ifdef CheckOblCommonPart
	sfcMsg << RepeatChar('=', 36) << endl;
	sfcMsg << "Common part for infinite frame:";
	if(commonPartInf.empty())
		sfcMsg << " None";
	else
		for(AigGateLit lit: commonPartInf)
			sfcMsg << " " << (isInv(lit) ? "!" : "") << getGateID(lit);
	sfcMsg << endl;
//	sfcMsg << "DC:";
//	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
//		if(isDC[ntk->getLatchID(i)])
//			sfcMsg << " " << ntk->getLatchID(i);
//	sfcMsg << endl;
	#endif
}

#ifdef UsePatternCheckSAT
}
#include <fstream>
using namespace std;

namespace _54ff
{
ofstream outFile("stat.out");

void
PdrChecker::checkPatForSat(size_t f, const PdrCube& cube)const
{
	clock_t t = clock();
	if(isVerboseON(PDR_VERBOSE_MISC))
		cout << RepeatChar('-', 36) << endl
		     << "Check pattern (cube): frame = " << f << ", " << cube << endl
		     << RepeatChar('-', 36) << endl;
	genDfsList(cube);
	static_assert(size_t(0xFFFFFFFFFFFFFFFF) == size_t(-1));
//	for(size_t i = 0; i < pattern.size(); ++i)
	for(size_t i = pattern.size() - 1; i != size_t(-1); --i)
	{
		matchTry += 1;
		simResult = false;
		simMask = size_t(-1);
		setPattern(i);
		checkPatFrame(f);
		if(simMask == 0)
			continue;
		size_t value = 0;
		for(AigGateLit lit: cube)
			value |= isInv(lit) ? simValue[getGateID(lit)] : ~simValue[getGateID(lit)];
		simMask &= value;
		if(simMask == 0)
			continue;
		matchFrame += 1;
		simAllAnd();
		value = size_t(-1);
		for(AigGateLit lit: cube)
		{
			AigGate* g = ntk->getGate(getGateID(lit));
			assert(g->getGateType() == AIG_LATCH);
			value &= isInv(lit) ^ g->isFanIn0Inv() ? ~simValue[g->getFanIn0ID()]
				                                   :  simValue[g->getFanIn0ID()];
		}
		if(simMask & value)
			{ matchSAT += 1; simResult = true; matchTime += (clock() - t); outFile << pattern.size() - i << endl; return; }
	}
	matchTime += (clock() - t);
	outFile << -1 << endl;
}

void
PdrChecker::checkPatForSat(size_t f, size_t ignoreIdx)const
{
	clock_t t = clock();
	if(isVerboseON(PDR_VERBOSE_MISC))
		cout << RepeatChar('-', 36) << endl
		     << "Check pattern (genCube): frame = " << f << endl
		     << RepeatChar('-', 36) << endl;
	genDfsList(ignoreIdx);
//	for(size_t i = 0; i < pattern.size(); ++i)
	for(size_t i = pattern.size() - 1; i != size_t(-1); --i)
	{
		matchTry += 1;
		simResult = false;
		simMask = size_t(-1);
		setPattern(i);
		checkPatFrame(f);
		if(simMask == 0)
			continue;
		size_t value = 0;
		for(size_t i = 0; i < genCube.size(); ++i)
			if(i != ignoreIdx)
				value |= isInv(genCube[i]) ?  simValue[getGateID(genCube[i])]
				                           : ~simValue[getGateID(genCube[i])];
		simMask &= value;
		if(simMask == 0)
			continue;
		matchFrame += 1;
		simAllAnd();
		value = size_t(-1);
		for(size_t i = 0; i < genCube.size(); ++i)
			if(i != ignoreIdx)
			{
				AigGate* g = ntk->getGate(getGateID(genCube[i]));
				assert(g->getGateType() == AIG_LATCH);
				value &= isInv(genCube[i]) ^ g->isFanIn0Inv() ? ~simValue[g->getFanIn0ID()]
				                                              :  simValue[g->getFanIn0ID()];
			}
		if(simMask & value)
			{ matchSAT += 1; simResult = true; matchTime += (clock() - t); outFile << pattern.size() - i << endl; return; }
	}
	matchTime += (clock() - t);
	outFile << -1 << endl;
}

void
PdrChecker::checkPatForSat(size_t f)const
{
	clock_t t = clock();
	if(isVerboseON(PDR_VERBOSE_MISC))
		cout << RepeatChar('-', 36) << endl
		     << "Check pattern (property): frame = " << f << endl
		     << RepeatChar('-', 36) << endl;
	genDfsList();
//	for(size_t i = 0; i < pattern.size(); ++i)
	for(size_t i = pattern.size() - 1; i != size_t(-1); --i)
	{
		matchTry += 1;
		simResult = false;
		simMask = size_t(-1);
		setPattern(i);
		checkPatFrame(f);
		if(simMask == 0)
			continue;
		matchFrame += 1;
		simAllAnd();
		AigGate* p = ntk->getGate(property);
		simValue[property] = p->isFanIn0Inv() ? ~simValue[p->getFanIn0ID()]
		                                      :  simValue[p->getFanIn0ID()];
		if(simMask & simValue[property])
			{ matchSAT += 1; simResult = true; matchTime += (clock() - t); outFile << pattern.size() - i << endl; return; }
	}
	matchTime += (clock() - t);
	outFile << -1 << endl;
}

void
PdrChecker::checkPatFrame(size_t f)const
{
	if(f >= frame.size())
		f = frame.size() - 1;
	if(f == 0)
	{
		for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
		{
			simMask &= ~simValue[ntk->getLatchID(i)];
			if(simMask == 0)
				return;
		}
		f += 1;
	}
	for(; f < frame.size(); ++f)
		for(const PdrCube& c: frame[f])
		{
			size_t value = 0;
/*
			for(AigGateLit lit: c)
			{
				assert(ntk->getGate(getGateID(lit))->getGateType() == AIG_LATCH);
				value |= isInv(lit) ? simValue[getGateID(lit)] : ~simValue[getGateID(lit)];
			}
*/
			for(unsigned i = 0; i < c.getSize(); ++i)
				value |= isInv(c.getLit(i)) ?  simValue[getGateID(c.getLit(i))]
				                            : ~simValue[getGateID(c.getLit(i))];
			simMask &= value;
			if(simMask == 0)
				return;
		}
}

void
PdrChecker::genDfsList(const PdrCube& cube)const
{
	dfsList.clear();
	AigGate::setGlobalRef();
	for(unsigned i = 0; i < cube.getSize(); ++i)
		ntk->getGate(getGateID(cube.getLit(i)))->getFanIn0Ptr()->genDfsList(dfsList);
}

void
PdrChecker::genDfsList(size_t ignoreIdx)const
{
	dfsList.clear();
	AigGate::setGlobalRef();
	for(size_t i = 0; i < genCube.size(); ++i)
		if(i != ignoreIdx)
			ntk->getGate(getGateID(genCube[i]))->getFanIn0Ptr()->genDfsList(dfsList);
}

void
PdrChecker::genDfsList()const
{
	dfsList.clear();
	AigGate::setGlobalRef();
	ntk->getGate(property)->getFanIn0Ptr()->genDfsList(dfsList);
}

void
PdrChecker::simAllAnd()const
{
	for(AigAnd* a: dfsList)
	{
		simValue[a->getGateID()]  = a->isFanIn0Inv() ? ~simValue[a->getFanIn0ID()]
		                                             :  simValue[a->getFanIn0ID()];
		simValue[a->getGateID()] &= a->isFanIn1Inv() ? ~simValue[a->getFanIn1ID()]
		                                             :  simValue[a->getFanIn1ID()];
	}
}

void
PdrChecker::setPattern(size_t p)const
{
	size_t idx = 0;
	for(size_t i = 0, I = ntk->getInputNum(); i < I; ++i, ++idx)
		simValue[ntk->getInputID(i)] = pattern[p][idx];
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i, ++idx)
		simValue[ntk->getLatchID(i)] = pattern[p][idx];
}

void
PdrChecker::collectPattern()const
{
	if(patIdx == 64)
		{ pattern.emplace_back(ntk->getInputNum() + ntk->getLatchNum()); patIdx = 0; }
	size_t idx = 0;
	for(size_t i = 0, I = ntk->getInputNum(); i < I; ++i, ++idx)
	{
		pattern.back()[idx] <<= 1;
		assert((pattern.back()[idx] & 1) == 0);
		if(solver->isConverted(ntk->getInputID(i), 0))
			pattern.back()[idx] |= size_t(solver->getValueBool(ntk->getInputID(i), 0));
	}
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i, ++idx)
	{
		pattern.back()[idx] <<= 1;
		assert((pattern.back()[idx] & 1) == 0);
		if(solver->isConverted(ntk->getLatchID(i), 0))
			pattern.back()[idx] |= size_t(solver->getValueBool(ntk->getLatchID(i), 0));
	}
	patIdx += 1;
}
#endif

#ifdef CheckOblCommonPart
void
PdrChecker::checkIsDC(const PdrCube& c)const
{
	for(AigGateLit lit: c)
		isDC[getGateID(lit)] = false;
}

void
PdrChecker::checkInfCommonPart(const PdrTCube& tc)const
{
	if(tc.getFrame() != FRAME_INF)
		return;
	const PdrCube& c = tc.getCube();
//	cout << "Inf " << c << endl;
	if(++numInf == 1)
	{
		assert(commonPartInf.empty());
		for(AigGateLit lit: c)
			commonPartInf.push_back(lit);
	}
	else
	{
		constexpr bool trivial = true;
		if constexpr(trivial)
		{
			size_t s = 0;
			for(size_t i = 0, n = commonPartInf.size(); i < n; ++i)
				if(inList(c, commonPartInf[i]))
					commonPartInf[s++] = commonPartInf[i];
			commonPartInf.resize(s);
		}
		else
		{}
	}

//	if(commonPartInf.empty() || numInf == 100)
//		throw CheckerBreak();
}
#endif

}
