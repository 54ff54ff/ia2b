/*========================================================================\
|: [Filename] pbcChecker.cpp                                             :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Implement the PBC checker                                  :|
<------------------------------------------------------------------------*/

#include "pbcChecker.h"

namespace _54ff
{

const string pbcStatStr[PBC_STAT_TOTAL] =
{
	"Ternary Simulation",
	"SAT Query for blockSolver",
	"SAT Query for inducSolver",
	"SAT Query for fixedSolver"
};

void
PbcTerSimStat::printStat()const
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
PbcSatStat::printStat()const
{
	cout << RepeatChar('=', 72) << endl
	     << "For " << solverName << endl;
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
		     << "Total SAT number  = " << getNum(1) + getNum(0)                     << endl
		     << "Total SAT runtime = " << getTotalTime(1) + getTotalTime(0) << " s" << endl
		     << setprecision(ss);
	}
}

PbcCube* newPbcCube(const vector<AigGateLit>& litList)
{
	const size_t s = litList.size();
	PbcCube* cube = (PbcCube*)operator new(sizeof(PbcCube) + sizeof(AigGateLit) * s);
	cube->size = s;
	for(size_t i = 0; i < s; ++i)
		cube->lit[i] = litList[i];
	return cube;
}

void delPbcCube(PbcCube* cube)
{
	operator delete(cube);
}

PbcChecker::PbcChecker(AigNtk* ntkToCheck, size_t outputIdx, bool _trace, size_t timeout, size_t maxF,
                       const Array<bool>& stat, bool _blockState, bool _verbose, const char* reachMethod)
: SafetyBNChecker (ntkToCheck, outputIdx, _trace, timeout)
, maxFrame        (maxF)
, blockSolver     (ntk)
, inducSolver     (ntk)
, fixedSolver     (ntk)
, terSimSup       (ntk)

, terSimStat      (stat[PBC_STAT_TERSIM])
, blockSatStat    (stat[PBC_STAT_BLOCK_SAT], "blockSolver")
, inducSatStat    (stat[PBC_STAT_INDUC_SAT], "inducSolver")
, fixedSatStat    (stat[PBC_STAT_FIXED_SAT], "fixedSolver")

, blockState      (_blockState)
, verbose         (_verbose)
{
	sfcMsg << "Max Frame  : " << maxFrame << endl
	       << "Method     : Property directed bounded model checking" << endl
	       << "Detail     : Compute reachability " << reachMethod << " k steps" << endl;
	if(blockState)
		sfcMsg << "             Add blocked cubes back to blockSolver" << endl;
	size_t numStatActive = 0;
	for(unsigned i = 0; i < PBC_STAT_TOTAL; ++i)
		if(stat[i])
			sfcMsg << "             "
			       << (numStatActive++ == 0 ? "Toggle verbose output for "
			                                : "                          ")
			       << "- " << pbcStatStr[i] << endl;

	/* Prepare for ternary simulation */
	genCube.reserve(ntk->getLatchNum());
	terSimSup.initValue();
	terSimSup.setConst0();
	terSimSup.initFanOut();
	terSimSup.initLevel(true);
	terSimSup.initEventList();
	terSimSup.reserveAndNum();

	/* Prepare solvers for diffferent usages */
	// 1. blockSolver: to block notPCube or CTI
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
		blockSolver->convertToCNF(ntk->getLatchID(i), 0);

	// 2. inducSolver: to find notPCube or CTI
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
		inducSolver->convertToCNF(ntk->getLatchID(i), 0);
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
		inducSolver->convertToCNF(ntk->getLatchID(i), 1);
	inducSolver->convertToCNF(property, 0);

	// 3. fixedSolver: to check fixpoint
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
		fixedSolver->convertToCNF(ntk->getLatchID(i), 0);

	/* Create the first timeframe */
	// 0. frame
	frame.emplace_back();

	// 1. blockSolver
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
		blockSolver->addClause(Lit(blockSolver->getVarInt(ntk->getLatchID(i), 0), true));

	// 2. inducSolver
	inducActVar.push_back(inducSolver->newVar());
	Lit inducActInit(inducActVar[0], true);
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
		inducSolver->addClause(inducActInit,
		                       Lit(inducSolver->getVarInt(ntk->getLatchID(i), 0), true));

	inducCheckIdx.push_back(0);
}

PbcChecker::~PbcChecker()
{
	if(terSimStat.isON())
		terSimStat->printStat();
	if(blockSatStat.isON())
		blockSatStat->printStat();
	if(inducSatStat.isON())
		inducSatStat->printStat();
	if(fixedSatStat.isON())
		fixedSatStat->printStat();

	for(const vector<PbcCube*>& f: frame)
		for(PbcCube* c: f)
			delPbcCube(c);
}

void
PbcChecker::check()
{
	try
	{
		while(true)
		{
			if(!blockNotPState())
				{ printCurFrames("F "); cout << "Observe a counter example at frame " << getFrame() << endl; return; }
			refineAllReach();
			if(checkFixPoint())
				{ printCurFrames("F "); cout << "Property proved at frame " << getFrame() << endl; return; }
			if(getFrame() == maxFrame)
				{ cout << "\rCannot determinie the property up to frame " << maxFrame << endl; return; }
			newFrame();
		}
	}
	catch(const CheckerBreak&) { cout << "Cannot determinie the property" << endl; }
}

bool
PbcChecker::blockNotPState()
{
	while(true)
		if(getNotPCube())
			if(blockCube(getFrame()))
				addBlockCube(getFrame());
			else return false;
		else { printCurFrames("N "); return true; }
}

void
PbcChecker::refineAllReach()
{
	for(size_t f = getFrame() - 1; make_signed_t<size_t>(f) >= 1 && refineReach(f); --f);
}

bool
PbcChecker::refineReach(size_t f)
{
	bool change = false;
	while(getCTI(f))
	{
		bool isBlocked = blockCube(f);
		assert(isBlocked);
		addBlockCube(f);
		change = true;
	}
	if(change) printCurFrames("R ");
	return change;
}

void
PbcChecker::newFrame()
{
	newFrameFrame();
	newFrameBlock();
	newFrameInduc();
	newFrameFixed();
}

void
PbcChecker::newFrameFrame()
{
	frame.emplace_back();
}

void
PbcChecker::newFrameBlock()
{
	const size_t f = getFrame();
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
		blockSolver->convertToCNF(ntk->getLatchID(i), f);
}

void
PbcChecker::newFrameInduc()
{
	inducActVar.push_back(inducSolver->newVar());
	inducCheckIdx.push_back(0);
}

bool
PbcChecker::getNotPCube()const
{
	CheckBreakPbc();
	inducSolver->clearAssump();
	inducSolver->addAssump(property, 0, false);
	inducSolver->addAssump(inducActVar[getFrame()], false);
	if(inducSatSolve())
		{ terSim(vector<AigGateID>(1, ntk->getGate(property)->getFanIn0ID())); return true; }
	else return false;
}

bool
PbcChecker::getCTI(size_t f)
{
	const vector<PbcCube*>& nextFrame = frame[f+1];
	size_t& checkIdx = inducCheckIdx[f+1];
	for(; checkIdx < nextFrame.size(); ++checkIdx)
		if(getCTIWithCube(f, nextFrame[checkIdx]))
			return true;
	return false;
}

bool
PbcChecker::getCTIWithCube(size_t f, PbcCube* c)
{
	CheckBreakPbc();
	inducSolver->clearAssump();
	inducSolver->addAssump(inducActVar[f], false);
	addNextStateInduc(c);
	if(inducSatSolve())
		{ terSim(genTarget(c)); return true; }
	else return false;
}

bool
PbcChecker::blockCube(size_t f)
{
	CheckBreakPbc();
	blockSolver->clearAssump();
	addStateBlock(f);
	if(blockSatSolve())
		return false;
	else { unsatGen(f); return true; }
}

void
PbcChecker::addBlockCube(size_t f)
{
	PbcCube* c = newPbcCube(genCube);
	addBlockCubeFrame(f, c);
	addBlockCubeBlock(f, c);
	addBlockCubeInduc(f, c);
	addBlockCubeFixed(f, c);
}

void
PbcChecker::addBlockCubeFrame(size_t f, PbcCube* c)
{
	frame[f].push_back(c);
}

void
PbcChecker::addBlockCubeBlock(size_t f, PbcCube* c)
{
	if(blockState)
	{
		vector<Lit> litList;
		litList.reserve(c->getSize());
		for(AigGateLit lit: genCube)
			litList.emplace_back(blockSolver->getVarInt(getGateID(lit), f), !isInv(lit));
		blockSolver->addClause(litList);
	}
}

void
PbcChecker::addBlockCubeInduc(size_t f, PbcCube* c)
{
	vector<Lit> litList;
	litList.reserve(1 + c->getSize());
	litList.emplace_back(inducActVar[f], true);
	for(AigGateLit lit: genCube)
		litList.emplace_back(inducSolver->getVarInt(getGateID(lit), 0), !isInv(lit));
	inducSolver->addClause(litList);
}

bool
PbcChecker::satSolve(const SolverPtr<CirSolver>& solver, const StatPtr<PbcSatStat>& stat)const
{
	if(stat.isON())
	{
		stat->startTime();
		bool isSAT = solver->solve();
		stat->countOne(isSAT);
		stat->finishTime(isSAT);
		return isSAT;
	}
	else return solver->solve();
}

void
PbcChecker::addStateBlock(size_t f)const
{
	for(AigGateLit lit: genCube)
		blockSolver->addAssump(getGateID(lit), f, isInv(lit));
}

void
PbcChecker::addNextStateInduc(PbcCube* c)const
{
	for(AigGateLit lit: *c)
		inducSolver->addAssump(getGateID(lit), 1, isInv(lit));
}

void
PbcChecker::terSim(const vector<AigGateID>& target)const
{
	if(terSimStat.isON())
		terSimStat->doOneTime(),
		terSimStat->startTime();

	/* Preparation */
	genCube.clear();
	terSimSup.clearDfsList();
	terSimSup.genDfsList(target);
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
		if(AigGateID id = ntk->getLatchID(i); ntk->getGate(id)->isGlobalRef())
			genCube.push_back(makeToLit(id, !inducSolver->getValueBool(id, 0)));

	/* Do First Simulation */
	for(size_t i = 0, I = ntk->getInputNum(); i < I; ++i)
		if(AigGateID id = ntk->getInputID(i); ntk->getGate(id)->isGlobalRef())
			terSimSup.setValue(id, inducSolver->getValueBool(id, 0));
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

	if(terSimStat.isON())
	{
		terSimStat->finishTime();
		terSimStat->incLitCount(genCube.size());
		AigGate::setGlobalRef();
		terSimSup.markDfsCone(target);
		size_t latchInCone = 0;
		for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
			if(ntk->getLatchNorm(i)->isGlobalRef())
				latchInCone += 1;
		terSimStat->incRemoveCount(latchInCone - genCube.size());
	}
}

void
PbcChecker::unsatGen(size_t f)const
{
	size_t j = 0;
	for(size_t i = 0, n = genCube.size(); i < n; ++i)
		if(blockSolver->inConflict(getGateID(genCube[i]), f))
			genCube[j++] = genCube[i];
	genCube.resize(j);
}

void
PbcChecker::printCurFrames(const char* prefix)const
{
	cout << prefix << getFrame() << ": 0";
	for(size_t i = 1, s = frame.size(); i < s; ++i)
		cout << " " << frame[i].size();
	cout << endl;
}

void
PbcChecker::checkBreak(const char* funcName)const
{
	if(checkBreakCond())
	{
		cout << " during " << funcName << endl;
		printCurFrames("U ");
		throw CheckerBreak();
	}
}

PbcAChecker::PbcAChecker(AigNtk* ntkToCheck, size_t outputIdx, bool _trace, size_t timeout, size_t maxF,
                         const Array<bool>& stat, bool _blockState, bool _verbose)
: PbcChecker (ntkToCheck, outputIdx, _trace, timeout, maxF, stat, _blockState, _verbose, "at")
{
	/* Make sure the constant gate is converted */
	fixedSolver->convertToCNF(0, 0);

	// 3. fixedSolver
	fixedActVar.push_back(fixedSolver->newVar());
	vector<Lit> litList;
	litList.reserve(ntk->getLatchNum());
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
		litList.emplace_back(fixedSolver->getVarInt(ntk->getLatchID(i), 0), true);
	fixedSolver->convertOr(fixedActVar[0], false, litList);

	// 3. fixedSolver
	fixedCheckIdx.push_back(-1);
}

bool
PbcAChecker::checkFixPoint()
{
	for(size_t f = 1; f <= getFrame(); ++f)
	{
		if(fixedCheckIdx[f] == frame[f].size())
			continue;
		CheckBreakPbc();
		fixedSolver->clearAssump();
		for(size_t ff = 0; ff < f; ++ff)
			fixedSolver->addAssump(fixedActVar[ff], false);
		fixedSolver->addAssump(fixedActVar[f], true);
		if(!fixedSatSolve()) return true;
		fixedCheckIdx[f] = frame[f].size();
	}
	return false;
}

void
PbcAChecker::addBlockCubeFixed(size_t f, PbcCube* c)
{
	// cube <-> c1 & c2 & .. & ck
	// newAct <-> act | cube
	vector<Lit> litList;
	litList.reserve(c->getSize());
	for(AigGateLit lit: *c)
		litList.emplace_back(fixedSolver->getVarInt(getGateID(lit), 0), isInv(lit));
	const Var cube = fixedSolver->newVar();
	fixedSolver->convertAnd(cube, false, litList);
	const Var newAct = fixedSolver->newVar();
	fixedSolver->convertOr(newAct, false, cube, false, fixedActVar[f], false);
	fixedActVar[f] = newAct;
}

vector<AigGateID>
PbcAChecker::genTarget(PbcCube* c)const
{
	vector<AigGateID> target;
	target.reserve(c->getSize());
	for(AigGateLit lit: *c)
		target.push_back(ntk->getGate(getGateID(lit))->getFanIn0ID());
	return target;
}

void
PbcAChecker::newFrameFixed()
{
	fixedActVar.push_back(fixedSolver->getVarInt(0, 0));
	fixedCheckIdx.push_back(-1);
}

PbcUChecker::PbcUChecker(AigNtk* ntkToCheck, size_t outputIdx, bool _trace, size_t timeout, size_t maxF,
                         const Array<bool>& stat, bool _blockState, bool _verbose)
: PbcChecker (ntkToCheck, outputIdx, _trace, timeout, maxF, stat, _blockState, _verbose, "up to")
{
	/* Transform the AIG network */
	AigGateV select(ntk->createInput(), false);
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
	{
		AigLatch* l = ntk->getLatch(i);
		AigGateV newFanIn = ntk->createAnd(select, l->getFanIn0());
		l->setFanIn0(newFanIn.getValue());
	}

	// 3. fixedSolver
	fixedActVar.push_back(var_Undef);
	genCube.push_back(0);
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
	{
		genCube[0] = makeToLit(ntk->getLatchID(i), true);
		frame[0].push_back(newPbcCube(genCube));
	}
	genCube.clear();

	fixedCheckIdx.push_back(0);
}

bool
PbcUChecker::checkFixPoint()
{
	for(size_t f = 1; f <= getFrame(); ++f)
	{
		const vector<PbcCube*>& prevFrame = frame[f-1];
		size_t& checkIdx = fixedCheckIdx[f-1];
		for(; checkIdx < prevFrame.size(); ++checkIdx)
		{
			CheckBreakPbc();
			fixedSolver->clearAssump();
			fixedSolver->addAssump(fixedActVar[f], false);
			addStateFixed(prevFrame[checkIdx]);
			if(fixedSatSolve())
				goto NOT_FIXED;
		}
		return true;
		NOT_FIXED: {}
	}
	return false;
}

void
PbcUChecker::newFrameFixed()
{
	fixedActVar.push_back(fixedSolver->newVar());
	fixedCheckIdx.push_back(0);
}

void
PbcUChecker::addBlockCubeFixed(size_t f, PbcCube* c)
{
	vector<Lit> litList;
	litList.reserve(1 + c->getSize());
	litList.emplace_back(fixedActVar[f], true);
	for(AigGateLit lit: genCube)
		litList.emplace_back(fixedSolver->getVarInt(getGateID(lit), 0), !isInv(lit));
	fixedSolver->addClause(litList);
}

vector<AigGateID>
PbcUChecker::genTarget(PbcCube* c)const
{
	vector<AigGateID> target;
	target.reserve(c->getSize());
	for(AigGateLit lit: *c)
		target.push_back(ntk->getGate(getGateID(lit))->getFanIn0Ptr()->getFanIn1ID());
	return target;
}

void
PbcUChecker::addStateFixed(PbcCube* c)const
{
	for(AigGateLit lit: *c)
		fixedSolver->addAssump(getGateID(lit), 0, isInv(lit));
}

/**************************************/
/**************************************/
/**************************************/
/**************************************/
/**************************************/
/**************************************/

PbcIChecker::PbcIChecker(AigNtk* ntkToCheck, size_t outputIdx, bool _trace, size_t timeout, size_t maxF,
                       const Array<bool>& stat, bool _verbose)
: SafetyBNChecker (ntkToCheck, outputIdx, _trace, timeout)
, maxFrame        (maxF)
, blkIndSolver    (ntk)
, fixedSolver     (ntk)
, terSimSup       (ntk)

, terSimStat      (stat[PBC_STAT_TERSIM])
, blockSatStat    (stat[PBC_STAT_BLOCK_SAT], "blkIndSolver (block part)")
, inducSatStat    (stat[PBC_STAT_INDUC_SAT], "blkIndSolver (induc part)")
, fixedSatStat    (stat[PBC_STAT_FIXED_SAT], "fixedSolver")

, verbose         (_verbose)
{
	sfcMsg << "Max Frame  : " << maxFrame << endl
	       << "Method     : Property directed bounded model checking" << endl
	       << "Detail     : Compute reachability at k steps, block increasingly" << endl;
	size_t numStatActive = 0;
	for(unsigned i = 0; i < PBC_STAT_TOTAL; ++i)
		if(stat[i])
			sfcMsg << "             "
			       << (numStatActive++ == 0 ? "Toggle verbose output for "
			                                : "                          ")
			       << "- " << pbcStatStr[i] << endl;

	/* Prepare for ternary simulation */
	genCube.reserve(ntk->getLatchNum());
	terSimSup.initValue();
	terSimSup.setConst0();
	terSimSup.initFanOut();
	terSimSup.initLevel(true);
	terSimSup.initEventList();
	terSimSup.reserveAndNum();

	/* Prepare solvers for diffferent usages */
	// 1. blkIndSolver: to find and block notPCube or CTI
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
		blkIndSolver->convertToCNF(ntk->getLatchID(i), 0);
	blkIndSolver->convertToCNF(property, 0);

	// 2. fixedSolver: to check fixpoint
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
		fixedSolver->convertToCNF(ntk->getLatchID(i), 0);

	/* Create the first timeframe */
	// 0. frame
	frame.emplace_back();

	// 1. blkIndSolver
	inducActVar.push_back(blkIndSolver->newVar());
	Lit inducActInit(inducActVar[0], true);
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
		blkIndSolver->addClause(inducActInit,
		                        Lit(blkIndSolver->getVarInt(ntk->getLatchID(i), 0), true));

	inducCheckIdx.push_back(0);

	/* Make sure the constant gate is converted */
	fixedSolver->convertToCNF(0, 0);

	// 2. fixedSolver
	fixedActVar.push_back(fixedSolver->newVar());
	vector<Lit> litList;
	litList.reserve(ntk->getLatchNum());
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
		litList.emplace_back(fixedSolver->getVarInt(ntk->getLatchID(i), 0), true);
	fixedSolver->convertOr(fixedActVar[0], false, litList);

	fixedCheckIdx.push_back(-1);
}

PbcIChecker::~PbcIChecker()
{
	if(terSimStat.isON())
		terSimStat->printStat();
	if(blockSatStat.isON())
		blockSatStat->printStat();
	if(inducSatStat.isON())
		inducSatStat->printStat();
	if(fixedSatStat.isON())
		fixedSatStat->printStat();

	for(const vector<PbcCube*>& f: frame)
		for(PbcCube* c: f)
			delPbcCube(c);
}

void
PbcIChecker::check()
{
	try
	{
		while(true)
		{
			if(!blockNotPState())
				{ printCurFrames("F "); cout << "Observe a counter example at frame " << getFrame() << endl; return; }
			refineAllReach();
			if(checkFixPoint())
				{ printCurFrames("F "); cout << "Property proved at frame " << getFrame() << endl; return; }
			if(getFrame() == maxFrame)
				{ cout << "\rCannot determinie the property up to frame " << maxFrame << endl; return; }
			newFrame();
		}
	}
	catch(const CheckerBreak&) { cout << "Cannot determinie the property" << endl; }
}

bool
PbcIChecker::blockNotPState()
{
	while(true)
		if(getNotPCube())
			if(blockCube(getFrame()))
				addBlockCube(getFrame());
			else return false;
		else { printCurFrames("N "); return true; }
}

void
PbcIChecker::refineAllReach()
{
	for(size_t f = getFrame() - 1; make_signed_t<size_t>(f) >= 1 && refineReach(f); --f);
}

bool
PbcIChecker::refineReach(size_t f)
{
	bool change = false;
	while(getCTI(f))
	{
		bool isBlocked = blockCube(f);
		assert(isBlocked);
		addBlockCube(f);
		change = true;
	}
	if(change) printCurFrames("R ");
	return change;
}

bool
PbcIChecker::checkFixPoint()
{
	for(size_t f = 1; f <= getFrame(); ++f)
	{
		if(fixedCheckIdx[f] == frame[f].size())
			continue;
		CheckBreakPbcI();
		fixedSolver->clearAssump();
		for(size_t ff = 0; ff < f; ++ff)
			fixedSolver->addAssump(fixedActVar[ff], false);
		fixedSolver->addAssump(fixedActVar[f], true);
		if(!fixedSatSolve()) return true;
		fixedCheckIdx[f] = frame[f].size();
	}
	return false;
}

void
PbcIChecker::newFrame()
{
	newFrameFrame();
	newFrameBlock();
	newFrameInduc();
	newFrameFixed();
}

void
PbcIChecker::newFrameFrame()
{
	frame.emplace_back();
}

void
PbcIChecker::newFrameBlock()
{
	const size_t f = getFrame();
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
		blkIndSolver->convertToCNF(ntk->getLatchID(i), f);
}

void
PbcIChecker::newFrameInduc()
{
	inducActVar.push_back(blkIndSolver->newVar());
	inducCheckIdx.push_back(0);
}

void
PbcIChecker::newFrameFixed()
{
	fixedActVar.push_back(fixedSolver->getVarInt(0, 0));
	fixedCheckIdx.push_back(-1);
}

bool
PbcIChecker::getNotPCube()const
{
	CheckBreakPbcI();
	blkIndSolver->clearAssump();
	blkIndSolver->addAssump(property, 0, false);
	blkIndSolver->addAssump(inducActVar[getFrame()], false);
	if(inducSatSolve())
		{ terSim(vector<AigGateID>(1, ntk->getGate(property)->getFanIn0ID())); return true; }
	else return false;
}

bool
PbcIChecker::getCTI(size_t f)
{
	const vector<PbcCube*>& nextFrame = frame[f+1];
	size_t& checkIdx = inducCheckIdx[f+1];
	for(; checkIdx < nextFrame.size(); ++checkIdx)
		if(getCTIWithCube(f, nextFrame[checkIdx]))
			return true;
	return false;
}

bool
PbcIChecker::getCTIWithCube(size_t f, PbcCube* c)
{
	CheckBreakPbcI();
	blkIndSolver->clearAssump();
	blkIndSolver->addAssump(inducActVar[f], false);
	addNextStateInduc(c);
	if(inducSatSolve())
		{ terSim(genTarget(c)); return true; }
	else return false;
}

bool
PbcIChecker::blockCube(size_t f)
{
	CheckBreakPbcI();
	for(size_t ff = 1; ff <= f; ++ff)
	{
		blkIndSolver->clearAssump();
		blkIndSolver->addAssump(inducActVar[f-ff], false);
		addStateBlock(ff);
		if(!blockSatSolve())
			{ unsatGen(ff); return true; }
	}
	return false;
}

void
PbcIChecker::addBlockCube(size_t f)
{
	PbcCube* c = newPbcCube(genCube);
	addBlockCubeFrame(f, c);
	addBlockCubeBlock(f, c);
	addBlockCubeInduc(f, c);
	addBlockCubeFixed(f, c);
}

void
PbcIChecker::addBlockCubeFrame(size_t f, PbcCube* c)
{
	frame[f].push_back(c);
}

void
PbcIChecker::addBlockCubeBlock(size_t, PbcCube*)
{
}

void
PbcIChecker::addBlockCubeInduc(size_t f, PbcCube* c)
{
	vector<Lit> litList;
	litList.reserve(1 + c->getSize());
	litList.emplace_back(inducActVar[f], true);
	for(AigGateLit lit: genCube)
		litList.emplace_back(blkIndSolver->getVarInt(getGateID(lit), 0), !isInv(lit));
	blkIndSolver->addClause(litList);
}

void
PbcIChecker::addBlockCubeFixed(size_t f, PbcCube* c)
{
	// cube <-> c1 & c2 & .. & ck
	// newAct <-> act | cube
	vector<Lit> litList;
	litList.reserve(c->getSize());
	for(AigGateLit lit: *c)
		litList.emplace_back(fixedSolver->getVarInt(getGateID(lit), 0), isInv(lit));
	const Var cube = fixedSolver->newVar();
	fixedSolver->convertAnd(cube, false, litList);
	const Var newAct = fixedSolver->newVar();
	fixedSolver->convertOr(newAct, false, cube, false, fixedActVar[f], false);
	fixedActVar[f] = newAct;
}

vector<AigGateID>
PbcIChecker::genTarget(PbcCube* c)const
{
	vector<AigGateID> target;
	target.reserve(c->getSize());
	for(AigGateLit lit: *c)
		target.push_back(ntk->getGate(getGateID(lit))->getFanIn0ID());
	return target;
}

bool
PbcIChecker::satSolve(const SolverPtr<CirSolver>& solver, const StatPtr<PbcSatStat>& stat)const
{
	if(stat.isON())
	{
		stat->startTime();
		bool isSAT = solver->solve();
		stat->countOne(isSAT);
		stat->finishTime(isSAT);
		return isSAT;
	}
	else return solver->solve();
}

void
PbcIChecker::addStateBlock(size_t f)const
{
	for(AigGateLit lit: genCube)
		blkIndSolver->addAssump(getGateID(lit), f, isInv(lit));
}

void
PbcIChecker::addNextStateInduc(PbcCube* c)const
{
	for(AigGateLit lit: *c)
		blkIndSolver->addAssump(getGateID(lit), 1, isInv(lit));
}

void
PbcIChecker::terSim(const vector<AigGateID>& target)const
{
	if(terSimStat.isON())
		terSimStat->doOneTime(),
		terSimStat->startTime();

	/* Preparation */
	genCube.clear();
	terSimSup.clearDfsList();
	terSimSup.genDfsList(target);
	for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
		if(AigGateID id = ntk->getLatchID(i); ntk->getGate(id)->isGlobalRef())
			genCube.push_back(makeToLit(id, !blkIndSolver->getValueBool(id, 0)));

	/* Do First Simulation */
	for(size_t i = 0, I = ntk->getInputNum(); i < I; ++i)
		if(AigGateID id = ntk->getInputID(i); ntk->getGate(id)->isGlobalRef())
			terSimSup.setValue(id, blkIndSolver->getValueBool(id, 0));
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

	if(terSimStat.isON())
	{
		terSimStat->finishTime();
		terSimStat->incLitCount(genCube.size());
		AigGate::setGlobalRef();
		terSimSup.markDfsCone(target);
		size_t latchInCone = 0;
		for(size_t i = 0, L = ntk->getLatchNum(); i < L; ++i)
			if(ntk->getLatchNorm(i)->isGlobalRef())
				latchInCone += 1;
		terSimStat->incRemoveCount(latchInCone - genCube.size());
	}
}

void
PbcIChecker::unsatGen(size_t f)const
{
	size_t j = 0;
	for(size_t i = 0, n = genCube.size(); i < n; ++i)
		if(blkIndSolver->inConflict(getGateID(genCube[i]), f))
			genCube[j++] = genCube[i];
	genCube.resize(j);
}

void
PbcIChecker::printCurFrames(const char* prefix)const
{
	cout << prefix << getFrame() << ": 0";
	for(size_t i = 1, s = frame.size(); i < s; ++i)
		cout << " " << frame[i].size();
	cout << endl;
}

void
PbcIChecker::checkBreak(const char* funcName)const
{
	if(checkBreakCond())
	{
		cout << " during " << funcName << endl;
		printCurFrames("U ");
		throw CheckerBreak();
	}
}

}