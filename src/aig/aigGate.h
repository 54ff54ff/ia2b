/*========================================================================\
|: [Filename] aigGate.h                                                  :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Define five types of gates in aig network                  :|
<------------------------------------------------------------------------*/

#ifndef HEHE_AIGGATE_H
#define HEHE_AIGGATE_H

#include <string>
#include <vector>
#include <iostream>
#include "aigDef.h"
#include "util.h"
using namespace std;

namespace _54ff
{

class AigGateV
{
friend class AigGate;
friend void swapGateV(AigGateV&, AigGateV&);

public:
	AigGateV() {}
	AigGateV(size_t v) { setValue(v); }
	AigGateV(AigGate* gate, bool inv) { setValue(gate, inv); }

	AigGate* getGatePtr()const { return getPtr<AigGate>(value); }
	AigGateID getGateID()const;
	bool isInv()const { return getFlag(value); }
	size_t getValue()const { return value; }
	bool isNone()const { return value == 0; }

	AigGateV operator~()const { return value ^ FLIP_FLAG_MASK; }
	bool operator==(const AigGateV& gv)const { return value == gv.value; }

	void checkEqGate();
	void setValue(size_t v) { value = v; }
	void setValue(AigGate* gate, bool inv) { value = setFlagToValue(gate, inv); }
	bool setRealValueCheck(AigNtk*);
	void setRealValue     (AigNtk*);
	void flipFlag() { value ^= FLIP_FLAG_MASK; }

private:
	size_t  value;
};

inline void swapGateV(AigGateV& gv1, AigGateV& gv2)
	{ gv1.value ^= gv2.value; gv2.value ^= gv1.value; gv1.value ^= gv2.value; }

class AigGate
{
public:
	AigGate(/*AigNtk* ntk, */AigGateID id):/* hostNtk(ntk),*/ gateID(id), ref(0) {}
	virtual ~AigGate() {}

	//fake virtual constructor
	AigGate* copyGate()const { return copyGate(getGateID()); }
	virtual AigGate* copyGate(AigGateID)const = 0;

	/*====================================*/

	string getTypeStr()const { return aigTypeStr[getGateType()]; }
	virtual AigGateType getGateType()const = 0;
	AigGateID getGateID()const { return gateID; }
	void printStat()const;
	void printGate()const;
	void printFanInCone(unsigned level, bool bound)const { setGlobalRef(); printFanInConeRec(0, level, bound); }

	void changeID(AigGateID newID) { const_cast<AigGateID&>(gateID) = newID; }

	/*====================================*/

	virtual bool isPI()const { return false; }
	virtual bool isCI()const { return false; }
	virtual bool isPO()const { return false; }
	virtual bool isCO()const { return false; }

	/*====================================*/

	AigGateV& getFanIn(size_t i)const { assert(i < 2); return const_cast<AigGateV&>(fanIn[i]); }
	AigGateV& getFanIn0()const { return getFanIn(0); }
	AigGateV& getFanIn1()const { return getFanIn(1); }

	AigGateID getFanInID(size_t i)const { return getFanIn(i).getGateID(); }
	AigGateID getFanIn0ID()const { return getFanIn0().getGateID(); }
	AigGateID getFanIn1ID()const { return getFanIn1().getGateID(); }

	AigGate* getFanInPtr(size_t i)const { return getFanIn(i).getGatePtr(); }
	AigGate* getFanIn0Ptr()const { return getFanIn0().getGatePtr(); }
	AigGate* getFanIn1Ptr()const { return getFanIn1().getGatePtr(); }

	bool isFanInInv(size_t i)const { return getFanIn(i).isInv(); }
	bool isFanIn0Inv()const { return getFanIn0().isInv(); }
	bool isFanIn1Inv()const { return getFanIn1().isInv(); }

	void setFanIn(size_t i, AigGate* ptr, bool inv) { getFanIn(i).setValue(ptr, inv); }
	void setFanIn0(AigGate* ptr, bool inv) { getFanIn0().setValue(ptr, inv); }
	void setFanIn1(AigGate* ptr, bool inv) { getFanIn1().setValue(ptr, inv); }
	void setFanIn(size_t i, size_t v) { getFanIn(i).setValue(v); }
	void setFanIn0(size_t v) { getFanIn0().setValue(v); }
	void setFanIn1(size_t v) { getFanIn1().setValue(v); }

	virtual size_t getFanInNum()const { return 0; }

	/*====================================*/
	/* we do not include fanout in current implementation!
	virtual AigGateID getFanOutID(size_t i)const { return 0; }
	virtual AigGate* getFanOut(size_t i)const { return 0; }
	virtual unsigned getFanOutPos(size_t i)const { return 0; }
	virtual void appendFanOut(unsigned v) {}
	virtual size_t getFanOutNum()const { return 0; }
	*/
	/*====================================*/

	static void setIncludeFlag(bool, bool, bool, bool, bool);
	static void setIncludeFlagAll() { setIncludeFlag(true, true, true, true, true); }
	void traverseFromCO()const;
	void traverseFromPO()const;
	void genDfsList(vector<AigGate*>&)const;
	void genDfsList(vector<AigAnd*>&)const;

	/*====================================*/

	static void setGlobalRef(unsigned diff = 1) { globalRef += diff; }
	bool isGlobalRef(unsigned diff = 0)const { return ref == globalRef - diff; }
	bool inGlobalRef(unsigned diff)const { return ref >= globalRef - diff; }
	void setToGlobalRef(unsigned diff = 0)const { ref = globalRef - diff; }

public:
//	unsigned counter1;
//	unsigned counter2;

protected:
//	AigNtk* const     hostNtk;
	const AigGateID   gateID;
	mutable unsigned  ref;
	AigGateV          fanIn[0]; //always let it be the last data member
	                            //depend on the order of data member, be careful!

private:
	void printFanInConeRec(unsigned, unsigned, bool, bool = false)const;

	static unsigned  globalRef;
	static bool      includeGateType[AIG_TOTAL];
};

class AigGateWithFanIn1 : public AigGate
{
public:
	AigGateWithFanIn1(AigGateID id): AigGate(id) {}
	AigGateWithFanIn1(AigGateID id, AigGateV in0)
	: AigGate(id), fanIn0(in0) {}

	size_t getFanInNum()const { return 1; }

protected:
	AigGateV  fanIn0;
};

class AigGateWithFanIn2 : public AigGateWithFanIn1
{
public:
	AigGateWithFanIn2(AigGateID id): AigGateWithFanIn1(id) {}
	AigGateWithFanIn2(AigGateID id, AigGateV in0, AigGateV in1)
	: AigGateWithFanIn1(id, in0), fanIn1(in1) {}

	size_t getFanInNum()const { return 2; }

protected:
	AigGateV  fanIn1;
};

/*
template <size_t N>
class AigGateWithFanIn : virtual public AigGate
{
public:
	AigGateID getFanInID(size_t i)const { return fanIn[i].getGateID(); }
	AigGate* getFanIn(size_t i)const { return hostNtk->getGate(getFanInID(i)); }
	AigGateIDWithFlag getFanInLit(size_t i)const { return fanIn[i]; }
	bool isFanInInv(size_t i)const { return fanIn[i].isInv(); }
	void setFanIn(size_t i, unsigned v) { fanIn[i] = v; }
	size_t getFanInNum()const { return N; }

protected:
	AigGateIDWithFlag  fanIn[N];
};
*/

//The original design is to let the AigGateWithFan{in<N>, out} be virtually inherited from AigGate
//Then all the gate types are inherited from these two base classes
/*
class AigGateWithFanOut : virtual public AigGate
{
public:
	AigGateID getFanOutID(size_t i)const { return fanOut[i].getGateID(); }
	AigGate* getFanOut(size_t i)const { return hostNtk->getGate(getFanOutID(i)); }
	unsigned getFanOutPos(size_t i)const { return fanOut[i].getPos(); }
	void appendFanOut(unsigned v) { fanOut.push_back(v); }
	size_t getFanOutNum()const { return fanOut.size(); }

protected:
	vector<AigGateIDWithFlag>  fanOut;
}
*/

class AigPi : public AigGate
{
public:
	AigPi(AigGateID id): AigGate(id) {}
	AigGate* copyGate(AigGateID id)const { return (new AigPi(id)); }

	AigGateType getGateType()const { return AIG_PI; }
	bool isPI()const { return true; }
	bool isCI()const { return true; }
};

class AigLatch : public AigGateWithFanIn1
{
public:
	AigLatch(AigGateID id): AigGateWithFanIn1(id) {}
	AigLatch(AigGateID id, AigGateV in0): AigGateWithFanIn1(id, in0) {}
	AigGate* copyGate(AigGateID id)const { return (new AigLatch(id)); }

	AigGateType getGateType()const { return AIG_LATCH; }
	bool isCI()const { return true; }
	bool isCO()const { return true; }
};

class AigPo : public AigGateWithFanIn1
{
public:
	AigPo(AigGateID id): AigGateWithFanIn1(id) {}
	AigPo(AigGateID id, AigGateV in0): AigGateWithFanIn1(id, in0) {}
	AigGate* copyGate(AigGateID id)const { return (new AigPo(id)); }

	AigGateType getGateType()const { return AIG_PO; }
	bool isPO()const { return true; }
	bool isCO()const { return true; }
};

class AigAnd : public AigGateWithFanIn2
{
public:
	AigAnd(AigGateID id): AigGateWithFanIn2(id), eqGate(0) {}
	AigAnd(AigGateID id, AigGateV in0, AigGateV in1): AigGateWithFanIn2(id, in0, in1), eqGate(0) {}
	AigGate* copyGate(AigGateID id)const { return (new AigAnd(id)); }

	AigGateType getGateType()const { return AIG_AND; }

	const AigGateV& getEqGate()const { return eqGate; }
	bool noEqGate()const { return  eqGate.isNone(); }
	bool toDelete()const { return !eqGate.isNone(); }
	void setEqGate(AigGate* gate, bool inv, const char* header = 0)
		{ eqGate.setValue(gate, inv); assert(checkEqGate()); printMerge(header); }
	void setEqGate(const AigGateV& eq, const char* header = 0)
		{ eqGate = eq; assert(checkEqGate()); printMerge(header); }
	bool checkEqGate()const
		{ AigGate* eq = eqGate.getGatePtr(); return eq->getGateType() != AIG_AND || ((AigAnd*)eq)->noEqGate(); }
	void checkFanInOrder() {
		//for binary format, fanin with larger ID is always at the left
		if(getFanIn0ID() * 2 + unsigned(isFanIn0Inv()) <
		   getFanIn1ID() * 2 + unsigned(isFanIn1Inv()))
			swapGateV(getFanIn0(), getFanIn1()); }

	bool checkCombLoop(bool, vector<AigAnd*>*)const;
	void genCombDfsList(vector<AigAnd*>&)const;

protected:
	void printMerge(const char*)const;

protected:
	AigGateV  eqGate;
};

class AigConst0 : public AigGate
{
public:
	AigConst0(): AigGate(0) {}
	AigGate* copyGate(AigGateID)const { return (new AigConst0()); }

	AigGateType getGateType()const { return AIG_CONST0; }
	bool isCI()const { return true; }
};

}

#endif
