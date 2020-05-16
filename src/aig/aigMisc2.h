/*========================================================================\
|: [Filename] aigMisc2.h                                                 :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Define various wrapper classes to perform operations on    :|
:|            AIG network                                                |:
|:            5. AigConster to calculate the set of state variables      :|
:|               reachable at only negative polarity                     |:
<------------------------------------------------------------------------*/

#ifndef HEHE_AIGMISC2_H
#define HEHE_AIGMISC2_H

#include "aigNtk.h"
#include "cirSolver.h"

namespace _54ff
{

class AigConster
{
public:
	AigConster(AigNtk* n): ntk(n) {}

	bool doSimpMono();
	bool doSimpPdr(size_t);

private:
	void replaceWithZero(const vector<AigGateID>&);

private:
	AigNtk*  ntk;
};

}

#endif