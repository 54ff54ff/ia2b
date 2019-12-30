/*========================================================================\
|: [Filename] Proof114.h                                                 :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Rework on the proof class in MiniSAT 1.14                  :|
<------------------------------------------------------------------------*/

/*****************************************************************************************[Proof.h]
MiniSat -- Copyright (c) 2003-2005, Niklas Een, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#ifndef Minisat114_PROOF_H
#define Minisat114_PROOF_H

#include <stdio.h>
#include "SolverTypes114.h"

namespace Minisat114 {

class Proof
{
public:
	Proof(): tmpFile(tmpfile()), id_counter(0) {}
	~Proof() { fclose(tmpFile); }

	ClauseId addRoot   (vec<Lit>& clause);
	void     beginChain(ClauseId start);
	void     resolve   (ClauseId next, Var x);
	ClauseId endChain  ();
	void     deleted   (ClauseId gone);
    ClauseId last      () { assert(id_counter != ClauseId_NULL); return id_counter - 1; }
	ClauseId now       () { return id_counter; }

	void   putUInt(uint64 val);
	uint64 getUInt();

	void backToBegin() { fseek(tmpFile, 0, SEEK_SET); }
	void goToEnd()     { fseek(tmpFile, 0, SEEK_END); }
	void finish() { fflush(tmpFile); }
	long tell()const { return ftell(tmpFile); }

	void seekPos    (int i)      { fseek(tmpFile, getPos(i), SEEK_SET); }
	void setToOnSet (int i)      { clauseInfo[i] |=  size_t(2); }
	void setToOffSet(int i)      { clauseInfo[i] &= ~size_t(2); }
	void setToCore  (int i)      { clauseInfo[i] |=  size_t(1); }
	void unsetCore  (int i)      { clauseInfo[i] &= ~size_t(1); }
	void setToLocal (int i)      { varLocalToA[i] = true; }
	void unsetLocal (int i)      { varLocalToA[i] = false; }
	long getPos     (int i)const { return clauseInfo[i] >> 2; }
	bool inOnSet    (int i)const { return clauseInfo[i] & size_t(2); }
	bool inUnsatCore(int i)const { return clauseInfo[i] & size_t(1); }
	bool localToA   (int i)const { return varLocalToA[i]; }
	int  getClsNum  ()     const { return clauseInfo.size(); }

	void addVar() { varLocalToA.push(false); }

private:
	void addClauseInfo() { clauseInfo.push(size_t(tell()) << 2); assert(tell() == getPos(id_counter)); }

private:
	FILE*          tmpFile;
    ClauseId       id_counter;
    vec<Lit>       clause;
    vec<ClauseId>  chain_id;
    vec<Var>       chain_var;
	vec<size_t>    clauseInfo; // equivalent to size_t pos  : 30;
	                           //               size_t on   :  1; -> default = false;
	                           //               size_t core :  1; -> default = false;
	vec<bool>      varLocalToA;
};

class Exception_EOF {};

}

#endif
