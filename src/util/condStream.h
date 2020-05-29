/*========================================================================\
|: [Filename] condStream.h                                               :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] 1. Define a wrapper class for ostream to activate it       :|
:|               conditionally based on the current setting              |:
|:            2. Define some utilities for CondStream to facilitate the  :|
:|               usage and make the code cleaner                         |:
<------------------------------------------------------------------------*/

#ifndef HEHE_CONDSTREAM_H
#define HEHE_CONDSTREAM_H

#include <iostream>
#include <fstream>
#include "util.h"
using namespace std;

namespace _54ff
{

class FStream
{
public:
	FStream(const char* fName)
	: fileName   (fName)
	, fileStream (fName)
	, refCount   (0) {}

	bool isValid()const { return fileStream.is_open(); }
	ofstream& getFileStream() { return fileStream; }
	const WrapStr& getFileName()const { return fileName; }

	void incRefCount() { refCount += 1; }
	void decRefCount() { refCount -= 1; }
	bool isRefZero()const { return refCount == 0; }

	bool sameFile(const char* fName)const { return strcmp(fileName, fName) == 0; }

private:
	WrapStr   fileName;
	ofstream  fileStream;
	unsigned  refCount;
};

class Stream
{

template<class T>
friend Stream& operator<<(Stream& s, T&& t)
{
	if(s.isOsON)
		s.os << forward<T>(t);
	for(FStream* fs: s.logFiles)
		(fs->getFileStream()) << forward<T>(t);
	return s;
}

using Manipulator = ostream&(*)(ostream&);
friend Stream& operator<<(Stream& s, Manipulator m)
{
	if(s.isOsON)
		m(s.os);
	for(FStream* fs: s.logFiles)
		m(fs->getFileStream());
	return s;
}

public:
	Stream(ostream& o)
	: os     (o)
	, isOsON (true) {}

	void setOsON(bool on) { isOsON = on; }

	void   setOsON() { setOsON(true); }
	void unsetOsON() { setOsON(false); }
	void  flipOsON() { setOsON(!isOsON); }

	bool setLogFile(const char*);
	void closeLogFile();
	void closeLogFile(size_t);

	size_t getLogNum()const { return logFiles.size(); }
	void printSetting()const;

private:
	ostream&          os;
	bool              isOsON;
	vector<FStream*>  logFiles;

private:
	static FStream* checkOpenFile(const char*);
	static vector<FStream*> openedFiles;
};

extern Stream outMsg;
extern Stream errMsg;

class CondStream
{

template<class T>
friend CondStream& operator<<(CondStream& cs, T&& t)
	{ if(cs.active) cs.os << forward<T>(t); return cs; }

using Manipulator = ostream&(*)(ostream&);
friend CondStream& operator<<(CondStream& cs, Manipulator m)
	{ if(cs.active) m(cs.os); return cs; }

public:
	CondStream(ostream& o, bool a = true)
	: os(o), active(a) {}

	bool isActive()const { return active; }

	void setActive(bool a) { active = a; }

	void   setActive() { setActive(true); }
	void unsetActive() { setActive(false); }
	void  flipActive() { setActive(!active); }

private:
	ostream&  os;
	bool      active;
};

class RepeatChar
{
friend ostream& operator<<(ostream&, const RepeatChar&);

public:
	RepeatChar(char c, unsigned n): ch(c), num(n) {}

private:
	char      ch;
	unsigned  num;
};

class CleanStrOnTerminal
{
friend ostream& operator<<(ostream&, const CleanStrOnTerminal&);

public:
	CleanStrOnTerminal(const char* s): str(s) {}

private:
	const char*  str;
};

template <class UIntType>
class CleanIntOnTerminal
{
static_assert(is_unsigned_v<UIntType> && is_integral_v<UIntType>);

friend ostream& operator<<(ostream& os, const CleanIntOnTerminal& ci)
{
	const size_t len = calDigit(ci.num);
	for(size_t i = 0; i < len; ++i) os << "\b";
	for(size_t i = 0; i < len; ++i) os << " ";
	return os;
}

public:
	CleanIntOnTerminal(UIntType n): num(n) {}

private:
	UIntType  num;
};

}

#endif
