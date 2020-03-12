/*========================================================================\
|: [Filename] pdrChecker.h                                               :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Define the checker about property directed reachability    :|
<------------------------------------------------------------------------*/

#ifndef HEHE_PDRCHECKER_H
#define HEHE_PDRCHECKER_H

#include <deque>
#include "sfcChecker.h"
#include "cirSolver.h"
#include "aigMisc1.h"
#include "stat.h"
#include "alg.h"
using namespace std;

//#define UsePatternCheckSAT
//#define CheckOblCommonPart

namespace _54ff
{

enum PdrSimType
{
	PDR_SIM_FORWARD_NORMAL,
	PDR_SIM_FORWARD_EVENT,
	PDR_SIM_BACKWARD_NORMAL,
	PDR_SIM_BACKWARD_INTERNAL
};

enum PdrOrdType
{
	PDR_ORD_INDEX            = 0b100,
	PDR_ORD_ACTIVITY         = 0b000,
	PDR_ORD_ACTIVITY_REVERSE = 0b001,
	PDR_ORD_DECAY            = 0b010,
	PDR_ORD_DECAY_REVERSE    = 0b011
};

enum PdrOblType
{
	PDR_OBL_NORMAL,
	PDR_OBL_PUSH,
	PDR_OBL_IGNORE
};

enum PdrDeqType
{
	PDR_DEQ_STACK,
	PDR_DEQ_QUEUE
};

enum PdrPrpType
{
	PDR_PRP_NORMAL,
	PDR_PRP_EAGER
};

enum PdrGenType
{
	PDR_GEN_NORMAL,
	PDR_GEN_APPROX,
	PDR_GEN_IGNORE
};

enum PdrStatType
{
	PDR_STAT_TERSIM = 0,
	PDR_STAT_SAT,
	PDR_STAT_GENERALIZE,
	PDR_STAT_PROPAGATE,
	PDR_STAT_RECYCLE,
	PDR_STAT_CUBE,
	PDR_STAT_STIMU,
	PDR_STAT_TOTAL
};

inline size_t getPdrStatMask(PdrStatType pst) { return size_t(1) << pst; }
inline bool isPdrStatON(size_t stats, PdrStatType pst) { return stats & getPdrStatMask(pst); }

enum PdrVerboseType
{
	PDR_VERBOSE_OBL = 0,
	PDR_VERBOSE_GEN,
	PDR_VERBOSE_PROP,
	PDR_VERBOSE_BLK,
	PDR_VERBOSE_TERSIM,
	PDR_VERBOSE_CUBE,
	PDR_VERBOSE_INF,
	PDR_VERBOSE_MISC,
	PDR_VERBOSE_FINAL,
	PDR_VERBOSE_STIMU,
	PDR_VERBOSE_TOTAL,
	PDR_VERBOSE_FRAME = PDR_VERBOSE_TOTAL
};

inline size_t getPdrVbsMask(PdrVerboseType pvt) { return size_t(1) << pvt; }

extern const string pdrStatStr[PDR_STAT_TOTAL];
extern const string pdrVerboseStr[PDR_VERBOSE_TOTAL];

enum PdrResultType
{
	PDR_RESULT_SAT,
	PDR_RESULT_UNSAT,
	PDR_RESULT_ABORT_FRAME,
	PDR_RESULT_ABORT_RES
};

enum PdrStimuType
{
	PDR_STIMU_LOCAL_INF,
	PDR_STIMU_LOCAL_ALL,
	PDR_STIMU_LOCAL_MIX,
	PDR_STIMU_NONE
};

class PdrTerSimStat : public Stat<3, 1>
{
public:
	void incLitCount   (size_t n) { countN(1, n); }
	void incRemoveCount(size_t n) { countN(2, n); }

	void printStat()const;
};

class PdrSatStat : public Stat<5, 4>
{
public:
	PdrSatStat()
	: Stat<5, 4> ()
	, minSAT_D   (-1)
	, maxUNSAT_D (0)
	, maxAbort_D (0) {}

	void setLastTime() { setTime(3); }
	void incDeciNum(size_t n) { countN(3, n); }
	void incConfNum(size_t n) { countN(4, n); }

	void printStat(bool)const;

	void setSD(size_t d) { if(d < minSAT_D)   minSAT_D   = d; }
	void setUD(size_t d) { if(d > maxUNSAT_D) maxUNSAT_D = d; }
	void setAD(size_t d) { if(d > maxAbort_D) maxAbort_D = d; }

protected:
	size_t  minSAT_D, maxUNSAT_D, maxAbort_D;
};

class PdrUnsatGenStat : public Stat<2, 2>
{
public:
	void incRemoveCount(size_t n) { countN(1, n); }

	void finishRemoveTime() { finishTime(0); }
	void finishPushTime  () { finishTime(1); }

	void printStat()const;
};

class PdrPropStat : public Stat<3, 1>
{
public:
	void incInfCubeCount() { countOne(1); }
	void incInfLitCount(size_t n) { countN(2, n); }

	void printStat()const;
};

class PdrRecycleStat : public Stat<1, 1>
{
public:
	PdrRecycleStat(): Stat<1, 1>(), maxRecNum(0) {}

	void checkMaxRecNum(size_t n) { if(n > maxRecNum) maxRecNum = n; }

	void printStat()const;

protected:
	size_t  maxRecNum;
};

class PdrCubeStat : public Stat<8, 1>
{
public:
	PdrCubeStat()
	: Stat<8, 1>   ()
	, maxBadNum    (0)
	, maxInfClsLen (0)
	, maxTreeSize  (0) {}

	void incSubsumeAddObl()        { countOne(1); }
	void incSelfSubsumeAddObl()    { countOne(2); }
	void incSubsumeBlockObl()      { countOne(3); }
	void incSubsumeBlockCube()     { countOne(4); }
	void incSelfSubsumeBlockCube() { countOne(5); }

	void incInfLitNum(size_t n) { countN(6, n); }
	void incInfCubeNum()        { countOne(7); }

	void checkMaxBNum (size_t n) { if(n > maxBadNum)    maxBadNum    = n; }
	void checkMaxCLen (size_t n) { if(n > maxInfClsLen) maxInfClsLen = n; }
	void checkMaxTSize(size_t n) { if(n > maxTreeSize)  maxTreeSize  = n; }

	void printStat()const;

protected:
	size_t  maxBadNum, maxInfClsLen, maxTreeSize;
};

class PdrStimuStat : public Stat<4, 1>
{
public:
	void incInfClsNum(size_t n) { countN(3, n); }

	void printStat()const;
};

/* The memory alignment of PdrCube

   prevCube   refCount   hashValue   litNum | (... literals ...)
 |<-  8   ->|<-  8   ->|<-   8   ->|<- 8  ->|<-   4 * litNum   ->|
                                         pointer
*/

class PdrCube
{
friend ostream& operator<<(ostream&, const PdrCube&);

public:
	PdrCube(): uint32Ptr(0) {}
	explicit PdrCube(const vector<AigGateLit>&, bool = false);
	~PdrCube() { clean(); }

	PdrCube(const PdrCube& c): uint32Ptr(c.uint32Ptr) { if(!isNone()) incCount(); }
	PdrCube(PdrCube&& c) noexcept : uint32Ptr(c.uint32Ptr) { c.uint32Ptr = 0; }
	PdrCube& operator=(const PdrCube& c)
		{ assert(this != &c); clean(); uint32Ptr = c.uint32Ptr; if(!isNone()) incCount(); return *this; }
	PdrCube& operator=(PdrCube&& c) noexcept
		{ assert(this != &c); clean(); uint32Ptr = c.uint32Ptr; c.uint32Ptr = 0; return *this; }

	      AigGateLit* begin()      { return uint32Ptr; }
	      AigGateLit* end  ()      { return uint32Ptr + getSize(); }
	const AigGateLit* begin()const { return uint32Ptr; }
	const AigGateLit* end  ()const { return uint32Ptr + getSize(); }

	bool operator< (const PdrCube&)const;
	bool operator==(const PdrCube&)const;

	void setSize(size_t s) { *(uint64Ptr - 1) = s; }
	void setLit(size_t i, AigGateLit lit) { *(uint32Ptr + i) = lit; }
	void calAbstract(size_t i) { *(uint64Ptr - 2) |= (size_t(1) << (getGateID(getLit(i)) & 63)); }
	void initAbstract() { *(uint64Ptr - 2) = 0; }

	size_t getSize()const  { return *(uint64Ptr - 1); }
	AigGateLit getLit(size_t i)const { return *(uint32Ptr + i); }
	size_t getAbs()const { return *(uint64Ptr - 2); }

	bool isCount(size_t c)const { return *(uint64Ptr - 3) == c; }
	void initCount() { *(uint64Ptr - 3) = 0; }
	void incCount() { *(uint64Ptr - 3) += 1; }
	void decCount() { *(uint64Ptr - 3) -= 1; }
	bool toDelete()const { return isCount(0); }
	void clear() { operator delete(getOriPtr()); }
	void decAndCheck() { decCount(); if(toDelete()) clear(); }
	void clean() { if(!isNone()) decAndCheck(); }

	bool isNone()const { return uint32Ptr == 0; }
	void* getOriPtr()const { return uint64Ptr - 4; }

	bool subsume(const PdrCube& c)const { return subsumeComplexN(c); }
	bool subsumeTrivial (const PdrCube&)const;
	bool subsumeComplexN(const PdrCube&)const;
	bool subsumeComplexB(const PdrCube&)const;

	AigGateLit selfSubsume(const PdrCube& c)const { return selfSubsumeComplex(c); }
	AigGateLit selfSubsumeTrivial(const PdrCube&)const;
	AigGateLit selfSubsumeComplex(const PdrCube&)const;

	PdrCube exactOneDiff(const PdrCube&)const;
	bool    exactOneLess(const PdrCube&)const;

private:
	union { unsigned* uint32Ptr; size_t* uint64Ptr; PdrCube* cubePtr; };
};

class PdrTCube
{
public:
	PdrTCube(size_t f, const PdrCube& c): frame(f), cube(c) {}
	PdrTCube(size_t f, PdrCube&& c): frame(f), cube(move(c)) {}

	size_t getFrame()const { return frame; }
	const PdrCube& getCube()const { return cube; }
	void setFrame(size_t f) { frame = f; }
	PdrTCube getNext()const { return PdrTCube(getFrame()+1, getCube()); }
	bool inInitFrame()const { return getFrame() == 0; }

private:
	size_t   frame;
	PdrCube  cube;
};

class PdrChecker : public SafetyBNChecker
{
public:
	PdrChecker(AigNtk*, size_t, bool, size_t, size_t, size_t, size_t,
	           PdrSimType, PdrOrdType, PdrOblType, PdrDeqType, PdrPrpType, PdrGenType,
	           bool, bool, bool, bool, bool, bool, size_t, size_t, bool,
	           PdrStimuType, size_t, size_t);
	~PdrChecker();

// For stimulator (Start)
protected:
	class PdrStimulator;
		class PdrStimulatorLocal;
			class PdrStimulatorLocalInfAll;
			class PdrStimulatorLocalMix;

	PdrStimulator* getStimulator(PdrStimuType, bool, size_t, size_t);
	bool mergeInf(const vector<PdrCube>&, bool, const PdrCube&);
	void disablePrintFrame() { assert(isVerboseON(PDR_VERBOSE_FRAME));
	                           verbosity &= ~getPdrVbsMask(PDR_VERBOSE_FRAME); }
	PdrChecker* cloneChecker(size_t)const;
	const vector<PdrCube>& getInfFrame()const { return frame.back(); }
	void setTargetCube(const PdrCube& c) { targetCube = c; }
// For stimulator (End)

protected:
	void check();
	PdrResultType checkInt();

protected:
	vector<AigGateID> genTarget(const PdrCube&)const;

	bool recBlockCube(const PdrTCube&);
	size_t isBlocked(const PdrTCube&)const;
	PdrTCube generalize(const PdrTCube&);

	bool propBlockedCubes();

	PdrCube getNotPCube()const;
	size_t isBlockedSAT(const PdrTCube&)const;
	bool isInitial(const PdrCube&)const;
	enum SolveType { DEFAULT, EXTRACT, NOIND };
	PdrTCube solveRelative(const PdrTCube&, SolveType = DEFAULT)const;

	void newFrame();
	void addBlockedCube(const PdrTCube&, size_t = 1);
	void addBlockedCubeFrame(size_t, const PdrCube&)const;
	void addBlockedCubeFrame(size_t)const;
	void addBlockedCubeInf()const;

	PdrCube terSim                (const vector<AigGateID>&)const;
	void    terSimForwardNormal   (const vector<AigGateID>&)const;
	void    terSimForwardEvent    (const vector<AigGateID>&)const;
	void    terSimBackwardNormal  (const vector<AigGateID>&)const;
	void    terSimBackwardInternal(const vector<AigGateID>&)const;
	void genSimCand(const vector<AigGateID>&)const;
	void sortGenCubeByAct()const;

	Var addCurNotState(const PdrCube&)const;
	void addCurState (const PdrCube& c)const { addState(c, 0); }
	void addNextState(const PdrCube& c)const { addState(c, 1); }
	void addState(const PdrCube&, size_t)const;
	void activateFrame(size_t)const;
	PdrCube unsatGen(const PdrCube&)const;
	size_t findLowestActPlus1(size_t)const;
	bool isInitial(const vector<AigGateLit>&, size_t = MAX_SIZE_T)const;
	Var addCurNotState(const vector<AigGateLit>&, size_t = MAX_SIZE_T)const;
	void addNextState(const vector<AigGateLit>&, size_t = MAX_SIZE_T)const;

	void printCurFrames(const char*)const;
	void printRemainObl()const;
	void checkBreak(const char*, bool)const;
	#define CheckBreakPdr(blockNow) checkBreak(__func__, blockNow)
	bool isOrdDecay()const { return ordType & 0b10; }
	bool isOrdReverse()const { return ordType & 0b01; }
	bool isOrdDynamic()const { return ordType != PDR_ORD_INDEX; }

	bool satSolve()const;
	bool satSolveLimited()const;
	void disableActVar(Var)const;
	void checkRecycle();
	void convertCNF()const;
	void convertCNF(const PdrCube&)const;
	void convertFrame(size_t)const;
	void addInitState()const;

	void refineInf();
	void checkMaxD(size_t d)const { if(d > maxUNSAT_D) maxUNSAT_D = d; }
	void checkThenPushObl(size_t, const PdrCube&);
	bool isVerboseON(PdrVerboseType pvt)const { return verbosity & getPdrVbsMask(pvt); }
	void checkAndPrintIndInv();
	size_t getCurRecycleNum()const { return recycleByQuery ? satQueryTime : unusedVarNum; }
	bool subsume(const PdrCube&, const PdrCube&)const;
	AigGateLit selfSubsume(const PdrCube&, const PdrCube&)const;
	vector<PdrCube> getCurIndSet(bool)const;

protected:
	size_t  curFrame;
	size_t  maxFrame;

	PdrCube         targetCube;
	PdrStimulator*  stimulator;

	mutable size_t  satQueryTime;
	mutable size_t  unusedVarNum;
	const size_t    recycleNum;

	mutable size_t  maxUNSAT_D;

	vector<vector<PdrCube>>  frame;
	SolverPtr<CirSolver>     solver;
	vector<Var>              actVar;

	vector<deque<PdrCube>>      badDequeVec;
	mutable AigSimulator        terSimSup;
	mutable vector<AigGateLit>  genCube;

	Array<double>  activity;
	double         actInc;

	StatPtr<PdrTerSimStat>    terSimStat;
	StatPtr<PdrSatStat>       satStat;
	StatPtr<PdrUnsatGenStat>  unsatGenStat;
	StatPtr<PdrPropStat>      propStat;
	StatPtr<PdrRecycleStat>   recycleStat;
	StatPtr<PdrCubeStat>      cubeStat;

	PdrSimType  simType;
	PdrOrdType  ordType;
	PdrOblType  oblType;
	PdrDeqType  deqType;
	PdrPrpType  prpType;
	PdrGenType  genType;
	bool        toRefineInf;
	bool        convertInNeedCone;
	bool        checkSelf;
	bool        assertFrame;
	bool        recycleByQuery;
	bool        checkIndInv;

	bool            convertInNeedFrame;
	mutable size_t  frameConverted;

	mutable vector<Lit>  litList;

	mutable size_t  totalSatQuery;
	const size_t    satQueryLimit;
	mutable size_t  oblTreeSize;
mutable size_t numConf, numDeci, maxObl, maxTree;

	size_t  verbosity;

	static constexpr size_t FRAME_NULL = MAX_SIZE_T - 1;
	static constexpr size_t FRAME_INF  = MAX_SIZE_T;

#ifdef UsePatternCheckSAT
protected:
	mutable vector<Array<size_t>>  pattern;
	mutable size_t                 patIdx;
	mutable size_t                 simMask;
	mutable Array<size_t>          simValue;
	mutable vector<AigAnd*>        dfsList;
	mutable size_t                 matchTry;
	mutable size_t                 matchFrame;
	mutable size_t                 matchSAT;
	mutable clock_t                matchTime;
	mutable bool                   simResult;

	void checkPatForSat(size_t, const PdrCube&)const;
	void checkPatForSat(size_t, size_t)        const;
	void checkPatForSat(size_t)                const;

	void genDfsList(const PdrCube&)const;
	void genDfsList(size_t)const;
	void genDfsList()const;

	void checkPatFrame(size_t)const;
	void simAllAnd()const;
	void setPattern(size_t)const;
	void collectPattern()const;
#endif

#ifdef CheckOblCommonPart
protected:
	mutable size_t             numObl;
	mutable size_t             idxOfLastChange;
	mutable vector<AigGateLit> commonPart;

	void checkOblCommonPart()const;
#endif
};

class PdrChecker::PdrStimulator
{
public:
	PdrStimulator(PdrChecker* c, bool statON)
	: checker(c), stimuStat(statON) {}
	virtual ~PdrStimulator();

	virtual bool stimulate(const PdrTCube&) = 0;

	void incInfClsNum(size_t n)
		{ if(stimuStat.isON()) stimuStat->incInfClsNum(n); }

protected:
	bool solveCand(const PdrCube&, size_t, const PdrCube&);
	bool notFailed(const PdrCube& c)const { return notInList(failCubes, c); }
	void addToFailed(const PdrCube& c) { failCubes.push_back(c); }

protected:
	PdrChecker*            checker;
	vector<PdrCube>        failCubes;
	StatPtr<PdrStimuStat>  stimuStat;
};

class PdrChecker::PdrStimulatorLocal : public PdrStimulator
{
public:
	PdrStimulatorLocal(PdrChecker* c, bool statON, size_t bn, size_t mn)
	: PdrStimulator(c, statON), backtrackNum(bn), matchNum(mn) { assert(bn >= mn); }

protected:
	size_t  backtrackNum;
	size_t  matchNum;
};

class PdrChecker::PdrStimulatorLocalInfAll : public PdrStimulatorLocal
{
public:
	PdrStimulatorLocalInfAll(PdrChecker* c, bool statON, size_t bn, size_t mn, bool onlyI)
	: PdrStimulatorLocal(c, statON, bn, mn), onlyInf(onlyI) {}

	bool stimulate(const PdrTCube&);

protected:
	bool  onlyInf;
};

class PdrChecker::PdrStimulatorLocalMix : public PdrStimulatorLocal
{
public:
	PdrStimulatorLocalMix(PdrChecker* c, bool statON, size_t bn, size_t mn)
	: PdrStimulatorLocal(c, statON, bn, mn), curIdx(0) {}

	bool stimulate(const PdrTCube&);

protected:
	size_t           curIdx;
	vector<PdrCube>  clsCache;
};

#endif

}
