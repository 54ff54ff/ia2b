/*========================================================================\
|: [Filename] condStream.cpp                                             :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Implement CondStream and the other utilities               :|
<------------------------------------------------------------------------*/

#include "condStream.h"

namespace _54ff
{

bool
Stream::setLogFile(const char* fileName)
{
	if(FStream* fs = checkOpenFile(fileName);
	   fs != 0)
		{ logFiles.push_back(fs); return true; }
	else return false;
}

FStream*
Stream::checkOpenFile(const char* fileName)
{
	FStream* retFS = 0;
	for(FStream* fs: openedFiles)
		if(fs->sameFile(fileName))
			{ retFS = fs; fs->incRefCount(); break; }
	if(retFS == 0)
	{
		FStream* fs = new FStream(fileName);
		if(fs->isValid())
			{ openedFiles.push_back(fs); retFS = fs; fs->incRefCount(); }
		else
			{ errMsg << "[Error] Cannot open file \"" << fileName << "\"!" << endl; delete fs; }
	}
	return retFS;
}

void
Stream::closeLogFile()
{}


void
Stream::closeLogFile(size_t)
{}

void
Stream::printSetting()const
{}

vector<FStream*> Stream::openedFiles;
Stream outMsg(cout);
Stream errMsg(cerr);

ostream& operator<<(ostream& os, const RepeatChar& rc)
{
	for(unsigned i = 0; i < rc.num; ++i)
		os << rc.ch;
	return os;
}

ostream& operator<<(ostream& os, const CleanStrOnTerminal& cs)
{
	const size_t len = strlen(cs.str);
	for(size_t i = 0; i < len; ++i) os << "\b";
	for(size_t i = 0; i < len; ++i) os << " ";
	return os;
}

}