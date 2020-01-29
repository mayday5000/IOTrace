#include <fstream>
#include <iomanip>
#include <iostream>
#include <string.h>
#include "pin.H"


#define FILE_FILTER   0x02

ofstream outFile;

typedef struct CallData
{
    string _name;
    string _image;
    ADDRINT _address;
    ADDRINT _ret_address; 
    string _ret_mnemonic;  
    UINT64 _ins_count;
    UINT64 _filter;
    struct CallData * _next;
} RTN_CALL;

RTN_CALL * RtnList = 0;


void docount(UINT64 * counter)
{
    (*counter)++;
}
   
const char * StripPath(const char * path)
{
    const char * file = strrchr(path,'/');
    if (file)
        return file+1;
    else
        return path;
}
    
const char *dumpInstruction(INS ins)
{
	ADDRINT address = INS_Address(ins);
	std::stringstream ss;

	// Generate instruction byte encoding
	for (size_t i=0;i<INS_Size(ins);i++)
	{
		ss << setfill('0') << setw(2) << hex << (((unsigned int) *(unsigned char*)(address + i)) & 0xFF) << " ";
	}

	for (size_t i=INS_Size(ins);i<8;i++)
	{
		ss << "   ";
	}

	// Generate diassembled string
	ss << INS_Disassemble(ins);

	return strdup(ss.str().c_str());
}

void RetCallback(ADDRINT address, RTN_CALL *rtn_data)
{
    rtn_data->_ret_address = address;

/*    outFile << setw(23) << rtn_data->_name << " "
            << setw(15) << rtn_data->_image << " "
            << setw(18) << "0x" << hex << rtn_data->_address << dec <<" "
            << setw(12) << "0x" << hex << rtn_data->_ret_address << " "
            << setw(23) << rtn_data->_ret_mnemonic << endl;*/
}

// Pin hace un llamado a esta funcion por cada llamado a una rutina...
VOID Routine(RTN rtn, VOID *v)
{
    // Allocate a counter for this routine
    RTN_CALL *rc = new RTN_CALL;
    int fg_ret_found = 0;        // Flag de deteccion de instruccion RET dentro de rutina
    
    // The RTN goes away when the image is unloaded, so save it now
    // because we need it in the fini

    std::string symbolName = RTN_Name(rtn);
    symbolName = PIN_UndecorateSymbolName(symbolName, UNDECORATION_COMPLETE);    
    
    rc->_name = symbolName;                    // Nombre de la rutina
    rc->_image = StripPath(IMG_Name(SEC_Img(RTN_Sec(rtn))).c_str());  // Nombre de la imagen
    rc->_address = RTN_Address(rtn);           // Direccion de memoria del Call
    rc->_ins_count = 0;                        // Cantidad de instrucciones
    rc->_ret_address = RTN_Address(rtn);       // Direccion de memoria del RET
    rc->_filter = 0;                           // Inicializa filtros.

    if((std::strstr(rc->_name.c_str(),"fopen")) || (std::strstr(rc->_name.c_str(),"fgets")) || 
       (std::strstr(rc->_name.c_str(),"fseek")) || (std::strstr(rc->_name.c_str(),"fclose")) ||
       (std::strstr(rc->_name.c_str(),"fread")) || (std::strstr(rc->_name.c_str(),"fwrite")))
    {
        rc->_filter |= FILE_FILTER;    // Setea bit de filtro FILE_FILTER
    }
 
    rc->_next = RtnList;
    RtnList = rc;
            
    RTN_Open(rtn);
    // Itera por cada instruccion dentro de la rutina...
    for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins))
    {
        // Cuenta la cantidad de instrucciones dentro de la rutina
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount, IARG_PTR, &(rc->_ins_count), IARG_END);
        
        if (INS_IsRet(ins) || INS_IsSysret(ins))
        {
            fg_ret_found = 1;
            rc->_ret_mnemonic = std::string(INS_Mnemonic(ins));

            INS_InsertCall(ins,
                IPOINT_BEFORE,
                (AFUNPTR) RetCallback,
                IARG_INST_PTR,         // direccion de memoria de la instruccion
                IARG_PTR, rc,          // puntero a estructura rc
                IARG_END);            
        } 

        if (fg_ret_found == 0) 
	    rc->_ret_mnemonic = std::string(INS_Mnemonic(ins));    // Si no se encontro instruccion RET, guardo la ultima instruccion a modo informativo.

   }
   RTN_Close(rtn);
}


VOID Fini(INT32 code, VOID *v)
{
    outFile << setw(23) << "Procedure" << " "
            << setw(19) << "Image" << " "
            << setw(26) << "Call Address" << " "
            << setw(24) << "Ret Address" << " "
            << setw(17) << "Ins Count" << " "
            << setw(19) << "Ret Mnemonic" << endl;
    outFile << "-------------------------------------------------------------------";
    outFile << "------------------------------------------------------------------" << endl;
    outFile << endl;

    for (RTN_CALL *rc = RtnList; rc; rc = rc->_next)
    {
        if (rc->_ins_count > 0)    // Filtro llamadas que den un conteo de instrucciones = 0
	{
            if ((rc->_filter & FILE_FILTER) == FILE_FILTER)
            {
                outFile << setw(23) << rc->_name << " "
                        << setw(19) << rc->_image << " "
                        << setw(14) << "0x" << hex << rc->_address << dec <<" "
                        << setw(12) << "0x" << hex << rc->_ret_address << " "
                        << setw(17) << dec << rc->_ins_count << " "
                        << setw(19) << rc->_ret_mnemonic << endl;
	    }
	}
    }
}


INT32 Usage()
{
    cerr << "This Pintool counts the number of times a routine is executed" << endl;
    cerr << "and the number of instructions executed in a routine" << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}


void WriteHeader(void)
{
    outFile.open("filetrace.txt");

/*    outFile << setw(23) << "Procedure" << " "
            << setw(15) << "Image" << " "
            << setw(30) << "Call Address" << " "
            << setw(24) << "Ret Address" << " "
            << setw(23) << "Ret Mnemonic" << endl;
    outFile << "--------------------------------------------------------------";
    outFile << "-------------------------------------------------------------" << endl;*/
}


/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char * argv[])
{
    // Initialize symbol table code, needed for rtn instrumentation
    PIN_InitSymbols();

    // Initialize pin
    if (PIN_Init(argc, argv)) return Usage();

    WriteHeader();
    
    // Register Routine to be called to instrument rtn
    RTN_AddInstrumentFunction(Routine, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);
    
    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}
