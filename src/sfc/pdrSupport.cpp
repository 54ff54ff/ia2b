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
	}

	if(diffIdx == MAX_SIZE_T)
		{ assert(*this == c); return ret; }
	ret.uint32Ptr = (unsigned*)operator new(sizeof(unsigned) * (s - 1) +
	                                        sizeof(size_t)   * 3);
	ret.uint64Ptr += 3;
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
PdrChecker::PdrStimulator::solveCand(const PdrCube& candCube, size_t satLimit, const PdrCube& oriCube)
{
	sfcMsg.unsetActive();
	if(stimuStat.isON())
		stimuStat->startTime();
	PdrChecker* stimuChecker = checker->cloneChecker(satLimit);
	stimuChecker->setTargetCube(candCube);
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
	constexpr unsigned mergeType = 0b111;
	bool ret = mergeType & (unsigned(1) << r)
	         ? checker->mergeInf(stimuChecker->getCurIndSet(false), true, oriCube)
	         : false;

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
PdrChecker::PdrStimulatorLocalInfAll::stimulate(const PdrTCube& tc)
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
			result = solveCand(candCube, 300, blockCube) || result;
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
PdrChecker::PdrStimulatorLocalMix::stimulate(const PdrTCube& tc)
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
			result = solveCand(candCube, 300, blockCube) || result;
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

}
