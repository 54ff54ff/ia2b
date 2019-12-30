/*========================================================================\
|: [Filename] bmcChecker.h                                               :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Define the checkers about bounded model checking           :|
<------------------------------------------------------------------------*/

#ifndef HEHE_BMCCHECKER_H
#define HEHE_BMCCHECKER_H

#include "sfcChecker.h"

namespace _54ff
{

enum BmcCheckType
{
	BMC_ASSERT,
	BMC_ONLY_LAST
};

class BmcChecker : public SafetyChecker
{
public:
	BmcChecker(AigNtk*, size_t, bool, size_t, size_t, BmcCheckType);
	~BmcChecker() {}

protected:
	void check();

protected:
	size_t        maxDepth;
	BmcCheckType  type;
};

enum IndCheckType
{
	IND_SIMPLE_NO,
	IND_SIMPLE_NEED,
	IND_SIMPLE_ALL
};

class IndChecker : public SafetyChecker
{
public:
	IndChecker(AigNtk*, size_t, bool, size_t, size_t, IndCheckType);
	~IndChecker() {}

protected:
	void check();

protected:
	size_t        maxDepth;
	IndCheckType  type;
};

enum ItpCheckType
{
	ITP_ALL_NOT_P,
	ITP_ASSERT,
	ITP_ONLY_LAST
};

class ItpChecker : public SafetyChecker
{
public:
	ItpChecker(AigNtk*, size_t, bool, size_t, size_t, ItpCheckType);
	~ItpChecker() {}

protected:
	void check();

protected:
	size_t        maxDepth;
	ItpCheckType  type;
};

}

#endif