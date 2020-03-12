/*========================================================================\
|: [Filename] pbcChecker.h                                               :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Define the checker about property directed bounded model   :|
:|            checking                                                   |:
<------------------------------------------------------------------------*/

#ifndef HEHE_PBCCHECKER_H
#define HEHE_PBCCHECKER_H

#include "sfcChecker.h"
#include "cirSolver.h"
#include "aigMisc1.h"
#include "stat.h"

namespace _54ff
{

enum PbcRchType
{
	PBC_RCH_AT,
	PBC_RCH_UPTO,
	PBC_RCH_INCREASE,
	PBC_RCH_NONE
};

enum PbcStatType
{
	PBC_STAT_TERSIM = 0,
	PBC_STAT_BLOCK_SAT,
	PBC_STAT_INDUC_SAT,
	PBC_STAT_FIXED_SAT,
	PBC_STAT_TOTAL,
	PBC_STAT_ERROR,
	PBC_STAT_ALL
};

extern const string pbcStatStr[PBC_STAT_TOTAL];

class PbcTerSimStat : public Stat<3, 1>
{
public:
	void incLitCount   (size_t n) { countN(1, n); }
	void incRemoveCount(size_t n) { countN(2, n); }

	void printStat()const;
};

class PbcSatStat : public Stat<2, 2>
{
public:
	PbcSatStat(const char* _solverName)
	: Stat<2, 2>(), solverName(_solverName) {}

	void printStat()const;

	const string  solverName;
};

class PbcCube
{
friend PbcCube* newPbcCube(const vector<AigGateLit>&);
friend void delPbcCube(PbcCube*);

public:
	unsigned getSize()const { return size; }
	AigGateLit getLit(unsigned i)const { return lit[i]; }

	AigGateLit* begin()const { return const_cast<AigGateLit*>(lit); }
	AigGateLit* end  ()const { return const_cast<AigGateLit*>(lit) + getSize(); }

private:
	unsigned    size;
	AigGateLit  lit[0];
};

class PbcChecker : public SafetyBNChecker
{
public:
	PbcChecker(AigNtk*, size_t, bool, size_t, size_t, const Array<bool>&, bool, bool, const char*);
	virtual ~PbcChecker();

protected:
	void check();

protected:
	bool blockNotPState();
	void refineAllReach();
	bool refineReach(size_t);
	virtual bool checkFixPoint() = 0;

	void newFrame();
	void newFrameFrame();
	void newFrameBlock();
	void newFrameInduc();
	virtual void newFrameFixed() = 0;

	size_t getFrame()const { return frame.size() - 1; }
	bool getNotPCube()const;
	bool getCTI(size_t);
	bool getCTIWithCube(size_t, PbcCube*);
	bool blockCube(size_t);

	void addBlockCube(size_t);
	void addBlockCubeFrame(size_t, PbcCube*);
	void addBlockCubeBlock(size_t, PbcCube*);
	void addBlockCubeInduc(size_t, PbcCube*);
	virtual void addBlockCubeFixed(size_t, PbcCube*) = 0;

	bool satSolve(const SolverPtr<CirSolver>&, const StatPtr<PbcSatStat>&)const;
	bool blockSatSolve()const { return satSolve(blockSolver, blockSatStat); }
	bool inducSatSolve()const { return satSolve(inducSolver, inducSatStat); }
	bool fixedSatSolve()const { return satSolve(fixedSolver, fixedSatStat); }

	void addStateBlock(size_t)const;
	void addNextStateInduc(PbcCube*)const;
	virtual vector<AigGateID> genTarget(PbcCube*)const = 0;
	void terSim(const vector<AigGateID>&)const;
	void unsatGen(size_t)const;

	void printCurFrames(const char*)const;
	void checkBreak(const char*)const;
	#define CheckBreakPbc() checkBreak(__func__)

protected:
	size_t  maxFrame;

	vector<vector<PbcCube*>>  frame;
	SolverPtr<CirSolver>      blockSolver;
	SolverPtr<CirSolver>      inducSolver;
	vector<Var>               inducActVar;
	SolverPtr<CirSolver>      fixedSolver;
	vector<Var>               fixedActVar;

	vector<size_t>  inducCheckIdx;
	vector<size_t>  fixedCheckIdx;

	mutable AigSimulator        terSimSup;
	mutable vector<AigGateLit>  genCube;

	StatPtr<PbcTerSimStat>  terSimStat;
	StatPtr<PbcSatStat>     blockSatStat;
	StatPtr<PbcSatStat>     inducSatStat;
	StatPtr<PbcSatStat>     fixedSatStat;

	bool  blockState;
	bool  verbose;
};

class PbcAChecker : public PbcChecker
{
public:
	PbcAChecker(AigNtk*, size_t, bool, size_t, size_t, const Array<bool>&, bool, bool);

protected:
	bool checkFixPoint();
	void newFrameFixed();
	void addBlockCubeFixed(size_t, PbcCube*);

	vector<AigGateID> genTarget(PbcCube*)const;
};

class PbcUChecker : public PbcChecker
{
public:
	PbcUChecker(AigNtk*, size_t, bool, size_t, size_t, const Array<bool>&, bool, bool);

protected:
	bool checkFixPoint();
	void newFrameFixed();
	void addBlockCubeFixed(size_t, PbcCube*);

	vector<AigGateID> genTarget(PbcCube*)const;

	void addStateFixed(PbcCube*)const;
};

class PbcIChecker : public SafetyBNChecker
{
public:
	PbcIChecker(AigNtk*, size_t, bool, size_t, size_t, const Array<bool>&, bool);
	~PbcIChecker();

protected:
	void check();

protected:
	bool blockNotPState();
	void refineAllReach();
	bool refineReach(size_t);
	bool checkFixPoint();

	void newFrame();
	void newFrameFrame();
	void newFrameBlock();
	void newFrameInduc();
	void newFrameFixed();

	size_t getFrame()const { return frame.size() - 1; }
	bool getNotPCube()const;
	bool getCTI(size_t);
	bool getCTIWithCube(size_t, PbcCube*);
	bool blockCube(size_t);

	void addBlockCube(size_t);
	void addBlockCubeFrame(size_t, PbcCube*);
	void addBlockCubeBlock(size_t, PbcCube*);
	void addBlockCubeInduc(size_t, PbcCube*);
	void addBlockCubeFixed(size_t, PbcCube*);

	bool satSolve(const SolverPtr<CirSolver>&, const StatPtr<PbcSatStat>&)const;
	bool blockSatSolve()const { return satSolve(blkIndSolver, blockSatStat); }
	bool inducSatSolve()const { return satSolve(blkIndSolver, inducSatStat); }
	bool fixedSatSolve()const { return satSolve(fixedSolver,  fixedSatStat); }

	void addStateBlock(size_t)const;
	void addNextStateInduc(PbcCube*)const;
	vector<AigGateID> genTarget(PbcCube*)const;
	void terSim(const vector<AigGateID>&)const;
	void unsatGen(size_t)const;

	void printCurFrames(const char*)const;
	void checkBreak(const char*)const;
	#define CheckBreakPbcI() checkBreak(__func__)

protected:
	size_t  maxFrame;

	vector<vector<PbcCube*>>  frame;
	SolverPtr<CirSolver>      blkIndSolver;
	vector<Var>               inducActVar;
	SolverPtr<CirSolver>      fixedSolver;
	vector<Var>               fixedActVar;

	vector<size_t>  inducCheckIdx;
	vector<size_t>  fixedCheckIdx;

	mutable AigSimulator        terSimSup;
	mutable vector<AigGateLit>  genCube;

	StatPtr<PbcTerSimStat>  terSimStat;
	StatPtr<PbcSatStat>     blockSatStat;
	StatPtr<PbcSatStat>     inducSatStat;
	StatPtr<PbcSatStat>     fixedSatStat;

	bool  verbose;
};

}

#endif
