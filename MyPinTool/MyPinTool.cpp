/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2014 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */
/*
 *  This file contains an ISA-portable PIN tool for tracing memory accesses.
 */

#include <stdio.h>
#include "pin.H"
#include <map>
#include <vector>
#include <iostream>
#include <fstream>

using namespace std;


static const unsigned int BUFFERSIZE = (uint64_t)1 << 20;
static const uint64_t MASK = ((uint64_t)1 << 20) - 1;

namespace PageStatistic{
	ofstream out;
	map<uint64_t, uint64_t> pages;

	VOID RecordMemRead(VOID * ip, VOID * addr){
	    pages[(uint64_t)addr >> 12]++;
	}

	VOID RecordMemWrite(VOID * ip, VOID * addr){
	    pages[(uint64_t)addr >> 12]++;
	}

	VOID Instruction(INS ins, VOID *v){
	    UINT32 memOperands = INS_MemoryOperandCount(ins);

	    for (UINT32 memOp = 0; memOp < memOperands; memOp++)
	    {
	        if (INS_MemoryOperandIsRead(ins, memOp))
	        {
	            INS_InsertPredicatedCall(
	                ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead,
	                IARG_INST_PTR,
	                IARG_MEMORYOP_EA, memOp,
	                IARG_END);
	        }
	        if (INS_MemoryOperandIsWritten(ins, memOp))
	        {
	            INS_InsertPredicatedCall(
	                ins, IPOINT_BEFORE, (AFUNPTR)RecordMemWrite,
	                IARG_INST_PTR,
	                IARG_MEMORYOP_EA, memOp,
	                IARG_END);
	        }
	    }
	}

	VOID Fini(INT32 code, VOID *v){
	    for(map<uint64_t, uint64_t>::iterator it = pages.begin(); it != pages.end(); it++){
	    	out << it->first << "\t" << it->second << endl;
	    }
	    out.close();
	}

	VOID Run(){
		out.open("PageStatistic.out", ios::out | ios::app);
		INS_AddInstrumentFunction(Instruction, 0);
    	PIN_AddFiniFunction(Fini, 0);
	}
}

namespace CoarsePageStat{

	uint64_t micount = 0; //memory instructions count
	uint64_t icount = 0; //instructions count
	uint64_t tmicount = 0;

	ofstream out;
	map<uint64_t, uint64_t> pages;

	
	VOID Output(){
		//out << icount << endl;
		out << endl;
		out << tmicount << endl;
		tmicount /= 4;
		for(map<uint64_t, uint64_t>::iterator it = pages.begin(); it != pages.end(); it++){
			if(it->second >= tmicount)
				out << it->first << " " << it->second << endl;
		}
		pages.clear();
		tmicount = 0;
	}
	
	VOID RecordMemRead(VOID * ip, VOID * addr){
	    pages[(uint64_t)addr >> 12]++;
	    micount++;
		tmicount++;
	}

	VOID RecordMemWrite(VOID * ip, VOID * addr){
	    pages[(uint64_t)addr >> 12]++;
	    micount++;
	    tmicount++;
	}

	VOID docount(){
		icount++;
		if((icount & MASK) == 0){
	    	Output();
	    }
	}

	VOID Instruction(INS ins, VOID *v){
		INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount, IARG_END);

	    UINT32 memOperands = INS_MemoryOperandCount(ins);

	    for (UINT32 memOp = 0; memOp < memOperands; memOp++)
	    {
	        if (INS_MemoryOperandIsRead(ins, memOp))
	        {
	            INS_InsertPredicatedCall(
	                ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead,
	                IARG_INST_PTR,
	                IARG_MEMORYOP_EA, memOp,
	                IARG_END);
	        }
	        if (INS_MemoryOperandIsWritten(ins, memOp))
	        {
	            INS_InsertPredicatedCall(
	                ins, IPOINT_BEFORE, (AFUNPTR)RecordMemWrite,
	                IARG_INST_PTR,
	                IARG_MEMORYOP_EA, memOp,
	                IARG_END);
	        }
	    }
	}

	VOID Fini(INT32 code, VOID *v){
		Output();
		cout << "Instruction Count : " << icount << endl;
		cout << "Memory access instruction : " << micount << endl;
		//cout << "Pages : " << pages.size() << endl;
	    out.close();
	}

	VOID Run(){
		out.open("CoarsePageStat.out", ios::out | ios::app);
		INS_AddInstrumentFunction(Instruction, 0);
    	PIN_AddFiniFunction(Fini, 0);
	}
}


INT32 Usage()
{
    PIN_ERROR( "This Pintool prints a trace of memory addresses\n" 
              + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}


int main(int argc, char *argv[])
{
    if (PIN_Init(argc, argv)) return Usage();

    //PageStatistic::Run();
    CoarsePageStat::Run();

    PIN_StartProgram();
    
    return 0;
}
