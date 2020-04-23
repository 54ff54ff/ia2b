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
	return ret;
}

bool
PdrCube::exactOneLess(const PdrCube& c)const
{
	return getSize() + 1 == c.getSize() && subsume(c);
}

PdrChecker::PdrStimulator::~PdrStimulator()
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

	cout << "Try " << candCube << flush;
	const string resultStr[] = { "PASS", "FAIL", "ABORT" };
	size_t r;

	if(shareType == PDR_SHARE_ALL)
	{
		// Store the parameter
		size_t curF = checker->curFrame;
		checker->curFrame = 1;
		size_t maxF = checker->maxFrame;
		checker->maxFrame = MAX_SIZE_T;
		checker->setTargetCube(candCube);
		checker->stimulator = 0;
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
		cout << " -> " << resultStr[r] << endl;
		if(verbose)
			cout << "#SAT query = " << checker->totalSatQuery << endl;
		if(r == 1) addToFailed(candCube);

		// Restore the parameter
		checker->curFrame = curF;
		checker->maxFrame = maxF;
		checker->resetTargetCube();
		checker->stimulator = this;
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
	}
	else
	{
		PdrChecker* stimuChecker = checker->cloneChecker(satLimit);
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
		cout << " -> " << resultStr[r] << endl;
		if(checker->isVerboseON(PDR_VERBOSE_STIMU))
			cout << "#SAT query = " << stimuChecker->totalSatQuery;

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
				checker->mergeInf(stimuChecker->getCurIndSet(false), true);
			if(r != 0) addToFailed(candCube);
		}

streamsize ss = cout.precision();
cout << fixed << setprecision(3);
cout << "STIMU "
     << (double(stimuChecker->numConf) / (stimuChecker->totalSatQuery)) << " "
     << (double(stimuChecker->numDeci) / (stimuChecker->totalSatQuery)) << " "
     << stimuChecker->maxObl << " " << stimuChecker->maxTree << endl;
cout << setprecision(ss);

		delete stimuChecker;
	}

	if(stimuStat.isON())
		stimuStat->countOne(r),
		stimuStat->finishTime();
	sfcMsg.setActive();
}

void
PdrChecker::PdrStimulatorLocalInfAll::stimulateWithOneCube(const PdrTCube& tc)
{
	if(tc.getFrame() != FRAME_INF && onlyInf)
		return;
	const PdrCube& blockCube = tc.getCube();
	const vector<PdrCube>& targetFrame = tc.getFrame() == FRAME_INF ? checker->getInfFrame()
	                                                                : checker->frame[tc.getFrame()];
//	assert(blockCube == targetFrame.back());
	size_t s = targetFrame.size() - 1;
	size_t i = backtrackNum > s ? 0 : s - backtrackNum;
	vector<pair<PdrCube, size_t>> candidate;
//cout << RepeatChar('-', 36) << endl;
//cout << blockCube << endl;
//cout << RepeatChar('=', 36) << endl;
	for(; i < s; ++i)
	{
//cout << targetFrame[i] << endl;

//for(const auto&[candCube, candNum]: candidate)
//	cout << candCube << " " << candNum << endl;
//cout << RepeatChar('-', 36) << endl;
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

//cout << RepeatChar('-', 36) << endl;
//for(const auto&[candCube, candNum]: candidate)
//	cout << candCube << " " << candNum << endl;

//	sfcMsg.unsetActive();
	for(const auto&[candCube, candNum]: candidate)
		if(candNum >= matchNum)
			solveCand(candCube, 0b111);

/*
	for(const auto&[candCube, candNum]: candidate)
		if(candNum >= matchNum)
		{
			if(stimuStat.isON())
				stimuStat->startTime();
			PdrChecker* stimuChecker = checker->cloneChecker();
			stimuChecker->setTargetCube(candCube);
			cout << "Try " << candCube << flush;
			size_t r;
			switch(stimuChecker->checkInt())
			{
				case PDR_RESULT_UNSAT : cout << " -> PASS";  result = true;         r = 0; break;
				case PDR_RESULT_SAT   : cout << " -> FAIL";  addToFailed(candCube); r = 1; break;
				default               : cout << " -> ABORT"; addToFailed(candCube); r = 2; break;
			}
			checker->mergeInf(stimuChecker->getCurIndSet(false), true);
			cout << ", " << stimuChecker->totalSatQuery << endl;
			delete stimuChecker;
			if(stimuStat.isON())
				stimuStat->countOne(r),
				stimuStat->finishTime();
		}
	sfcMsg.setActive();
*/
//cout << RepeatChar('-', 36) << endl;
}

void
PdrChecker::PdrStimulatorLocalMix::stimulateWithOneCube(const PdrTCube& tc)
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
PdrChecker::PdrStimulatorHalf::stimulateAtEndOfFrame()
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

}
