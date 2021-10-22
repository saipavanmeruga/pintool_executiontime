#include <fstream>
#include <iostream>
#include <numeric>
#include<functional>

#include<time.h>
#include "pin.H"
using std::cerr;
using std::dec;
using std::endl;
using std::hex;
using std::ofstream;
using std::setw;
using std::string;
using namespace std;
ofstream outFile;


float average(std::vector<int> const& v) {
    if (v.empty()) {
        return 0;
    }

    return 1.0 * std::accumulate(v.begin(), v.end(), 0LL) / v.size();
}
map<string, vector<int>> StartTimeMap;
map<string, vector<int>> EndTimeMap;
typedef struct RtnCount
{
    string _name;
    RTN _rtn;
    UINT64 _rtnCount;
    UINT64 _icount;
    struct RtnCount* _next;
} RTN_COUNT;
RTN_COUNT* RtnList = 0;

VOID CalculateStartTime(string* _name) {

    time_t begin;
    StartTimeMap[*_name].push_back(time(&begin));
}

VOID CalculateEndTime(string* _name) {

    time_t end;
    EndTimeMap[*_name].push_back(time(&end));
}

VOID docount(UINT64* counter) { (*counter)++; }

VOID Routine(RTN rtn, VOID* v)
{
    RTN_COUNT* rc = new RTN_COUNT;

    // The RTN goes away when the image is unloaded, so save it now
    // because we need it in the fini
    rc->_name = RTN_Name(rtn);
    rc->_icount = 0;
    rc->_rtnCount = 0;

    // Add to list of routines
    rc->_next = RtnList;
    RtnList = rc;
    RTN_Open(rtn);
    RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)CalculateStartTime, IARG_PTR, &(rc->_name), IARG_END);
    for (INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins))
    {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)docount, IARG_PTR, &(rc->_icount), IARG_END);
    }
    RTN_InsertCall(rtn, IPOINT_AFTER, (AFUNPTR)CalculateEndTime, IARG_PTR, &(rc->_name), IARG_END);
    RTN_Close(rtn);
}

VOID Fini(INT32 code, VOID* v)
{
    cout <<"Procedure"<< setw(40) 
        << " "<< "Min Execution time"
        << " " << setw(12) << "Max Execution time"
        <<" " << setw(12) << "Avg Execution time" << endl;
    
    for (auto it_m1 = StartTimeMap.begin(), end_m1 = StartTimeMap.end(), it_m2 = EndTimeMap.begin(), end_m2 = EndTimeMap.end();
        it_m1 != end_m1 || it_m2 != end_m2;) {
        vector<int> result;
        transform(it_m2->second.begin(), it_m2->second.end(), it_m1->second.begin(),
            back_inserter(result),
            minus<int>());
        cout<< it_m1->first<< setw(40)<< (*min_element(result.begin(), result.end())) << setw(12)
            << (*max_element(result.begin(), result.end())) << setw(12)<<
            average(result) << endl;
            ++it_m1;
            ++it_m2;

    }
}
INT32 Usage()
{
    cerr << "This Pintool counts the number of times a routine is executed" << endl;
    cerr << "and the number of instructions executed in a routine" << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}
int main(int argc, char* argv[])
{
    // Initialize symbol table code, needed for rtn instrumentation
    PIN_InitSymbols();

    outFile.open("customtool.out");

    // Initialize pin
    if (PIN_Init(argc, argv)) return Usage();

    // Register Routine to be called to instrument rtn

    RTN_AddInstrumentFunction(Routine, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);

    // Start the program, never returns
    PIN_StartProgram();

    return 0;
}


