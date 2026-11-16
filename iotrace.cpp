/*
 * IOTrace - Intel Pin tool that reports the stdio file I/O routines
 * (fopen, fgets, fseek, fclose, fread, fwrite) a program executes.
 *
 * For every matching routine, in the main executable and in every shared
 * library, it records the routine's entry address, the address of the last
 * RET it executed, how many instructions ran inside it and the mnemonic of
 * its return instruction. The table is written when the program exits.
 *
 * Copyright (c) 2026 Damian Andres Ulanowicz
 * SPDX-License-Identifier: MIT
 */

#include <array>
#include <atomic>
#include <deque>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "pin.H"

namespace {

// Name fragments of the I/O routines to trace. Matching is by substring, so
// libc internals such as _IO_file_fopen, _IO_fwrite or fopen64 match too.
const std::array<const char*, 6> kTracedNames = {
    "fopen", "fgets", "fseek", "fclose", "fread", "fwrite"};

KNOB<std::string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "filetrace.txt",
                                 "name of the output file");

struct TracedRoutine
{
    TracedRoutine(const std::string& routineName, const std::string& imageName, ADDRINT entry)
        : name(routineName), image(imageName), address(entry)
    {
    }

    const std::string name;    // demangled routine name
    const std::string image;   // file name of the image that contains it
    const ADDRINT address;     // routine entry address
    std::string retMnemonic;   // mnemonic of the (last) RET, or of the last instruction if none

    // Updated by analysis code, possibly from several application threads at once.
    std::atomic<UINT64> insCount{0};        // instructions executed inside the routine
    std::atomic<ADDRINT> lastRetAddress{0}; // address of the last RET executed (0 = none)
};

// std::deque never moves its elements on emplace_back, so analysis calls can
// keep raw pointers to them. Instrumentation callbacks are serialized by Pin.
std::deque<TracedRoutine> g_routines;
std::ofstream g_outFile;

bool IsTracedName(const std::string& name)
{
    for (const char* fragment : kTracedNames)
    {
        if (name.find(fragment) != std::string::npos) return true;
    }
    return false;
}

std::string StripPath(const std::string& path)
{
    const std::string::size_type slash = path.find_last_of("/\\");
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

std::string ToHex(ADDRINT value)
{
    std::ostringstream ss;
    ss << "0x" << std::hex << value;
    return ss.str();
}

// ---------------------------------------------------------------------------
// Analysis routines (run every time the instrumented code executes)
// ---------------------------------------------------------------------------

VOID CountInstruction(TracedRoutine* routine)
{
    routine->insCount.fetch_add(1, std::memory_order_relaxed);
}

VOID RecordReturn(TracedRoutine* routine, ADDRINT retAddress)
{
    routine->lastRetAddress.store(retAddress, std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------
// Instrumentation routines (run once per routine, when its image is loaded)
// ---------------------------------------------------------------------------

VOID InstrumentRoutine(RTN rtn, VOID*)
{
    const std::string name = PIN_UndecorateSymbolName(RTN_Name(rtn), UNDECORATION_COMPLETE);
    if (!IsTracedName(name)) return;

    // The RTN handle is gone once the image unloads, so copy what Fini needs now.
    g_routines.emplace_back(name, StripPath(IMG_Name(SEC_Img(RTN_Sec(rtn)))), RTN_Address(rtn));
    TracedRoutine* routine = &g_routines.back();

    bool foundRet = false;
    RTN_Open(rtn);
    for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins))
    {
        INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CountInstruction), IARG_PTR, routine, IARG_END);

        if (INS_IsRet(ins))
        {
            foundRet = true;
            routine->retMnemonic = INS_Mnemonic(ins);
            INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(RecordReturn), IARG_PTR, routine,
                           IARG_INST_PTR, IARG_END);
        }
        else if (!foundRet)
        {
            // No RET seen yet: keep the last instruction (usually a tail JMP) for reference.
            routine->retMnemonic = INS_Mnemonic(ins);
        }
    }
    RTN_Close(rtn);
}

VOID Fini(INT32, VOID*)
{
    g_outFile << std::setw(23) << "Procedure" << ' ' << std::setw(19) << "Image" << ' '
              << std::setw(18) << "Call Address" << ' ' << std::setw(18) << "Ret Address" << ' '
              << std::setw(12) << "Ins Count" << ' ' << std::setw(14) << "Ret Mnemonic" << '\n'
              << std::string(109, '-') << "\n\n";

    for (const TracedRoutine& routine : g_routines)
    {
        const UINT64 insCount = routine.insCount.load();
        if (insCount == 0) continue; // never executed

        const ADDRINT retAddress = routine.lastRetAddress.load();
        g_outFile << std::setw(23) << routine.name << ' ' << std::setw(19) << routine.image << ' '
                  << std::setw(18) << ToHex(routine.address) << ' '
                  << std::setw(18) << (retAddress != 0 ? ToHex(retAddress) : "-") << ' '
                  << std::setw(12) << insCount << ' ' << std::setw(14) << routine.retMnemonic
                  << '\n';
    }
    g_outFile.close();
}

INT32 Usage()
{
    std::cerr << "IOTrace reports the stdio file I/O routines (fopen, fgets, fseek, fclose,\n"
                 "fread, fwrite) executed by a program: entry address, last RET address,\n"
                 "executed instruction count and return mnemonic.\n\n"
              << KNOB_BASE::StringKnobSummary() << std::endl;
    return -1;
}

} // namespace

int main(int argc, char* argv[])
{
    PIN_InitSymbols(); // needed for routine names
    if (PIN_Init(argc, argv)) return Usage();

    g_outFile.open(KnobOutputFile.Value().c_str());
    if (!g_outFile)
    {
        std::cerr << "IOTrace: cannot open output file '" << KnobOutputFile.Value() << "'" << std::endl;
        PIN_ExitProcess(1);
    }

    RTN_AddInstrumentFunction(InstrumentRoutine, nullptr);
    PIN_AddFiniFunction(Fini, nullptr);

    PIN_StartProgram(); // never returns
    return 0;
}
