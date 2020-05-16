/*========================================================================\
|: [Filename] pdrSupport.cpp                                             :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Implement some utilities to support PDR checker            :|
<------------------------------------------------------------------------*/

#include "pdrChecker.h"

namespace _54ff
{

PdrCube
PdrCube::exactOneDiff(const PdrCube& c)const
{
	PdrCube ret;
	if(getSize() != c.getSize() || getSize() == 1)
		return ret;

	const unsigned s = getSize();
	size_t diffIdx = MAX_SIZE_T;
	constexpr bool trivial = true;
	if constexpr(trivial)
	{
		for(unsigned i = 0; i < s; ++i)
		{
			bool match = false;
			for(unsigned j = 0; j < s; ++j)
				if(getLit(i) == c.getLit(j))
					{ match = true; break; }
			if(!match)
			{
				if(diffIdx != MAX_SIZE_T)
					return ret;
				else diffIdx = i;
			}
		}
	}
	else
	{
		//TODO
	}

	if(diffIdx == MAX_SIZE_T)
		{ assert(*this == c); return ret; }
	ret.allocMem(s-1);
	ret.initCount();
	ret.setSize(s-1);
	ret.initAbstract();
	unsigned i = 0;
	for(; i < diffIdx; ++i)
		ret.setLit(i, getLit(i)), ret.calAbstract(i);
	for(++i; i < s; ++i)
		ret.setLit(i-1, getLit(i)), ret.calAbstract(i-1);
	for(i = 1; i < s-1; ++i)
		assert(ret.getLit(i-1) < ret.getLit(i));
	ret.incCount();
	ret.initMark();
	ret.setPrevCube(0);
	ret.setBadDepth(0);
	return ret;
}

bool
PdrCube::exactOneLess(const PdrCube& c)const
{
	return getSize() + 1 == c.getSize() && subsume(c);
}

void
PdrChecker::PdrStimulator::printStats()const
{
	if(stimuStat.isON())
		stimuStat->printStat();
}

void
PdrChecker::PdrStimulator::solveCand(const PdrCube& candCube, unsigned mergeType)
{
	sfcMsg.unsetActive();
	if(stimuStat.isON())
		stimuStat->startTime();

	if(checker->isVerboseON(PDR_VERBOSE_STIMU))
		cout << "Try " << candCube << flush;
	const string resultStr[] = { "PASS", "FAIL", "ABORT" };
	size_t r;

	if(shareType == PDR_SHARE_ALL)
	{
		// Mark the infinite frame
		if(stimuStat.isON())
			for(PdrCube& c: checker->frame.back())
				{ assert(!c.getMarkB()); c.setMarkB(true); }

		// Store the parameter
		size_t curF = checker->curFrame;
		checker->curFrame = 1;
		size_t maxF = checker->maxFrame;
		checker->maxFrame = MAX_SIZE_T;
		checker->setTargetCube(candCube);
		PdrClsStimulator* clsStimuor = checker->clsStimulator;
		checker->clsStimulator = 0;
		PdrOblStimulator* oblStimuor = checker->oblStimulator;
		checker->oblStimulator = 0;
		vector<deque<PdrCube>> badDV(move(checker->badDequeVec));
		checker->badDequeVec.resize(checker->oblType == PDR_OBL_PUSH ? checker->frame.size()
		                                                             : checker->frame.size() - 1);
		bool checkII = checker->checkIndInv;
		checker->checkIndInv = false;
		size_t totalSQ = checker->totalSatQuery;
		checker->totalSatQuery = 0;
		size_t satQL = checker->satQueryLimit;
		checker->satQueryLimit = satLimit;
		size_t oblTS = checker->oblTreeSize;
		size_t numO = checker->numObl;
		checker->numObl = 0;
		size_t numOL = checker->numOblLimit;
		checker->numOblLimit = 0;
		bool verbose = checker->isVerboseON(PDR_VERBOSE_STIMU);
		size_t _verbosity = checker->verbosity;
		checker->verbosity = 0;

		switch(checker->checkInt())
		{
			case PDR_RESULT_UNSAT : r = 0; break;
			case PDR_RESULT_SAT   : r = 1; break;
			default               : r = 2; break;
		}
		if(verbose)
			cout << " -> " << resultStr[r] << endl
			     << "#SAT query = " << checker->totalSatQuery << endl;
		if(r == 1) addToFailed(candCube);

		// Restore the parameter
		checker->curFrame = curF;
		checker->maxFrame = maxF;
		checker->resetTargetCube();
		checker->clsStimulator = clsStimuor;
		checker->oblStimulator = oblStimuor;
		checker->badDequeVec.swap(badDV);
		checker->badDequeVec.resize(checker->oblType == PDR_OBL_PUSH ? checker->frame.size()
		                                                             : checker->frame.size() - 1);
		checker->checkIndInv = checkII;
		checker->totalSatQuery = totalSQ;
		checker->satQueryLimit = satQL;
		checker->oblTreeSize = oblTS;
		checker->numObl = numO;
		checker->numOblLimit = numOL;
		checker->verbosity = _verbosity;

		// Count the infinite frame
		if(stimuStat.isON())
		{
			size_t numNewCls = 0;
			for(PdrCube& c: checker->frame.back())
				if(c.getMarkB())
					{ numNewCls += 1; c.setMarkB(false); }
			stimuStat->incInfClsNum(numNewCls);
		}
	}
	else
	{
		PdrChecker* stimuChecker = checker->cloneChecker();
		stimuChecker->setSatLimit(satLimit);
		stimuChecker->disablePrintFrame();
		stimuChecker->setTargetCube(candCube);
		if(shareType == PDR_SHARE_INF)
		{
			// Be careful, we already know stimuChecker is done currently
			// If we still need frame.back(), modify this line
			stimuChecker->frame.back().swap(checker->frame.back());
			stimuChecker->addBlockedCubeInf();
			for(PdrCube& c: stimuChecker->frame.back())
				{ assert(!c.getMarkB()); c.setMarkB(true); }
		}

		switch(stimuChecker->checkInt())
		{
			case PDR_RESULT_UNSAT : r = 0; break;
			case PDR_RESULT_SAT   : r = 1; break;
			default               : r = 2; break;
		}
		if(checker->isVerboseON(PDR_VERBOSE_STIMU))
			cout << " -> " << resultStr[r] << endl
			     << "#SAT query = " << stimuChecker->totalSatQuery;

		if(shareType == PDR_SHARE_INF)
		{
			if(mergeType & (unsigned(1) << r))
			{
				stimuChecker->collectInd();
				// Be careful, we already know stimuChecker is done currently
				// If we still need stimuChecker, modify this line
				checker->frame.back().swap(stimuChecker->frame.back());
				vector<PdrCube>& infFrame = checker->frame.back();
				size_t numNewCls = 0;
				for(const PdrCube& c: infFrame)
					if(!c.getMarkB())
					{
						checker->checkSubsumeOthers(c, 1, checker->frame.size() - 2);
						checker->addBlockedCubeFrame(FRAME_INF, c);
						numNewCls += 1;
					}
				if(checker->isVerboseON(PDR_VERBOSE_STIMU))
					cout << ", #new clauses = " << numNewCls << endl;
				incInfClsNum(numNewCls);
				for(PdrCube& c: infFrame)
					if(c.getMarkB())
						c.setMarkB(false);
			}
//			if(r == 1) addToFailed(candCube);
			if(r != 0) addToFailed(candCube);
		}
		else
		{
			if(mergeType & (unsigned(1) << r))
			{
				size_t numNewCls = checker->mergeInf(stimuChecker->getCurIndSet(false), true);
				incInfClsNum(numNewCls);
			}
			if(r != 0) addToFailed(candCube);
		}
		delete stimuChecker;
	}

	if(stimuStat.isON())
		stimuStat->countOne(r),
		stimuStat->finishTime();
	sfcMsg.setActive();
}

void
PdrChecker::PdrClsStimulatorLocalInfAll::stimulateWithOneCube(const PdrTCube& tc)
{
/*
size_t b = backtrackNum, m = matchNum;
if(tc.getFrame() == FRAME_INF)
	backtrackNum = 20, matchNum = 1;
else
	backtrackNum = 10, matchNum = 2;
*/

	if(tc.getFrame() != FRAME_INF && onlyInf)
		return;
	const PdrCube& blockCube = tc.getCube();
	const vector<PdrCube>& targetFrame = tc.getFrame() == FRAME_INF ? checker->getInfFrame()
	                                                                : checker->frame[tc.getFrame()];
	size_t s = targetFrame.size() - 1;
	size_t i = backtrackNum > s ? 0 : s - backtrackNum;
	vector<pair<PdrCube, size_t>> candidate;
	for(; i < s; ++i)
	{
		bool finish = false;
		for(auto&[candCube, candNum]: candidate)
			if(candCube.exactOneLess(targetFrame[i]))
				{ candNum += 1; finish = true; break; }
		if(finish) continue;
		PdrCube common = blockCube.exactOneDiff(targetFrame[i]);
		if(!common.isNone() && notFailed(common)) {
			if(checker->isInitial(common)) addToFailed(common);
			else                           candidate.emplace_back(common, 1); }
	}

	for(const auto&[candCube, candNum]: candidate)
		if(candNum >= matchNum)
			solveCand(candCube, 0b111);

//backtrackNum = b, matchNum = m;
}

void
PdrChecker::PdrClsStimulatorLocalMix::stimulateWithOneCube(const PdrTCube& tc)
{
	const PdrCube& blockCube = tc.getCube();
	vector<pair<PdrCube, size_t>> candidate;
	for(const PdrCube& c: clsCache)
	{
		bool finish = false;
		for(auto&[candCube, candNum]: candidate)
			if(candCube.exactOneLess(c))
				{ candNum += 1; finish = true; break; }
		if(finish) continue;
		PdrCube common = blockCube.exactOneDiff(c);
		if(!common.isNone() && notFailed(common)) {
			if(checker->isInitial(common)) addToFailed(common);
			else                           candidate.emplace_back(common, 1); }
	}

	for(const auto&[candCube, candNum]: candidate)
		if(candNum >= matchNum)
			solveCand(candCube, 0b111);

	if(clsCache.size() < backtrackNum)
		clsCache.push_back(tc.getCube());
	else
	{
		clsCache[curIdx++] = tc.getCube();
		if(curIdx == backtrackNum)
			curIdx = 0;
	}
}

void
PdrChecker::PdrClsStimulatorHalf::stimulateAtEndOfFrame()
{
	const vector<PdrCube>& inf = checker->getInfFrame();
	
	vector<pair<PdrCube, size_t>> candidate;
	for(size_t i = 0, s = inf.size(); i < s; ++i)
		for(size_t j   = i + 1,
		           end = i + observeNum > s ? s : i + observeNum; j < end; ++j)
		{
			PdrCube common = inf[i].exactOneDiff(inf[j]);
			if(!common.isNone() && notFailed(common))
			{
				if(checker->isInitial(common))
					addToFailed(common);
				else
				{
					bool newCand = true;
					for(const auto& candPair: candidate)
						if(candPair.first == common)
							{ newCand = false; break; }
					if(newCand)
					{
						size_t num = 0;
						for(const PdrCube& c: inf)
							if(common.subsume(c))
								num += 1;
						candidate.emplace_back(common, num);
					}
				}
			}
		}

	for(const auto&[candCube, candNum]: candidate)
		if(candNum >= matchNum)
			solveCand(candCube, 0b111);
}

void
PdrChecker::PdrOblStimulatorAll::resetInt()
{
	numObl = 0;
	commonPart.clear();
}

void
PdrChecker::PdrOblStimulatorAll::stimulateInt()
{
	if(!commonPart.empty())
	{
		PdrCube candidate(commonPart);
		if(notFailed(candidate)) {
			if(checker->isInitial(candidate)) addToFailed(candidate);
			else                              solveCand(candidate, 0b111); }
	}
	else if(checker->isVerboseON(PDR_VERBOSE_STIMU))
		cout << "No common part of proof obligation!" << endl;
}

void
PdrChecker::PdrOblStimulatorAll::checkCommonPart(const PdrCube& c)
{
	if(++numObl == 1)
	{
		assert(commonPart.empty());
		for(AigGateLit lit: c)
			commonPart.push_back(lit);
	}
	else
	{
		constexpr bool trivial = true;
		if constexpr(trivial)
		{
			size_t s = 0;
			for(size_t i = 0, n = commonPart.size(); i < n; ++i)
				if(inList(c, commonPart[i]))
					commonPart[s++] = commonPart[i];
			commonPart.resize(s);
		}
		else
		{}
	}
}

void
PdrChecker::PdrOblStimulatorAll::printCommon()const
{
	cout << RepeatChar('=', 36) << endl;
	if(numObl == 0)
		cout << "No proof obligation!" << endl;
	else
	{
		cout << "Number of proof obligation = " << numObl << endl
		     << "Common part:";
		if(commonPart.empty())
			cout << " None";
		else
			for(AigGateLit lit: commonPart)
				cout << " " << (isInv(lit) ? "!" : "") << getGateID(lit);
		cout << endl;
	}
}

void
PdrChecker::PdrOblStimulatorDepth::resetInt()
{
	for(size_t& n: numObl)
		n = 0;
	for(vector<AigGateLit>& cp: commonPart)
		cp.clear();
}

void
PdrChecker::PdrOblStimulatorDepth::stimulateInt()
{
	size_t b = commonPart.size() - 1;
	for(; b != size_t(-1) && commonPart.empty(); --b);
	for(; b != size_t(-1); --b)
		if(!commonPart[b].empty())
		{
			PdrCube candidate(commonPart[b]);
			if(notFailed(candidate)) {
				if(checker->isInitial(candidate)) addToFailed(candidate);
				else                              solveCand(candidate, 0b111); }
		}
		else if(checker->isVerboseON(PDR_VERBOSE_STIMU))
			cout << "No common part of proof obligation for bad depth " << b << "!" << endl;
}

void
PdrChecker::PdrOblStimulatorDepth::checkCommonPart(const PdrCube& c)
{
	assert(numObl.size() == commonPart.size());
	const unsigned badDepth = c.getBadDepth();
	if(badDepth >= commonPart.size())
	{
		assert(badDepth == commonPart.size());
		numObl.push_back(0);
		commonPart.emplace_back();
	}
	vector<AigGateLit>& common = commonPart[badDepth];
	if(++numObl[badDepth] == 1)
	{
		assert(common.empty());
		for(AigGateLit lit: c)
			common.push_back(lit);
	}
	else
	{
		constexpr bool trivial = true;
		if constexpr(trivial)
		{
			size_t s = 0;
			for(size_t i = 0, n = common.size(); i < n; ++i)
				if(inList(c, common[i]))
					common[s++] = common[i];
			common.resize(s);
		}
		else
		{}
	}
}

void
PdrChecker::PdrOblStimulatorDepth::printCommon()const
{
	cout << RepeatChar('=', 36) << endl;
	size_t end = 0;
	for(size_t n = numObl.size(); end < n && numObl[end] != 0; ++end);
	if(end == 0)
		cout << "No proof obligation!" << endl;
	else
	{
		cout << "Number of proof obligation:" << endl;
		for(size_t i = 0; i < end; ++i)
			cout << i << ": " << numObl[i] << endl;
		cout << RepeatChar('-', 36) << endl;
		cout << "Common part:" << endl;
		for(size_t i = 0; i < end; ++i)
		{
			cout << i << ":";
			if(commonPart[i].empty())
				cout << " None";
			else
				for(AigGateLit lit: commonPart[i])
					cout << " " << (isInv(lit) ? "!" : "") << getGateID(lit);
			cout << endl;
		}
	}
}

}
