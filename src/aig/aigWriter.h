/*========================================================================\
|: [Filename] aigWriter.h                                                :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Define the AIGER writer for aig network                    :|
<------------------------------------------------------------------------*/

#ifndef HEHE_AIGWRITER_H
#define HEHE_AIGWRITER_H

#include <fstream>
#include "aigNtk.h"
using namespace std;

namespace _54ff
{

extern AigWriter* const aigWriter[TOTAL_PARSE_TYPE];

class AigWriter
{
public:
	AigWriter() {}
	virtual ~AigWriter() {}

	bool writeAig(const char*, const AigNtk*);

protected:
	ofstream          outFile;
	const AigNtk*     targetNtk;
	Array<AigGateID>  idMap;

protected:
	virtual bool check() = 0;
	void writeHeader();
	AigGateID getFirstOutputID()const { return targetNtk->getGateNum() - targetNtk->getOutputNum(); }
	AigGateID getMaxVarID()const { return getFirstOutputID() - 1; }
	virtual void writeInput() = 0;
	virtual void writeLatch() = 0;
	void writeOutput();
	virtual void writeAnd() = 0;
	void writeComment();

	virtual const char* getIdentifier()const = 0;
};

class AigAsciiWriter : public AigWriter
{
protected:
	bool check();
	void writeInput();
	void writeLatch();
	void writeAnd();

	const char* getIdentifier()const { return "aag"; }
};

class AigBinaryWriter : public AigWriter
{
protected:

protected:
	bool check();
	void writeInput() {}
	void writeLatch();
	void writeAnd();

	const char* getIdentifier()const { return "aig"; }

	void decode(unsigned);

	vector<AigAnd*>  dfsList;
};

}

#endif