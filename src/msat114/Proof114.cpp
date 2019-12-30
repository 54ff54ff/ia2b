/*========================================================================\
|: [Filename] Proof114.cpp                                               :|
:| [Author]   Chiang Chun-Yi                                             |:
|: [Synopsis] Modify some part of the proof class                        :|
<------------------------------------------------------------------------*/

/*****************************************************************************************[Proof.C]
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

#include "Proof114.h"
#include "Sort114.h"

#include <assert.h>
#include <iostream>
using namespace std;

namespace Minisat114 {

ClauseId
Proof::addRoot(vec<Lit>& cl)
{
	cl.copyTo(clause);
	sortUnique(clause);

	addClauseInfo();
	putUInt(index(clause[0]) << 1);
	for (int i = 1; i < clause.size(); i++)
		putUInt(index(clause[i]) - index(clause[i-1]));
	putUInt(0);     // (0 is safe terminator since we removed duplicates)

	return id_counter++;
}

void
Proof::beginChain(ClauseId start)
{
	assert(start != ClauseId_NULL);
	chain_id .clear();
	chain_var.clear();
	chain_id.push(start);
}

void
Proof::resolve(ClauseId next, Var x)
{
	assert(next != ClauseId_NULL);
	chain_id .push(next);
	chain_var.push(x);
}

ClauseId
Proof::endChain()
{
	assert(chain_id.size() == chain_var.size() + 1);
	if (chain_id.size() == 1)
		return chain_id[0];
	else
	{
		for(int i = 0; i < chain_id.size(); ++i)
			assert(id_counter > chain_id[i]);
		addClauseInfo();
		putUInt(((id_counter - chain_id[0]) << 1) | 1);
		for (int i = 0; i < chain_var.size(); i++)
			putUInt(chain_var[i] + 1),
			putUInt(id_counter - chain_id[i+1]);
		putUInt(0);

		return id_counter++;
	}
}

void
Proof::deleted(ClauseId gone)
{
	putUInt(((id_counter - gone) << 1) | 1);
	putUInt(0);
}

void
Proof::putUInt(uint64 val)
{
	if (val < 0x20000000){
		uint v = (uint)val;
		if (v < 0x80)
			fputc(v, tmpFile);
		else{
			if (v < 0x2000)
				fputc(0x80 | (v >> 8), tmpFile),
				fputc((uchar)v,        tmpFile);
			else if (v < 0x200000)
				fputc(0xA0 | (v >> 16), tmpFile),
				fputc((uchar)(v >> 8),  tmpFile),
				fputc((uchar)v,         tmpFile);
			else
				fputc(0xC0 | (v >> 24), tmpFile),
				fputc((uchar)(v >> 16), tmpFile),
				fputc((uchar)(v >> 8),  tmpFile),
				fputc((uchar)v,         tmpFile);
		}
	}else
		fputc(0xE0,               tmpFile),
		fputc((uchar)(val >> 56), tmpFile),
		fputc((uchar)(val >> 48), tmpFile),
		fputc((uchar)(val >> 40), tmpFile),
		fputc((uchar)(val >> 32), tmpFile),
		fputc((uchar)(val >> 24), tmpFile),
		fputc((uchar)(val >> 16), tmpFile),
		fputc((uchar)(val >> 8),  tmpFile),
		fputc((uchar)val,         tmpFile);
}

uint64
Proof::getUInt()
{
    uint byte0, byte1, byte2, byte3, byte4, byte5, byte6, byte7;
    byte0 = fgetc(tmpFile);
    if (byte0 == (uint)EOF)
        throw Exception_EOF();
    if (!(byte0 & 0x80))
        return byte0;
    else{
        switch ((byte0 & 0x60) >> 5){
        case 0:
            byte1 = fgetc(tmpFile);
            return ((byte0 & 0x1F) << 8) | byte1;
        case 1:
            byte1 = fgetc(tmpFile);
            byte2 = fgetc(tmpFile);
            return ((byte0 & 0x1F) << 16) | (byte1 << 8) | byte2;
        case 2:
            byte1 = fgetc(tmpFile);
            byte2 = fgetc(tmpFile);
            byte3 = fgetc(tmpFile);
            return ((byte0 & 0x1F) << 24) | (byte1 << 16) | (byte2 << 8) | byte3;
        default:
            byte0 = fgetc(tmpFile);
            byte1 = fgetc(tmpFile);
            byte2 = fgetc(tmpFile);
            byte3 = fgetc(tmpFile);
            byte4 = fgetc(tmpFile);
            byte5 = fgetc(tmpFile);
            byte6 = fgetc(tmpFile);
            byte7 = fgetc(tmpFile);
            return ((uint64)((byte0 << 24) | (byte1 << 16) | (byte2 << 8) | byte3) << 32)
                 |  (uint64)((byte4 << 24) | (byte5 << 16) | (byte6 << 8) | byte7);
        }
    }
}

}
