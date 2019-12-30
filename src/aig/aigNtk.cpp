/*========================================================================\
|: [Filename] aigNtk.cpp                                                 :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Implement the aig network                                  :|
<------------------------------------------------------------------------*/

#include <iostream>
#include <iomanip>
#include <fstream>
#include <tuple>
#include "aigNtk.h"
#include "aigFraig.h"
#include "aigBalance.h"
#include "aigMisc.h"
#include "condStream.h"
using namespace std;

namespace _54ff
{

AigNtk* aigNtk = 0;

AigNtk::~AigNtk()
{
	for(AigGate* g: gateList) delete g;
}

AigNtk*
AigNtk::copyNtk()const
{
	AigNtk* newNtk = new AigNtk(ntkName.c_str());
	newNtk->PIList      = PIList;
	newNtk->latchList   = latchList;
	newNtk->POList      = POList;
	newNtk->recycleList = recycleList;
	unsigned m = gateList.size();
	newNtk->gateList.resize(m, 0);
	for(AigGateID i = 1; i < m; ++i)
		if(AigGate* g = getGate(i); g != 0)
		{
			AigGate* newGate = g->copyGate();
			newNtk->setGate(i, newGate);
			for(size_t j = 0, n = g->getFanInNum(); j < n; ++j)
				newGate->setFanIn(j, size_t(g->getFanInID(j)) * 2 + size_t(g->isFanInInv(j)));
		}
	for(AigGateID i = 1; i < m; ++i)
		if(AigGate* g = newNtk->getGate(i); g != 0)
			for(size_t j = 0, n = g->getFanInNum(); j < n; ++j)
				g->getFanIn(j).setRealValue(newNtk);
	return newNtk;
}

AigNtk*
AigNtk::moveNtk()
{
	AigNtk* newNtk = new AigNtk;
	ntkName    .swap(newNtk->ntkName);
	PIList     .swap(newNtk->PIList);
	latchList  .swap(newNtk->latchList);
	POList     .swap(newNtk->POList);
	gateList   .swap(newNtk->gateList);
	recycleList.swap(newNtk->recycleList);
	return newNtk;
}

void
AigNtk::swap(AigNtk* ntk)
{
	ntkName    .swap(ntk->ntkName);
	PIList     .swap(ntk->PIList);
	latchList  .swap(ntk->latchList);
	POList     .swap(ntk->POList);
	gateList   .swap(ntk->gateList);
	recycleList.swap(ntk->recycleList);
}

void
AigNtk::printSummary()const
{
	cout << RepeatChar('=', 36)    << endl
	     << "Network: " << ntkName << endl
	     << RepeatChar('=', 36)    << endl
	     << "   " << left << setw(7) << aigTypeStr[AIG_PI]     << right << setw(23) << getInputNum()  << endl
	     << "   " << left << setw(7) << aigTypeStr[AIG_LATCH]  << right << setw(23) << getLatchNum()  << endl
	     << "   " << left << setw(7) << aigTypeStr[AIG_PO]     << right << setw(23) << getOutputNum() << endl
	     << "   " << left << setw(7) << aigTypeStr[AIG_AND]    << right << setw(23) << getAndNum()    << endl
	     << "   " << left << setw(7) << aigTypeStr[AIG_CONST0] << right << setw(23) << getConst0Num() << endl
	     << RepeatChar('-', 36)    << endl
	     << "   " << left << setw(7) << "Total"                << right << setw(23) << getGateNum()   << endl;
}

void
AigNtk::printAll()const
{
	for(size_t i = 0; i < getMaxGateNum(); ++i)
		if(AigGate* g = getGate(i); g != 0)
			{ cout << "[" << i << "] "; g->printGate(); }
}

void
AigNtk::printNetlist()const
{
	AigGate::setIncludeFlagAll();
	AigGate::setGlobalRef();
	vector<AigGate*> dfsList;
	for(AigGateID id: POList)
		getGate(id)->genDfsList(dfsList);
	for(size_t i = 0; i < dfsList.size(); ++i)
		{ cout << "[" << i << "] "; dfsList[i]->printGate(); }
}

void
AigNtk::printPIs()const
{
	if(PIList.empty())
		cout << "No PI!";
	else { cout << "PI(s):"; for(AigGateID id: PIList) cout << " " << id; }
	cout << endl;
}

void
AigNtk::printLatches()const
{
	if(latchList.empty())
		cout << "No latch!";
	else { cout << "Latch(s):"; for(AigGateID id: latchList) cout << " " << id; }
	cout << endl;
}

void
AigNtk::printPOs()const
{
	if(POList.empty())
		cout << "No PO!";
	else { cout << "PO(s):"; for(AigGateID id: POList) cout << " " << id; }
	cout << endl;
}

void
AigNtk::printRecycled()const
{
	if(recycleList.empty())
		cout << "No recycled!";
	else { cout << "Recycled(s):"; for(AigGateID id: recycleList) cout << " " << id; }
	cout << endl;
}

bool
AigNtk::printLevel()const
{
	const size_t coNum = getLatchNum() + getOutputNum();
	if(coNum == 0)
		{ cout << "No CO!" << endl; return true; }

	AigLeveler aigLeveler;
	if(!aigLeveler.init(this, true))
		return false;
	auto [maxLevel, minLevel, aveLevel] = aigLeveler.getAllLevel();
	cout << "Total " << coNum << " CO(s)!" << endl
	     << "Max = " << maxLevel << ", Min = " << minLevel
	     << ", Average = "
	     << fixed << setprecision(2) << aveLevel << endl;
	return true;
}

bool
AigNtk::printInfluence()const
{
	vector<AigAnd*> dfsList;
	if(!checkCombLoop(true, dfsList))
		return false;
	if(dfsList.empty())
		{ cout << "No AND gate!" << endl; return true; }
	Array<unsigned> allInflu(getMaxGateNum());
	enum { INFLU_CONST = 0, INFLU_PI = 1, INFLU_LATCH = 2, INFLU_BOTH = 3, INFLU_TOTAL };
	enum { INFLU_L = 0, INFLU_O, INFLU_A, INFLU_COUNT };
	allInflu[0] = INFLU_CONST;
	for(size_t i = 0, I = getInputNum(); i < I; ++i)
		allInflu[getInputID(i)] = INFLU_PI;
	for(size_t i = 0, L = getLatchNum(); i < L; ++i)
		allInflu[getLatchID(i)] = INFLU_LATCH;
	Array<unsigned, 2> influCount(INFLU_COUNT, INFLU_TOTAL);
	for(unsigned i = 0; i < INFLU_COUNT; ++i)
		for(unsigned j = 0; j < INFLU_TOTAL; ++j)
			influCount[i][j] = 0;
	const size_t A = dfsList.size();
	const size_t paddingA = calDigit(A);
	for(AigAnd* a: dfsList)
		influCount[INFLU_A][allInflu[a->getGateID  ()] =
		                    allInflu[a->getFanIn0ID()] |
		                    allInflu[a->getFanIn1ID()]] += 1;
	const size_t O = getOutputNum();
	const size_t paddingO = calDigit(O);
	for(size_t i = 0; i < O; ++i)
		influCount[INFLU_O][allInflu[getOutputNorm(i)->getFanIn0ID()]] += 1;
	const size_t L = getLatchNum();
	const size_t paddingL = calDigit(L);
	for(size_t i = 0; i < L; ++i)
		influCount[INFLU_L][allInflu[getLatchNorm(i)->getFanIn0ID()]] += 1;

	cout << setw(7) << "|" << setw((paddingA+14)/2) << "A";
	if(O != 0 || L != 0) cout << setw((paddingA+15)/2) << "|";
	if(O != 0) { cout << setw((paddingO+14)/2) << "O"; if(L != 0) cout << setw((paddingO+15)/2) << "|"; }
	if(L != 0) cout << setw((paddingL+14)/2) << "L";
	cout << endl;
	size_t n = paddingA + 20;
	if(O != 0) n += (paddingO + 14);
	if(L != 0) n += (paddingL + 14);
	for(; n > 0; --n) cout << "-";
	cout << endl;
	cout << right << fixed;
	cout << "Const | " << setw(paddingA) << influCount[INFLU_A][INFLU_CONST]
	     << " (" << setprecision(2) << setw(6) << (double(influCount[INFLU_A][INFLU_CONST] * 100) / A) << " %)";
	if(O != 0)
		cout << " | " << setw(paddingO) << influCount[INFLU_O][INFLU_CONST]
		     << " (" << setprecision(2) << setw(6) << (double(influCount[INFLU_O][INFLU_CONST] * 100) / O) << " %)";
	if(L != 0)
		cout << " | " << setw(paddingL) << influCount[INFLU_L][INFLU_CONST]
		     << " (" << setprecision(2) << setw(6) << (double(influCount[INFLU_L][INFLU_CONST] * 100) / L) << " %)";
	cout << endl;
	cout << "PI    | " << setw(paddingA) << influCount[INFLU_A][INFLU_PI]
	     << " (" << setprecision(2) << setw(6) << (double(influCount[INFLU_A][INFLU_PI] * 100) / A) << " %)";
	if(O != 0)
		cout << " | " << setw(paddingO) << influCount[INFLU_O][INFLU_PI]
		     << " (" << setprecision(2) << setw(6) << (double(influCount[INFLU_O][INFLU_PI] * 100) / O) << " %)";
	if(L != 0)
		cout << " | " << setw(paddingL) << influCount[INFLU_L][INFLU_PI]
		     << " (" << setprecision(2) << setw(6) << (double(influCount[INFLU_L][INFLU_PI] * 100) / L) << " %)";
	cout << endl;
	cout << "Latch | " << setw(paddingA) << influCount[INFLU_A][INFLU_LATCH]
	     << " (" << setprecision(2) << setw(6) << (double(influCount[INFLU_A][INFLU_LATCH] * 100) / A) << " %)";
	if(O != 0)
		cout << " | " << setw(paddingO) << influCount[INFLU_O][INFLU_LATCH]
		     << " (" << setprecision(2) << setw(6) << (double(influCount[INFLU_O][INFLU_LATCH] * 100) / O) << " %)";
	if(L != 0)
		cout << " | " << setw(paddingL) << influCount[INFLU_L][INFLU_LATCH]
		     << " (" << setprecision(2) << setw(6) << (double(influCount[INFLU_L][INFLU_LATCH] * 100) / L) << " %)";
	cout << endl;
	cout << "Both  | " << setw(paddingA) << influCount[INFLU_A][INFLU_BOTH]
	     << " (" << setprecision(2) << setw(6) << (double(influCount[INFLU_A][INFLU_BOTH] * 100) / A) << " %)";
	if(O != 0)
		cout << " | " << setw(paddingO) << influCount[INFLU_O][INFLU_BOTH]
		     << " (" << setprecision(2) << setw(6) << (double(influCount[INFLU_O][INFLU_BOTH] * 100) / O) << " %)";
	if(L != 0)
		cout << " | " << setw(paddingL) << influCount[INFLU_L][INFLU_BOTH]
		     << " (" << setprecision(2) << setw(6) << (double(influCount[INFLU_L][INFLU_BOTH] * 100) / L) << " %)";
	cout << endl;
	cout << "Total | " << setw(paddingA) << A << " (100.00 %)";
	if(O != 0) cout << " | " << setw(paddingO) << O << " (100.00 %)";
	if(L != 0) cout << " | " << setw(paddingL) << L << " (100.00 %)";
	cout << endl;
	return true;
}

bool
AigNtk::compress()
{
	if(recycleList.empty()) return true;
	AigGateID j = 1;
	unsigned m = getMaxGateNum();
	Array<AigGateID> idMap(m);
	for(AigGateID i = 1; i < m; ++i)
		if(AigGate* g = getGate(i); g != 0)
			{ g->changeID(j); idMap[i] = j; setGate(j++, g); }
	for(AigGateID& id: PIList)    id = idMap[id];
	for(AigGateID& id: latchList) id = idMap[id];
	for(AigGateID& id: POList)    id = idMap[id];
	gateList.resize(j);
	cout << "Remove " << recycleList.size() << " recycled!" << endl;
	vector<AigGateID>().swap(recycleList);
	return true;
}

bool
AigNtk::oneLvlStrucSimp()
{
	return aigFraiger->oneLvlStrucSimp(this);
}

bool
AigNtk::twoLvlStrucSimp()
{
	return aigFraiger->twoLvlStrucSimp(this);
}

bool
AigNtk::collectCOI()
{
	AigGate::setGlobalRef();
	for(size_t i = 0, O = getOutputNum(); i < O; ++i)
		getOutput(i)->getFanIn0Ptr()->traverseFromPO();
	unsigned PICount = 0, latchCount = 0, andCount = 0;
	for(size_t i = 1, m = gateList.size(); i < m; ++i)
		if(AigGate* g = getGate(i); g != 0 && !g->isGlobalRef())
		{
			switch(g->getGateType())
			{
				case AIG_PI    :    PICount += 1; continue;
				case AIG_LATCH : latchCount += 1; break;
				case AIG_AND   :   andCount += 1; break;
				default: continue;
			}
			cout << "Extract: remove " << g->getTypeStr()
			     << " (" << i << ")..." << endl;
			removeGate(i);
		}
	size_t j = 0;
	for(size_t i = 0, L = getLatchNum(); i < L; ++i)
		if(getLatch(i) != 0)
			latchList[j++] = latchList[i];
	latchList.resize(j);
	if(PICount != 0 || latchCount != 0 || andCount != 0)
		cout << RepeatChar('-', (PICount != 0 && (latchCount != 0 || andCount != 0) ? 72 : 36)) << endl;
	if(andCount != 0 || latchCount != 0) cout << "Remove ";
	if(andCount != 0) cout << andCount << " And(s)";
	if(latchCount != 0) { if(andCount != 0) cout << ", "; cout << latchCount << " Latch(s)"; }
	if(PICount != 0) { if(andCount != 0 || latchCount != 0) cout << ". "; cout << "Reserve " << PICount << " unused PI(s)"; }
	if(PICount != 0 || latchCount != 0 || andCount != 0) cout << endl;
	return true;
}

bool
AigNtk::calReachable()
{
	AigGate::setGlobalRef();
	for(size_t i = 0, O = getOutputNum(); i < O; ++i)
		getOutput(i)->getFanIn0Ptr()->traverseFromCO();
	for(size_t i = 0, L = getLatchNum(); i < L; ++i)
		getLatch(i)->getFanIn0Ptr()->traverseFromCO();
	unsigned count = 0;
	for(size_t i = 0, m = gateList.size(); i < m; ++i)
		if(AigGate* g = getGate(i); g != 0 &&
		   !g->isGlobalRef() && g->getGateType() == AIG_AND)
		{
			cout << "Sweep: remove And (" << i << ")..." << endl;
			removeGate(i); count += 1;
		}
	if(count != 0)
		cout << RepeatChar('-', 36) << endl
		     << "Remove " << count << " And(s)" << endl;
	return true;
}

bool
AigNtk::fraig()
{
	return aigFraiger->funcSimp(this);
}

bool
AigNtk::balance()
{
	return aigBalancer->balance(this);
}

bool
AigNtk::simulate(const char* patternFileName, bool printAll, const char* outFileName)const
{
	ofstream outFile;
	if(outFileName != 0)
		if(outFile.open(outFileName); !outFile.is_open())
			{ cerr << "[Error] Cannot open file \"" << outFileName << "\"!" << endl; return false; }
	ostream& os = outFileName != 0 ? outFile : cout;
	ifstream pattern(patternFileName);
	if(!pattern.is_open())
		{ cerr << "[Error] Cannot open file \"" << patternFileName << "\"!" << endl; return false; }

	AigSimulator aigSim(this);
	if(!aigSim.checkCombLoop())
		return false;
	aigSim.initValue();
	aigSim.setAllToDCorNone();
	aigSim.setConst0();
	aigSim.setInitState();

	for(unsigned t = 1; true; ++t)
	{
		auto getChar = [&pattern, t]()->tuple<char, bool>
		{
			char v = pattern.get();
			if(pattern.eof())
				{ cerr << "[Error] At line " << t << ", Unexpected EOF!" << endl; return {'\0', false}; }
			if(pattern.fail())
				{ cerr << "[Error] At line " << t << ", Unexpected accident!" << endl; return {'\0', false}; }
			return {v, true};
		};

		for(size_t i = 0, I = getInputNum(); i < I; ++i)
		{
			auto [v, valid] = getChar();
			if(!valid) return false;
			switch(v)
			{
				case '0': aigSim.setInputValue(i, ThreeValue_False); break;
				case '1': aigSim.setInputValue(i, ThreeValue_True);  break;
				case 'X': aigSim.setInputValue(i, ThreeValue_DC);    break;
				default : cerr << "[Error] At line " << t << ", Unexpected character!" << endl;
				          return false;
			}
		}
		if(auto [n, valid] = getChar(); 
		   !valid) return false;
		else if(n != '\n')
			{ cerr << "[Error] At line " << t << ", Missing newline!" << endl; return false; }

		aigSim.simDfsList();
		aigSim.simAllLatch();
		aigSim.simAllOutput();
		printAll ? aigSim.printAllGate(os) : aigSim.printAllOutput(os);
		os << endl;

		if(pattern.peek(); pattern.eof())
			return true;
	}
}

AigGate*
AigNtk::createInput()
{
	AigGateID id = getValidID();
	AigPi* gate = new AigPi(id);
	setGate(id, gate);
	PIList.push_back(id);
	return gate;
}

AigGate*
AigNtk::createLatch(AigGateV in0)
{
	AigGateID id = getValidID();
	AigLatch* gate = new AigLatch(id, in0);
	setGate(id, gate);
	latchList.push_back(id);
	return gate;
}

AigGate*
AigNtk::createOutput(AigGateV in0)
{
	AigGateID id = getValidID();
	AigPo* gate = new AigPo(id, in0);
	setGate(id, gate);
	POList.push_back(id);
	return gate;
}

AigGateV
AigNtk::createAnd(AigGateV in0, AigGateV in1)
{
	AigGateID id = getValidID();
	AigAnd* gate = new AigAnd(id, in0, in1);
	setGate(id, gate); return AigGateV(gate, false);
}

AigGateV
AigNtk::createAnd_(vector<AigGateV>& gateList)
{
	while(gateList.size() > 1)
	{
		size_t i = 0, j = 0, n = gateList.size();
		for(size_t _n = n / 2; j < _n; i += 2)
			gateList[j++] = createAnd(gateList[i], gateList[i+1]);
		if(i != n)
			gateList[j++] = gateList[i];
		gateList.resize(j);
	}
	return gateList[0];
}

AigGateV
AigNtk::createAndConstProp(AigGateV in0, AigGateV in1)
{
	if(in0.getGateID() * 2 + unsigned(in0.isInv()) >
	   in1.getGateID() * 2 + unsigned(in1.isInv())) swapGateV(in0, in1);
	if(in0.getGatePtr()->getGateType() == AIG_CONST0)
		return in0.isInv() ? in1 : getConst0V();
	else if(in0.getGatePtr() == in1.getGatePtr())
		return in0.isInv() ^ in1.isInv() ? getConst0V() : in0;
	else return createAnd(in0, in1);
}

AigGateID
AigNtk::getValidID()
{
	AigGateID newID;
	if(recycleList.empty()) { newID = getMaxGateNum(); gateList.resize(newID+1, 0); }
	else                    { newID = recycleList.back(); recycleList.pop_back(); }
	return newID;
}

struct SegFault {};
static void segFaultHandler(int) { throw SegFault(); }

bool
AigNtk::checkValidity(bool repErr)const
{
	struct SegFaultSetter
	{
		SegFaultSetter()  { signal(SIGSEGV, segFaultHandler); }
		~SegFaultSetter() { signal(SIGSEGV, SIG_DFL); }
	} segFaultSetter;
	try
		{ return checkValidID(repErr) && checkValidFanIns(repErr); }
	catch(const SegFault& sf)
		{ cerr << "[Error] Receive a segmentation violation signal, an invalid address is accessed!" << endl; return false; }
}

bool
AigNtk::noLatchInCone(size_t outputIdx)const
{
	assert(outputIdx < getOutputNum());
	AigGate::setGlobalRef();
	getOutputNorm(outputIdx)->getFanIn0Ptr()->traverseFromCO();
	for(size_t i = 0, L = getLatchNum(); i < L; ++i)
		if(getLatchNorm(i)->isGlobalRef())
			return false;
	return true;
}

bool
AigNtk::checkCombLoop(bool repErr, vector<AigAnd*>* dfsList)const
{
	AigGate::setGlobalRef(2);
	bool result = true;
	for(AigGateID i = 1, n = getMaxGateNum(); i < n; ++i)
		if(AigGate* g = getGate(i); g != 0 &&
		   g->getGateType() == AIG_AND && !g->isGlobalRef())
			result = ((AigAnd*)g)->checkCombLoop(repErr, dfsList) && result;
	return result;
}

bool
AigNtk::checkValidID(bool repErr)const
{
	AigGate::setGlobalRef();
	bool i = checkInputID  (repErr);
	bool l = checkLatchID  (repErr);
	bool o = checkOutputID (repErr);
	bool r = checkRecycleID(repErr);
	bool a = checkAllGateID(repErr);
	return i && l && o && r && a;
}

bool
AigNtk::checkValidIDSub(bool repErr, const vector<AigGateID>& idList, AigGateType gateType)const
{
	bool valid = true;
	for(size_t i = 0; i < idList.size(); ++i)
		if(AigGateID id = idList[i]; id > getMaxGateID())
		{
			valid = false;
			if(repErr)
				cerr << "[Error] " << aigTypeStr[gateType] << " " << i << " with ID " << id
				     << " exceeds the maximum variable ID (" << getMaxGateID() << ")!" << endl;
		}
		else if(AigGate* g = getGate(id); g == 0)
		{
			valid = false;
			if(repErr)
				cerr << "[Error] " << aigTypeStr[gateType] << " " << i << " with ID " << id
				     << " does not exist in the network!" << endl;
		}
		else if(g->getGateType() != gateType)
		{
			valid = false;
			if(repErr)
				cerr << "[Error] " << aigTypeStr[gateType] << " " << i << " with ID " << id
				     << " is actually a \"" << g->getTypeStr() << "\"!" << endl;
		}
		else if(g->isGlobalRef())
		{
			valid = false;
			if(repErr)
				cerr << "[Error] " << aigTypeStr[gateType] << " " << i << " with ID " << id
				     << " is duplicated in the list!" << endl;
		}
		else g->setToGlobalRef();
	return valid;
}

bool
AigNtk::checkRecycleID(bool repErr)const
{
	bool valid = true;
	for(size_t i = 0; i < recycleList.size(); ++i)
		if(AigGateID id = recycleList[i]; id > getMaxGateID())
		{
			valid = false;
			if(repErr)
				cerr << "[Error] Recycled " << i << " with ID " << id
				     << " exceeds the maximum variable ID (" << getMaxGateID() << ")!" << endl;
		}
		else if(AigGate* g = getGate(id); g != 0)
		{
			valid = false;
			if(repErr)
				cerr << "[Error] Recycled " << i << " with ID " << id
					 << " is actually a \"" << g->getTypeStr() << "\"!" << endl;
		}
		else if(g == (AigGate*)1)
		{
			valid = false;
			if(repErr)
				cerr << "[Error] Recycled " << i << " with ID " << id
				     << " is duplicated in the list!" << endl;
		}
		else const_cast<AigNtk*>(this)->setGate(id, (AigGate*)1);
	return valid;
}

bool
AigNtk::checkAllGateID(bool repErr)const
{
	bool valid = true;
	if(gateList.empty())
	{
		valid = false;
		if(repErr) cerr << "[Error] No gate is in this network. There should be at least a Const0!" << endl;
	}
	else if(AigGate* g = getGate(0); g == 0)
	{
		valid = false;
		if(repErr) cerr << "[Error] No Const0 with ID 0 exists!" << endl;
	}
	else if(g->getGateType() != AIG_CONST0)
	{
		valid = false;
		if(repErr)
			cerr << "[Error] The gate with ID 0 is expected to be a \"Const0\" instead of \""
			     << g->getTypeStr() << "\"!" << endl;
	}
	else g->setToGlobalRef();
	for(AigGateID i = 1, n = getMaxGateNum(); i < n; ++i)
		switch(AigGate* g = getGate(i); reinterpret_cast<size_t>(g))
		{
			case 0:
				valid = false;
				if(repErr) cerr << "[Error] ID " << i << " with no gate is not in the recycle list!" << endl;
				break;

			case 1:
				const_cast<AigNtk*>(this)->setGate(i, 0);
				break;

			default:
				if(g->getGateID() != i)
				{
					valid = false;
					if(repErr)
						cerr << "[Error] " << g->getTypeStr() << " with ID " << i
						     << " actually has ID " << g->getGateID() << "!" << endl;
				}
				if(!g->isGlobalRef())
					switch(g->getGateType())
					{
						case AIG_AND:
							g->setToGlobalRef();
							break;

						case AIG_CONST0:
							valid = false;
							if(repErr) cerr << "[Error] Extra \"Const0\" with ID " << i << "!" << endl;
							break;

						default:
							valid = false;
							if(repErr)
								cerr << "[Error] " << g->getTypeStr() << " with ID " << i
								     << " is not in the corresponding list!" << endl;
							break;
					} break;
		}
	return valid;
}

bool
AigNtk::checkValidFanIns(bool repErr)const
{
	//pre-condition: all gates in the list are set to globalRef
	bool valid = true;
	for(AigGateID i = 0, n = getMaxGateNum(); i < n; ++i)
		if(AigGate* g = getGate(i); g != 0)
			switch(g->getFanInNum())
			{
				case 1:
					if(!(g->getFanIn0Ptr()->isGlobalRef()))
					{
						valid = false;
						if(repErr)
							cerr << "[Error] The first fanin of gate with ID " << i
							     << " is not in the same network!" << endl;
					}
					if(g->getFanIn0Ptr()->getGateType() == AIG_PO)
					{
						valid = false;
						if(repErr)
							cerr << "[Error] The first fanin of gate with ID " << i
							     << " is PO!" << endl;
					}
					break;

				case 2:
					if(!(g->getFanIn0Ptr()->isGlobalRef()))
					{
						valid = false;
						if(repErr)
							cerr << "[Error] The first fanin of gate with ID " << i
							     << " is not in the same network!" << endl;
					}
					if(g->getFanIn0Ptr()->getGateType() == AIG_PO)
					{
						valid = false;
						if(repErr)
							cerr << "[Error] The first fanin of gate with ID " << i
							     << " is PO!" << endl;
					}
					if(!(g->getFanIn1Ptr()->isGlobalRef()))
					{
						valid = false;
						if(repErr)
							cerr << "[Error] The second fanin of gate with ID " << i
							     << " is not in the same network!" << endl;
					}
					if(g->getFanIn1Ptr()->getGateType() == AIG_PO)
					{
						valid = false;
						if(repErr)
							cerr << "[Error] The second fanin of gate with ID " << i
							     << " is PO!" << endl;
					}
					break;

				default:
					break;
			}
	return valid;
}

}
