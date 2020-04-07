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

bool
PdrChecker::PdrStimulator::solveCand(const PdrCube& candCube, unsigned mergeType, const PdrCube& oriCube)
{
	sfcMsg.unsetActive();
	if(stimuStat.isON())
		stimuStat->startTime();
	PdrChecker* stimuChecker = checker->cloneChecker(satLimit);
	stimuChecker->setTargetCube(candCube);

//for(const PdrCube& infCube: checker->getInfFrame())
//	stimuChecker->addBlockedCube(PdrTCube(FRAME_INF, infCube));

	cout << "Try " << candCube << flush;
	size_t r;
	switch(stimuChecker->checkInt())
	{
		case PDR_RESULT_UNSAT : r = 0; break;
		case PDR_RESULT_SAT   : r = 1; break;
		default               : r = 2; break;
	}
	const string resultStr[] = { "PASS", "FAIL", "ABORT" };
	cout << " -> " << resultStr[r] << endl;
	if(checker->isVerboseON(PDR_VERBOSE_STIMU))
		cout << "#SAT query = " << stimuChecker->totalSatQuery;
	bool ret = false;
	if(mergeType & (unsigned(1) << r))
	{
		vector<PdrCube> indSet = stimuChecker->getCurIndSet(false);
		checker->mergeInf(indSet, true);
		if(r == 0)
		{
			ret = true;
			vector<PdrCube> tmp{candCube};
			checker->mergeInf(tmp, true);
		}
		else if(!oriCube.isNone())
			for(const PdrCube& c: indSet)
				if(checker->subsume(c, oriCube))
					{ ret = true; break; }
	}

streamsize ss = cout.precision();
cout << fixed << setprecision(3);
cout << "STIMU "
     << (double(stimuChecker->numConf) / (stimuChecker->totalSatQuery)) << " "
     << (double(stimuChecker->numDeci) / (stimuChecker->totalSatQuery)) << " "
     << stimuChecker->maxObl << " " << stimuChecker->maxTree << endl;
cout << setprecision(ss);

	if(r != 0) addToFailed(candCube);
	delete stimuChecker;
	if(stimuStat.isON())
		stimuStat->countOne(r),
		stimuStat->finishTime();
	sfcMsg.setActive();
	return ret;
}

bool
PdrChecker::PdrStimulatorLocalInfAll::stimulateWithOneCube(const PdrTCube& tc)
{
	if(tc.getFrame() != FRAME_INF && onlyInf)
		return false;
	const PdrCube& blockCube = tc.getCube();
	const vector<PdrCube>& targetFrame = tc.getFrame() == FRAME_INF ? checker->getInfFrame()
	                                                                : checker->frame[tc.getFrame()];
	size_t s = targetFrame.size();
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
	bool result = false;
	for(const auto&[candCube, candNum]: candidate)
		if(candNum >= matchNum)
			result = solveCand(candCube, 0b111, blockCube) || result;
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
	return result;
}

bool
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

	bool result = false;
	for(const auto&[candCube, candNum]: candidate)
		if(candNum >= matchNum)
			result = solveCand(candCube, 0b111, blockCube) || result;
	if(!result)
	{
		if(clsCache.size() < backtrackNum)
			clsCache.push_back(tc.getCube());
		else
		{
			clsCache[curIdx++] = tc.getCube();
			if(curIdx == backtrackNum)
				curIdx = 0;
		}
	}
	return result;
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
			solveCand(candCube, 0b111, PdrCube());
}

}
