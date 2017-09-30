/*
 * sdlcc.c - main() routine for the SDL compiler
 *
 * Written by Graham Wheeler, January 1994
 * Data Network Architecture Laboratory
 * Department of Computer Science
 * University of Cape Town
 * 
 * (c) 1994 Graham Wheeler and the Data Network Architectures Laboratory
 * All Rights Reserved
 * Last modified: 13-6-94
 */

#pragma hdrfile "PASS1.SYM"
#include "sdlast.h"
#include "sdlc1.h"
#pragma hdrstop

// Input file stream

ifstream inStr;

ext_synonym_t ext_synonym_tbl[MAX_EXT_SYN];
int num_ext_synonym = 0;
int lineNum = 1;
char **fileList;	// Files to compile

void STRLWR(register char *s)
{
	while (*s)
	{
		if (isupper(*s))
			*s = tolower(*s);
		s++;
	}
}

int getLineNumber()
{
	return lineNum - (*token == '\n'/* || !*token*/);
}
	    
void showErrorPlace(void)
{
	cerr << "line " <<  getLineNumber();
	if (isprint(*token))
		cerr << " near <" << token << ">";
	cerr << ":" << endl << "\t";
}

static char *errorMessage[] = {
	"%s name at end differs from that at the beginning",
	"Syntax error",
	"Remote definition or <eof> expected",
	"File '%s' could not be opened for input",
	"Bad character in string",

	"%s", // special case!
	"%s character is not allowed",
	"(local) Integer out of range",
	"%s does not contain an alphabetic character",
	"%s expected",

	"Invalid remote reference, definition or qualifier",
	"%s identifier at end differs from that at the beginning",
	"(Local) Informal text is ignored!",
	"(Local) Macros are not currently supported",
	"(Local) Only simple arrays and structure types are supported",

	"(Local) Undefined external synonym %s",
	"(Local) Syntypes are not currently supported",
	"(Local) Generators are not currently supported",
	"(Local) Selections are not currently supported",
	"(Local) Signal refinements are not currently supported",

	"Remotely referenced block %s redefined",
	"Initial number of processes different to earlier value",
	"Maximum number of processes different to earlier value",
	"Illegal endstate name",
	"Bad real literal",

	"Select expression must be of type boolean",
	"Missing select expression",
	"End-of-file encountered in selection",
	"Endselect encountered while not in selection",
	NULL
};

// Error classes

#define WARN	0
#define ERROR	1
#define FATAL	2

// Error class names and counts

static char *errClassNames[] = { "Warning", "Error", "Fatal Error" };
int errorCount[3] = { 0, 0, 0 };

static void _showError(int severity, const error_t err, const char *arg)
{
	char erbuf[128];
	cerr << "[" << errClassNames[severity];
	if (severity != FATAL) cerr << ' ' << ++errorCount[severity];
	cerr << "] ";
	showErrorPlace();
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

static void useage(void)
{
	fprintf(stderr,"Useage: sdlcc [-H[<level>]] [-S] <file list>\n");
	fprintf(stderr,"where:\t-H shows heaps statistics (levels 1, 2, or 3)\n");
	fprintf(stderr,"\t-S suppresses primary file name (used by sdlcc)\n");
	exit(-1);
}

int main(int argc, char *argv[])
{
	assert(sizeof(s_code_word_t)==sizeof(s_code_cast_t));
	int i, fileNum = 0;
	cerr << "SDL*Design Parser v1.0" << endl;
	cerr << "written by Graham Wheeler" << endl;
	cerr << "(c) 1994 Graham Wheeler and the University of Cape Town" << endl;
	initGlobalObjects();
	int suppressfirst = 0;
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
			case 'S':
				suppressfirst = 1;
				break;
			default: useage();
			}
		}
		else if (strchr(argv[i],'='))
		{
			char *p = strchr(argv[i],'=');
			assert(num_ext_synonym<MAX_EXT_SYN);// should be error
			STRLWR(argv[i]);
			ext_synonym_tbl[num_ext_synonym].name = argv[i];
			ext_synonym_tbl[num_ext_synonym++].value = p+1;
			*p = 0;
		}
		else
		{
			fileNum = i;
			break;
		}
	}
	if (fileNum == 0) useage();
	fileList = &argv[fileNum];
	inStr.open(*fileList);
	if (!inStr) 
	{
		char fname[120];
		strcpy(fname,*fileList);
		strcat(fname,".sdl");
		inStr.open(fname);
		if (!inStr) 
			SDLfatal(ERR_NOFILE, *fileList);
		else if (!suppressfirst)
			cerr << "File: " << fname << endl;
	}
	else if (!suppressfirst)
		cerr << "File: " << *fileList << endl;
	fileTable->addName(*fileList);
	fileList++;
	SDLparse();
	DeleteAST();
	if (errorCount[ERROR] == 0)
	{
		if (errorCount[WARN] == 0)
			cout << "Parse successful" << endl;
		else
			cout << "Parse successful - " << errorCount[WARN] << " warning(s)" << endl;
	} else
	{
		cout << "Parse failed - " << errorCount[WARN] << " warning(s) and ";
		cout << errorCount[ERROR] << " error(s)" << endl;
	}
	exit(errorCount[ERROR]);
   return 0;
}

