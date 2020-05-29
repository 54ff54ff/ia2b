/*========================================================================\
|: [Filename] aigParser.cpp                                              :|
:| [Author]   Chung-Yang (Ric) Huang, Chiang Chun-Yi                     |:
|: [Synopsis] Implement the AIGER parser for aig network                 :|
<------------------------------------------------------------------------*/

#include "aigParser.h"

namespace _54ff
{

AigParser* const aigParser[TOTAL_PARSE_TYPE] =
	{ new AigAsciiParser, new AigBinaryParser };

bool
AigParser::parseError(AigParseError err)const
{
	switch(err)
	{
		case CANNOT_OPEN_FILE:
			cerr << "[Error] Cannot open AIGER file \"" << errStr << "\"!" << endl;
			break;

		case MISSING_SPACE:
			cerr << "[Error] At line " << lineNo << ", column " << colNo + 1
			     << ": Expect a space character here!" << endl;
			break;

		case EXTRA_SPACE:
			cerr << "[Error] At line " << lineNo << ", column " << colNo + 1
			     << ": Unexpected space character!" << endl;
			break;

		case MISSING_DEFINITION:
			cerr << "[Error] At line " << lineNo << ", column " << colNo + 1
			     << ": Expect " << errStr << " here!" << endl;
			break;

		case ILLEGAL_CHARACTER:
			cerr << "[Error] At line " << lineNo << ", column " << colNo + 1
			     << ": Unexpected unprintable character (" << unsigned((unsigned char)getCurChar()) << ")!" << endl
			     << "This kind of characters are only possible to appear in the \"And\" part of the binary format!" << endl;
			break;

		case ILLEGAL_NUMBER:
			cerr << "[Error] At line " << lineNo << ", column " << colNo + 1
			     << ": Illegal unsigned number (" << errStr << ")!" << endl;
			break;

		case NUMBER_OVERFLOW:
			cerr << "[Error] At line " << lineNo << ", column " << colNo + 1
			     << ": The number " << errStr << " is too big. Currently only "
			     << sizeof(unsigned) * 8 << " bits unisgned number is support!" << endl;
			break;

		case ILLEGAL_IDENTIFIER:
			cerr << "[Error] At line " << lineNo << ", column " << colNo + 1
			     << ": Illegal identifier \"" << errStr << "\". It should be \"" << getIdentifier() << "\"!" << endl;
			break;

		case UNKNOWN_ACCIDENT:
			cerr << "[Error] At line " << lineNo
			     << ": Unknown accident happens. Stop parsing!" << endl;
			break;

		case UNEXPECTED_EOF:
			cerr << "[Error] At line " << lineNo << ", column " << colNo + 1
			     << ": Unexpected EOF. Expect " << errStr << " here!" << endl;
			break;

		case MISSING_NEWLINE:
			cerr << "[Error] At line " << lineNo << ", column " << colNo + 1
			     << ": Expect a newline character here!" << endl;
			break;

		case NUMBER_TOO_BIG:
			cerr << "[Error] At line " << lineNo << ",column " << colNo + 1
			     << ": " << errStr << " is too big (" << errInt << ")!" << endl;
			break;

		case NUMBER_TOO_SMALL:
			cerr << "[Error] At line " << lineNo << ", column " << colNo + 1
			     << ": " << errStr << " is too small (" << errInt << ")!" << endl;
			break;

		case MAX_LIT_ID:
			cerr << "[Error] At line " << lineNo << ", column " << colNo + 1
			     << ": The literal of " << errStr << " (" << errInt << ") exceeds the maximum valid ID!" << endl;
			break;

		case CANNOT_BE_INVERTED:
			cerr << "[Error] At line " << lineNo << ", column " << colNo + 1
			     << ": The literal (" << errInt << ") for declaration cannot be inverted!" << endl;
			break;

		case REDEF_CONST:
			cerr << "[Error] At line " << lineNo << ", colunm " << colNo + 1
			     << ": The constant gate cannot be redefined as \"" << errStr << "\"!" << endl;
			break;

		case REDEF_GATE:
			cerr << "[Error] At line " << lineNo << ", column " << colNo + 1
			     << ": The literal " << errInt << " is previously defined as \""
			     << errGate->getTypeStr() << "\"!" << endl;
			break;

		case UNDEFINED_FANIN:
			cerr << "[Error] For the gate with ID " << errGate->getGateID() << ", its "
			     << errStr << "fanin with ID " << errInt << " is not defined!" << endl;
			break;

		case ILLEGAL_SYMBOL_TYPE:
			cerr << "[Error] At line " << lineNo << ", column " << colNo + 1
			     << ": Illegal symbol type \'" << getCurChar()
			     << "\', it can only be \"ilobcjf\"!" << endl;
			break;

		case REDEF_SYMBOL_NAME:
			cerr << "[Error] At line " << lineNo
			     << ": The name of " << errStr << " " << errInt << " is redefined!" << endl;
			break;

		case UNDEFINED_PROP:
			cerr << "[Error] For " << errStr << ", the gate of literal "
			     << errInt << " is not defined!" << endl;
			break;

		case ILLEGAL_DEFAULT:
			cerr << "[Error] At line " << lineNo << ", column " << colNo + 1
			     << ": Illegal initialized value (" << errInt << ") for latch with ID "
			     << errGate->getGateID() << ", it can only be 0, 1 or itself!" << endl;
			break;
	}
	return false;
}

AigNtk*
AigParser::parseAig(const char* fileName)
{
	if(!initParsing(fileName) ||
	   !parseHeader()      ||
	   !parseInput()       ||
	   !parseLatch()       ||
	   !parseOutput()      ||
	   !parseBad()         ||
	   !parseConstraint()  ||
	   !parseJustice()     ||
	   !parseFairness()    ||
	   !parseAnd()         ||
	   !checkFanIn()       ||
	   !checkProp()        ||
	   !parseSymbol()      ||
	   !parseComment())
		{ delete targetNtk; closeFile(); return 0; }
	closeFile();
	return targetNtk;
}

bool
AigParser::initParsing(const char* fileName)
{
	targetNtk = 0;
	inFile.open(fileName);
	if(!inFile.is_open())
		{ errStr = fileName; return parseError(CANNOT_OPEN_FILE); }
	lineNo = 0;
	targetNtk = new AigNtk(fileName);
	return true;
}

bool
AigParser::parseHeader()
{
	if(!readOneLine())
		return false;
	if(!checkDef(false, getErrIdentifier()) || !checkIdentifier())
		return false;
	const string portStr[TOTAL_PARSE_PORTS] = 
		{ "max variable ID", "PIs", "latches", "POs", "AND gates",
		  "bad state properties", "invariant constraints",
		  "justice properties", "fairness constraints" };
	for(unsigned i = 0; i < OLD_PARSE_PORTS; ++i)
		if(!getUInt(true, num[i], "the number of " + portStr[i]))
			return false;
	for(unsigned i = BAD; i < TOTAL_PARSE_PORTS; ++i)
		num[i] = 0;
	for(unsigned i = BAD; !isEndOfLine(); ++i)
		if(i == TOTAL_PARSE_PORTS)
			return parseError(MISSING_NEWLINE);
		else if(!getUInt(true, num[i], "the number of " + portStr[i]))
			return false;
	if(!checkMaxID())
		return false;

	targetNtk->PIList   .reserve(num[PI]);
	targetNtk->POList   .reserve(num[PO]);
	targetNtk->latchList.reserve(num[LATCH]);
	targetNtk->gateList .resize(num[MAX]+1+num[PO], 0);
	badLits       .resize(num[BAD]);
	constraintLits.resize(num[CONSTRAINT]);
	justiceLits   .resize(num[JUSTICE]);
	fairnessLits  .resize(num[FAIRNESS]);

	return checkEOF();
}

bool
AigParser::parseOutput()
{
	for(unsigned i = 0; i < num[PO]; ++i)
	{
		if(!readOneLine())
			return false;
		unsigned litID;
		if(!checkID(false, litID, "the input of PO"))
			return false;
		targetNtk->createOutputInt(num[MAX]+1+i);
		targetNtk->getGate(num[MAX]+1+i)->setFanIn0(litID);
		if(!isEndOfLine())
			return parseError(MISSING_NEWLINE);
		if(!checkEOF())
			return false;
	}
	return true;
}

bool
AigParser::parseBad()
{
	for(unsigned i = 0; i < num[BAD]; ++i)
	{
		if(!readOneLine())
			return false;
		if(!checkID(false, badLits[i], "bad state property"))
			return false;
		if(!isEndOfLine())
			return parseError(MISSING_NEWLINE);
		if(!checkEOF())
			return false;
	}
	if(num[BAD] != 0)
		cout << "[Note] Ignore " << num[BAD] << " bad state property!" << endl;
	return true;
}

bool
AigParser::parseConstraint()
{
	for(unsigned i = 0; i < num[CONSTRAINT]; ++i)
	{
		if(!readOneLine())
			return false;
		if(!checkID(false, constraintLits[i], "invariant constraint"))
			return false;
		if(!isEndOfLine())
			return parseError(MISSING_NEWLINE);
		if(!checkEOF())
			return false;
	}
	if(num[CONSTRAINT] != 0)
		cout << "[Note] Ignore " << num[CONSTRAINT] << " invariant constraint!" << endl;
	return true;
}

bool
AigParser::parseJustice()
{
	for(unsigned i = 0; i < num[JUSTICE]; ++i)
	{
		if(!readOneLine())
			return false;
		unsigned justiceNum;
		if(!getUInt(false, justiceNum, "the number of local constraints of justice property"))
			return false;
		justiceLits[i].resize(justiceNum);
		if(!isEndOfLine())
			return parseError(MISSING_NEWLINE);
		if(!checkEOF())
			return false;
	}
	unsigned localNum = 0;
	for(unsigned i = 0; i < num[JUSTICE]; localNum += justiceLits[i++].size())
		for(size_t j = 0, n = justiceLits[i].size(); j < n; ++j)
		{
			if(!readOneLine())
				return false;
			if(!checkID(false, justiceLits[i][j], "local constraint of justice property"))
				return false;
			if(!isEndOfLine())
				return parseError(MISSING_NEWLINE);
			if(!checkEOF())
				return false;
		}
	if(num[JUSTICE] != 0)
		cout << "[Note] Ignore " << num[JUSTICE] << " justice property ("
		     << localNum << " local constraint)!" << endl;
	return true;
}

bool
AigParser::parseFairness()
{
	for(unsigned i = 0; i < num[FAIRNESS]; ++i)
	{
		if(!readOneLine())
			return false;
		if(!checkID(false, fairnessLits[i], "fairness constraint"))
			return false;
		if(!isEndOfLine())
			return parseError(MISSING_NEWLINE);
		if(!checkEOF())
			return false;
	}
	if(num[FAIRNESS] != 0)
		cout << "[Note] Ignore " << num[FAIRNESS] << " fairness constraint!" << endl;
	return true;
}

bool
AigParser::parseSymbol()
{
	sArray<Array<bool>, TOTAL_PARSE_PORTS> nameList;
	for(unsigned i = PI; i <= PO; ++i)
		nameList[i].init(num[i]);
	for(unsigned i = BAD; i < TOTAL_PARSE_PORTS; ++i)
		nameList[i].init(num[i]);
	const string portStr[TOTAL_PARSE_PORTS] = 
		{ "", "PI", "latch", "PO", "",
		  "bad state property", "invariant constraint",
		  "justice property", "fairness constraint" };
	unsigned ignoreName = 0;
	while(true)
	{
		if(!readOneLine())
			return false;
		if(isEndOfLine())
			return isEOF() ? true : (errStr = "symbol type", parseError(MISSING_DEFINITION));
		char typeChar = getCurChar();
		incColNo();
		ParsePort type;
		switch(typeChar)
		{
			case 'c':
				if(isEndOfLine()) {
					if(ignoreName != 0)
						cout << "[Note] Ignore " << ignoreName << " symbol name!" << endl;
					return true; }
			          type = CONSTRAINT; break;
			case 'i': type = PI;         break;
			case 'l': type = LATCH;      break;
			case 'o': type = PO;         break;
			case 'b': type = BAD;        break;
			case 'j': type = JUSTICE;    break;
			case 'f': type = FAIRNESS;   break;
			default :
				resetColNo();
				return parseError(isPrint() ? ILLEGAL_SYMBOL_TYPE : ILLEGAL_CHARACTER);
		}
		unsigned portID;
		if(!getUInt(false, portID, "the index of " + portStr[type]))
			return false;
		if(portID >= num[type])
			{ errStr = "The index of " + portStr[type]; errInt = portID; setColNo();
			  return parseError(NUMBER_TOO_BIG); }
		if(nameList[type][portID])
			{ errStr = portStr[type]; errInt = portID; return parseError(REDEF_SYMBOL_NAME); }
		nameList[type][portID] = true;
		if(!checkDef(true, "the symbol name of " + portStr[type]))
			return false;
		for(; !isEndOfLine(); incColNo())
			if(!isPrint())
				return parseError(ILLEGAL_CHARACTER);
		if(!checkEOF())
			return false;
		ignoreName += 1;
	}
}

bool
AigParser::parseComment()
{
	//different to the standard format
	//after the character 'c' tailing with a newline, everything is acceptable
	//just generate warning for the non-standard behavior
	unsigned unprint = 0;
	do
	{
		if(!readOneLine())
			return false;
		for(; !isEndOfLine(); incColNo())
			if(!isPrint())
				unprint += 1;
	} while(!isEOF());
	if(unprint != 0)
		cerr << "[Warning] Total " << unprint << " unprintable character(s) in the comment!" << endl;
	if(buffer.length() != 0)
		cerr << "[Warning] At line " << lineNo << ", column " << colNo + 1
		     << ": Expect a newline after the comment!" << endl;
	return true;
}

bool
AigParser::checkIdentifier()
{
	setOldColNo();
	for(unsigned i = 0; getIdentifier()[i] != '\0'; ++i, incColNo())
		if(isEndOfLine())
			{ setColNo(); return parseError(ILLEGAL_IDENTIFIER); }
		else if(getCurChar() != getIdentifier()[i])
			return getErrorToken() ? parseError(ILLEGAL_IDENTIFIER) : false;
	if(!isEndOfLine() && !isSpace())
		return getErrorToken() ? parseError(ILLEGAL_IDENTIFIER) : false;
	return true;
}

bool
AigParser::checkID(bool leadSpace, unsigned& litID, const string& err)
{
	if(!getUInt(leadSpace, litID, "the literal of " + err))
		return false;
	if(litID / 2 > num[MAX])
		{ errInt = litID; errStr = err; setColNo(); return parseError(MAX_LIT_ID); }
	return true;
}

bool
AigParser::checkGate(unsigned& litID, const string& err)
{
	if(!checkID(false, litID, err))
		return false;
	if(litID % 2 != 0)
		{ errStr = err; errInt = litID; setColNo(); return parseError(CANNOT_BE_INVERTED); }
	if(litID < 2)
		{ errStr = err; setColNo(); return parseError(REDEF_CONST); }
	AigGate* gate = targetNtk->getGate(litID/2);
	if(gate != 0)
		{ errInt = litID; setColNo(); errGate = gate; return parseError(REDEF_GATE); }
	return true;
}

bool
AigParser::checkDef(bool leadSpace, const string& definition)
{
	//after a leading space(depend on leadSpace), there must be something
	if(leadSpace)
	{
		if(isEndOfLine() || !isSpace())
			return parseError(MISSING_SPACE);
		incColNo();
	}
	if(isEndOfLine() || isSpace())
		{ errStr = definition; return parseError(MISSING_DEFINITION); }
	return true;
}

bool
AigParser::readOneLine()
{
	incLineNo();
	resetColNo();
	getline(inFile, buffer, '\n');
	if(inFile.fail() && !inFile.eof())
		return parseError(UNKNOWN_ACCIDENT);
	return true;
}

bool
AigParser::getUInt(bool leadSpace, unsigned& num, const string& err)
{
	if(!checkDef(leadSpace, err))
		return false;
	num = 0;
	setOldColNo();
	bool warning = false;
	bool overflow = false;
	for(; !isEndOfLine() && !isSpace(); incColNo())
		if(isDigit())
		{
			unsigned tmp = num;
			num *= 10;
			num += unsigned(getCurChar()-'0');
			if(tmp > num)
				overflow = true;
			if(tmp == 0 && colNo != oldColNo)
				warning = true;
		}
		else
			return getErrorToken() ? parseError(ILLEGAL_NUMBER) : false;
	if(overflow)
		{ setErrStr(); return parseError(NUMBER_OVERFLOW); }
	if(warning)
		cerr << "[Warning] At line " << lineNo << ", column " << oldColNo + 1
		     << ": Leading zero(s) on a number should be avoided!" << endl;
	return true;
}

bool
AigParser::getErrorToken()
{
	//if entering this section
	//the characters between oldColNo and colNo are legal with respect to the need
	//hence just start at the current colNo
	for(; !isEndOfLine() && !isSpace(); incColNo())
		if(!isPrint())
			return parseError(ILLEGAL_CHARACTER);
	setErrStr();
	return true;
}

bool
AigAsciiParser::parseInput()
{
	for(unsigned i = 0; i < num[PI]; ++i)
	{
		if(!readOneLine())
			return false;
		unsigned litID;
		if(!checkGate(litID, "PI"))
			return false;
		targetNtk->createInputInt(litID/2);
		if(!isEndOfLine())
			return parseError(MISSING_NEWLINE);
		if(!checkEOF())
			return false;
	}
	return true;
}

bool
AigAsciiParser::parseLatch()
{
	unsigned ignoreDefault = 0;
	for(unsigned i = 0; i < num[LATCH]; ++i)
	{
		if(!readOneLine())
			return false;
		unsigned litID;
		if(!checkGate(litID, "latch"))
			return false;
		targetNtk->createLatchInt(litID/2);
		AigGate* gate = targetNtk->getGate(litID/2);
		if(!checkID(true, litID, "the input of latch"))
			return false;
		gate->setFanIn0(litID);
		if(!isEndOfLine())
		{
			unsigned initValue;
			if(!getUInt(true, initValue, "initialized value of latch"))
				return false;
			if(initValue != 0 && initValue != 1 && initValue != i * 2)
				{ errGate = gate; errInt = initValue; return parseError(ILLEGAL_DEFAULT); }
			if(!isEndOfLine())
				return parseError(MISSING_NEWLINE);
			ignoreDefault += 1;
		}
		if(!checkEOF())
			return false;
	}
	if(ignoreDefault != 0)
		cout << "[Note] Ignore " << ignoreDefault << " initialized value!" << endl;
	return true;
}

bool
AigAsciiParser::parseAnd()
{
	for(unsigned i = 0; i < num[AND]; ++i)
	{
		if(!readOneLine())
			return false;
		unsigned litID;
		if(!checkGate(litID, "AND gate"))
			return false;
		targetNtk->createAndInt(litID/2);
		AigGate* gate = targetNtk->getGate(litID/2);
		const string ordinal[] = { "first", "second" };
		for(unsigned j = 0; j < 2; ++j)
			if(!checkID(true, litID, "the " + ordinal[j] + " input of AND gate"))
				return false;
			else
				gate->setFanIn(j, litID);
		if(!isEndOfLine())
			return parseError(MISSING_NEWLINE);
		if(!checkEOF())
			return false;
	}
	return true;
}

bool
AigAsciiParser::checkFanIn()
{
	for(AigGateID i = 0, m = targetNtk->getMaxGateNum(); i < m; ++i)
		if(AigGate* g = targetNtk->getGate(i); g != 0)
			switch(g->getFanInNum())
			{
				case 1:
					if(!g->getFanIn0().setRealValueCheck(targetNtk))
						{ errGate = g; errStr = ""; errInt = g->getFanIn0().getValue()/2;
						  return parseError(UNDEFINED_FANIN); }
					break;

				case 2:
					if(!g->getFanIn0().setRealValueCheck(targetNtk))
						{ errGate = g; errStr = "first ";  errInt = g->getFanIn0().getValue()/2;
						  return parseError(UNDEFINED_FANIN); }
					if(!g->getFanIn1().setRealValueCheck(targetNtk))
						{ errGate = g; errStr = "second "; errInt = g->getFanIn1().getValue()/2;
						  return parseError(UNDEFINED_FANIN); }
					break;

				default: break;
			}
		else targetNtk->recycleList.push_back(i);
	return true;
}

bool
AigAsciiParser::checkProp()
{
	for(size_t i = 0, B = badLits.size(); i < B; ++i)
		if(targetNtk->getGate(badLits[i]/2) == 0)
			{ errStr = "bad " + to_string(i); errInt = badLits[i];
			  return parseError(UNDEFINED_PROP); }
	for(size_t i = 0, C = constraintLits.size(); i < C; ++i)
		if(targetNtk->getGate(constraintLits[i]/2) == 0)
			{ errStr = "constraint " + to_string(i); errInt = constraintLits[i];
			  return parseError(UNDEFINED_PROP); }
	for(size_t i = 0, J = justiceLits.size(); i < J; ++i)
		for(size_t j = 0, JSub = justiceLits[i].size(); j < JSub; ++j)
			if(targetNtk->getGate(justiceLits[i][j]/2) == 0)
				{ errStr = "justice " + to_string(i) + "-" + to_string(j);
				  errInt = justiceLits[i][j]; return parseError(UNDEFINED_PROP); }
	for(size_t i = 0, F = fairnessLits.size(); i < F; ++i)
		if(targetNtk->getGate(fairnessLits[i]/2) == 0)
			{ errStr = "fairness " + to_string(i); errInt = fairnessLits[i];
			  return parseError(UNDEFINED_PROP); }
	return true;
}

bool
AigAsciiParser::checkMaxID()
{
	if(num[MAX] < num[PI] + num[LATCH] + num[AND])
		{ colNo = strlen(getIdentifier())+1; errStr = "The number of max variable ID"; errInt = num[MAX];
		  return parseError(NUMBER_TOO_SMALL); }
	return true;
}

bool
AigBinaryParser::parseInput()
{
	for(unsigned i = 1, m = i + num[PI]; i < m ; ++i)
		targetNtk->createInputInt(i);
	return true;
}

bool
AigBinaryParser::parseLatch()
{
	unsigned ignoreDefault = 0;
	for(unsigned i = 1 + num[PI], m = i + num[LATCH]; i < m; ++i)
	{
		targetNtk->createLatchInt(i);
		AigGate* gate = targetNtk->getGate(i);
		if(!readOneLine())
			return false;
		unsigned litID;
		if(!checkID(false, litID, "the input of latch"))
			return false;
		gate->setFanIn0(litID);
		if(!isEndOfLine())
		{
			unsigned initValue;
			if(!getUInt(true, initValue, "initialized value of latch"))
				return false;
			if(initValue != 0 && initValue != 1 && initValue != i * 2)
				{ errGate = gate; errInt = initValue; return parseError(ILLEGAL_DEFAULT); }
			if(!isEndOfLine())
				return parseError(MISSING_NEWLINE);
			ignoreDefault += 1;
		}
		if(!checkEOF())
			return false;
	}
	if(ignoreDefault != 0)
		cout << "[Note] Ignore " << ignoreDefault << " initialized value!" << endl;
	return true;
}

bool
AigBinaryParser::parseAnd()
{
	incLineNo(); resetColNo();
	for(unsigned lhs = (1 + num[PI] + num[LATCH]) * 2, m = lhs + num[AND] * 2; lhs < m; lhs += 2)
	{
		targetNtk->createAndInt(lhs/2);
		AigGate* gate = targetNtk->getGate(lhs/2);
		unsigned delta;
		if(!decode(delta))
			return false;
		if(delta > lhs)
			{ errStr = "The delta"; errInt = delta; setColNo();
			  return parseError(NUMBER_TOO_BIG); }
		unsigned rhs0 = lhs - delta;
		gate->setFanIn0(targetNtk->getGate(rhs0/2), rhs0%2);
		if(!decode(delta))
			return false;
		if(delta > rhs0)
			{ errStr = "The delta"; errInt = delta; setColNo();
			  return parseError(NUMBER_TOO_BIG); }
		unsigned rhs1 = rhs0 - delta;
		gate->setFanIn1(targetNtk->getGate(rhs1/2), rhs1%2);
	}
	return true;
}

bool
AigBinaryParser::checkFanIn()
{
	for(size_t i = 0, L = targetNtk->getLatchNum(); i < L; ++i)
		targetNtk->getLatch(i)->getFanIn0().setRealValue(targetNtk);
	for(size_t i = 0, O = targetNtk->getOutputNum(); i < O; ++i)
		targetNtk->getOutput(i)->getFanIn0().setRealValue(targetNtk);
	return true;
}

bool
AigBinaryParser::checkMaxID()
{
	if(num[MAX] < num[PI] + num[LATCH] + num[AND])
		{ colNo = strlen(getIdentifier())+1; errStr = "The number of max variable ID"; errInt = num[MAX];
		  return parseError(NUMBER_TOO_SMALL); }
	if(num[MAX] > num[PI] + num[LATCH] + num[AND])
		{ colNo = strlen(getIdentifier())+1; errStr = "The number of max variable ID"; errInt = num[MAX];
		  return parseError(NUMBER_TOO_BIG); }
	return true;
}

//modified from the AIGER document by Armin Biere
bool
AigBinaryParser::decode(unsigned& num)
{
	constexpr unsigned  int maxI = (sizeof(unsigned) * 8) / 7;
	constexpr unsigned char maxC = 1 << ((sizeof(unsigned) * 8) % 7);
	constexpr char GET_VALUE_MASK = 0x7F;
	constexpr char CHECK_END_MASK = 0x80;
	num = 0;
	unsigned i = 0;
	char c;
	setOldColNo();
	do
	{
		if(isEOF())
			{ errStr = "the binary encoding of AND gate"; return parseError(UNEXPECTED_EOF); }
		if(inFile.get(c).fail())
			return parseError(UNKNOWN_ACCIDENT);
		incColNo();
		//for "(unsigned char)c >= maxC"
		//1. if c & 0x80 = 0x80, violate trivially
		//2. if c & 0x80 = 0, compare c & 0x7F and maxC
		if(i == maxI && (unsigned char)c >= maxC)
			{ setColNo(); return parseError(NUMBER_OVERFLOW); }
		num |= unsigned(c & GET_VALUE_MASK) << (7 * i++);
	} while(c & CHECK_END_MASK);
	return true;
}

}
