/*
 * SDL checker and e-code generator
 *
 * Written by Graham Wheeler, February 1994
 * Data Network Architecture Laboratory
 * Department of Computer Science
 * University of Cape Town
 * 
 * (c) 1994 Graham Wheeler and the Data Network Architectures Laboratory
 * All Rights Reserved
 *
 * Last modified: 13-6-94
 *
 */

#pragma hdrfile "PASS2.SYM"
#include "sdlast.h"
#include "sdlc2.h"

ifstream inStr;
int verbose = 0;

int getLineNumber()
{
	return 0;
}

static char *errorMessage[] = {
	"Must have at least one block in system %s",
	"Must have at least one process and one signal route in block %s",
	"Must have a block definition in substructure %s",
	"Parameter name %s is not unique",
	"Variable name %s is not unique",
	"No definition for remote reference %s",
	"Initial number of instances must be positive",
	"Initial number of instances may not be more than maximum",
	"Maximum number of instances must be greater than zero",
	"Signal route endpoint must be process identifier or ENV",
	"Channel endpoint must be block identifier or ENV",
	"Communication path cannot have environment as both endpoints",
	"Communication path cannot have identical endpoints",
	"Communication paths must be in opposite directions",
	"Variables in procedures cannot be exported",
	"Variables in procedures cannot be revealed",
	"Timers not allowed in valid input signal set",
	"Identifier %s is not a valid signal",
	"Identifier %s is not a valid signal list",
	"Invalid sort identifier %s",
	"(Internal) Maximum allowed number of unique signals exceeded",
	"Bad variable reference (identifier %s)",
	"Invalid variable type identifier %s",
	"Invalid parameter type identifier %s",
	"Array element selector not allowed on non-array",
	"Structure field selector not allowed on non-structure",
	"Unknown structure field selector",
	"Expression must be Boolean",
	"THEN and ELSE expressions in conditional have different types",
	"Expression must be of type integer",
	"Expression must be of type integer or character",
	"Expression must be of a basic type",
	"Incompatible types in expression, operator %s",
	"Rvalue expression type incompatible with lvalue in assignment",
	"Array index expression must be of integer type",
	"Identifier %s is not a timer in timer active expression",
	"Too few parameters in actual parameter list",
	"Parameter %d is of incorrect type",
	"Too many parameters in actual parameter list",
	"Identifier %s is not a signal in OUTPUT",
	"Identifier %s is not a process ID in CREATE",
	"Identifier %s is not a procedure ID in CALL",
	"(Local) Priority outputs are not currently supported",
	"(Local) `%s' is not currently supported",
	"(Local) Only simple default variable values are supported",
	"(Local) Question expression must evaluate to a simple type",
	"(Internal) Maximum number of ranges in decision exceeded",
	"Ranges must be complete and mutually exclusive",
	"Range conflicts with earlier one on line %d",
	"Range conditions do not cover the type value space",
	"Unreachable code",
	"RETURN not allowed in process definition",
	"STOP not allowed in procedure definition",
	"NEXSTATE must be explicit in initial transition",
	"Transition does not have a valid terminator statement",
	"Ground (constant) expression expected",
	"Initial variable or synonym value is not of type %s",
	"Answer is of different type to question in DECISION",
	"Process %s has no non-asterisk states",
	"Label %s is not unique within process",
	"Cannot resolve VIA route in OUTPUT",
	"OUTPUT is ambiguous and requires a process ID",
	"No admissible communication path for OUTPUT",
	"(Internal) Channel table overflow",
	"(Internal) Route table overflow",
	"At least one channel endpoint must be the containing block",
	"At least one route endpoint must be ENV",
	"JOIN label %s is undefined",
	"%s is not a valid state in NEXTSTATE",
	"Process %s must have a valid input signal set",
	"Variable in VIEW is not a valid REVEALED variable",
	"Variable in IMPORT is not a valid EXPORTED variable",
	"Signal route %s is not unique in VIA",
	"Invalid channel ID %s in CONNECT",
	"%s in CONNECT is not a signal route in containing block",
	"Channel %s is not properly connected to signalroutes",
	"Signals conveyed by channel %s different to connected routes",
	"Expression should be of process identifier type in %s",
	"Import identifier %s is ambiguous",
	"Sort %s is not a valid sort in IMPORTED",
	"No import definition for identifier %s",
	"Identifier %s is ambiguous in VIEW",
	"Identifier %s is ambiguous in VIEWED definition",
	"No view definition for identifier %s",
	"Reference parameters must be variables",
	"Continuous signals in state are missing priorities",
	"Duplicate PRIORITY %d in state",
	"(Local) Priority %d is out of allowed range 0...63",
	"Input %s is not a valid signal or timer",
	"Inputs within a state must be disjoint",
	"Duplicate enumeration element %s",
	"Duplicate field %s in structure definition",
	"Integer or real type argument expected",
	"Integer, character or real type argument expected",
	"Expression must be of type `natural'",
	"Invalid time expression type",
	"Cannot assign value %ld to natural variable(s)",
	"(Local) Read variable reference must be of integer type",
	"(Local) Write expression must evaluate to a simple type",
	"Output destination must be of PId type, not %s",
	"Argument of FIX must have real type",
	"Argument of FLOAT must have integer or natural type",
	NULL
};

// Error class names and counts

static char *errClassNames[] = { "Warning", "Error", "Fatal Error" };
int errorCount[3] = { 0, 0, 0 };

static void _showError(int severity, const fil, const ln,
	const error_t err, const char *arg)
{
	char erbuf[128];
	cerr << "[" << errClassNames[severity] << " ";
	cerr << ++errorCount[severity] << "] ";
	cerr << '(' << fileTable->getName(fil) << ':' << ln << ')' << ' ';
	sprintf(erbuf, errorMessage[(int)err], arg);
	cerr << erbuf << endl;
	if (severity == FATAL)
		exit(-1);
}

void SDLfatal(const fil, const ln, const error_t err, const char *arg)
{
	_showError(FATAL, fil, ln, err, arg);
}

void SDLerror(const fil, const ln, const error_t err, const char *arg)
{
	_showError(ERROR, fil, ln, err, arg);
}

void SDLwarning(const fil, const ln, const error_t err, const char *arg)
{
	_showError(WARN, fil, ln, err, arg);
}

void useage()
{
	cerr << "Useage: sdlc2 [-H[<level>]] [-v]" << endl;
	cerr << "where:\t-H shows heap statistics (levels 1, 2, or 3)" << endl;
	cerr << "\t-v produces verbose (debugging) output" << endl;
	exit(0);
}

int main(int argc, char *argv[])
{
	int doLink = 1;
	cerr << "SDL*Design Checker/Code Generator v1.0" << endl;
	cerr << "written by Graham Wheeler" << endl;
	cerr << "(c) 1994 Graham Wheeler and the University of Cape Town" << endl;

	RestoreAST("ast.out");

	for (int i=1;i<argc;i++)
	{
		if (argv[i][0]=='-')
		{
			switch(argv[i][1])
			{
			case 'H':
				if (isdigit(argv[i][2]))
					localHeap->debug=argv[i][2]-'0';
				else
					localHeap->debug++;
				break;
			case 'v':
				verbose = 1;
				break;
			case 'l':
				doLink = 0;
				break;
			default: useage();
			}
		}
		else useage();
	}

	sys->Address(0);
	sys->Check();
	if ((errorCount[ERROR] + errorCount[FATAL]) == 0)
		sys->Emit(doLink);
	if ((errorCount[ERROR] + errorCount[FATAL]) == 0)
	{
		cerr << "Compilation successful" << endl;
		SaveAST("ast.sym");
	}
	else
		cerr << "Compilation failed" << endl;
	extern void printTables();
	if (verbose) printTables(); // debug
	DeleteAST();
	return 0;
}

