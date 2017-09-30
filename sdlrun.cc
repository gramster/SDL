/*
 * SDL interpreter driver
 *
 * Written by Graham Wheeler, February 1994
 * Data Network Architecture Laboratory
 * Department of Computer Science
 * University of Cape Town
 * 
 * (c) 1994 Graham Wheeler and the Data Network Architectures Laboratory
 * All Rights Reserved
 *
 * Last modified: 28-6-94
 *
 */

#pragma hdrfile "RUN.SYM"
#include "sdlrun.h"

char fname[MAX_FILES][MAX_FNAME_LEN];

// stub needed by sdlast

int getLineNumber()
{
	return 0;
}

static char *errorMessage[] =
{
	"%s", // custom
	"Divide by zero",
	"Process table full",
	"No destination for signal %s",
	"Multiple destinations for signal",
	"System has deadlocked",
	"Only 32 exported variables allowed per process",
	"Size of import is different to export",
	"No value has been exported for import",
	"Operation applied to undefined value",
	"Cannot assign value %ld to natural variable",
	NULL
};

// Error class names and counts

static char *errClassNames[] = { "Warning", "Error", "Fatal Error" };
int errorCount[3] = { 0, 0, 0 };

static void _showError(int severity, const error_t err, const char *arg)
{
	char erbuf[128];
	cerr << "[" << errClassNames[severity] << " ";
	cerr << ++errorCount[severity] << "] ";
	sprintf(erbuf, errorMessage[(int)err], arg);
	cerr << erbuf << endl;
	if (severity == FATAL)
		exit(-1);
}

void SDLfatal(const error_t err, const char *arg)
{
	_showError(FATAL, err, arg);
}

void SDLerror(const error_t err, const char *arg)
{
	_showError(ERROR, err, arg);
}

void SDLwarning(const error_t err, const char *arg)
{
	_showError(WARN, err, arg);
}

ofstream *log = NULL;

void LogWriter(char *buf)
{
	if (log==NULL) log = new ofstream("sdlrun.log");
	assert(log);
	cout << buf << endl;
	if (log) *log << buf << endl;
}

static void useage()
{
	cerr << "Useage: sdlrun [-L] [-H]\nwhere:\t-L enables logging\n"
		"\t-H enables heap statistics" << endl;
	exit(-1);
}

int main(int argc, char *argv[])
{
	cerr << "SDL*Design Interpreter v1.0" << endl;
	cerr << "written by Graham Wheeler" << endl;
	cerr << "(c) 1994 Graham Wheeler and the University of Cape Town" << endl;

	// Get the s-code

#ifdef __MSDOS__
	ifstream is("sdl.cod", ios::binary);
#else
	ifstream is("sdl.cod");
#endif
	for (int i = 0; i<MAX_FILES; i++)
		is.read(fname[i], MAX_FNAME_LEN);
	Code = new codespace_t;
	assert(Code);
	if (Code->Read(NULL, &is)!=0)
	{
		cerr << "Cannot read code file sdl.cod!" << endl;
		exit(0);
	}

	// Get the AST

	RestoreAST("ast.sym");

	int logger = 0;
	for (i=1;i<argc;i++)
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
			case 'L':
				logger = 1;
				break;
			default: useage();
			}
		}
		else useage();
	}

	// create and execute the scheduler

	if (logger)
		Scheduler = new scheduler_t(NULL, LogWriter);
	else
		Scheduler = new scheduler_t;
	assert(Scheduler);
	Scheduler->Execute();

	// free everything up

	delete Scheduler;
	delete Code;
	DeleteAST();
	return 0;
}

