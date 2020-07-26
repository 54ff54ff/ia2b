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
	PDR_RESULT_ABORT_RES,
	PDR_RESULT_ERROR
};

enum PdrClsStimuType
{
	PDR_CLS_STIMU_LOCAL_INF,
	PDR_CLS_STIMU_LOCAL_ALL,
	PDR_CLS_STIMU_LOCAL_GOLD,
	PDR_CLS_STIMU_LOCAL_MIX,
	PDR_CLS_STIMU_HALF,
	PDR_CLS_STIMU_NONE
};

enum PdrOblStimuType
{
	PDR_OBL_STIMU_ALL,
	PDR_OBL_STIMU_DEPTH,
	PDR_OBL_STIMU_NONE
};

enum PdrShareType
{
	PDR_SHARE_NONE,
	PDR_SHARE_INF,
	PDR_SHARE_ALL
};

enum PdrMainType
{
	PDR_MAIN_NORMAL,
	PDR_MAIN_ONE_BY_ONE
};

enum PdrInitType
{
	PDR_INIT_DEFAULT,
	PDR_INIT_CLAUSE,
	PDR_INIT_CUBE
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

class PdrPropStat : public Stat<5, 1>
{
public:
	void incPropCubeCount() { countOne(1); }
	void incPropSuccessCount() { countOne(2); }
	void incInfCubeCount() { countOne(3); }
	void incInfLitCount(size_t n) { countN(4, n); }

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

    badDepth    prevCube   hashValue   markA   markB    refCount    litNum | (... literals ...)
 |<-   4   ->|<-   8   ->|<-   8   ->|<- 1 ->|<- 1 ->|<-   2   ->|<-  4  ->|<-   4 * litNum   ->|
                                                                        pointer
*/

class PdrCube;
class PdrChecker;

class PdrCube
{
friend ostream& operator<<(ostream&, const PdrCube&);

public:
	PdrCube(): uint32Ptr(0) {}
	explicit PdrCube(const vector<AigGateLit>&, bool = false, const PdrCube* = 0, bool = false);
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

	void allocMem(unsigned s)
		{ uint32Ptr = (unsigned*)operator new(sizeof(unsigned) * s + sizeof(unsigned) * 7) + 7; }

	void setSize(unsigned s) { *(uint32Ptr - 1) = s; }
	void setLit(unsigned i, AigGateLit lit) { *(uint32Ptr + i) = lit; }
	void calAbstract(unsigned i) { *(uint64Ptr - 2) |= (size_t(1) << (getGateID(getLit(i)) & 63)); }
	void initAbstract() { *(uint64Ptr - 2) = 0; }
	void initMark() { setMarkA(false); setMarkB(false); }

	unsigned getSize()const  { return *(uint32Ptr - 1); }
	// We use gate ID instead of latch index here
	// Since we preserve the flexibility to use internal signal in representation some day
	AigGateLit getLit(unsigned i)const { return *(uint32Ptr + i); }
	size_t getAbs()const { return *(uint64Ptr - 2); }

	bool isCount(unsigned short c)const { return *(uint16Ptr - 3) == c; }
	void initCount() { *(uint16Ptr - 3) = 0; }
	void incCount() { *(uint16Ptr - 3) += 1; }
	void decCount() { *(uint16Ptr - 3) -= 1; }
	bool toDelete()const { return isCount(0); }
	// TODO, when the trace is too long, stack overflow occurs
	void clear() { getPrevCube().~PdrCube(); operator delete(getOriPtr()); }
	void decAndCheck() { decCount(); if(toDelete()) clear(); }
	void clean() { if(!isNone()) decAndCheck(); }
	void reset() { clean(); uint32Ptr = 0; }

	bool getMarkA()const { return *(boolPtr - 8); }
	void setMarkA(bool value) { *(boolPtr - 8) = value; }
	bool getMarkB()const { return *(boolPtr - 7); }
	void setMarkB(bool value) { *(boolPtr - 7) = value; }

	void setPrevCube(const PdrCube* c) { assert(c == 0 || !c->isNone());
	                                     if(c == 0) new (cubePtr - 3) PdrCube();
	                                     else       new (cubePtr - 3) PdrCube(*c); }
	PdrCube& getPrevCube()const { return *(cubePtr - 3); }

	void setBadDepth(unsigned d) { *(uint32Ptr - 7) = d; }
	unsigned getBadDepth()const { return *(uint32Ptr - 7); }

	bool isNone()const { return uint32Ptr == 0; }
	void* getOriPtr()const { return uint32Ptr - 7; }

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
	union {
		bool*            boolPtr;
		unsigned short*  uint16Ptr;
		unsigned*        uint32Ptr;
		size_t*          uint64Ptr;
		PdrCube*         cubePtr; };
};
static_assert(sizeof(PdrCube) == 8);

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

class PdrChecker;
PdrChecker* getDefaultPdr(AigNtk*);

class PdrChecker : public SafetyBNChecker
{
public:
	PdrChecker(AigNtk*, size_t, bool, size_t, size_t, size_t, size_t,
	           PdrSimType, PdrOrdType, PdrOblType, PdrDeqType, PdrPrpType, PdrGenType,
	           bool, bool, bool, bool, bool, bool, bool, bool, size_t, size_t, size_t, bool,
	           PdrClsStimuType, PdrShareType, size_t, size_t, size_t,
	           PdrOblStimuType, PdrShareType, size_t, size_t);
	~PdrChecker();

	/*====================================*/

	PdrResultType checkInt();
	void startWithIndSet(const vector<PdrCube>&);
	vector<PdrCube> getCurIndSet(bool);

	/*====================================*/

	void disablePrintFrame() { assert(isVerboseON(PDR_VERBOSE_FRAME));
	                           verbosity &= ~getPdrVbsMask(PDR_VERBOSE_FRAME); }
	void enablePrintFrame() { assert(!isVerboseON(PDR_VERBOSE_FRAME));
	                          verbosity |= getPdrVbsMask(PDR_VERBOSE_FRAME); }

	void setTargetCube(const PdrCube& c) { targetCube = c; }
	void resetTargetCube() { targetCube.reset(); }

	void setSatLimit(size_t satLimit) { satQueryLimit = satLimit; }
	void setOblLimit(size_t oblLimit) { numOblLimit   = oblLimit; }

// For stimulator (Start)
protected:
	class PdrStimulator;
		class PdrClsStimulator;
			class PdrClsStimulatorLocal;
				class PdrClsStimulatorLocalInfAll;
				class PdrClsStimulatorLocalMix;
			class PdrClsStimulatorHalf;
		class PdrOblStimulator;
			class PdrOblStimulatorAll;
			class PdrOblStimulatorDepth;

	void mergeInf(const vector<PdrCube>&, bool, StatPtr<PdrStimuStat>&);
	void mergeFrames(const vector<vector<PdrCube>>&, bool, StatPtr<PdrStimuStat>&);
	PdrChecker* cloneChecker()const;
	const vector<PdrCube>& getInfFrame()const { return frame.back(); }

	PdrClsStimulator* getClsStimulator(PdrClsStimuType, PdrShareType, bool, size_t, size_t, size_t);
	PdrOblStimulator* getOblStimulator(PdrOblStimuType, PdrShareType, bool, size_t, size_t);
// For stimulator (End)

protected:
	void check();

	PdrResultType checkIntNormal();
	PdrResultType checkIntOneByOne();

protected:
	vector<AigGateID> genTarget(const PdrCube&)const;

	bool recBlockCube(const PdrTCube&);
	size_t isBlocked(const PdrTCube&)const;
	PdrTCube generalize(const PdrTCube&);
	void generalizeDefault(size_t&);
	void generalizeClause (size_t&);
	void generalizeCube   (size_t&);

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

	PdrCube terSim                (const vector<AigGateID>&, const PdrCube* = 0)const;
	void    terSimForwardNormal   (const vector<AigGateID>&)const;
	void    terSimForwardEvent    (const vector<AigGateID>&)const;
	void    terSimBackwardNormal  (const vector<AigGateID>&)const;
	void    terSimBackwardInternal(const vector<AigGateID>&)const;
	void satGenBySAT(const vector<AigGateLit>&)const;
	void genSimCand(const vector<AigGateID>&)const;
	void sortGenCubeByAct()const;

	Var addCurNotState (const PdrCube&)const;
	void addState    (const PdrCube&, size_t)const;
	void addCurState (const PdrCube& c)const { addState(c, 0); }
	void addNextState(const PdrCube& c)const { addState(c, 1); }
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
	void pushToFrameInf(size_t);
	bool checkIsSubsumed(const PdrCube&, size_t, size_t)const;
	void checkSubsumeOthers(const PdrCube&, size_t, size_t);
	void printTrace()const;
	void collectInd();

	void newInitState();
	void setAllToDC();
	void setInitValue(AigGateID id, ThreeValue value) { initState[id-initIdBase] = value; }
	ThreeValue getInitValue(AigGateID id)const { return initState[id-initIdBase]; }
	bool diffPolar(AigGateLit)const;
	bool isInitNumMatch()const;
	void printStats()const;

protected:
	PdrMainType  mainType;
	PdrInitType  initType;

	size_t  curFrame;
	size_t  maxFrame;

	// Just a workaround, for binary aig format, the latches are nearby
	Array<ThreeValue>  initState;
	unsigned           initIdBase;
	size_t             numLitInit;
	PdrCube            targetCube;
	PdrClsStimulator*  clsStimulator;
	PdrOblStimulator*  oblStimulator;

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
	bool        convertInNeedFrame;
	bool        checkSelf;
	bool        assertFrame;
	bool        recycleByQuery;
	bool        lazyProp;
	bool        sortByBadDepth;
	bool        checkIndInv;

	mutable size_t  frameConverted;
	mutable size_t  minBlockFrame;

	mutable vector<Lit>  litList;

	mutable size_t  totalSatQuery;
	        size_t  satQueryLimit;
	mutable size_t  oblTreeSize;
	mutable size_t  numObl;
	        size_t  numOblLimit;

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
	mutable Array<bool>         isDC;

	mutable size_t              numInf;
	mutable vector<AigGateLit>  commonPartInf;

	void checkIsDC(const PdrCube&)const;
	void checkInfCommonPart(const PdrTCube&)const;
#endif
};

class PdrChecker::PdrStimulator
{
public:
	PdrStimulator(PdrChecker* c, PdrShareType shareT, size_t satL, bool statON)
	: checker(c), shareType(shareT), satLimit(satL), stimuStat(statON) {}
	~PdrStimulator() {}

	void incInfClsNum(size_t n)
		{ if(stimuStat.isON()) stimuStat->incInfClsNum(n); }

	void printStats()const;

protected:
	void solveCand(const PdrCube&, unsigned);
	bool notFailed(const PdrCube& c)const { return notInList(failCubes, c); }
	void addToFailed(const PdrCube& c) { failCubes.push_back(c); }

protected:
	PdrChecker*            checker;
	PdrShareType           shareType;
	size_t                 satLimit;
	vector<PdrCube>        failCubes;
	StatPtr<PdrStimuStat>  stimuStat;
};

class PdrChecker::PdrClsStimulator : public PdrStimulator
{
public:
	PdrClsStimulator(PdrChecker* c, PdrShareType shareT, size_t satL, bool statON)
	: PdrStimulator(c, shareT, satL, statON) {}
	virtual ~PdrClsStimulator() {}

	virtual void stimulateWithOneCube (const PdrTCube&) = 0;
	virtual void stimulateAtEndOfFrame() = 0;
};

class PdrChecker::PdrClsStimulatorLocal : public PdrClsStimulator
{
public:
	PdrClsStimulatorLocal(PdrChecker* c, PdrShareType shareT, bool statON, size_t bn, size_t mn, size_t sl)
	: PdrClsStimulator(c, shareT, sl, statON), backtrackNum(bn), matchNum(mn) { assert(bn >= mn && mn > 0); }
	PdrClsStimulatorLocal(PdrChecker* c, PdrShareType shareT, bool statON, size_t sl)
	: PdrClsStimulator(c, shareT, sl, statON) {}

protected:
	size_t  backtrackNum;
	size_t  matchNum;
};

class PdrChecker::PdrClsStimulatorLocalInfAll : public PdrClsStimulatorLocal
{
public:
	PdrClsStimulatorLocalInfAll(PdrChecker* c, PdrShareType shareT, bool statON, size_t bn, size_t mn, size_t sl, bool onlyI)
	: PdrClsStimulatorLocal(c, shareT, statON, bn, mn, sl), onlyInf(onlyI), golden(false) {}
	PdrClsStimulatorLocalInfAll(PdrChecker* c, PdrShareType shareT, bool statON, size_t sl)
	: PdrClsStimulatorLocal(c, shareT, statON, sl), onlyInf(false), golden(true) {}

	void stimulateWithOneCube(const PdrTCube&);
	void stimulateAtEndOfFrame() {}

protected:
	bool  onlyInf;
	bool  golden;
};

class PdrChecker::PdrClsStimulatorLocalMix : public PdrClsStimulatorLocal
{
public:
	PdrClsStimulatorLocalMix(PdrChecker* c, PdrShareType shareT, bool statON, size_t bn, size_t mn, size_t sl)
	: PdrClsStimulatorLocal(c, shareT, statON, bn, mn, sl), curIdx(0) {}

	void stimulateWithOneCube(const PdrTCube&);
	void stimulateAtEndOfFrame() {}

protected:
	size_t           curIdx;
	vector<PdrCube>  clsCache;
};

class PdrChecker::PdrClsStimulatorHalf : public PdrClsStimulator
{
public:
	PdrClsStimulatorHalf(PdrChecker* c, PdrShareType shareT, bool statON, size_t on, size_t mn, size_t sl)
	: PdrClsStimulator(c, shareT, sl, statON), observeNum(on), matchNum(mn) { assert(on > 0 && mn >= 2); }

	void stimulateWithOneCube(const PdrTCube&) {}
	void stimulateAtEndOfFrame();

protected:
	size_t  observeNum;
	size_t  matchNum;
};

class PdrChecker::PdrOblStimulator : public PdrStimulator
{
public:
	PdrOblStimulator(PdrChecker* c, PdrShareType shareT, bool statON, size_t oblTH, size_t satL)
	: PdrStimulator (c, shareT, satL, statON)
	, oblThreshold  (oblTH) { assert(oblTH > 0); }
	virtual ~PdrOblStimulator() {}

	virtual void stimulate() = 0;
	virtual void checkCommonPart(const PdrCube&) = 0;
	virtual void printCommon()const = 0;

protected:
	size_t  oblThreshold;
};

class PdrChecker::PdrOblStimulatorAll : public PdrOblStimulator
{
public:
	PdrOblStimulatorAll(PdrChecker* c, PdrShareType shareT, bool statON, size_t oth, size_t sl)
	: PdrOblStimulator(c, shareT, statON, oth, sl), numObl(0) {}

	void reset();

	void stimulate();
	void checkCommonPart(const PdrCube&);
	void printCommon()const;

protected:
	size_t              numObl;
	vector<AigGateLit>  commonPart;
};

class PdrChecker::PdrOblStimulatorDepth : public PdrOblStimulator
{
public:
	PdrOblStimulatorDepth(PdrChecker* c, PdrShareType shareT, bool statON, size_t oth, size_t sl)
	: PdrOblStimulator(c, shareT, statON, oth, sl) {}

	void reset(size_t);

	void stimulate();
	void checkCommonPart(const PdrCube&);
	void printCommon()const;

protected:
	vector<size_t>              numObl;
	vector<vector<AigGateLit>>  commonPart;
	vector<size_t>              candDepth;
};

#endif

}
