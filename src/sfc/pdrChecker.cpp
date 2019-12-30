/*========================================================================\
|: [Filename] pdrChecker.cpp                                             :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Implement the PDR checker                                  :|
<------------------------------------------------------------------------*/

#include <queue>
#include <sstream>
#include "pdrChecker.h"
#include "condStream.h"
using namespace std;

namespace _54ff
{

const string pdrStatStr[PDR_STAT_TOTAL] =
{
	"Ternary Simulation",
	"SAT Query",
	"UNSAT Generalization",
	"Cube Propagation",
	"Solver Recycling"
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
PdrSatStat::printStat()const
{
	cout << RepeatChar('=', 72) << endl;
	if(getNum(0) + getNum(1) == 0)
		cout << "No SAT query!" << endl;
	else
	{
		streamsize ss = cout.precision();
		cout << fixed << setprecision(3)
		     << "  SAT number      = " << getNum(1)                                 << endl
		     << "  SAT runtime     = " << getTotalTime(1)                   << " s" << endl
		     << "UNSAT number      = " << getNum(0)                                 << endl
		     << "UNSAT runtime     = " << getTotalTime(0)                   << " s" << endl
		     << "Total number      = " << getNum(1) + getNum(0)                     << endl
		     << "Total runtime     = " << getTotalTime(1) + getTotalTime(0) << " s" << endl
		     << "Last solving time = " << getTotalTime(2)                   << " s" << endl
		     << setprecision(ss)
		     << "Min   SAT decision = " << minSAT_D   << endl
		     << "Max UNSAT decision = " << maxUNSAT_D << endl;
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
		     << "Number of cubes added to Inf by eager propagation = " << getNum(1) << endl
		     << "Number of literals removed in Inf                 = " << getNum(2) << endl
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
		     << "Max number at recycling     = " << getNum(1)              << endl
		     << "Runtime on solver recycling = " << getTotalTime() << " s" << endl
		     << setprecision(ss);
	}
}

ostream& operator<<(ostream& os, const PdrCube& c)
{
	os << "Size = " << c.getSize()
	   << ", Lit =";
	for(size_t i = 0; i < c.getSize(); ++i)
	{
		os << " ";
		if(isInv(c.getLit(i))) os << "!";
		os << getGateID(c.getLit(i));
	}
	return os;
}

PdrCube::PdrCube(const vector<unsigned>& litList, bool toSort)
{
	const size_t s = litList.size();
	uint32Ptr = (unsigned*)operator new(sizeof(unsigned) * s +
	                                    sizeof(size_t)   * 4);
	uint64Ptr += 4;
	initCount();
	setSize(s);
	initAbstract();
	for(size_t i = 0; i < s; ++i)
		setLit(i, litList[i]), calAbstract(i);
	if(toSort)
		sort(uint32Ptr, uint32Ptr + s);
	for(size_t i = 1; i < s; ++i)
		assert(getLit(i-1) < getLit(i));
	incCount();
}

bool
PdrCube::subsumeTrivial(const PdrCube& c)const
{
	for(size_t i = 0; i < getSize(); ++i)
	{
		bool match = false;
		for(size_t j = 0; j < c.getSize(); ++j)
			if(getLit(i) == c.getLit(j))
				{ match = true; break; }
		if(!match) return false;
	}
	return true;
}

bool
PdrCube::subsumeComplex(const PdrCube& c)const
{
	if(getSize() > c.getSize() ||
	   (getAbs() & ~c.getAbs()) != 0)
		return false;
	for(size_t i = 0, j = 0; i < getSize(); ++i, ++j)
	{
		for(; j < c.getSize(); ++j)
			if(getLit(i) == c.getLit(j))
				goto OK;
			else if(getGateID(getLit(i)) <= getGateID(c.getLit(j)))
				return false;
		return false;
		OK: {}
	}
	return true;
}

PdrChecker::PdrChecker(AigNtk* ntkToCheck, size_t outputIdx, bool _trace, size_t timeout, size_t maxF, size_t recycleVN, const Array<bool>& stat,
                       PdrSimType simT, PdrOrdType ordT, PdrOblType oblT, PdrDeqType deqT, PdrPrpType prpT, PdrGenType genT,
                       bool rInf, bool cInNeed, bool _verbose)
: SafetyBChecker (ntkToCheck, outputIdx, _trace, timeout)
, curFrame       (0)
, maxFrame       (maxF)
, unusedVarNum   (0)
, recycleVarNum  (recycleVN)
, solver         (ntk)
, terSimSup      (ntk)
, actInc         (1.0)

, terSimStat     (stat[PDR_STAT_TERSIM])
, satStat        (stat[PDR_STAT_SAT])
, unsatGenStat   (stat[PDR_STAT_GENERALIZE])
, propStat       (stat[PDR_STAT_PROPAGATE])
, recycleStat    (stat[PDR_STAT_RECYCLE])

, simType        (simT)
, ordType        (ordT)
, oblType        (oblT)
, deqType        (deqT)
, prpType        (prpT)
, genType        (genT)
, toRefineInf    (rInf)
, convertInNeed  (cInNeed)

, verbose        (_verbose)
{
	cout << "Max Frame  : " << maxFrame << endl
	     << "Method     : Property directed reachability" << endl
	     << "Detail     : Number of unused variable to recycle the solver = ";
	if(recycleVarNum == 0) cout << "Infinity"; else cout << recycleVarNum;
	cout << endl;
	cout << "             ";
	switch(simType)
	{
		case PDR_SIM_FORWARD_NORMAL    : cout << "Forward ternary simulation, Normal mode";              break;
		case PDR_SIM_FORWARD_EVENT     : cout << "Forward ternary simulation, Event-driven mode";        break;
		case PDR_SIM_BACKWARD_NORMAL   : cout << "BackWard SAT generalization, Only latch variable";     break;
		case PDR_SIM_BACKWARD_INTERNAL : cout << "BackWard SAT generalization, Involve internal signal"; break;
	}
	cout << endl;
	cout << "             ";
	switch(ordType)
	{
		case PDR_ORD_INDEX            : cout << "Static order, Follow index";                     break;
		case PDR_ORD_ACTIVITY         : cout << "Dynamic order, Follow activity";                 break;
		case PDR_ORD_ACTIVITY_REVERSE : cout << "Dynamic order, Follow reverse activity";         break;
		case PDR_ORD_DECAY            : cout << "Dynamic order, Follow decayed activity";         break;
		case PDR_ORD_DECAY_REVERSE    : cout << "Dynamic order, Follow reverse decayed activity"; break;
	}
	cout << endl;
	cout << "             ";
	switch(oblType)
	{
		case PDR_OBL_NORMAL : cout << "Find new proof obligation at each new timeframe";     break;
		case PDR_OBL_PUSH   : cout << "Preserve all proof obligations during the procedure"; break;
		case PDR_OBL_IGNORE : cout << "Never push proof obligation to further frame";        break;
	}
	cout << endl;
	cout << "             ";
	switch(deqType)
	{
		case PDR_DEQ_STACK : cout << "Apply stack-like behavior when getting bad cubes"; break;
		case PDR_DEQ_QUEUE : cout << "Apply queue-like behavior when getting bad cubes"; break;
	}
	cout << endl;
	cout << "             ";
	switch(prpType)
	{
		case PDR_PRP_NORMAL : cout << "Propagate blocking cubes up to the currently maximum timeframe";  break;
		case PDR_PRP_EAGER  : cout << "Propagate blocking cubes out of the currently maximum timeframe"; break;
	}
	cout << endl;
	cout << "             ";
	switch(genType)
	{
		case PDR_GEN_NORMAL : cout << "Try to remove a literal once a time during UNSAT generalization";                    break;
		case PDR_GEN_APPROX : cout << "Try to remove a literal once a time during UNSAT generalization by approximate SAT"; break;
		case PDR_GEN_IGNORE : cout << "Do not try to remove any literal during UNSAT generalization";                       break;
	}
	cout << endl;
	if(toRefineInf)
		cout << "             Try to further refine timeframe Inf" << endl;
	if(convertInNeed)
		cout << "             Convert CNF formula of latch only if needed" << endl;
	size_t numStatActive = 0;
	for(unsigned i = 0; i < PDR_STAT_TOTAL; ++i)
		if(stat[i])
			cout << "             "
			     << (numStatActive++ == 0 ? "Toggle verbose output for "
			                              : "                          ")
			     << "- " << pdrStatStr[i] << endl;

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

	/* Convert all circuit for a timeframe */
	convertCNF();

	/* Prepare for the initial state and infinite frame */
	frame.emplace_back();
	newFrame();
	addInitState();

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
}

PdrChecker::~PdrChecker()
{
	if(terSimStat.isON())
		terSimStat->printStat();
	if(satStat.isON())
		satStat->printStat();
	if(unsatGenStat.isON())
		unsatGenStat->printStat();
	if(propStat.isON())
		propStat->printStat();
	if(recycleStat.isON())
		recycleStat->printStat();

	#ifdef UsePatternCheckSAT
	cout << RepeatChar('=', 36) << endl
	     << "matchTry   = " << matchTry   << endl
	     << "matchFrame = " << matchFrame << endl
	     << "matchSAT   = " << matchSAT   << endl
	     << "matchTime  = " << (double(matchTime) / CLOCKS_PER_SEC) << endl;
	#endif
}

void
PdrChecker::check()
{
	try
	{
		while(true)
		{
			if(oblType == PDR_OBL_PUSH)
				if(!recBlockCube(PdrTCube(FRAME_NULL, PdrCube())))
					{ cout << "Observe a counter example at frame " << curFrame << endl; return; }

			if(PdrCube notPCube = getNotPCube();
			   notPCube.isNone())
			{
				//finish blocking
				if(curFrame == maxFrame)
					{ cout << "Cannot determinie the property up to frame " << maxFrame << endl; return; }
				if(propBlockedCubes())
					{ cout << "Property proved at frame " << curFrame << endl; return; }
			}
			else if(!recBlockCube(PdrTCube(curFrame, notPCube)))
				{ cout << "Observe a counter example at frame " << curFrame << endl; return; }
		}
	}
	catch(const CheckerBreak&) { cout << "Cannot determinie the property" << endl; }
}

vector<AigGateID>
PdrChecker::genTarget(const PdrCube& c)const
{
	vector<AigGateID> target;
	target.reserve(c.getSize());
	for(size_t i = 0; i < c.getSize(); ++i)
		target.push_back(ntk->getGate(getGateID(c.getLit(i)))->getFanIn0ID());
	return target;
}

bool
PdrChecker::recBlockCube(const PdrTCube& notPTCube)
{
	auto returnBy = [this](bool value) -> bool
		{ printCurFrames(value ? "B " : "F "); return value; };

	assert(notPTCube.getFrame() == curFrame ||
	       (oblType == PDR_OBL_PUSH && notPTCube.getFrame() == FRAME_NULL));
	for(size_t i = 0, f = oblType == PDR_OBL_PUSH ? curFrame : curFrame + 1; i < f; ++i)
		assert(badDequeVec[i].empty());

	if(notPTCube.getFrame() != FRAME_NULL)
		badDequeVec[notPTCube.getFrame()].push_back(notPTCube.getCube());
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
	while(checkFrame < maxFrame)
	{
		assert(!badDequeVec[checkFrame].empty());
		if(checkFrame == 0)
			return returnBy(false);
		const PdrCube& badCube = deqType == PDR_DEQ_STACK ? badDequeVec[checkFrame].back()
		                                                  : badDequeVec[checkFrame].front();
		if(verbose)
			cout << RepeatChar('-', 36) << endl
			     << "Check: frame = " << checkFrame
			     << ", " << badCube << endl
			     << RepeatChar('-', 36) << endl;
		PdrTCube badTCube(checkFrame, badCube);
		if(size_t blockFrame = isBlocked(badTCube); blockFrame == FRAME_NULL)
		{
			assert(!isInitial(badCube));
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
							badDequeVec[newTCube.getFrame()+1].push_back(badCube);
				checkEmpty();
			}
			else //SAT, not blocked
				badDequeVec[--checkFrame].push_back(newTCube.getCube());
			checkRecycle();
		}
		else
		{
			if(oblType == PDR_OBL_PUSH && blockFrame != FRAME_INF)
				badDequeVec[blockFrame+1].push_back(badCube);
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
	for(; f < maxF; ++f)
		for(const PdrCube& frameCube: frame[f])
			if(frameCube.subsume(badTCube.getCube()))
				goto DONE;
DONE:
	assert(f <= maxF);
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
	for(size_t i = 0; i < c.getSize(); ++i)
		genCube.push_back(c.getLit(i));
	if(isOrdDynamic())
		sortGenCubeByAct();
	size_t newF = s.getFrame();

	/* Calculate the number of positive literal */
	size_t numPos = 0;
	for(size_t i = 0, n = genCube.size(); i < n; ++i)
		if(!isInv(genCube[i]))
			numPos += 1;
	assert(numPos != 0);
	for(size_t i = 0, n = genCube.size(); i < n; ++i)
		assert(solver->isConverted(getGateID(genCube[i]), 1));

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
//				checkRecycle();
			}
			else i += 1;
	}

	if(unsatGenStat.isON())
		unsatGenStat->finishRemoveTime();

	/* Try to propagate to further timeframe */
	if(unsatGenStat.isON())
		unsatGenStat->startTime();

	while(newF < curFrame)
	{
		#ifdef UsePatternCheckSAT
		checkPatForSat(newF, numeric_limits<size_t>::max());
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
//		checkRecycle();
		if(Break) break;
	}

	if(unsatGenStat.isON())
		unsatGenStat->finishPushTime(),
		unsatGenStat->incRemoveCount(s.getCube().getSize() - genCube.size());

	if(verbose)
	{
		PdrTCube newTCube(newF, PdrCube(genCube, isOrdDynamic()));
		cout << RepeatChar('-', 36) << endl
		     << "UNSAT generalization: frame = ";
		if(newTCube.getFrame() == FRAME_INF) cout << "Inf"; else cout << newTCube.getFrame();
		cout << ", " << newTCube.getCube() << endl
		     << RepeatChar('-', 36) << endl;
		return newTCube;
	}
	else return PdrTCube(newF, PdrCube(genCube, isOrdDynamic()));
}

bool
PdrChecker::propBlockedCubes()
{
	auto returnBy = [this](bool value) -> bool
		{ printCurFrames(value ? "F " : "P "); return value; };

	if(++curFrame == 1)
		{ newFrame(); return false; }

	if(propStat.isON())
		propStat->doOneTime(),
		propStat->startTime();

	for(size_t f = 1; true; ++f)
	{
		assert(f != frame.size() - 1);
		if(f == curFrame && prpType == PDR_PRP_NORMAL)
			break;
		if(f >= curFrame && frame[f].empty())
			{ assert(prpType == PDR_PRP_EAGER); break; }
		if(f + 1 == frame.size() - 1)
			newFrame();

		vector<PdrCube> tmp(frame[f]);
		for(const PdrCube& c: tmp)
			if(c.isCount(2))
			{
				if(PdrTCube s = solveRelative(PdrTCube(f+1, c), NOIND);
				   s.getFrame() != FRAME_NULL)
				{
					if(verbose)
					{
						cout << RepeatChar('-', 36) << endl
						     << "Propagate cube from frame " << f << " to ";
						if(s.getFrame() == FRAME_INF) cout << "Inf"; else cout << s.getFrame();
						cout << ":"                       << endl
						     << "Before: " << c           << endl
						     << "After:  " << s.getCube() << endl
						     << RepeatChar('-', 36)       << endl;
					}
					addBlockedCube(s, f);
				}
				CheckBreakPdr(false);
			}
			else assert(c.isCount(1));
		if(frame[f].empty())
		{
			if(f < curFrame)
			{
				if(f != curFrame - 1)
					curFrame -= 1;
				return returnBy(true);
			}
			assert(prpType == PDR_PRP_EAGER);
			f += 1;
			const size_t inf = frame.size() - 1;
			assert(f < inf);
			for(; f < inf; ++f)
			{
				for(const PdrCube& c: frame[f])
				{
					if(verbose)
					{
						cout << RepeatChar('-', 36) << endl
						     << "Propagate cube by induction from frame " << f << " to Inf";
						cout << ":" << c            << endl
						     << RepeatChar('-', 36) << endl;
					}
					addBlockedCube(PdrTCube(FRAME_INF, c), inf);
					if(propStat.isON())
						propStat->incInfCubeCount();
				}
				frame[f].clear();
			}
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
	solver->clearAssump();
	solver->addAssump(property, 0, false);
	activateFrame(curFrame);
	return satSolve() ? terSim(vector<AigGateID>(1, ntk->getGate(property)->getFanIn0ID())) : PdrCube();
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
	for(size_t i = 0; i < cube.getSize(); ++i)
		if(!isInv(cube.getLit(i)))
			return false;
	return true;
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
	if(convertInNeed) convertCNF(s.getCube());
	addNextState(s.getCube());
	return satSolve() ? PdrTCube(FRAME_NULL, type == EXTRACT ? terSim(genTarget(s.getCube())) : PdrCube())
	                  : PdrTCube(findLowestActPlus1(s.getFrame()-1), unsatGen(s.getCube()));
}

void
PdrChecker::newFrame()
{
	frame.push_back(move(frame.back()));
	frame[frame.size()-2].clear();
	actVar.push_back(solver->newVar());
	badDequeVec.emplace_back();
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
	for(size_t f = initF; f <= k; ++f)
	{
		size_t s = 0;
		for(size_t i = 0, n = frame[f].size(); i < n; ++i)
			if(!blockCube.subsume(frame[f][i]))
			{
				if(s < i)
					frame[f][s++] = move(frame[f][i]);
				else { assert(s == i); s += 1; }
			}
		frame[f].resize(s);
	}

	for(size_t f = 0; f < frame.size(); ++f)
		for(size_t i = 0; i < frame[f].size(); ++i)
			assert(!frame[f][i].isNone());

	if(isOrdDynamic())
	{
		for(size_t i = 0; i < blockCube.getSize(); ++i)
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
	vector<Lit> litList;
	if(blockFrame != FRAME_INF)
		litList.push_back(Lit(actVar[blockFrame], true));
	for(size_t i = 0; i < blockCube.getSize(); ++i)
		litList.push_back(Lit(solver->getVarInt(getGateID(blockCube.getLit(i)), 0),
		                      !isInv(blockCube.getLit(i))));
	solver->addClause(litList);

	if(verbose)
	{
		cout << RepeatChar('-', 36) << endl
		     << "Add blocked Cube: frame = ";
		if(blockFrame == FRAME_INF) cout << "Inf"; else cout << blockFrame;
		cout << ", " << blockCube << endl
		     << RepeatChar('-', 36) << endl;
	}
}

PdrCube
PdrChecker::terSim(const vector<AigGateID>& target)const
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
	PdrCube c(genCube, true);
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
		terSimStat->incRemoveCount(latchInCone - c.getSize());
	}
	if(verbose)
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
PdrChecker::terSimBackwardInternal(const vector<AigGateID>& target)const
{}

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
	vector<Lit> litList;
	litList.push_back(Lit(act, true));
	for(size_t i = 0; i < c.getSize(); ++i)
		litList.push_back(Lit(solver->getVarInt(getGateID(c.getLit(i)), 0),
		                      !isInv(c.getLit(i))));
	solver->addClause(litList);
	solver->addAssump(act, false);
	return act;
}

void
PdrChecker::addState(const PdrCube& c, size_t level)const
{
	for(size_t i = 0; i < c.getSize(); ++i)
		solver->addAssump(solver->getVarInt(getGateID(c.getLit(i)), level),
		                  isInv(c.getLit(i)));
}

void
PdrChecker::activateFrame(size_t f)const
{
	for(size_t maxF = actVar.size(); f < maxF; ++f)
		solver->addAssump(actVar[f], false);
}

PdrCube
PdrChecker::unsatGen(const PdrCube& c)const
{
	assert(!isInitial(c));
	genCube.clear();
	for(size_t i = 0; i < c.getSize(); ++i)
		if(solver->inConflict(getGateID(c.getLit(i)), 1))
			genCube.push_back(c.getLit(i));
	if(isInitial(genCube))
	{
		for(size_t i = c.getSize() - 1; i != size_t(-1); --i)
			if(!isInv(c.getLit(i)))
			{
				size_t j = genCube.size();
				genCube.push_back(c.getLit(i));
				for(; j > 0 && c.getLit(i) < genCube[j-1]; --j)
					genCube[j] = genCube[j-1];
				genCube[j] = c.getLit(i);
				break;
			}
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
	for(size_t i = 0, n = cubeVec.size(); i < n; ++i)
		if(!isInv(cubeVec[i]) && i != ignoreIdx)
			return false;
	return true;
}

Var
PdrChecker::addCurNotState(const vector<AigGateLit>& cubeVec, size_t ignoreIdx)const
{
	const Var act = solver->newVar();
	vector<Lit> litList;
	litList.push_back(Lit(act, true));
	for(size_t i = 0, n = cubeVec.size(); i < n; ++i)
		if(i != ignoreIdx)
			litList.push_back(Lit(solver->getVarInt(getGateID(cubeVec[i]), 0),
			                      !isInv(cubeVec[i])));
	solver->addClause(litList);
	solver->addAssump(act, false);
	return act;
}

void
PdrChecker::addNextState(const vector<AigGateLit>& cubeVec, size_t ignoreIdx)const
{
	for(size_t i = 0, n = cubeVec.size(); i < n; ++i)
		if(i != ignoreIdx)
			solver->addAssump(solver->getVarInt(getGateID(cubeVec[i]), 1),
			                  isInv(cubeVec[i]));
}

void
PdrChecker::printCurFrames(const char* prefix)const
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

void
PdrChecker::printRemainObl()const
{
	cout << "Remaining proof obligation:";
	for(const deque<PdrCube>& f: badDequeVec)
		cout << " " << f.size();
	cout << endl;
}

void
PdrChecker::checkBreak(const char* funcName, bool blockNow)const
{
	if(checkBreakCond())
	{
		cout << " during " << funcName << endl;
		printCurFrames("U ");
		if(blockNow) printRemainObl();
		throw CheckerBreak();
	}
}

bool
PdrChecker::satSolve()const
{
	size_t d = solver->getDecisionNum();
	if(satStat.isON())
		satStat->startTime();
	bool isSAT = solver->solve();
	if(satStat.isON())
		satStat->countOne(isSAT),
		satStat->finishTime(isSAT),
		satStat->setLastTime();
	d = solver->getDecisionNum() - d;
	if(satStat.isON())
		isSAT ? satStat->setSD(d) : satStat->setUD(d);
	#ifdef UsePatternCheckSAT
	assert(!simResult || isSAT);
	if(isSAT) collectPattern();
	#endif
	return isSAT;


	if(satStat.isON())
	{
		size_t d = solver->getDecisionNum();
		satStat->startTime();
		bool isSAT = solver->solve();
		satStat->countOne(isSAT);
		satStat->finishTime(isSAT);
		satStat->setLastTime();
		d = solver->getDecisionNum() - d;
		isSAT ? satStat->setSD(d) : satStat->setUD(d);
		#ifdef UsePatternCheckSAT
		assert(!simResult || isSAT);
		if(isSAT) collectPattern();
		#endif
		return isSAT;
	}
	else return solver->solve();
}

bool
PdrChecker::satSolveLimited()const
{
	solver->setDeciLimit(200);
	return solver->solveLimited() != l_False;
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
	if(recycleVarNum != 0 && unusedVarNum >= recycleVarNum)
	{
		if(recycleStat.isON())
			recycleStat->doOneTime(),
			recycleStat->checkMaxRecNum(unusedVarNum),
			recycleStat->startTime();
		unusedVarNum = 0;
		solver->resetSolver();
		convertCNF();
		for(Var& act: actVar)
			act = solver->newVar();
		addInitState();
		vector<Lit> litList;
		for(size_t i = 1; i <= curFrame; ++i)
			for(const PdrCube& c: frame[i])
			{
				litList.push_back(Lit(actVar[i], true));
				for(size_t i = 0; i < c.getSize(); ++i)
					litList.push_back(Lit(solver->getVarInt(getGateID(c.getLit(i)), 0),
										  !isInv(c.getLit(i))));
				solver->addClause(litList);
				litList.clear();
			}
		for(const PdrCube& c: frame.back())
		{
			for(size_t i = 0; i < c.getSize(); ++i)
				litList.push_back(Lit(solver->getVarInt(getGateID(c.getLit(i)), 0),
									  !isInv(c.getLit(i))));
			solver->addClause(litList);
			litList.clear();
		}
		if(recycleStat.isON())
			recycleStat->finishTime();
	}
}

void
PdrChecker::convertCNF()const
{
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
		solver->convertToCNF(ntk->getLatchID(i), 0);
	if(!convertInNeed)
		for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
			solver->convertToCNF(ntk->getLatchID(i), 1);
	solver->convertToCNF(property, 0);
}

void
PdrChecker::convertCNF(const PdrCube& c)const
{
	for(size_t i = 0; i < c.getSize(); ++i)
		solver->convertToCNF(getGateID(c.getLit(i)), 1);
}

void
PdrChecker::addInitState()
{
	Lit actInit(actVar[0], true);
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
		solver->addClause(actInit,
		                  Lit(solver->getVarInt(ntk->getLatchID(i), 0), true));
}

void
PdrChecker::refineInf()
{
	for(PdrCube& c: frame.back())
	{
		if(convertInNeed) convertCNF(c);
		const PdrTCube newTCube = generalize(PdrTCube(FRAME_INF, c));
		assert(newTCube.getFrame() == FRAME_INF);
		const PdrCube& newCube = newTCube.getCube();
		if(newCube.getSize() != c.getSize())
		{
			assert(newCube.getSize() < c.getSize());
			if(verbose)
				cout << RepeatChar('-', 36) << endl
				     << "Further decrease " << (c.getSize() - newCube.getSize()) << " literals!" << endl
				     << RepeatChar('-', 36) << endl;
			if(propStat.isON())
				propStat->incInfLitCount(c.getSize() - newCube.getSize());
			c = newCube;

			vector<Lit> litList;
			for(size_t i = 0; i < newCube.getSize(); ++i)
				litList.push_back(Lit(solver->getVarInt(getGateID(newCube.getLit(i)), 0),
				                  !isInv(newCube.getLit(i))));
			solver->addClause(litList);
		}
		checkRecycle();
	}
}

#ifdef UsePatternCheckSAT
void
PdrChecker::checkPatForSat(size_t f, const PdrCube& cube)const
{
	clock_t t = clock();
	if(verbose)
		cout << RepeatChar('-', 36) << endl
		     << "Check pattern (cube): frame = " << f << ", " << cube << endl
		     << RepeatChar('-', 36) << endl;
	genDfsList(cube);
	static_assert(size_t(0xFFFFFFFFFFFFFFFF) == size_t(-1));
	for(size_t i = 0; i < pattern.size(); ++i)
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
			{ matchSAT += 1; simResult = true; matchTime += (clock() - t); return; }
	}
	matchTime += (clock() - t);
}

void
PdrChecker::checkPatForSat(size_t f, size_t ignoreIdx)const
{
	clock_t t = clock();
	if(verbose)
		cout << RepeatChar('-', 36) << endl
		     << "Check pattern (genCube): frame = " << f << endl
		     << RepeatChar('-', 36) << endl;
	genDfsList(ignoreIdx);
	for(size_t i = 0; i < pattern.size(); ++i)
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
			{ matchSAT += 1; simResult = true; matchTime += (clock() - t); return; }
	}
	matchTime += (clock() - t);
}

void
PdrChecker::checkPatForSat(size_t f)const
{
	clock_t t = clock();
	if(verbose)
		cout << RepeatChar('-', 36) << endl
		     << "Check pattern (property): frame = " << f << endl
		     << RepeatChar('-', 36) << endl;
	genDfsList();
	for(size_t i = 0; i < pattern.size(); ++i)
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
			{ matchSAT += 1; simResult = true; matchTime += (clock() - t); return; }
	}
	matchTime += (clock() - t);
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
			for(size_t i = 0; i < c.getSize(); ++i)
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
	for(size_t i = 0; i < cube.getSize(); ++i)
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

}
