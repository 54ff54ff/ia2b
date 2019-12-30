/*========================================================================\
|: [Filename] aigPaser.h                                                 :|
:| [Author]   Chung-Yang (Ric) Huang, Chiang Chun-Yi                     |:
|: [Synopsis] Define the AIGER parser for aig network                    :|
<------------------------------------------------------------------------*/

#ifndef HEHE_AIGPARSER_H
#define HEHE_AIGPARSER_H

#include <ctype.h>
#include <limits.h>
#include <string>
#include <fstream>
#include "aigNtk.h"
using namespace std;

namespace _54ff
{

extern AigParser* const aigParser[TOTAL_PARSE_TYPE];

class AigParser
{
protected:
enum AigParseError
{
	CANNOT_OPEN_FILE,
	MISSING_SPACE,
	EXTRA_SPACE,
	MISSING_DEFINITION,
	ILLEGAL_CHARACTER,
	ILLEGAL_NUMBER,
	NUMBER_OVERFLOW,
	ILLEGAL_IDENTIFIER,
	UNKNOWN_ACCIDENT,
	UNEXPECTED_EOF,
	MISSING_NEWLINE,
	NUMBER_TOO_BIG,
	NUMBER_TOO_SMALL,
	MAX_LIT_ID,
	CANNOT_BE_INVERTED,
	REDEF_CONST,
	REDEF_GATE,
	UNDEFINED_FANIN,
	ILLEGAL_SYMBOL_TYPE,
	REDEF_SYMBOL_NAME,
	UNDEFINED_PROP,
	ILLEGAL_DEFAULT
};
enum ParsePort { MAX = 0, PI, LATCH, PO, AND, OLD_PARSE_PORTS,
                 BAD = 5, CONSTRAINT, JUSTICE, FAIRNESS,
                 TOTAL_PARSE_PORTS };

public:
	AigParser() {}
	virtual ~AigParser() {}

	AigNtk* parseAig(const char*);

protected:
	ifstream                  inFile;
	unsigned                  lineNo;
	unsigned                  oldColNo;
	unsigned                  colNo;
	string                    buffer;
	string                    errMsg;
	unsigned                  errInt;
	AigGate*                  errGate;
	AigNtk*                   targetNtk;
	unsigned                  num[TOTAL_PARSE_PORTS];
	vector<unsigned>          badLits;
	vector<unsigned>          constraintLits;
	vector<vector<unsigned>>  justiceLits;
	vector<unsigned>          fairnessLits;

	/*====================================*/

	virtual encodeType getEncodeType()const = 0;

	/*====================================*/

	bool initParsing(const char*);
	bool parseHeader();
	virtual bool parseInput() = 0;
	virtual bool parseLatch() = 0;
	bool parseOutput();
	bool parseBad();
	bool parseConstraint();
	bool parseJustice();
	bool parseFairness();
	virtual bool parseAnd() = 0;
	virtual bool checkFanIn() = 0;
	virtual bool checkProp() = 0;
	bool parseSymbol();
	bool parseComment();

	/*====================================*/

	bool checkIdentifier();
	virtual const char* getIdentifier()const = 0;
	string getErrIdentifier() { return "identifier \"" + string(getIdentifier()) + "\""; }
	virtual bool checkMaxID() = 0;
	bool checkID(bool, unsigned&, const string&);
	bool checkGate(unsigned&, const string&);
	bool checkDef(bool, const string&);
	bool readOneLine();
	bool isEOF()const { return inFile.eof(); }
	bool checkEOF() { return isEOF() ? (errMsg = "a newline character", parseError(UNEXPECTED_EOF)) : true; }
	bool getUInt(bool, unsigned&, const string&);
	bool getErrorToken();
	char getCurChar()const { return buffer[colNo]; }
	bool isEndOfLine()const { return colNo == buffer.length(); }
	bool isSpace()const { return getCurChar() == ' '; }
	bool isPrint()const { return isprint(getCurChar()); }
	bool isDigit()const { return isdigit(getCurChar()); }
	bool parseError(AigParseError)const;
	void incOldColNo() { oldColNo += 1; }
	void incColNo() { colNo += 1; }
	void setColNo() { colNo = oldColNo; }
	void resetColNo() { colNo = 0; }
	void setOldColNo() { oldColNo = colNo; }
	void incLineNo() { lineNo += 1; }
	void closeFile() { inFile.close(); }
	void setErrMsg() { errMsg = buffer.substr(oldColNo, colNo - oldColNo); setColNo(); }
};

class AigAsciiParser : public AigParser
{
protected:
	encodeType getEncodeType()const { return ASCII; }

	bool parseInput();
	bool parseLatch();
	bool parseAnd();
	bool checkFanIn();
	bool checkProp();
	const char* getIdentifier()const { return "aag"; }
	bool checkMaxID();
};

class AigBinaryParser : public AigParser
{
protected:
	encodeType getEncodeType()const { return BINARY; }

	bool parseInput();
	bool parseLatch();
	bool parseAnd();
	bool checkFanIn();
	bool checkProp() { return true; }
	const char* getIdentifier()const { return "aig"; }
	bool checkMaxID();

	bool decode(unsigned&);
};

}

#endif