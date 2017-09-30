/* sdlpp.cpp
 *
 * A preprocessor for textual SDL.
 * This program replaces macro calls with the corresponding definitions.
 * It operates in 2 passes. It also removes comments, but adds comments
 * containing source code line numbers for error handling in the first pass.
 *
 * Written by Graham Wheeler
 * Data Network Architecture Laboratory
 * Department of Computer Science
 * University of Cape Town
 * 
 * (c) 1994 Graham Wheeler and the Data Network Architectures Laboratory
 * All Rights Reserved
 *
 * Last modified: 8-8-94
 */

#pragma hdrfile "PREPROC.SYM"
#include "sdlast.h"
#pragma hdrstop
#include <stdarg.h>

// Error types

typedef enum
{
	ERR_COMMENT, ERR_BADCHAR, ERR_INTEGER, ERR_REAL,
	ERR_BADNAME, ERR_NOFILE, ERR_DUPMACRO, ERR_BADCOMMENT,
	ERR_SEMICOLON, ERR_MACROARG, ERR_MACCLOSENAME, ERR_UNDEFMACRO,
	ERR_MACRONAME, ERR_NESTMACRO, ERR_MACARGCNT, ERR_RPAREXPECT
}
error_t;

void SDLfatal(const error_t err, const char *arg = NULL);
void SDLerror(const error_t err, const char *arg = NULL);
void SDLwarning(const error_t err, const char *arg = NULL);

SDL_token_t symbol;
char token[MAX_TOKEN_LENGTH];
static s_code_word_t tokenVal;
static name_t *envName;
static ident_t *envIdent;

int lineNum = 1;
char **fileList;	// Files to compile

//-----------------------------------------------------------------------

void STRLWR(register char *s)
{
	while (*s)
	{
		if (isupper(*s))
			*s = tolower(*s);
		s++;
	}
}

unsigned long STRTOUL(char *v)
{
	unsigned long rtn = 0;
	while (isdigit(*v))
		rtn = rtn*10 + (*v++) - '0';
	return rtn;
}

//---------------------------------------------------------------
// Lexical analyser
//---------------------------------------------------------------

#ifdef DEBUG
#define TOKEN(x)	#x
#else
#define TOKEN(x)	x
#endif

#define END(v)	(v-1+sizeof(v) / sizeof(v[0]))
#define ETX		(-1)
static char ch = ' ';
#ifdef DEBUG
static char *lookupToken(void);
#else
static SDL_token_t lookupToken(void);
#endif

/*******************************************************************
Names may consist of a single word or a list of words separated by
underscore (strictly speaking the underscore is only required in
cases of ambiguity, but for machine lexical analysis it is required).

The underscore character can also be used as a continuation character
allowing the splitting of a name over more than one line. In this
case the terminal underscores are deleted before assembling the name.
That is:

	CONNECT_ ME is the same as CONNECT_ME

while
	CONNECT_
	ME
is the same as CONNECTME.

********************************************************************/

static int inComment = 0;
static int _lineNum = 1;
static FILE *inf = NULL;

//-----------------------------------------------------------------------

static int _nextchar(void)
{
	while ((ch = fgetc(inf)) != EOF)
	{
		if (ch=='\n') _lineNum++;
		if (ch=='\n' || ch=='\t' || ch>=32)
			return 0;
	}
	return -1; // EOF
}

void nextchar(void)
{
	if (_nextchar() != 0)
		ch = ETX;
}
		
//-----------------------------------------------------------------------
/*
 * Reserved word lookup
 */

typedef struct
{
	char *rw_name;
#ifdef DEBUG
	char			*rwtoken;
#else
	SDL_token_t	rw_token;
#endif
} rw_entry_t;

#include "rwtable.cpp"

//-----------------------------------------------------------------------

#ifdef DEBUG
static char *lookupToken(void)
#else
static SDL_token_t lookupToken(void)
#endif
{
	rw_entry_t *low = rwtable,
		       *high = END(rwtable),
		       *mid;
	int c;
	STRLWR(token);
	while (low <= high)
	{
		mid = low + (high-low)/2;
		if ((c = strcmp(mid->rw_name, token)) == 0)
			return mid->rw_token;
		else if (c < 0)
			low = mid + 1;
		else
			high = mid - 1;
	}
	tokenVal = (int)(nameStore->insert(token));
	return (TOKEN(T_NAME));
}		

void nextsymbol(void)
{
	s_code_word_t length, IDindex, digit;
	char *tokenstring, tch;
	int t;
	symbol = T_UNDEF;
	while (symbol==T_UNDEF)
	{
		tokenstring = token;
		*tokenstring = '\0';
		lineNum = _lineNum;
		switch (ch)
		{
		// End-of-input and whitespace
		case ETX:
			symbol = TOKEN(T_EOF);
			break;
		case '\n':
			symbol = TOKEN(T_NEWLINE);
			// fall thru
		case '\t':
		case ' ':
			nextchar();
			break;
		// String literals
		case '\'':
        	symbol = TOKEN(T_STRING);
			nextchar();
			for (int l = MAX_TOKEN_LENGTH;l>0;)
			{
				while (ch != '\'' && --l>0)
				{
					*tokenstring++ = (char)ch;
					nextchar();
				}
				nextchar();
				if (ch == '\'' && --l>0)
				{
					nextchar();
					*tokenstring++ = '\'';
				} else
				{
					*tokenstring = '\0';
					break;
				}
			}
			break;
		case '+':
			nextchar();
		        symbol = TOKEN(T_PLUS);
	        	break;
		case '-':
			nextchar();
			if (ch=='>')
			{
				nextchar();
				symbol = TOKEN(T_RET);
			}
			else symbol = TOKEN(T_MINUS);
	        	break;
		case '!':
			nextchar();
		        symbol = TOKEN(T_BANG);
	        	break;
		case '/':
			nextchar();
			if (ch == '=')
			{
				nextchar();
				symbol = TOKEN(T_NE);
			}
			else if (ch == '/')
			{
				nextchar();
				symbol = TOKEN(T_CONCAT);
			}
			else if (ch == '*') // Comment?
			{
				int start = lineNum;
				inComment = 1;
				nextchar();
				for (;;)
				{
					while (ch != '*' && ch != ETX) nextchar();
					if (ch == ETX)
					{
						char msg[80];
						sprintf(msg, "EOF reached in comment started on line %d", start);
						SDLfatal(ERR_COMMENT, msg);
					}
					nextchar();
					if (ch == '/') 
					{
						inComment = 0;
						nextchar();
						break;
					}
				}
				continue;
			}
			else symbol = TOKEN(T_SLASH);
			break;
	      case '>':
			nextchar();
			if (ch=='=')
			{
				nextchar();
				symbol = TOKEN(T_GE);
			}
			else symbol = TOKEN(T_GT);
        		break;
		case '*':
			nextchar();
		        symbol = TOKEN(T_ASTERISK);
	        	break;
		case '(':
			nextchar();
			if (ch == '.')
			{
				nextchar();
				symbol = TOKEN(T_SLST);
			}
			else symbol = TOKEN(T_LEFTPAR);
        		break;
		case ')':
			symbol = TOKEN(T_RIGHTPAR);
			nextchar();
			break;
		case '%':
			symbol = TOKEN(T_PERCENT); // sdlpp only
			nextchar();
			break;
		case '"':
			nextchar();
			symbol = TOKEN(T_QUOTE);
			break;
		case ',':
			nextchar();
		        symbol = TOKEN(T_COMMA);
	        	break;
		case ';':
			nextchar();
			symbol = TOKEN(T_SEMICOLON);
	        	break;
		case '<':
			nextchar();
			if (ch=='=')
			{
				nextchar();
				symbol = TOKEN(T_LE);
			}
			else symbol = TOKEN(T_LESS);
        		break;
		case '=':
			nextchar();
			if (ch=='=')
			{
				nextchar();
				if (ch=='>')
				{
					symbol = TOKEN(T_IMPLIES);
					nextchar();
				} else symbol = TOKEN(T_EQUALS);
			}
			else if (ch=='>')
			{
				nextchar();
				symbol = TOKEN(T_BIMP);
			}
			else symbol = TOKEN(T_EQ);
        		break;
		case ':':
			nextchar();
			if (ch == '=')
			{
				nextchar();
				symbol = TOKEN(T_ASGN);
			}
			else symbol = TOKEN(T_COLON);
        		break;
		default:
			if (!isalnum(ch)) 
			{
				char bc[2];
				bc[0] = (char)ch; bc[1] = '\0';
				SDLerror(ERR_BADCHAR, bc);
				nextchar();
				continue;
			}
			// Note that the spec doesn't seem to deal with integer
			// literals properly. We do so here.
			if (isdigit(ch))
			{
				tokenVal = 0;
				while (isdigit(ch))
				{
					*tokenstring++ = ch;
               				nextchar();
				}
				if (ch != '.')
				{
					*tokenstring++ = 0;
					if (token[0]=='-' || STRTOUL(token)<=MAXINT)
					{
						if (token[0]=='-' &&
							STRTOUL(token)>MAXINT)
								SDLerror(ERR_INTEGER);
						symbol = TOKEN(T_INTEGER);
						tokenVal = (s_code_word_t)atol(token);
					}
					else
					{
						symbol = TOKEN(T_NATURAL);
						tokenVal = (s_code_word_t)STRTOUL(token);
					}
				}
				else // Real type.
				{
					*tokenstring++ = ch;
					nextchar();
					while (isdigit(ch))
					{
						*tokenstring++ = ch;
	               				nextchar();
					}
					if (ch=='e' || ch=='E')
					{
						*tokenstring++ = 'e';
	               				nextchar();
						if (ch=='-')
						{
							*tokenstring++ = '-';
	               					nextchar();
						}
						if (!isdigit(ch))
							SDLerror(ERR_REAL);
						while (isdigit(ch))
						{
							*tokenstring++ = ch;
		               				nextchar();
						}
					}
					*tokenstring++ = 0;
					s_code_cast_t cv;
					cv.rv = (s_code_real_t)atof(token);
					tokenVal = cv.iv;
					symbol = TOKEN(T_REAL);
				}
				break;
			}
			// else fall thru to name handler...
		case '.':
		case '_':
		case '#':
		case '`':
		case '@':
		case '~':
		case '^':
		case '[':
		case ']':
		case '{':
		case '}':
		case '|':
		case '\\':
			int gotAlpha = 0;
			for (;;)
			{
				if (!isalnum(ch) && ch != '.' && ch != '_' && ch != '#'
						&& ch != '`' && ch != '@' && ch != '~' && ch != '^'
						&& ch != '[' && ch != ']' && ch != '{' && ch != '}'
						&& ch != '|' && ch != '\\')
							break;
				if (ch == '_')
				{
					nextchar();
					if (ch == '\n') 
						nextchar();
					else
						*tokenstring++ = '_';
					while (ch=='\n' || ch==' ' || ch=='\t') nextchar();
					continue;
				}
				else if (isalpha(ch)) gotAlpha = 1;
				*tokenstring ++ = ch;
				nextchar();
			}
			*tokenstring = '\0';
			if (!gotAlpha) SDLerror(ERR_BADNAME, token);
			symbol = lookupToken();
			break;
		}
	}
}

//-----------------------------------------------------------------------

static void InitLex()
{
	_lineNum = 1;
	inComment = 0;
	ch = ' ';
	nextsymbol();
}

//-----------------------------------------------------------------------
// Return next token in a printable form

char *Printable()
{
	static char rtn[160], *prt;
	switch (symbol)
	{
		default:
			assert(symbol >= T_ACTIVE && symbol <= T_XOR);
			prt = rwtable[symbol].rw_name; break;
		case T_TRUE:
			prt = "true"; break;
		case T_FALSE:
			prt = "false"; break;
		case T_SEMICOLON:
			prt = ";"; break;
		case T_COMMA:
			prt = ","; break;
		case T_LEFTPAR:
			prt = "("; break;
		case T_RIGHTPAR:
			prt = ")"; break;
		case T_MINUS:
			prt = "-"; break;
		case T_PLUS:
			prt = "+"; break;
		case T_ASTERISK:
			prt = "*"; break;
		case T_SLASH:
			prt = "/"; break;
		case T_EQUALS:
			prt = "=="; break;
		case T_COLON:
			prt = ":"; break;
		case T_LESS:
			prt = "<"; break;
		case T_GT:
			prt = ">"; break;
		case T_BANG:
			prt = "!"; break;
		case T_QUOTE:
			prt = "'"; break;
		case T_EQ:
			prt = "="; break;
		case T_IMPLIES:
			prt = "=>"; break;
		case T_NE:
			prt = "/="; break;
		case T_LE:
			prt = "<="; break;
		case T_GE:
			prt = ">="; break;
		case T_CONCAT:
			prt = "//"; break;
		case T_ASGN:
			prt = ":="; break;
		case T_BIMP:
			prt = "==>"; break;
		case T_RET:
			prt = "->"; break;
		case T_SLST:
			prt = "(."; break;
		case T_ELST:
			prt = ".)"; break;
		case T_NAME:
		case T_INTEGER:
		case T_REAL:
			prt = token; break;
		case T_STRING:
			sprintf(prt = rtn, "\"%s\"", token); break;
		case T_UNDEF:
			assert(0);
		case T_NEWLINE:
			prt = "\n"; break;
	}
	if (prt != rtn)
		strcpy(rtn, prt);
	nextsymbol();
	return rtn;
}

//-----------------------------------------------------------------------

#define NUM_MACROS	64
#define MAC_ARGS	16
#define MAC_TEXT_LEN	4096

class macro_t
{
private:
	char	*name;
	char	*args[MAC_ARGS];
	char	*text;
	int	argcnt;
public:
	macro_t();
	~macro_t();

	void DefineArg(char *aname)
	{
		assert(argcnt<MAC_ARGS);
		args[argcnt] = new char[strlen(aname)+1];
		assert(args[argcnt]);
		strcpy(args[argcnt], aname);
		argcnt++;
	}
	void DefineName(char *nm)
	{
		assert(name==NULL);
		name = new char[strlen(nm)+1];
		assert(name);
		strcpy(name, nm);
	}
	void DefineText(char *txt)
	{
		assert(text==NULL);
		text = new char[strlen(txt)+1];
		assert(text);
		strcpy(text, txt);
	}
	char *Name()
	{
		return name;
	}
	char *Text()
	{
		return text;
	}
	int Args()
	{
		return argcnt;
	}
	char *Arg(int n)
	{
		assert(n>=0 && n<argcnt);
		return args[n];
	}
};

macro_t::macro_t()
{
	name = NULL;
	for (int i = 0; i < MAC_ARGS; i++)
		args[i] = NULL;
	text = NULL;
	argcnt = 0;
}

macro_t::~macro_t()
{
	if (name) delete [] name;
	for (int i = 0; i < MAC_ARGS; i++)
		if (args[i])
			delete [] args[i];
	if (text) delete [] text;
}

macro_t	Macros[NUM_MACROS];
int macnum = 0;
char mactext[2][MAC_TEXT_LEN];

static void End()
{
	while (symbol == T_NEWLINE) nextsymbol();
	if (symbol == T_COMMENT)
	{
		nextsymbol();
		while (symbol == T_NEWLINE) nextsymbol();
		if (symbol != T_STRING)
			SDLerror(ERR_BADCOMMENT, token);
		nextsymbol();
	}
	while (symbol == T_NEWLINE) nextsymbol();
	if (symbol != T_SEMICOLON)
		SDLerror(ERR_SEMICOLON, token);
	else nextsymbol();
}

void DefineMacro()
{
	char *mt = mactext[0];
	assert(macnum<NUM_MACROS);
	nextsymbol();
	if (symbol != T_NAME)
		SDLerror(ERR_MACRONAME, token);
	else
	{
		for (int i = 0; i < macnum; i++)
		{
			if (Macros[i].Name() && strcmp(token, Macros[i].Name())==0)
				SDLerror(ERR_DUPMACRO, token);
		}
	}
	Macros[macnum].DefineName(token);
	nextsymbol();
	while (symbol == T_NEWLINE) nextsymbol();
	if (symbol == T_FPAR)
	{
		nextsymbol();
		for (;;)
		{
			while (symbol == T_NEWLINE) nextsymbol();
			Macros[macnum].DefineArg(token);
			if (symbol != T_NAME)
				SDLerror(ERR_MACROARG, token);
			nextsymbol();
			while (symbol == T_NEWLINE) nextsymbol();
			if (symbol == T_COMMA)
				nextsymbol();
			else break;
		}
	}
	End();
	mt[0] = 0;
	int l = 1;
	while (symbol != T_ENDMACRO)
	{
		char *prt;
		if (symbol == T_MACRODEFINITION)
			SDLerror(ERR_NESTMACRO);
		if (symbol==T_NAME || symbol==T_PERCENT || symbol==T_MACROID)
		{
			if (symbol==T_PERCENT)
			{
				nextsymbol();
				if (symbol != T_NAME && symbol != T_MACROID)
					SDLerror(ERR_MACROARG, token);
			}
			int a = -1;
			if (symbol != T_MACROID)
				for (a = 0; a < Macros[macnum].Args(); a++)
				{
					if (strcmp(Macros[macnum].Arg(a), token)==0)
						break;
				}
			if (a < Macros[macnum].Args())
			{
				prt = mactext[1];
				sprintf(prt, "%%%d", a+1);
				nextsymbol();
			}
			else prt = Printable();
		}
		else prt = Printable();
		l+= strlen(prt);
		assert(l<(MAC_TEXT_LEN-1));
		strcat(mt, prt);
		if (prt[0] != '\n' && symbol != T_ENDMACRO && symbol!=T_PERCENT)
		{
			strcat(mt, " ");
			l++;
		}
	}
	Macros[macnum].DefineText(mt);
	nextsymbol();
	if (symbol == T_NAME)
	{
		if (strcmp(Macros[macnum].Name(), token) != 0)
			SDLwarning(ERR_MACCLOSENAME, token);
		nextsymbol();
	}
	End();
	macnum++;
}

char *MacroReference()
{
	char *marg[MAC_ARGS+1];
	// create unique macroID
	{
		static int macid = 0;
		char buf[20];
		sprintf(buf, "macroid_%d", macid++);
		marg[0] = new char[strlen(buf)+1];
		strcpy(marg[0], buf);
	}
	nextsymbol(); // skip MACRO
	while (symbol == T_NEWLINE) nextsymbol();
	int bn = 0, m, change = 1;
	char *rtn;
	for (m = 0; m < macnum; m++)
		if (strcmp(Macros[m].Name(), token)==0)
			break;
	if (m >= macnum)
	{
		SDLerror(ERR_UNDEFMACRO, token);
		while (symbol != T_SEMICOLON) nextsymbol(); // inadequate
		return "";
	}
	nextsymbol(); // skip macro name
	while (symbol == T_NEWLINE) nextsymbol();
	int arg = 1;
	if (symbol == T_LEFTPAR)
	{
		nextsymbol();
		while (symbol == T_NEWLINE) nextsymbol();
		while (arg<=Macros[m].Args())
		{
			marg[arg] = NULL;
			mactext[1][0] = 0;
			while (symbol != T_COMMA && symbol != T_RIGHTPAR)
			{
				strcat(mactext[1], Printable());
				strcat(mactext[1], " ");
			}
			if (mactext[1][0]) // strip off last space
				mactext[1][strlen(mactext[1])-1] = 0;
			marg[arg] = new char[strlen(mactext[1])+1];
			strcpy(marg[arg], mactext[1]);
			arg++;
			if (symbol == T_RIGHTPAR) break;
			nextsymbol(); // skip comma
		}
		if (symbol != T_RIGHTPAR)
			SDLerror(ERR_RPAREXPECT, Macros[m].Name());
		else nextsymbol();
	}
	if (arg != (Macros[m].Args()+1))
		SDLerror(ERR_MACARGCNT, Macros[m].Name());
	while (symbol == T_NEWLINE) nextsymbol();
	End();
	strcpy(mactext[0], Macros[m].Text());
	while (change)
	{
		change = 0;
		char *s = mactext[bn], *p;
		rtn = mactext[1-bn];
		rtn[0] = 0;
		while (*s && (p = strchr(s, '%'))!=NULL)
		{
			*p = 0;
			strcat(rtn, s);
			s = p+1;
			assert(isdigit(*s));
			int anum = 0;
			while (isdigit(*s))
			{
				anum = anum*10 + (*s) - '0';
				s++;
			}
			assert(anum<=Macros[m].Args());
			change = 1;
			strcat(rtn, marg[anum]);
		}
		if (*s) strcat(rtn, s);
		bn = 1-bn;
	}
	for (int i=0; i < arg; i++)
		delete [] marg[i];
	return rtn;
}

//-----------------------------------------------------------------------

void pass1()
{
	int l = fileTable->getIndex();
	for (int fi=0; fi<=l; fi++)
	{
		inf = fopen(fileTable->getName(fi), "r");
		assert(inf);
		InitLex();
		while (symbol != T_EOF)
		{
			if (symbol == T_MACRODEFINITION)
				DefineMacro();
			nextsymbol();
		}
		fclose(inf);
	}
}

//-----------------------------------------------------------------------

void pass2(FILE *out)
{
	int l = fileTable->getIndex();
	for (int fi=0; fi<=l; fi++)
	{
		inf = fopen(fileTable->getName(fi), "r");
		assert(inf);
		InitLex();
		fprintf(out, "/*%%File %s*/\n",  fileTable->getName(fi));
		while (symbol != T_EOF)
		{
			char *prt = NULL;
			if (symbol == T_MACRODEFINITION)
			{
				while (symbol != T_ENDMACRO)
					nextsymbol();
				while (symbol != T_SEMICOLON)
					nextsymbol();
				nextsymbol();
			}
			else if (symbol == T_MACRO)
				prt = MacroReference();
			else if (symbol == T_NEWLINE)
			{
				fprintf(out, "\n/*%%Line %d*/\n", lineNum);
				nextsymbol();
			}
			else prt = Printable();
			if (prt)
			{
				fputs(prt, out);
				fputs(" ", out);
			}
		}
		fclose(inf);
	}
}

//-----------------------------------------------------------------------

int getLineNumber()
{
	return lineNum - (*token == '\n'/* || !*token*/);
}
	    
void showErrorPlace(void)
{
	fprintf(stderr, "line %d", getLineNumber());
	if (isprint(*token))
		fprintf(stderr, " near <%s>", token);
	fprintf(stderr,":\n\t");
}

static char *errorMessage[] = {
	"%s", // special case!
	"%s character is not allowed",
	"(local) Integer out of range",
	"Bad real literal",
	"%s does not contain an alphabetic character",
	"File '%s' could not be opened for input",
	"Macro %s is already defined",
	"Invalid comment %s; should be string",
	"Semicolon expected (got %s)",
	"Macro argument expected (got %s)",
	"Closing name %s of macro is different to opening name",
	"Undefined macro %s",
	"Macro name expected (got %s)",
	"Macrodefinitions may not be nested",
	"Wrong number of arguments to macro %s",
	"Right parenthesis expected after last argument in macro %s",
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
	if (severity == FATAL)
		fprintf(stderr, "[%s]", errClassNames[severity]);
	else 
		fprintf(stderr, "[%s %d]", errClassNames[severity],
			++errorCount[severity]);
	showErrorPlace();
	fprintf(stderr, errorMessage[(int)err], arg);
	fprintf(stderr,"\n");
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

//-----------------------------------------------------------------------

static void useage(void)
{
	fprintf(stderr,"Useage: sdlpp [-H[<level>]] [-o <file>] <file list>\n");
	fprintf(stderr,"where:\t-H shows heaps statistics (levels 1, 2, or 3)\n");
	fprintf(stderr,"\t-o sets the output file\n");
	fprintf(stderr,"\t<file list> is a list of input files\n");
	exit(-1);
}

int main(int argc, char *argv[])
{
	assert(sizeof(s_code_word_t)==sizeof(s_code_cast_t));
	int i, fileNum = 0, outArg = -1;
	FILE *outf = NULL;
	fprintf(stderr, "SDL*Design Preprocessor v0.1\n");
	fprintf(stderr, "written by Graham Wheeler\n");
	fprintf(stderr, "(c) 1994 Graham Wheeler and the University of Cape Town\n");
	initGlobalObjects();
	for (i=1;i<argc;i++)
	{
		// Process command line flags (none yet, but skeleton code
		// is useful as there will no doubt be some soon).

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
			case 'o':
				outArg = ++i;
				break;
			default: useage();
			}
		}
		else
		{
			fileNum = i;
			break;
		}
	}
	if (fileNum == 0) useage();
	fileList = &argv[fileNum];
	while (*fileList)
	{
		inf = fopen(*fileList, "r");
		if (!inf) 
		{
			char fname[120];
			strcpy(fname,*fileList);
			strcat(fname,".sdl");
			inf = fopen(fname, "r");
			if (!inf) 
				SDLfatal(ERR_NOFILE, *fileList);
			else fileTable->addName(fname);
		}
		else fileTable->addName(*fileList);
		fclose(inf);
		fileList++;
	}
	if (outArg>=0) outf = fopen(argv[outArg], "w");
	if (outf==NULL) outf = stdout; // default
	pass1();
	pass2(outf);
	delete localHeap;
	if (errorCount[ERROR] == 0)
		if (errorCount[WARN] == 0)
			fprintf(stderr, "Preprocess successful\n");
		else
			fprintf(stderr, "Preprocess successful - %d warning(s)\n",
				errorCount[WARN]);
	else
		fprintf(stderr, "Preprocess failed - %d error(s) and %d warning(s)\n",
				errorCount[ERROR], errorCount[WARN]);
	exit(errorCount[ERROR]);
	return 0;
}

