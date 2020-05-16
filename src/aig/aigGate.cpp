/*========================================================================\
|: [Filename] aigGate.cpp                                                :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] implement the details of AigGate                           :|
<------------------------------------------------------------------------*/

#include "aigNtk.h"

namespace _54ff
{

const string aigTypeStr[AIG_TOTAL] =
{
	"PI",
	"Latch",
	"PO",
	"And",
	"Const0"
};

unsigned AigGate::globalRef = 0;
bool AigGate::includeGateType[AIG_TOTAL];

AigGateID
AigGateV::getGateID()const
{
	return getGatePtr()->getGateID();
}

void
AigGateV::checkEqGate()
{
	union { AigGate* p1; AigAnd* p2; };
	p1 = getGatePtr();
	if(p1->getGateType() != AIG_AND ||
	   p2->getEqGate().isNone()) return;
	bool inv = isInv();
	setValue(p2->getEqGate().getValue());
	if(inv) flipFlag();
}

bool
AigGateV::setRealValueCheck(AigNtk* ntk)
{
	bool inv = isInv();
	AigGate* g = ntk->getGate(value/2);
	if(g == 0) return false;
	setValue(g, inv);
	return true;
}

void
AigGateV::setRealValue(AigNtk* ntk)
{
	bool inv = isInv();
	setValue(ntk->getGate(value/2), inv);
}

void
AigGate::printStat()const
{
	cout << "ID = " << getGateID() << ", Type = " << getTypeStr();
	switch(getFanInNum())
	{
		case 1:
			cout << ", Fanin = ";
			if(isFanIn0Inv()) cout << "!";
			cout << getFanIn0ID();
			break;

		case 2:
			cout << ", Fanins = ";
			if(isFanIn0Inv()) cout << "!";
			cout << getFanIn0ID();
			cout << " && ";
			if(isFanIn1Inv()) cout << "!";
			cout << getFanIn1ID();
			break;

		default: assert(getFanInNum() == 0);
	}
	cout << endl;
}

void
AigGate::printGate()const
{
	cout << getTypeStr() << " " << getGateID();
	for(size_t i = 0; i < getFanInNum(); ++i)
		cout << " " << (isFanInInv(i) ? "!" : "") << getFanInID(i);
	cout << endl;
}

void
AigGate::setIncludeFlag(bool i, bool l, bool o, bool a, bool c)
{
	includeGateType[AIG_PI]     = i;
	includeGateType[AIG_LATCH]  = l;
	includeGateType[AIG_PO]     = o;
	includeGateType[AIG_AND]    = a;
	includeGateType[AIG_CONST0] = c;
}

void
AigGate::traverseFromCO()const
{
	if(isGlobalRef())
		return;
	setToGlobalRef();
	if(isCI()) return;
	assert(getGateType() == AIG_AND);
	getFanIn0Ptr()->traverseFromCO();
	getFanIn1Ptr()->traverseFromCO();
}

void
AigGate::traverseFromPO()const
{
	if(isGlobalRef())
		return;
	setToGlobalRef();

	switch(getFanInNum())
	{
		case 1: getFanIn0Ptr()->traverseFromPO(); break;
		case 2: getFanIn0Ptr()->traverseFromPO();
		        getFanIn1Ptr()->traverseFromPO(); break;
		default: assert(getFanInNum() == 0); break;
	}
}

void
AigGate::genDfsList(vector<AigGate*>& dfsList)const
{
	if(isGlobalRef())
		return;
	setToGlobalRef();

	switch(getFanInNum())
	{
		case 1: getFanIn0Ptr()->genDfsList(dfsList); break;
		case 2: getFanIn0Ptr()->genDfsList(dfsList);
		        getFanIn1Ptr()->genDfsList(dfsList); break;
		default: assert(getFanInNum() == 0); break;
	}

	if(includeGateType[getGateType()])
		dfsList.push_back(const_cast<AigGate*>(this));
}

void
AigGate::genDfsList(vector<AigAnd*>& dfsList)const
{
	if(isGlobalRef())
		return;
	setToGlobalRef();

	if(getGateType() == AIG_AND)
	{
		getFanIn0Ptr()->genDfsList(dfsList);
		getFanIn1Ptr()->genDfsList(dfsList);
		dfsList.push_back(reinterpret_cast<AigAnd*>(const_cast<AigGate*>(this)));
	}
}

void 
AigGate::printFanInConeRec(unsigned curLevel, unsigned maxLevel, bool bound, bool isInv)const
{
	const char* indentation = "  ";
	for(unsigned i = 0; i < curLevel; ++i)
		cout << indentation;
	if(isInv) cout << "!";
	cout << getGateID() << " " << getTypeStr();
	if(curLevel == maxLevel) { cout << endl; return; }
	if(isGlobalRef())
		{ if(getFanInNum() != 0) cout << " (*)"; cout << endl; return; }
	cout << endl;
	if(bound && isCI() && curLevel != 0) return;
	setToGlobalRef();
	for(size_t i = 0; i < getFanInNum(); ++i)
		getFanInPtr(i)->printFanInConeRec(curLevel+1, maxLevel, bound, isFanInInv(i));
}

bool
AigAnd::checkCombLoop(bool repErr, vector<AigAnd*>* dfsList)const
{
	assert(!isGlobalRef() && !isGlobalRef(1));
	setToGlobalRef(1);
	union { AigGate* fanInG; AigAnd* fanInA; };
	bool result = true;
	fanInG = getFanIn0Ptr();
	if(fanInG->getGateType() == AIG_AND)
	{
		if(fanInA->isGlobalRef(1))
		{
			result = false;
			if(repErr)
				cerr << "[Error] A combinational loop is deteced between gate with ID "
				     << getGateID() << " and " << fanInA->getGateID() << "!" << endl;
		}
		else if(!fanInA->isGlobalRef(0))
			//it must dfs into deeper, not blocked by previous false
			result = fanInA->checkCombLoop(repErr, dfsList) && result;
	}
	fanInG = getFanIn1Ptr();
	if(fanInG->getGateType() == AIG_AND)
	{
		if(fanInA->isGlobalRef(1))
		{
			result = false;
			if(repErr)
				cerr << "[Error] A combinational loop is deteced between gate with ID "
				     << getGateID() << " and " << fanInA->getGateID() << "!" << endl;
		}
		else if(!fanInA->isGlobalRef(0))
			result = fanInA->checkCombLoop(repErr, dfsList) && result;
	}
	if(dfsList != 0)
		(*dfsList).push_back(const_cast<AigAnd*>(this));
	setToGlobalRef(0);
	return result;
}

void
AigAnd::genCombDfsList(vector<AigAnd*>& dfsList)const
{
	if(isGlobalRef())
		return;
	setToGlobalRef();
	union { AigGate* fanInG; AigAnd* fanInA; };
	if(fanInG = getFanIn0Ptr(); fanInG->getGateType() == AIG_AND)
		fanInA->genCombDfsList(dfsList);
	if(fanInG = getFanIn1Ptr(); fanInG->getGateType() == AIG_AND)
		fanInA->genCombDfsList(dfsList);
	dfsList.push_back(const_cast<AigAnd*>(this));
}

void
AigAnd::printMerge(const char* header)const
{
	if(header == 0) return;
	simpMsg << header << " : merge And (" << getGateID() << ") with ";
	simpMsg << eqGate.getGatePtr()->getTypeStr() << " (";
	if(eqGate.isInv()) simpMsg << "!";
	simpMsg << eqGate.getGateID() << ")..." << endl;
}

}