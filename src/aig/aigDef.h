/*========================================================================\
|: [Filename] aigDef.h                                                   :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Define basic components in aig network                     :|
<------------------------------------------------------------------------*/

#ifndef HEHE_AIGDEF_H
#define HEHE_AIGDEF_H

#include <assert.h>
#include <string>
#include <limits>
using namespace std;

namespace _54ff
{

enum AigGateType
{
	AIG_PI = 0,
	AIG_LATCH,
	AIG_PO,
	AIG_AND,
	AIG_CONST0,

	AIG_TOTAL
};

extern const string aigTypeStr[AIG_TOTAL];

typedef unsigned AigGateID;
constexpr AigGateID UNDEF_GATEID = numeric_limits<AigGateID>::max();

using AigGateLit = unsigned;
inline AigGateID getGateID(AigGateLit lit) { return lit >> 1; }
inline bool isInv(AigGateLit lit) { return lit & 1; }
inline AigGateLit makeToLit(AigGateID id, bool inv) { return (id << 1) | AigGateLit(inv); }

constexpr AigGateLit UNDEF_GATELIT = numeric_limits<AigGateID>::max();
constexpr AigGateLit ERROR_GATELIT = numeric_limits<AigGateID>::max() - 1;

//forward declaration
class AigNtk;
class AigGateV;
class AigGate;
class AigPi;
class AigLatch;
class AigPo;
class AigAnd;
class AigConst0;
class AigParser;
class AigAsciiParser;
class AigBinaryParser;
class AigWriter;
class AigAsciiWriter;
class AigBinaryWriter;

}

#endif
