/* sdlparse.cpp
 *
 * A recursive descent parser for an SDL subset.
 * The primary differences are in the handling of ADTs. The following
 * are not supported yet:
 *  - string literals in expressions
 *  - selections, transition options, and macros
 *  - floating point types
 *  - channel substructures
 *  - service definitions
 *  - signal refinements
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

#pragma hdrfile "PASS1.SYM"
#include "sdlast.h"
#include "sdlc1.h"
#pragma hdrstop
#include <stdarg.h>

SDL_token_t symbol;
char token[MAX_TOKEN_LENGTH];
static s_code_word_t tokenVal;
static name_t *envName;
static ident_t *envIdent;

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
static int wasPreprocessed = 0;

//-------------------------------------------------------------------

unsigned long STRTOUL(char *v)
{
	unsigned long rtn = 0;
	while (isdigit(*v))
		rtn = rtn*10 + (*v++) - '0';
	return rtn;
}

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

For now we don't do these special cases, as they are messy to handle
in lex.
********************************************************************/

static int inComment = 0;
static int _lineNum = 1;

static int _nextchar(void)
{
	while (inStr.get(ch))
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
		if (!wasPreprocessed)
			lineNum = _lineNum;
		switch (ch)
		{
		// End-of-input and whitespace
		case ETX:
			symbol = TOKEN(T_EOF);
			break;
		case '\n':
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
					if (ch<32 || ch>126) SDLerror(ERR_STRING);
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
				if (ch=='%' && lineNum==1)
					wasPreprocessed = 1;
				if (wasPreprocessed && ch=='%')
				{
					while (ch != ' ')
						nextchar();
					nextchar();
					if (ch>='0' && ch<='9') // line number
					{
						lineNum = 0;
						while (ch>='0' && ch<='9')
						{
							lineNum = lineNum*10+
								(ch - '0');
							nextchar();
						}
					}
					else // file name
					{
						while (ch != '*')
						{
							*tokenstring++ = ch;
							nextchar();
						}
						*tokenstring = 0;
						fileTable->addName(token);
						fprintf(stderr,"File %s\n", token);
					}
				}
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
			if (isdigit(ch) || ch=='-')
			{
				if (ch=='-')
				{
					*tokenstring++ = ch;
               				nextchar();
					assert(isdigit(ch)); // should be error
				}
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

/*
 * Reserved word lookup
 */

#include "rwtable.cpp"

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

//---------------------------------------------------------------
// Set Types (still in C)
//---------------------------------------------------------------

#define MAX_SET_ELTS		(256)
#define MAX_SET_LONGS	((MAX_SET_ELTS+31)/32)

typedef struct {
	unsigned long v[MAX_SET_LONGS];
} set_t;

/* Test for membership */

int In(set_t S, int v)
{
	assert(v>=0 && v<MAX_SET_ELTS);
	return ( (S.v[v/32] & (1l << (v%32))) != 0l);
}

/* Print the elements of a set */

void dumpSet(char *title, set_t S)
{
	int i, f = 1;
	unsigned long m;
	fprintf(stdout,"%s = { ", title);
	for (i=0;i<MAX_SET_ELTS;i++)
		if (In(S,i))
		{
			if (!f) fprintf(stdout,", ");
			fprintf(stdout,"%d", i);
			f = 0;
		}
	fprintf(stdout," }\n");
}

/* Union `operator' */

set_t Union(set_t S1, set_t S2)
{
	static set_t S;
	int i;
	for (i=0; i< MAX_SET_LONGS; i++)
		S.v[i] = S1.v[i] | S2.v[i];
	return S;
}

/* Intersection operator */

set_t Intersection(set_t S1, set_t S2)
{
	static set_t S;
	int i;
	for (i=0; i< MAX_SET_LONGS; i++)
		S.v[i] = S1.v[i] & S2.v[i];
	return S;
}

/* Combine `operator' */

set_t Combine(set_t S, ...)
{
	va_list args;
	static set_t R;
	va_start(args, S);
	for (;;)
	{
		int t = (int)va_arg(args, int);
		assert(t>=-1 && t < MAX_SET_ELTS);
		if (t == -1) break;
		S.v[t / 32] |= 1l << (t % 32 );
	}
	va_end(args);
	return R = S;
}

/* Set initialiser */

set_t Set(int e, ...)
{
	va_list args;
	static set_t S;
	int i;
	for (i=0;i<MAX_SET_LONGS;i++) S.v[i] = 0l;
	assert(e>=-1 && e < MAX_SET_ELTS);
	va_start(args, e);
	S.v[e / 32] |= 1l << (e % 32 );
	for (;;)
	{
		int t = va_arg(args, int);
		assert(t>=-1 && t < MAX_SET_ELTS);
		if (t == -1) break;
		S.v[t / 32] |= 1l << (t % 32 );
	}
	va_end(args);
	return S;
}

//---------------------------------------------------------------------
// PARSER
//---------------------------------------------------------------------

static void Transition(transition_t *t, set_t F);
static heap_ptr_t Expression(set_t F);
static void ExpressionList(lh_list_t<heap_ptr_t> *el, set_t F);

/**********************************/
/* Types of names and identifiers */
/**********************************/

typedef enum
{
	N_VARIABLE, N_STATE, N_SIGNALROUTE, N_CHANNEL, N_SORT, N_SYNTYPE,
	N_SUBCHANNEL, N_MACROPARAM, N_VALUE, N_FIELD, N_GENERATOR
} name_class_t;

static set_t FirstID;
static set_t FirstTransition;

/*******************************/
/* Error checking and recovery */
/*******************************/

static void skipto(set_t S)
{
	set_t S2 = Combine(S, T_EOF, -1);
	while (!In(S2,symbol))
		nextsymbol();
}

static void expect(SDL_token_t s)
{
	if (symbol == s) nextsymbol();
	else 	{
		char *tok;
		switch(s) {
			case T_SEMICOLON:
				tok = ";";
				break;
			case T_COMMA:
				tok = ",";
				break;
			case T_LEFTPAR:
				tok = "(";
				break;
			case T_RIGHTPAR:
				tok = ")";
				break;
			case T_MINUS:
				tok = "-";
				break;
			case T_PLUS:
				tok = "+";
				break;
			case T_ASTERISK:
				tok = "*";
				break;
			case T_SLASH:
				tok = "/";
				break;
			case T_EQ:
				tok = "=";
				break;
			case T_COLON:
				tok = ":";
				break;
			case T_LESS:
				tok = "<";
				break;
			case T_GT:
				tok = ">";
				break;
			case T_BANG:
				tok = "!";
				break;
			case T_QUOTE:
				tok = "\"";
				break;
			case T_EQUALS:
				tok = "==";
				break;
			case T_IMPLIES:
				tok = "=>";
				break;
			case T_NE:
				tok = "/=";
				break;
			case T_LE:
				tok = "<=";
				break;
			case T_GE:
				tok = ">=";
				break;
			case T_CONCAT:
				tok = "//";
				break;
			case T_ASGN:
				tok = ":=";
				break;
			case T_BIMP:
				tok = "==>";
				break;
			case T_RET:
				tok = "->";
				break;
			case T_SLST:
				tok = "(.";
				break;
			case T_ELST:
				tok = ".)";
				break;
			case T_NAME:
				tok = "Name";
				break;
			case T_STRING:
				tok = "String literal";
				break;
			case T_INTEGER:
				tok = "Integer";
				break;
			case T_NATURAL:
				tok = "Natural";
				break;
			case T_REAL:
				tok = "Real";
				break;
			default:
				tok = rwtable[s].rw_name;
				break;
		}
		SDLerror(ERR_EXPECT, tok);
	}
}

static int entrySync(set_t first, set_t follow)
{
	if (!In(first,symbol))
		SDLerror(ERR_SYNTAX, NULL);
	skipto(Union(first,follow));
	return In(first,symbol);
}

static void exitSync(set_t follow)
{
	if (!In(follow,symbol))
		SDLerror(ERR_SYNTAX, NULL);
	skipto(follow);
}

static void CheckClosingName(const char *name, const char *arg)
{
	if (symbol == T_NAME)
	{
		/* Make sure the name is the same as at the start */
		if (strcmp(token, name))
				SDLwarning(ERR_NAMES, arg);
		nextsymbol();
	}
}

/*******************/
/* Parser routines */
/*******************/

static void End(set_t F)
{
	if (!entrySync(Set(T_SEMICOLON,T_COMMENT,-1), F))
		return;
	if (symbol == T_COMMENT)
	{
		nextsymbol();
		expect(T_STRING);
	}
	expect(T_SEMICOLON);
	exitSync(F);
}

static name_t *Name(set_t F)
{
	static name_t n("");
	if (!entrySync(Set(T_NAME,-1), F))
		n.name("Unknown");
	else
	{
		n.name(token);
		expect(T_NAME);
	}
	exitSync(F);
	return &n;
}

ident_t *Identifier(ident_t *id, set_t F)
{
	set_t Q = Set(T_BLOCK, T_PROCEDURE, T_PROCESS, T_SERVICE,
			T_SIGNAL, T_SUBSTRUCTURE, T_SYSTEM,
			T_TYPE, -1);
	while (In(Q, symbol))
	{
		typeclass_t q;
		switch(symbol)
		{
		case T_BLOCK:
			q = Q_BLOCK;
			break;
		case T_PROCEDURE:
			q = Q_PROCEDURE;
			break;
		case T_PROCESS:
			q = Q_PROCESS;
			break;
		case T_SERVICE:
			q = Q_SERVICE;
			break;
		case T_SIGNAL:
			q = Q_SIGNAL;
			break;
		case T_SUBSTRUCTURE:
			q = Q_SUBSTRUCTURE;
			break;
		case T_SYSTEM:
			q = Q_SYSTEM;
			break;
		case T_TYPE:
			q = Q_TYPE;
			break;
		}
		nextsymbol();
		id->qualify(q,*Name(Combine(F,T_NAME, T_SLASH,-1)));
		if (symbol==T_SLASH)
		{
			nextsymbol();
			continue;
		}
		else break;
	}
	id->name(*Name(F));
	exitSync(F);
	return id;
}

static void CheckClosingIdent(const ident_t *id, const char *arg,
	typeclass_t qt, set_t F)
{
	if (In(FirstID,symbol))
	{
		ident_t tmp;
		if (ScopeStack->dequalify(id,qt) != ScopeStack->dequalify(Identifier(&tmp,F),qt))
				SDLwarning(ERR_IDENTS, arg);
	}
	exitSync(F);
}

static int NameList(lh_list_t<name_t> *l, set_t F)
{
	int cnt = 0;
	for (;;)
	{
		lh_list_node_t<name_t> *n = 
			new lh_list_node_t<name_t>(*Name(Combine(F, T_COMMA, -1)));
		assert(n);
		l->append(n);
		cnt++;
		if (symbol == T_COMMA) nextsymbol();
		else break;
	}
	exitSync(F);
	return cnt;
}

static int EnumEltList(lh_list_t<enum_elt_t> *l, set_t F)
{
	int cnt = 0;
	for (;;)
	{
		NEWNODE(enum_elt_t, ee);
		ee->info.name(*Name(Combine(F, T_COMMA, -1)));
		ee->info.value = ++cnt;
		l->append(ee);
		if (symbol == T_COMMA) nextsymbol();
		else break;
	}
	exitSync(F);
	return cnt;
}

static void FieldNameList(lh_list_t<field_name_t> *l, set_t F)
{
	for (;;)
	{
		NEWNODE(field_name_t, pn);
		pn->info.name(*Name(Combine(F, T_COMMA, -1)));
		l->append(pn);
		if (symbol == T_COMMA) nextsymbol();
		else break;
	}
	exitSync(F);
}

static void VariableNameList(lh_list_t<variable_name_t> *l, set_t F)
{
	for (;;)
	{
		NEWNODE(variable_name_t, vn);
		vn->info.name(*Name(Combine(F, T_COMMA, -1)));
		l->append(vn);
		if (symbol == T_COMMA) nextsymbol();
		else break;
	}
	exitSync(F);
}

static void ProcedureParamNameList(lh_list_t<procedure_param_name_t> *l, set_t F)
{
	for (;;)
	{
		NEWNODE(procedure_param_name_t, pn);
		pn->info.name(*Name(Combine(F, T_COMMA, -1)));
		l->append(pn);
		if (symbol == T_COMMA) nextsymbol();
		else break;
	}
	exitSync(F);
}

static void ProcessParamNameList(lh_list_t<process_param_name_t> *l, set_t F)
{
	for (;;)
	{
		NEWNODE(process_param_name_t, pn);
		pn->info.name(*Name(Combine(F, T_COMMA, -1)));
		l->append(pn);
		if (symbol == T_COMMA) nextsymbol();
		else break;
	}
	exitSync(F);
}

static void IdentList(lh_list_t<ident_t> *l, set_t F)
{
	for (;;)
	{
		NEWNODE(ident_t, id);
		if (In(FirstID,symbol))
			(void)Identifier(&id->info, Combine(F, T_COMMA, -1));
		l->append(id);
		if (symbol == T_COMMA) nextsymbol();
		else break;
	}
	exitSync(F);
}

/*********************/
/* Macro Definitions */
/*********************/

static void MacroDefinition(set_t F)
{
	(void)F;
	SDLfatal(ERR_NOMACROS);
}

/********************/
/* Type Definitions */
/********************/

/* Note that generators and syntypes are not supported. Partial type
	definitions are restricted to structures, arrays and enums. */

static heap_ptr_t ArrayDefinition(array_def_t *ap, set_t F)
{
	nextsymbol();
	expect(T_LEFTPAR);
	ap->dimension = Expression(Combine(F, T_RIGHTPAR, -1));
	expect(T_RIGHTPAR);
	expect(T_OF);
	(void)Identifier(&ap->sort, Combine(F, T_SEMICOLON, T_COMMENT, -1));
	End(F);
	return GetOffset(ap);
} 

static heap_ptr_t StructureDefinition(struct_def_t *sp, set_t F)
{
	expect(T_STRUCT);
	for (;;)
	{
		NEWNODE(fieldgrp_t, d);
		FieldNameList(&d->info.names, Union(F, FirstID));
		/* Must be sort or syntype ID */
		(void)Identifier(&d->info.sort,Combine(F, T_SEMICOLON, T_ADDING, T_NAME, -1));
		sp->fieldgrps.append(d);
		End(Combine(F, T_ADDING, T_NAME, -1));
		if (symbol!=T_NAME) break;
	}
	if (symbol==T_ADDING) 
	{
		// I can't seem to find a reference to what this keyword
		// implies anywhere, so I'm just consuming it for now
		nextsymbol();
	}
	return GetOffset(sp);
}

static heap_ptr_t EnumDefinition(enum_def_t *ed, set_t F)
{
	expect(T_LITERALS);
	ed->nelts = EnumEltList(&ed->elts, Combine(F, T_COMMENT, T_SEMICOLON, -1));
	End(F);
	return GetOffset(ed);
}

static void MyExtendedProperties(data_def_t *dp, set_t F)
{
	if (symbol==T_ARRAY)
	{
		array_def_t *ad = new array_def_t;
		assert(ad);
		dp->contents = ArrayDefinition(ad, F);
		dp->tag = ARRAY_TYP;
	}
	else if (symbol==T_STRUCT) // fairly conformant with SDL spec
	{
		struct_def_t *sd = new struct_def_t;
		assert(sd);
		dp->contents = StructureDefinition(sd, F);
		dp->tag = STRUCT_TYP;
	}
	else if (symbol==T_LITERALS)
	{
		enum_def_t *ed = new enum_def_t;
		assert(ed);
		dp->contents = EnumDefinition(ed, F);
		dp->tag = ENUM_TYP;
	}
	else SDLerror(ERR_NEWTYPE);
}
		
static void PartialTypeDefinition(lh_list_t<data_def_t> *dlp, set_t F)
{
	NEWNODE(data_def_t, ddp);
	ScopeStack->EnterScope(Q_TYPE, ddp);
	// None-standard
	expect(T_NEWTYPE);
	ddp->info.nm = *Name(Combine(F, T_STRUCT, T_ARRAY, T_LITERALS, -1));
	MyExtendedProperties(&ddp->info, Combine(F,T_ENDNEWTYPE,-1));
	expect(T_ENDNEWTYPE);
	CheckClosingName(ddp->info.name(), "Type");
	ScopeStack->ExitScope();
	dlp->append(ddp);
}

static void SynonymDefinition(lh_list_t<data_def_t> *dlp, set_t F)
{
	NEWNODE(data_def_t, ddp);
	ddp->info.tag = SYNONYM_TYP;
	expect(T_SYNONYM);
	ddp->info.nm = *Name(Combine(F, T_NAME, T_EQ, -1));
	synonym_def_t *sdp = new synonym_def_t;
	assert(sdp);
	if (symbol!=T_EQ)
	{
		(void)Identifier(&sdp->sort, Combine(F, T_EQ, -1));
		sdp->has_sort = 1;
	}
	else sdp->has_sort = 0;
	expect(T_EQ);
	if (symbol == T_EXTERNAL)
	{
		sdp->expr = 0;
		nextsymbol();
		char *vp = NULL;
		for (int i = 0; i < num_ext_synonym; i++)
		{
			if (strcmp(ddp->info.nm.name(), ext_synonym_tbl[i].name)==0)
			{
				vp = ext_synonym_tbl[i].value;
				break;
			}
		}
		if (vp==NULL)
			SDLerror(ERR_SYNONYM, ddp->info.nm.name());
		else
		{
			if (strcmp(vp,"false")==0)
			{
				sdp->value = 0;
				sdp->type = GetOffset(boolType);
			}
			else if (strcmp(vp,"true")==0)
			{
				sdp->value = 1;
				sdp->type = GetOffset(boolType);
			}
			else if (strchr(vp,'.'))
			{
				float v = atof(vp);
				sdp->value = *((s_code_word_t *)&v);
				sdp->type = GetOffset(realType);
			}
			else if (vp[0]=='-')
			{
				sdp->value = (s_code_word_t)atol(vp);
				sdp->type = GetOffset(intType);
			}
			else if (isdigit(vp[0]))
			{
				sdp->value = (s_code_word_t)STRTOUL(vp);
				sdp->type = GetOffset(naturalType);
			}
			else SDLerror(ERR_SYNONYM, ddp->info.nm.name());
		}
	}
	else
	{
		data_def_t *dd;
		sdp->expr = Expression(Combine(F, T_SEMICOLON, T_COMMENT, -1));
		expression_t *e = GetExprP(sdp->expr);
		sdp->value = e->EvalGround(dd);
		sdp->type = GetOffset(dd);
	}
	ddp->info.contents = GetOffset(sdp);
	dlp->append(ddp);
	End(F);
}

static void SynTypeDefinition(set_t F)
{
	// Not supported
	SDLerror(ERR_SYNTYPE);
	nextsymbol();
	skipto(F);
}

static void GeneratorDefinition(set_t F)
{
	// Not supported
	SDLerror(ERR_GENERATOR);
	nextsymbol();
	skipto(F);
}

static void DataDefinition(lh_list_t<data_def_t> *dlp, set_t F)
{
	if (symbol == T_NEWTYPE)
		PartialTypeDefinition(dlp, F);
	else if (symbol == T_SYNONYM)
		SynonymDefinition(dlp, F);
	else if (symbol == T_GENERATOR)
		GeneratorDefinition(F);
	else SynTypeDefinition(F);
}

/***************/
/* Expressions */
/***************/

static void VariableModifiers(var_ref_t *vr, set_t F)
{
	set_t F2 = Combine(F, T_LEFTPAR, T_BANG, -1);
	for (;;)
	{
		if (symbol==T_BANG)
		{
			// Field selector
			nextsymbol();
			NEWNODE(selector_t, s);
			name_t *n = new name_t(*Name(F2));
			assert(n);
			s->info.val = GetOffset(n);
			s->info.type = SEL_FIELD;
			vr->sel.append(s);
		}
		else if (symbol==T_LEFTPAR)
		{
			// array index
			nextsymbol();
			NEWNODE(selector_t, s);
			s->info.val = Expression(Combine(F2,T_RIGHTPAR, -1));
			s->info.type = SEL_ELT;
			vr->sel.append(s);
			expect(T_RIGHTPAR);
		}
		else break;
	}
}

static heap_ptr_t VariableReference(var_ref_t *vr, set_t F)
{
	set_t F2 = Combine(F, T_LEFTPAR, T_BANG, -1);
	ident_t *id = new ident_t;
	assert(id);
	(void)Identifier(id, F2);
	vr->id = GetOffset(id);
	VariableModifiers(vr, F);
	return GetOffset(vr);
}

static heap_ptr_t ViewOrImportExpr(set_t F)
{
	view_expr_t *ie = new view_expr_t;
	assert(ie);
	nextsymbol();
	expect(T_LEFTPAR);
	Identifier(&ie->var, Combine(F, T_COMMA, T_RIGHTPAR, -1));
	if (symbol == T_COMMA)
	{
		expect(T_COMMA);
		ie->pid_expr = Expression(Combine(F, T_RIGHTPAR, -1)); /* Must be PId expression */
	}
	else ie->pid_expr = 0;
	expect(T_RIGHTPAR);
	exitSync(F);
	return GetOffset(ie);
}

static heap_ptr_t Primary(set_t F, primexp_type_t *type)
{
	heap_ptr_t rtn = 0;
	switch(symbol)
	{
	case T_STRING:
		assert(0); // not done
	case T_TRUE: // done
		*type = PE_BLITERAL;
		rtn = (u_s_code_word_t)1;
		nextsymbol();
		break;
	case T_FALSE:
		*type = PE_BLITERAL;
		rtn = (u_s_code_word_t)0;
		nextsymbol();
		break;
	case T_REAL:
		*type = PE_RLITERAL;
		real_literal_t *r = new real_literal_t(tokenVal);
		assert(r);
		nextsymbol();
		rtn = GetOffset(r);
		break;
	case T_INTEGER:
	{
		*type = PE_ILITERAL;
		int_literal_t *i = new int_literal_t(tokenVal);
		assert(i);
		nextsymbol();
		rtn = GetOffset(i);
		break;
	}
	case T_NATURAL:
	{
		*type = PE_NLITERAL;
		int_literal_t *i = new int_literal_t(tokenVal);
		assert(i);
		nextsymbol();
		rtn = GetOffset(i);
		break;
	}
	case T_LEFTPAR:
		nextsymbol();
		*type = PE_EXPR;
		rtn = Expression(Combine(F, T_RIGHTPAR, -1));
		expect(T_RIGHTPAR);
		break;
	case T_FIX:
		nextsymbol();
		*type = PE_FIX;
		expect(T_LEFTPAR);
		rtn = Expression(Combine(F, T_RIGHTPAR, -1));
		expect(T_RIGHTPAR);
		break;
	case T_FLOAT:
		nextsymbol();
		*type = PE_FLOAT;
		expect(T_LEFTPAR);
		rtn = Expression(Combine(F, T_RIGHTPAR, -1));
		expect(T_RIGHTPAR);
		break;
	case T_IF:
		*type = PE_CONDEXPR;
		cond_expr_t *ce = new cond_expr_t;
		assert(ce);
		nextsymbol();
		ce->if_part = Expression(Combine(F, T_THEN, -1));
		expect(T_THEN);
		ce->then_part = Expression(Combine(F, T_ELSE, T_FI, -1));
		if (symbol == T_ELSE)
		{
			nextsymbol();
			ce->else_part = Expression(Combine(F, T_FI, -1));
		}
		else ce->else_part = (u_s_code_word_t)0;
		rtn = GetOffset(ce);
		expect(T_FI);
		break;
	case T_NOW:
		nextsymbol();
		*type = PE_NOW;
		break;
	case T_SELF:
		nextsymbol();
		*type = PE_SELF;
		break;
	case T_PARENT:
		nextsymbol();
		*type = PE_PARENT;
		break;
	case T_OFFSPRING:
		nextsymbol();
		*type = PE_OFFSPRING;
		break;
	case T_SENDER:
		nextsymbol();
		*type = PE_SENDER;
		break;
	case T_ACTIVE:
		*type = PE_TIMERACTIV;
		timer_active_expr_t *tae = new timer_active_expr_t;
		assert(tae);
		nextsymbol();
		expect(T_LEFTPAR);
		(void)Identifier(&tae->timer,Combine(F, T_LEFTPAR, T_RIGHTPAR, -1));
		if (symbol == T_LEFTPAR)
		{
			expect(T_LEFTPAR);
			ExpressionList(&tae->params, Combine(F, T_RIGHTPAR, -1));
			expect(T_RIGHTPAR);
		}
		expect(T_RIGHTPAR);
		rtn = GetOffset(tae);
		break;
	case T_IMPORT:
		*type = PE_IMPORT;
		rtn = ViewOrImportExpr(F);
		break;
	case T_VIEW:
		*type = PE_VIEW;
		rtn = ViewOrImportExpr(F);
		break;
#if 0
	case T_SLST:
		assert(0); // not done
		nextsymbol();
		ExpressionList(Combine(F, T_ELST, -1));
		expect(T_ELST);
		break;
#endif
	default:
		set_t F2 = Combine(F, T_LEFTPAR, T_BANG, -1);
		ident_t *id = new ident_t;
		assert(id);
		(void)Identifier(id, F2);
		// is it a synonym?
		data_def_t *dd = (data_def_t *)ScopeStack->dequalify(id, Q_TYPE);
		if (dd && dd->tag == SYNONYM_TYP)
		{
			synonym_ref_t *sr = new synonym_ref_t(id->nm.index());
			assert(sr);
			sr->defn = GetOffset(dd);
			*type = PE_SYNONYM;
			rtn = GetOffset(sr);
			break;
		}
		// is it an enum elt?
		dd = (data_def_t *)ScopeStack->dequalify(id, Q_ENUMELT);
		if (dd)
		{
			enum_def_t *ed = GetEnumDefP(dd->contents);
			enum_ref_t *er = new enum_ref_t(id->nm.index());
			assert(er);
			er->type = GetOffset(dd);
			// dequalify doesn't give us the value so we 
			// search the list again...
			lh_list_node_t<enum_elt_t> *et = ed->elts.front();
			while (et)
			{
				if (et->info.index() == id->nm.index())
				{
					er->value = et->info.value;
					break;
				}
				et = et->next();
			}
			assert(et); // must have value
			rtn = GetOffset(er);
			*type = PE_ENUMREF;
		}
		else
		{
			// else we assume a variable reference
			var_ref_t *vr = new var_ref_t;
			assert(vr);
			vr->id = GetOffset(id);
			VariableModifiers(vr, F);
			rtn = GetOffset(vr);
			*type = PE_VARREF;
		}
      		break;
	}
	exitSync(F);
	return rtn;
}

static heap_ptr_t Operand5(set_t F)
{
	expression_t *e = new expression_t;
	assert(e);
	unop_t *ue = new unop_t;
	assert(ue);
	e->l_op = GetOffset(ue);
	e->op = T_UNDEF;
	e->r_op = (u_s_code_word_t)0;
	if (symbol == T_MINUS || symbol==T_NOT)
	{
		ue->op = symbol;
		nextsymbol();
	}
	else ue->op = T_UNDEF;
	ue->prim = Primary(F, &ue->type);
	return GetOffset(e);
}

static heap_ptr_t Operand4(set_t F)
{
	heap_ptr_t rtn = Operand5(Combine(F, T_ASTERISK, T_SLASH, T_MOD, T_REM, -1));
	switch (symbol)
	{
	case T_ASTERISK:
	case T_SLASH:
	case T_MOD:
	case T_REM:
		expression_t *e = new expression_t;
		assert(e);
		e->l_op = rtn;
		e->op = symbol;
		nextsymbol();
		e->r_op = Operand4(F);
		rtn = GetOffset(e);
		break;
	default:
		exitSync(F);
	}
	return rtn;
}

static heap_ptr_t Operand3(set_t F)
{
	heap_ptr_t rtn = Operand4(Combine(F, T_PLUS, T_MINUS, T_CONCAT, -1));
	switch (symbol)
	{
	case T_MINUS:
	case T_CONCAT:
	case T_PLUS:
		expression_t *e = new expression_t;
		assert(e);
		e->l_op = rtn;
		e->op = symbol;
		nextsymbol();
		e->r_op = Operand3(F);
		rtn = GetOffset(e);
		break;
	default:
		exitSync(F);
	}
	return rtn;
}

static heap_ptr_t Operand2(set_t F)
{
	heap_ptr_t rtn = Operand3(Combine(F, T_EQ, T_NE, T_GT, T_GE, T_LESS, T_LE, T_IN, -1));
	switch(symbol)
	{
	case T_NE:
	case T_GT:
	case T_GE:
	case T_LESS:
	case T_LE:
	case T_IN:
	case T_EQ:
		expression_t *e = new expression_t;
		assert(e);
		e->l_op = rtn;
		e->op = symbol;
		nextsymbol();
		e->r_op = Operand2(F);
		rtn = GetOffset(e);
		break;
	default:
		exitSync(F);
	}
	return rtn;
}

static heap_ptr_t Operand1(set_t F)
{
	heap_ptr_t rtn = Operand2(Combine(F, T_AND, -1));
	if (symbol == T_AND)
	{
		expression_t *e = new expression_t;
		assert(e);
		e->l_op = rtn;
		e->op = symbol;
		nextsymbol();
		e->r_op = Operand1(F);
		rtn = GetOffset(e);
	}
	else exitSync(F);
	return rtn;
}

static heap_ptr_t Operand0(set_t F)
{
	heap_ptr_t rtn = Operand1(Combine(F, T_OR, T_XOR, -1));
	if (symbol == T_OR || symbol==T_XOR)
	{
		expression_t *e = new expression_t;
		assert(e);
		e->l_op = rtn;
		e->op = symbol;
		nextsymbol();
		e->r_op = Operand0(F);
		rtn = GetOffset(e);
	}
	else exitSync(F);
	return rtn;
}

static heap_ptr_t Expression(set_t F)
{
	heap_ptr_t rtn = Operand0(Combine(F, T_BIMP, -1));
	if (symbol == T_BIMP)
	{
		expression_t *e = new expression_t;
		assert(e);
		e->l_op = rtn;
		e->op = symbol;
		nextsymbol();
		e->r_op = Expression(F);
		rtn = GetOffset(e);
	}
	else exitSync(F);
	return rtn;
}

static void ExpressionList(lh_list_t<heap_ptr_t> *el, set_t F)
{
	for (;;)
	{
		NEWNODE(heap_ptr_t, e);
		if (symbol!=T_COMMA && symbol!=T_RIGHTPAR)
			e->info = Expression(Combine(F, T_COMMA, -1));
		else e->info = (u_s_code_word_t)0;
		el->append(e);
		if (symbol == T_COMMA) nextsymbol();
		else break;
	}
	exitSync(F);
}

/**************/
/* Selections */
/**************/

static void skipSelection(set_t F)
{
	int nest = 1;
	for (;;)
	{
		if (symbol == T_SELECT)
		{
			nextsymbol();
			nest++;
		}
		else if (symbol == T_ENDSELECT)
		{
			nextsymbol();
			End(F);
			if (--nest == 0) break;
		}
		else if (symbol == T_EOF) SDLfatal(ERR_SELEOF);
		else nextsymbol();
	}
}

static int checkSelect(int &selcnt, set_t F)
{
	if (symbol==T_SELECT)
	{
		nextsymbol();
		selcnt++;
		expect(T_IF);
		expect(T_LEFTPAR);
		heap_ptr_t cond = Expression(Combine(F, T_RIGHTPAR, -1));
		expect(T_RIGHTPAR);
		End(F);
		if (cond)
		{
			data_def_t *dd;
			expression_t *e = GetExprP(cond);
			s_code_word_t val = e->EvalGround(dd);
			delete e;
			if (dd != boolType)
			{
				SDLerror(ERR_SELTYPE);
				skipSelection(F);
				return 1;
			}
			else if (val == 0)
			{
				skipSelection(F);
				return 1;
			}
		}
		else SDLerror(ERR_SELEXPR);
	}
	return 0;
}

/****************/	  
/* Signal Lists */
/****************/

static void SignalList(signallist_t *p, set_t F)
{
	for (;;)
	{
		NEWNODE(ident_t, id);
		if (symbol == T_LEFTPAR)
		{
			nextsymbol();
			// Must be signal list ID 
			(void)Identifier(&id->info,Combine(F, T_RIGHTPAR, -1));
			p->signallists.append(id);
			expect(T_RIGHTPAR);
		}
		else
		{
			// Timer, signal or priority signal ID 
			(void)Identifier(&id->info,Combine(F, T_COMMA, -1));
			p->signals.append(id);
		}
		if (symbol == T_COMMA) nextsymbol();
		else break;
	}
	exitSync(F);
}

/***************************/
/* Signal List Definitions */
/***************************/

/*
 * A signal list definition may be used in channel definitions,
 * signal route definitions, signal list definitions, valid input
 * signal sets, and savelists, as a shorthand to list signal 
 * identifiers and timer signals.
 */

static void SignalListDefinition(siglist_def_t *sl, set_t F)
{
	expect(T_SIGNALLIST);
	sl->nm = *Name(Combine(F, T_EQ, -1));
	expect(T_EQ);
	SignalList(&sl->s, Combine(F, T_SEMICOLON, -1));
	End(F);
}

/**********************/
/* Signal Definitions */
/**********************/

/*
 * A signal instance is a flow of information between processes
 * and is an instantiation of a signal type defined by a signal
 * definition. A signal instance can be sent by either the environment
 * or a process and is always directed to either a process or the 
 * environment. Each signal instance is associated with two PId
 * values denoting the origin and destination processes, a signal
 * identifier, and other values whose sorts are defined in the
 * signal definition.
 */

static void SignalDefinitions(lh_list_t<signal_def_t> *l, set_t F)
{
	set_t F2 = Combine(F, T_LEFTPAR, T_REFINEMENT, T_COMMA, T_SEMICOLON, -1);
	expect(T_SIGNAL);
	while (symbol == T_NAME)
	{
		NEWNODE(signal_def_t, sp);
		ScopeStack->EnterScope(Q_SIGNAL, sp);
		sp->info.nm = *Name(F2);
		if (symbol == T_LEFTPAR)
		{
			expect(T_LEFTPAR);
			// Must be sort IDs
			IdentList(&sp->info.sortrefs, Combine(F, T_RIGHTPAR, T_COMMA, T_SEMICOLON, -1));
			expect(T_RIGHTPAR);
		}
		if (symbol == T_REFINEMENT)
			SDLerror(ERR_NOREFINE);
		l->append(sp);
		ScopeStack->ExitScope();
		if (symbol == T_COMMA) nextsymbol();
		else break;
	}
	End(F);
}

/****************************************/	  
/* Channel and Signal Route definitions */
/****************************************/	  

static void ChannelConnection(connect_def_t *con, set_t F)
{
	expect(T_CONNECT);
	// Must be channel ID
	(void)Identifier(&con->channel,Combine(F, T_AND, -1));
	expect(T_AND);
	// Must be subchannel IDs
	IdentList(&con->subchannels, Combine(F, T_SEMICOLON, -1));
	End(F);
}

static void Path(path_t *p, set_t F)
{
	if (!entrySync(Set(T_FROM,-1), F))
		return;
	expect(T_FROM);
	if (symbol==T_ENV)
	{
		p->originator = *envIdent;
		nextsymbol();
	} else (void)Identifier(&p->originator,Combine(F, T_TO, -1));
	expect(T_TO);
	if (symbol==T_ENV) 
	{
		p->destination = *envIdent;
		nextsymbol();
	} else (void)Identifier(&p->destination,Combine(F, T_WITH, -1));
	expect(T_WITH);
	SignalList(&p->s, Combine(F, T_SEMICOLON, -1));
	End(F);
}
				  
/*
 * A signal route is a transportation route for signals, and
 * can be considered as one or two indpendent unidirectional
 * signal route paths between two processes or between a 
 * process and its environment. Signals conveyed by signal routes
 * are delivered to the destination endpoint with no delay.
 * No signal route connects process instances of the same
 * type; in this case an output implies that the signal
 * is placed directly in the input port of the receiving
 * process. Several signal routes
 * may exist between the same two endpoints, and the same
 * signal type can be conveyed on different signal routes.
 */		

static void SignalRouteDefinition(route_def_t *rp, set_t F)
{
	expect(T_SIGNALROUTE);
	/* At least one path must be a process; if both
		are they must be different. If both paths
		are specified they must be in reverse
		direction to each other. */
	rp->nm = *Name(Combine(F, T_FROM, -1));
	Path(&rp->paths[0], Combine(F, T_FROM, T_SUBSTRUCTURE, T_ENDCHANNEL, -1));
	if (symbol == T_FROM)
		Path(&rp->paths[1], F);
	else
		exitSync(F);
}

/*
 * A channel is a transportation route for signals. It can be
 * considered as one or two independent unidirectional channel
 * paths between two blocks or between a block and the 
 * environment. Signals conveyed by channels are delivered
 * to the destination endpoint, after spending an indeterminant
 * and non-constant time interval in a FIFO delaying queue
 * associated with the direction in a channel. Several channels
 * may exist between the same two endpoints, and the same
 * signal type can be conveyed on different channels.
 */		

static void ChannelDefinition(channel_def_t *cp, set_t F)
{
	/* At least one path must be a block; if both are, they
		must be different, and the two paths must be in
		the reverse direction to each other. The block 
		must be defined in the current scope. */
	expect(T_CHANNEL);
	cp->nm = *Name(Combine(F, T_FROM, -1));
	Path(&cp->paths[0], Combine(F, T_FROM, T_SUBSTRUCTURE, T_ENDCHANNEL, -1));
	if (symbol==T_FROM)
		Path(&cp->paths[1], Combine(F, T_SUBSTRUCTURE, T_ENDCHANNEL, -1));
#if 0
	if (symbol == T_SUBSTRUCTURE)
		ChannelSubstructureDefinition(??, Combine(F, T_ENDCHANNEL, -1));
#endif
	expect(T_ENDCHANNEL);
	CheckClosingName(cp->name(), "Channel");
	End(F);
}

/*
 * Each channel identifier connected to the enclosing block must
 * be mentioned in exactly one channel to route connection. The 
 * channel identifier must denote a channel connected to the
 * enclosing block. Each signal route identifier in the connection
 * must be defined in the same block as the connection and must have 
 * that block's boundary as one of its endpoints. Each signal route
 * identifier defined in the surrounding block which has its 
 * environment as one of its endpoints must be mentioned in one
 * and only one channel-to-route connection. For a given direction
 * the union of the signal identifier sets in the signal routes
 * in a c2r connection must be equal to the set of signals conveyed
 * by the channel in the same c2r connection and corresponding to
 * the same direction.
 */

static void ChannelToRouteConnection(c2r_def_t *c2r, set_t F)
{
	expect(T_CONNECT);
	// Must be channel ID
	(void)Identifier(&c2r->channel,Combine(F, T_AND, -1));
	expect(T_AND);
	// Must be signal route IDs
	IdentList(&c2r->routes, Combine(F, T_SEMICOLON, -1));
	End(F);
}

/************************/
/* Variable Definitions */
/************************/

/*
 * The value of a variable can only be modified by its owner
 * (the process or procedure where the variable is declared).
 * The value of a variable is only known to its owner, unless
 * the variable is REVEALED, in which case other processes in
 * the same block can VIEW the value.
 */

static void VariableDefinition(variable_def_t *vp, set_t F)
{
	expect(T_DCL);
	for (;;)
	{
		if (symbol == T_EXPORTED && !vp->isExported)
			vp->isExported = True;
		else if (symbol == T_REVEALED && !vp->isRevealed)
			vp->isRevealed = True;
		else break;
		nextsymbol();
	}
	for (;;)
	{
		NEWNODE(variable_decl_t, d);
		VariableNameList(&d->info.names, Union(F, FirstID));
		/* Must be sort or syntype ID */
		Identifier(&d->info.sort,Combine(F, T_COMMA, T_SEMICOLON, T_ASGN, -1));
		if (symbol == T_ASGN)
		{
			nextsymbol();
			d->info.value = Expression(Combine(F, T_COMMA, T_SEMICOLON, -1));
		}
		vp->decls.append(d);
		if (symbol == T_COMMA) nextsymbol();
		else break;
	}
	End(F);
}

/*
 * The variables in a view definition must have the REVEALED
 * attribute in the definition, and must be of the same sort.
 */

static void ViewDefinitions(lh_list_t<view_def_t> *vl, set_t F)
{
	expect(T_VIEWED);
	for (;;)
	{
		NEWNODE(view_def_t, v);
		IdentList(&v->info.variables, Union(F, FirstID));
		/* Must be sort or syntype ID */
		Identifier(&v->info.sort,Combine(F, T_COMMA, T_SEMICOLON, -1));
		vl->append(v);
		if (symbol == T_COMMA) nextsymbol();
		else break;
	}
	End(F);
}

/*********************/
/* Import Definition */
/*********************/

static void ImportDefinitions(lh_list_t<import_def_t> *il, set_t F)
{
	expect(T_IMPORTED);
	for (;;)
	{
		NEWNODE(import_def_t, ip);
		NameList(&ip->info.names, Combine(F, T_NAME, -1));
		// must be sort or syntype ID
		(void)Identifier(&ip->info.sort,Combine(F, T_COMMA, T_SEMICOLON, -1));
		il->append(ip);
		if (symbol == T_COMMA) nextsymbol();
		else break;
	}
	End(F);
}

static void TimerDefinitions(lh_list_t<timer_def_t> *tl, set_t F)
{
	expect(T_TIMER);
	for (;;)
	{
		NEWNODE(timer_def_t, tp);
		tp->info.nm = *Name(Combine(F, T_SEMICOLON, T_COMMA, T_LEFTPAR, -1));
		if (symbol == T_LEFTPAR)
		{
			expect(T_LEFTPAR);
			IdentList(&tp->info.paramtypes,Combine(F, T_RIGHTPAR, -1));
			expect(T_RIGHTPAR);
		}
		tl->append(tp);
		if (symbol == T_COMMA) nextsymbol();
		else break;
	}
	End(F);
}

/******************/
/* Process Bodies */
/******************/

static void InformalText(set_t F)
{
	expect(T_STRING);
	exitSync(F);
}

static void Task(lh_list_t<assignment_t> *tl, set_t F)
{
	set_t F2 = Combine(F, T_COMMA, -1);
	expect(T_TASK);
	if (symbol == T_STRING)
	{
		/* Informal text list */
		for (;;)
		{
			InformalText(Combine(F, T_COMMA, -1));
			if (symbol == T_COMMA) nextsymbol();
			else break;
		}
		SDLwarning(ERR_INFORMAL);
	}
	else for (;;)
	{
		NEWNODE(assignment_t, a);
		VariableReference(&a->info.lval, Combine(F2, T_ASGN, -1));
		expect(T_ASGN);
		a->info.rval = Expression(F2);
		tl->append(a);
		if (symbol == T_COMMA) nextsymbol();
		else break;
	}
	exitSync(F);
}

static void Invocation(invoke_node_t *i, set_t F)
{
	nextsymbol();
	(void)Identifier(&i->ident, Combine(F, T_LEFTPAR, -1)); /* Must be process ID */
	if (symbol == T_LEFTPAR)
	{
		expect(T_LEFTPAR);
		if (symbol != T_RIGHTPAR)
			ExpressionList(&i->args, Combine(F, T_RIGHTPAR, -1));
		expect(T_RIGHTPAR);
	}
	exitSync(F);
}

static void Reset(lh_list_t<timer_reset_t> *rl, set_t F)
{
	expect(T_RESET);
	expect(T_LEFTPAR);
	for (;;)
	{
		NEWNODE(timer_reset_t, rn);
		(void)Identifier(&rn->info.timer, Combine(F, T_LEFTPAR, T_COMMA, T_RIGHTPAR, -1)); /* Must be a timer ID */
		if (symbol == T_LEFTPAR)
		{
			nextsymbol();
			ExpressionList(&rn->info.args, Combine(F, T_RIGHTPAR, -1));
			expect(T_RIGHTPAR);
		}
		rl->append(rn);
		if (symbol == T_COMMA) nextsymbol();
		else break;
	}
	expect(T_RIGHTPAR);
	exitSync(F);
}

static void Set(lh_list_t<timer_set_t> *sl, set_t F)
{
	expect(T_SET);
	for (;;)
	{
		NEWNODE(timer_set_t, sn);
		expect(T_LEFTPAR);
		sn->info.time = Expression(Combine(F, T_COMMA, -1)); /* Should be time expression */
		expect(T_COMMA);
		(void)Identifier(&sn->info.timer, Combine(F, T_LEFTPAR, T_RIGHTPAR, -1));
		if (symbol == T_LEFTPAR)
		{
			nextsymbol();
			ExpressionList(&sn->info.args, Combine(F, T_RIGHTPAR, -1));
			expect(T_RIGHTPAR);
		}
		expect(T_RIGHTPAR);
		sl->append(sn);
		if (symbol == T_COMMA) nextsymbol();
		else break;
	}
	exitSync(F);
}

static void Output(output_t *op, set_t F)
{
	expect(T_OUTPUT);
	for (;;)
	{
		NEWNODE(output_arg_t, oa);
		(void)Identifier(&oa->info.signal, Combine(F, T_TO, T_VIA, T_COMMA, T_LEFTPAR, -1));
		if (symbol == T_LEFTPAR)
		{
			expect(T_LEFTPAR);
			if (symbol != T_RIGHTPAR)
				ExpressionList(&oa->info.args, Combine(F, T_RIGHTPAR, -1));
			expect(T_RIGHTPAR);
		}
		op->signals.append(oa);
		if (symbol == T_COMMA) nextsymbol();
		else break;
	}
	if (symbol == T_TO)
	{
		nextsymbol();
		op->hasDest = True;
		op->dest = Expression(Combine(F, T_VIA, -1)); /* Must be a PId */
	}
	else op->hasDest = False;
	if (symbol == T_VIA)
	{
		nextsymbol();
		IdentList(&op->via, F); /* comma separated, and must be either all channel
				Ids or all sigroute Ids. Is this defined yet? */
	}
	else exitSync(F);

}

static void Export(lh_list_t<ident_t> *l, set_t F)
{
	expect(T_EXPORT);
	expect(T_LEFTPAR);
	IdentList(l, Combine(F, T_RIGHTPAR, -1));
	expect(T_RIGHTPAR);
	exitSync(F);
}

static void Read(read_node_t *r, set_t F)
{
	nextsymbol();
	VariableReference(&r->varref, F);
}

static void Write(write_node_t *w, set_t F)
{
	nextsymbol();
	for (;;)
	{
		NEWNODE(heap_ptr_t, e);
		e->info = Expression(Combine(F, T_COMMA, -1));
		w->exprs.append(e);
		if (symbol == T_COMMA) nextsymbol();
		else break;
	}
}

#if 0
/********************/
/* Priority Outputs */
/********************/

static void PriorityOutput(pri_output_t *op, set_t F)
{
	expect(T_PRIORITY);
	expect(T_OUTPUT);
	for (;;)
	{
		NEWNODE(output_arg_t,oa);
		(void)Identifier(&oa->info.signal, Combine(F, T_TO, T_VIA, T_COMMA, T_LEFTPAR, -1));
		if (symbol == T_LEFTPAR)
		{
			expect(T_LEFTPAR);
			if (symbol != T_RIGHTPAR)
				ExpressionList(&oa->info.args, Combine(F, T_RIGHTPAR, -1));
			expect(T_RIGHTPAR);
		}
		op->signals.append(oa);
		if (symbol == T_COMMA) nextsymbol();
		else break;
	}
}
#endif

static void RangeConditions(lh_list_t<range_condition_t> *rcl, set_t F)
{
	set_t F2 = Combine(F, T_EQ, T_NE, T_LESS, T_LE, T_GT, T_GE, T_INTEGER,
		T_NATURAL, T_REAL, T_STRING, T_NAME, T_MINUS, T_NOT, T_TRUE,
		T_FALSE, -1);
	heap_ptr_t ep;
	if (!entrySync(F2, F))
		return;
	for (;;)
	{
		range_op_t op;
		NEWNODE(range_condition_t, rc);
		switch (symbol)
		{
			case T_EQ:
				op = RO_EQU; goto getRight;
			case T_NE:
				op = RO_NEQ; goto getRight;
			case T_LESS:
				op = RO_LE; goto getRight;
			case T_LE:
				op = RO_LEQ; goto getRight;
			case T_GT:
				op = RO_GT; goto getRight;
			case T_GE:
				op = RO_GTQ; goto getRight;
			getRight:
				rc->info.op = op;
				nextsymbol();
				rc->info.upper = Expression(F2);
				break;
			default:
				ep = Expression(Combine(F2, T_COLON, -1)); /* Must be constant expression */
				if (symbol == T_COLON)
				{
					rc->info.lower = ep;
					rc->info.op = RO_IN;
					nextsymbol();
					rc->info.upper = Expression(F2);
				}
				else 
				{
					rc->info.op = RO_NONE;
					rc->info.upper = ep;
				}
				break;
		}
		rcl->append(rc);
		if (symbol == T_COMMA) nextsymbol();
		else break;
	}
	exitSync(F);
}

static heap_ptr_t Question(set_t F)
{
	if (symbol == T_STRING)
	{
		InformalText(F);
		return 0;
	} else
		return Expression(F);
}

static void Answer(subdecision_node_t *sd, set_t F)
{
	expect(T_LEFTPAR);
	if (symbol == T_STRING)
		InformalText(Combine(F, T_RIGHTPAR, -1));
	else
		RangeConditions(&sd->answer, Combine(F, T_RIGHTPAR, -1));
	expect(T_RIGHTPAR);
	expect(T_COLON);
	if (symbol != T_LEFTPAR)
		Transition(&sd->transition, F);
}

static void Decision(decision_node_t *dc, set_t F)
{
	expect(T_DECISION);
	dc->question = Question(Combine(F, T_SEMICOLON, -1));
	End(Combine(F, T_LEFTPAR, -1));
	while (symbol == T_LEFTPAR)
	{
		NEWNODE(subdecision_node_t, sd);
		Answer(&sd->info, Combine(Union(F,FirstTransition),T_ELSE, T_ENDDECISION, T_LEFTPAR, -1)) ;
		dc->answers.append(sd);
	}
	if (symbol == T_ELSE)
	{
		nextsymbol();
		expect(T_COLON);
		if (symbol != T_ENDDECISION)
		{
			NEWNODE(subdecision_node_t, sd);
			Transition(&sd->info.transition, Combine(F, T_ENDDECISION, -1));
			dc->answers.append(sd);
		}
	}
	expect(T_ENDDECISION);
	exitSync(F);
}

static void AddNode(transition_t *tr, name_t n, gnode_type_t t, void *ptr)
{
	NEWNODE(gnode_t, g);
	g->info.label = n;
	g->info.type = t;
	g->info.node = GetOffset(ptr);
	tr->nodes.append(g);
}

static void Transition(transition_t *t, set_t F)
{
	set_t Terms = Set(T_NEXTSTATE, T_JOIN, T_STOP, T_RETURN, -1);
	set_t F2 = Combine(Union(F, FirstTransition), T_SEMICOLON, -1);
	if (!entrySync(FirstTransition, F))
		return;
	while (!In(Terms, symbol) && In(FirstTransition,symbol))
	{
		name_t n;
		if (symbol == T_NAME) // label?
		{
			n.name(Name(Combine(F2, T_COLON, -1))->index());
			expect(T_COLON);
		}
		if (In(Terms, symbol)) t->label = n;
		else
		{
			switch(symbol)
			{
				case T_TASK:
					NEWLIST(assignment_t, al);
					assert(al);
					Task(al, F2);
					AddNode(t, n, N_TASK, al);
					break;
				case T_OUTPUT:
					output_t *o = new output_t;
					assert(o);
					Output(o, F2);
					AddNode(t, n, N_OUTPUT, o);
					break;
#if 0
				case T_PRIORITY:
					pri_output_t *po = new pri_output_t;
					assert(po);
					PriorityOutput(po, F2);
					AddNode(t, n, N_PRI_OUTPUT, po);
					break;
#endif
				case T_CREATE:
					invoke_node_t *ci = new invoke_node_t;
					assert(ci);
					ci->tag = T_CREATE;
					Invocation(ci, F2);
					AddNode(t, n, N_CREATE, ci);
					break;
				case T_DECISION:
					decision_node_t *dc = new decision_node_t;
					assert(dc);
					Decision(dc, F2);
					AddNode(t, n, N_DECISION, dc);
					break;
#if 0
				case T_ALTERNATIVE:
					TransitionOption(al, F2);
					AddNode(t, n, N_ALTERNATIVE, al);
					break;
#endif
				case T_SET:
					NEWLIST(timer_set_t, sl);
					Set(sl, F2);
					AddNode(t, n, N_SET, sl);
					break;
				case T_RESET:
					NEWLIST(timer_reset_t, rl);
					Reset(rl, F2);
					AddNode(t, n, N_RESET, rl);
					break;
				case T_EXPORT:
					NEWLIST(ident_t, il);
					Export(il, F2);
					AddNode(t, n, N_EXPORT, il);
     					break;
				case T_CALL:
					invoke_node_t *pc = new invoke_node_t;
					assert(pc);
					pc->tag = T_CALL;
					Invocation(pc, F2);
					AddNode(t, n, N_CALL, pc);
					break;
				// non-standard
				case T_READ:
					read_node_t *rd = new read_node_t;
					assert(rd);
					Read(rd, F2);
					AddNode(t, n, N_READ, rd);
					break;
				case T_WRITE:
					write_node_t *wr = new write_node_t;
					assert(wr);
					Write(wr, F2);
					AddNode(t, n, N_WRITE, wr);
					break;
			}
			End(F2);
		}
	}
	if (In(Terms, symbol))
	{
		switch(symbol)
		{
		case T_RETURN:
			t->type = RETURN;
			nextsymbol();
			break;
		case T_STOP:
			t->type = STOP;
			nextsymbol();
			break;
		case T_JOIN:
			t->type = JOIN;
			nextsymbol();
			t->next.name(Name(F2)->index()); /* connector name */
			break;
		case T_NEXTSTATE:
			t->type = NEXTSTATE;
			nextsymbol();
			if (symbol == T_MINUS)
			{
				t->next.name(noName->index());
				nextsymbol();
	 		}
			else t->next.name(Name(F2)->index());
			break;
		}
		End(F);
	}
}

static void ContinuousSignal(continuous_signal_t *s, set_t F)
{
	set_t F2 = Union(FirstTransition, F);
	expect(T_PROVIDED);
	s->condition = Expression(Combine(F2, T_SEMICOLON, -1));
	End(Combine(F2, T_PRIORITY, -1));
	if (symbol == T_PRIORITY)
	{
		nextsymbol();
		s->priority = tokenVal;
		if (symbol != T_INTEGER && symbol != T_NATURAL)
			expect(T_INTEGER);
		else nextsymbol();
		End(F2);
	}
	else 
	{
		s->priority = UNDEFINED;
		exitSync(F2);
	}
	Transition(&s->transition, F);
}

static void Save(save_part_t *s, set_t F)
{
	expect(T_SAVE);
	if (symbol == T_ASTERISK)
	{
		s->isAsterisk = True;
		nextsymbol();
	}
	else 
	{
		s->isAsterisk = False;
		SignalList(&s->savesigs, Combine(F, T_SEMICOLON, -1));
	}
	End(F);
}

static void Stimulus(stimulus_t *s, set_t F)
{
	/* Must be timer or signal ID */
	(void)Identifier(&s->signal,Combine(F, T_LEFTPAR, -1));
	if (symbol == T_LEFTPAR)
	{
		nextsymbol();
		if (symbol != T_RIGHTPAR)
			/* Must be variable IDs */
			IdentList(&s->variables, Combine(F, T_RIGHTPAR,-1));
		expect(T_RIGHTPAR);
	}
	exitSync(F);
}

static void StimulusList(lh_list_t<stimulus_t> *sl, set_t F)
{
	set_t F2 = Combine(F, T_COMMA, -1);
	for (;;)
	{
		NEWNODE(stimulus_t, s);
		Stimulus(&s->info,F2);
		sl->append(s);
		if (symbol == T_COMMA) nextsymbol();
		else break;
	}
	exitSync(F);
}

#if 0
static void PriorityInput(input_part_t *i, set_t F)
{
	set_t F2 = Union(FirstTransition, F);
	expect(T_PRIORITY);
	expect(T_INPUT);
	i->isAsterisk = False;
	StimulusList(&i->stimuli, Combine(F2, T_SEMICOLON, -1));
	End(F2);
	Transition(&i->transition, F);
}
#endif

static void Input(input_part_t *i, set_t F)
{
	set_t F2 = Union(FirstTransition, F);
	expect(T_INPUT);
	if (symbol==T_ASTERISK)
	{
		i->isAsterisk = True;
		nextsymbol();
	}
	else
	{
		i->isAsterisk = False;
		StimulusList(&i->stimuli, Combine(F2, T_SEMICOLON, -1));
	}
	End(Combine(F2, T_PROVIDED, -1));
	if (symbol == T_PROVIDED)
	{
		nextsymbol();
		i->enabler = Expression(Combine(F2, T_SEMICOLON, -1));
		End(F2);
	}
	else i->enabler = (u_s_code_word_t)0;
	Transition(&i->transition, F);
}

static int StateList(Bool_t *isAsterisk, lh_list_t<name_t> *states, set_t F)
{
	int rtn;
	if (symbol==T_ASTERISK)
	{
		*isAsterisk = True;
		nextsymbol();
		if (symbol==T_LEFTPAR)
		{
			nextsymbol();
			rtn = NameList(states, Combine(F, T_RIGHTPAR, -1)); /* Must be state names */
			expect(T_RIGHTPAR);
		}
	}
	else
	{
		*isAsterisk = False;
		rtn = NameList(states, F); /* Must be state names */
	}
	return rtn;
}

static void State(lh_list_t<state_node_t> *states, set_t F)
{
	NEWNODE(state_node_t, s);
	Bool_t moreBody = True;
	set_t F2 = Combine(F, T_SAVE, T_INPUT, T_PRIORITY, T_PROVIDED, T_ENDSTATE, -1);
	expect(T_STATE);
	int cnt = StateList(&s->info.isAsterisk, &s->info.states, Combine(F, T_SEMICOLON, -1));
	End(F2);
	while (moreBody)
	{
		switch(symbol)
		{
		case T_SAVE:
			Save(s->info.newsave(), F2);
			break;
		case T_INPUT:
			Input(s->info.newinput(), F2);
			break;
#if 0
		case T_PRIORITY:
			PriorityInput(s->info.newpinput(), F2);
			break;
#endif
		case T_PROVIDED:
			ContinuousSignal(s->info.newcsig(), F2);
			break;
		default:
			moreBody = False;
			break;
		}
	}
	if (symbol==T_ENDSTATE)
	{
		nextsymbol();
		if (symbol == T_NAME)
		{
			int idx = Name(Combine(F, T_SEMICOLON, -1))->index();
			int idx2 = (s->info.states.front())->info.index();
			if (s->info.isAsterisk || cnt != 1 || idx != idx2)
				SDLerror(ERR_BADENDSTATE);
		}
		End(F);
	}
	exitSync(F);
	states->append(s);
}


static void ProcessBody(transition_t *t, lh_list_t<state_node_t> *states,
	set_t F)
{
	set_t F2 = Combine(F, T_STATE, -1);
	expect(T_START);
	End(FirstTransition);
	Transition(t, F2);
	while (symbol == T_STATE)
		State(states, F2);
	exitSync(F);
}

/*************************/
/* Procedure Definitions */
/*************************/

/*
 * A procedure is a means of giving a name to an assembly of items,
 * and representing this assembly by a single reference. The rules
 * for procedures impose a discipline upon the way in which the
 * assembly of items is chosen, and limit the scope of the name of
 * variables defined in the procedure. These procedure variables
 * cannot be revealed, viewed, exported or imported. Procedure variables
 * are created when the start node is interpreted, and destroyed when
 * the return node is interpreted.
 * A procedure definition is interpreted only when a process instance
 * calls it, and is interpreted by that process instance. The interpretation
 * of a procedure definition cause sthe creation of a procedure instance
 * and the interpretation to commence in the following way:
 *
 * i) A local variable is created for each in-parameter with the
 *	specified name and sort. The variable is assigned the
 *	value of the expression given by the corresponding actual
 *	parameter, which may be undefined. Note that a formal parameter
 *	with no explicit attribute has an implicit in attribute.
 *	In-Out parameters are reference parameters, so no variables
 *	are created for these.
 * ii) A local variable is created for each Variable definition,
 *	with the appropriate name and sort.
 * iii) The transition contained in the procedure start node is
 *	interpreted.
 */


static void ProcedureFormalParameters(lh_list_t<procedure_formal_param_t> *l, set_t F)
{
	set_t F2 = Combine(F, T_COMMA, T_SEMICOLON, -1);
	expect(T_FPAR);
	for (;;)
	{
		NEWNODE(procedure_formal_param_t, p);
		p->info.isValue = True;
		if (symbol == T_IN)
		{
			nextsymbol();
			if (symbol == T_SLASH)
			{
				nextsymbol();
				expect(T_OUT);
				p->info.isValue = False;
			}
		}
		ProcedureParamNameList(&p->info.names, Union(F2, FirstID));
		(void)Identifier(&p->info.sort, F2);
		l->append(p);
		if (symbol == T_COMMA) nextsymbol();
		else break;
	}
	End(F);
}

static Bool_t ProcedureDefinition(procedure_def_t *ppp, set_t F)
{
	ident_t id;
	set_t F2 = Combine(F, T_PROCEDURE, T_NEWTYPE, T_SYNTYPE, T_SYNONYM,
			T_DCL, T_SELECT, T_MACRODEFINITION, T_EOF, T_START,
			T_ENDSELECT, -1);
	Bool_t moreDefs = True, isRemote = ( (ppp == NULL) ? True : False ),
		rtn = True;
	expect(T_PROCEDURE);
	if (isRemote)
	{
		(void)Identifier(&id,Combine(F, T_LEFTPAR, T_SEMICOLON, -1));
		ppp = (procedure_def_t *)ScopeStack->qualify(&id, Q_PROCEDURE);
		if (ppp == NULL || ppp->IsReference(RF_DEFINED))
		{
			if (ppp) SDLerror(ERR_DUPREMDEF, ppp->name());
			else SDLerror(ERR_QUALIFY);
			ppp = new procedure_def_t;
			assert(ppp);
		}
		ppp->nm.name(id.name());
	}
	else
	{
		ppp->name(Name(Combine(F, T_REFERENCED, T_SEMICOLON, -1))->index());
		if (symbol == T_REFERENCED)
		{
			nextsymbol();
			End(F);
			ppp->SetReference(RF_REMOTE);
			return True;
		}
		// See if already defined via a reference. If so
		// we look it up and set rtn to False to force the
		// allocated block to be deleted
		typeclass_t t = Q_PROCEDURE;
		int lvl;
		heap_ptr_t olddef = ScopeStack->findNodeInScope(ppp->nm, &t, &lvl);
		if (olddef && ScopeStack->RelLvl(lvl)==0)
		{
			rtn = False;
			ppp = GetProcdDefP(olddef);
		}
	}
	ppp->SetReference(RF_DEFINED);
	ScopeStack->EnterScope(Q_PROCEDURE, ppp);
	ppp->level = ScopeStack->Depth();
	End(Combine(F2, T_FPAR, -1));
	/* Parse the optional formal parameters */
	if (symbol == T_FPAR)
		ProcedureFormalParameters(&ppp->params, F2);
	int selection = 0;
	while (moreDefs)
	{
		if (checkSelect(selection, F2)) continue;
		switch (symbol)
		{
		// Nested procedures are more tricky...
		case T_PROCEDURE: /* Procedure definition or textual reference */
			lh_list_t<procedure_def_t> *pl
				= (lh_list_t<procedure_def_t> *)GetVoidP(ppp->procedures);
			NEWNODE(procedure_def_t, p);
			if (ProcedureDefinition(&p->info, F2))
				pl->append(p);
			else delete p;
			break;
		case T_NEWTYPE:
		case T_SYNTYPE:
		case T_SYNONYM:
		case T_GENERATOR:
			DataDefinition(&ppp->types,F2);
			break;
		case T_DCL:
			VariableDefinition(ppp->newvariable(), F2);
			break;
		case T_SELECT:
			continue;
		case T_ENDSELECT:
			if (selection<=0) SDLerror(ERR_NOSELECT);
			else selection--;
			nextsymbol();
			End(F2);
			break;
		case T_MACRODEFINITION:
			MacroDefinition(F2);
			break;
		case T_EOF:
		default:
			moreDefs = False;
			break;
		}
	}
	ProcessBody(&ppp->start, &ppp->states, Combine(F, T_ENDPROCEDURE, T_SEMICOLON, -1));
	expect(T_ENDPROCEDURE);
	if (!In(Set(T_SEMICOLON, -1), symbol))
	{
		if (isRemote)
			CheckClosingIdent(&id, "Procedure", Q_PROCESS, Combine(F, T_COMMENT, T_SEMICOLON, -1));
		else
			CheckClosingName(ppp->name(), "Procedure");
	}
	ScopeStack->ExitScope();
	End(F);
	return isRemote ? False : rtn;
}


/***********************/
/* Process Definitions */
/***********************/

/*
 * A process definition introduces the type of a process which is
 * intended to represent a dynamic behaviour. A process instance is
 * an extended  communicating FSM performing a certain set of actions
 * or transitions according to the reception of a given signal whenever
 * it is in a state.
 * If the number of instances is specified in both the process definition
 * and a process reference then these must agree. The initial number
 * of instances must be less than or equal to the maximum number, and 
 * the maximum number must be greater than zero.
 * Several instances of the same process type may exist at the same
 * time and execute asynchronously and in parallel. When a system is
 * created, the initial processes are created in a random order. The
 * signal communication between the processes commences only when all
 * the initial processes have been created. The formal parameters of
 * these initial processes are initialised to an undefined value.
 * Processes exist from the time the system is created or can be created
 * by create-request actions which start the processes being interpreted;
 * they may cease to exist by performing stop actions.
 * Signals received by process instances are denoted as input signals, 
 * while signals sent to process instances are denoted as output signals.
 * Signals may be consumed by a process only when it is in a state.
 * The complete valid input signal set it the union of the set of
 * signals in all signal routes leading to the process, the valid input
 * signal set, the implicit signals, and the timer signals.
 * One and only one input port is associated with each process instance.
 * When an input signal arrives at the process, it is put into the input
 * port of the process instance.
 * The process is either waiting in a state or active performing a 
 * transition. For each state, there is a save signal set. When waiting
 * in a state, the first input signal whose identifier is not in the save
 * signal set is taken from the queue and consumed by the process.
 * The input port may retain any number of input signals, so that several
 * input signals are queued for the process instance. The set of retained
 * signals are ordered in the queue according to their arrival time.
 * If two or more siganls arrive on different paths `simultaneously'
 * they are arbitrarily ordered.
 * When the process is created it is given an empty input port, and local 
 * variables are created with values assigned to them.
 * For all process instances present at system initialisation, the PARENT
 * expression always has the value NULL. For all newly created processes
 * the SENDER and OFFSPRING expressions have the value NULL.
 */

static void ProcessFormalParameters(lh_list_t<process_formal_param_t> *l, set_t F)
{
	set_t F2 = Combine(F, T_COMMA, T_SEMICOLON, -1);
	expect(T_FPAR);
	for (;;)
	{
		NEWNODE(process_formal_param_t, p);
		ProcessParamNameList(&p->info.names, Union(F2, FirstID));
		(void)Identifier(&p->info.sort,F2);
		l->append(p);
		if (symbol == T_COMMA) nextsymbol();
		else break;
	}
	End(F);
}

static Bool_t ProcessDefinition(process_def_t *ppp, set_t F)
{
	heap_ptr_t initP = 0, maxP = 0;
	ident_t id;
	Bool_t moreDefs = True, isRemote = ((ppp == NULL) ? True : False ),
		rtn = True;
	set_t F2 = Combine(F, T_SIGNAL, T_SIGNALLIST, T_PROCEDURE, T_NEWTYPE, 
			T_SYNTYPE, T_SYNONYM, T_GENERATOR, T_DCL, T_SELECT,
			T_MACRODEFINITION, T_EOF, T_TIMER, T_IMPORTED, T_EOF,
			T_START, T_VIEWED, T_SELECT, /* Whatever a service decomposition starts with, */ -1);
	expect(T_PROCESS);
	if (isRemote)
	{
		(void)Identifier(&id,Combine(F, T_LEFTPAR, T_SEMICOLON, -1));
		ppp = (process_def_t *)ScopeStack->qualify(&id, Q_PROCESS);
		if (ppp == NULL || ppp->IsReference(RF_DEFINED))
		{
			if (ppp) SDLerror(ERR_DUPREMDEF, ppp->name());
			else SDLerror(ERR_QUALIFY);
			ppp = new process_def_t;
			assert(ppp);
		}
		ppp->nm.name(id.name());
	}
	else
	{
		ppp->name(Name(Combine(F, T_REFERENCED, T_LEFTPAR, T_SEMICOLON, T_COMMENT,-1))->index());
		if (symbol == T_REFERENCED)
		{
			nextsymbol();
			End(F);
			ppp->SetReference(RF_REMOTE);
			return True;
		}
		// See if already defined via a reference. If so
		// we look it up and set rtn to False to force the
		// allocated block to be deleted
		typeclass_t t = Q_PROCESS;
		int lvl;
		heap_ptr_t olddef = ScopeStack->findNodeInScope(ppp->nm, &t, &lvl);
		if (olddef && ScopeStack->RelLvl(lvl)==0)
		{
			rtn = False;
			ppp = GetProcsDefP(olddef);
		}
	}
	/* Parse the optional initial & maximum number of process instances */
	if (symbol == T_LEFTPAR)
	{
		nextsymbol();
		if (symbol != T_COMMA)
			initP = Expression(Combine(F, T_COMMA, -1));
		expect(T_COMMA);
		if (symbol != T_RIGHTPAR)
			maxP = Expression(Combine(F, T_RIGHTPAR, -1));
		expect(T_RIGHTPAR);
	}
	ScopeStack->EnterScope(Q_PROCESS, ppp);
	ppp->SetReference(RF_DEFINED);
	ppp->level = ScopeStack->Depth();

	if (initP)
	{
		data_def_t *dd;
		expression_t *e = GetExprP(initP);
		if (ppp->initInst == 0)
		{
			ppp->initInst = initP;
			ppp->initval = e->EvalGround(dd);
		}
		else
		{
			if (e->EvalGround(dd) != ppp->initval)
				SDLerror(ERR_DIFINIT);
		}
	}	
	if (maxP)
	{
		data_def_t *dd;
		expression_t *e = GetExprP(maxP);
		if (ppp->maxInst == 0)
		{
			ppp->maxInst = maxP;
			ppp->maxval = e->EvalGround(dd);
		}
		else
		{
			if (e->EvalGround(dd) != ppp->maxval)
				SDLerror(ERR_DIFMAX);
		}
	}	
	End(Combine(F2, T_FPAR, T_SIGNALSET, -1));

	/* Parse the optional formal parameters */
	if (symbol == T_FPAR)
		ProcessFormalParameters(&ppp->params, Combine(F2, T_SIGNALSET, -1));
	/* Parse the optional valid input signal set */
	if (symbol == T_SIGNALSET)
	{
		nextsymbol();
		if (!In(Set(T_SEMICOLON, T_COMMENT, -1), symbol))
			SignalList(&ppp->validsig,Combine(F2, T_SEMICOLON, -1));
		End(F2);
	} // else??
	int selection = 0;
	while (moreDefs)
	{
		if (checkSelect(selection, F2)) continue;
		switch(symbol)
		{
		case T_SIGNAL:
			SignalDefinitions(&ppp->signals,F2);
			break;
		case T_SIGNALLIST:
			SignalListDefinition(ppp->newsiglist(), F2);
			break;
		case T_VIEWED:
			ViewDefinitions(&ppp->views,F2);
			break;
		case T_IMPORTED:
			ImportDefinitions(&ppp->imports,F2);
			break;
		case T_TIMER:
			TimerDefinitions(&ppp->timers,F2);
			break;
		case T_DCL:
			VariableDefinition(ppp->newvariable(), F2);
			break;
		case T_PROCEDURE: /* Procedure definition or textual reference */
			NEWNODE(procedure_def_t, p);
			if (ProcedureDefinition(&p->info, F2))
				ppp->procedures.append(p);
			else delete p;
			break;
		case T_NEWTYPE:
		case T_SYNTYPE:
		case T_SYNONYM:
		case T_GENERATOR:
			DataDefinition(&ppp->types,F2);
			break;
		case T_SELECT:
			continue;
		case T_ENDSELECT:
			if (selection<=0) SDLerror(ERR_NOSELECT);
			else selection--;
			nextsymbol();
			End(F2);
			break;
		case T_MACRODEFINITION:
			MacroDefinition(F2);
			break;
		case T_EOF:
		default:
			moreDefs = False;
			break;
		}
	}
	if (symbol==T_START)
		ProcessBody(&ppp->start, &ppp->states, Combine(F, T_ENDPROCESS, -1));
#if 0
	else
		ServiceDecomposition(Combine(F, T_ENDPROCESS, -1));
#endif
	expect(T_ENDPROCESS);
	if (!In(Set(T_SEMICOLON, -1), symbol))
	{
		if (isRemote)
			CheckClosingIdent(&id, "Process", Q_PROCESS, Combine(F, T_COMMENT, T_SEMICOLON, -1));
		else
			CheckClosingName(ppp->name(), "Process");
	}
	ScopeStack->ExitScope();
	End(F);
	return isRemote ? False : rtn;
}

static Bool_t BlockDefinition(block_def_t *bp, set_t F);

// Block substructures 

static void BlockSubstructureBody(subblock_def_t *b, set_t F)
{
	Bool_t moreDefs = True;
	set_t F2 = Combine(F, T_BLOCK, T_CHANNEL, T_CONNECT, T_SIGNAL,
			T_SIGNALLIST, T_NEWTYPE, T_SYNTYPE, T_SYNONYM,
			T_GENERATOR, T_SELECT, T_MACRODEFINITION, 
			T_ENDSUBSTRUCTURE, T_EOF, T_ENDSELECT, -1);
	lh_list_t<block_def_t> *bl = new lh_list_t<block_def_t>;
	assert(bl);
	b->blocks = GetOffset(bl);
	int selection = 0;
	while (moreDefs)
	{
		if (checkSelect(selection, F2)) continue;
		switch(symbol)
		{
		case T_BLOCK:
			NEWNODE(block_def_t, blk);
			if (BlockDefinition(&blk->info, F2))
				bl->append(blk);
			// else delete blk;
			break;
		case T_CHANNEL:
			ChannelDefinition(b->newchannel(), F2);
			break;
		case T_CONNECT:
			ChannelConnection(b->newchanconn(), F2);
			break;
		case T_SIGNAL:
			SignalDefinitions(&b->signals, F2);
			break;
		case T_SIGNALLIST:
			SignalListDefinition(b->newsiglist(), F2);
			break;
		case T_NEWTYPE:
		case T_SYNTYPE:
		case T_SYNONYM:
		case T_GENERATOR:	
			DataDefinition(&b->types, F2);
			break;
		case T_SELECT:
			continue;
		case T_ENDSELECT:
			if (selection<=0) SDLerror(ERR_NOSELECT);
			else selection--;
			nextsymbol();
			End(F2);
			break;
		case T_MACRODEFINITION:
			MacroDefinition(F2);
			break;
		default:
			moreDefs = False;
			break;
		}
	}
}

static void BlockSubstructureDefinition(subblock_def_t *sb, set_t F)
{
	set_t F2 = Combine(F, T_BLOCK, T_CHANNEL, T_CONNECT, T_SIGNAL,
			T_SIGNALLIST, T_NEWTYPE, T_SYNTYPE, T_SYNONYM,
			T_GENERATOR, T_SELECT, T_MACRODEFINITION, 
			T_ENDSUBSTRUCTURE, T_EOF, -1);
	if (!entrySync(Set(T_SUBSTRUCTURE,-1), F))
		return;
	expect(T_SUBSTRUCTURE);
	sb->name(Name(Combine(F, T_REFERENCED, T_SEMICOLON, -1))->index());
	if (symbol == T_REFERENCED) /* Textual block reference */
	{
		sb->SetReference(RF_REMOTE);
		nextsymbol();
		End(F);
		return;
	}
	End(F2);
	ScopeStack->EnterScope(Q_SUBSTRUCTURE, sb);
	BlockSubstructureBody(sb,F);
	expect(T_ENDSUBSTRUCTURE);
	CheckClosingName(sb->name(), "Block substructure");
	ScopeStack->ExitScope();
	End(F);
}

static void RemoteSubstructure(set_t F)
{
	ident_t id;
	expect(T_SUBSTRUCTURE);
	/* If the ident is a channel name or ID, we have a channel
		substructure, otherwise we have a block substructure */
	(void)Identifier(&id,Combine(F, T_SEMICOLON, -1));
	subblock_def_t *sb = (subblock_def_t *)ScopeStack->qualify(&id, Q_SUBSTRUCTURE);
	if (sb == NULL)
	{
#if 0
		channel_def_t *ch = (channel_def_t *)ScopeStack->dequalify(&id, Q_SUBSTRUCTURE);
		if (ch == NULL || ch->IsReference(RF_DEFINED))
		{
			if (ch) 
				SDLerror(ERR_DUPREMDEF, ch->name());
			else 
#endif
				SDLerror(ERR_QUALIFY);
#if 0
			ch = new channel_def_t
			assert(ch);
		}
		End(F2);
		ch->SetReference(RF_DEFINED);
		ChannelSubstructureBody(ch,F);
		expect(T_ENDSUBSTRUCTURE);
		CheckClosingIdent(&id, "Channel substructure", Q_SUBSTRUCTURE, Combine(F, T_COMMENT, T_SEMICOLON, -1));
#endif
	} else
	{
		set_t F2 = Combine(F, T_BLOCK, T_CHANNEL, T_CONNECT, T_SIGNAL,
			T_SIGNALLIST, T_NEWTYPE, T_SYNTYPE, T_SYNONYM,
			T_GENERATOR, T_SELECT, T_MACRODEFINITION, 
			T_ENDSUBSTRUCTURE, T_EOF, -1);
		End(F2);
		if (sb->IsReference(RF_DEFINED))
		{
			SDLerror(ERR_DUPREMDEF, sb->name());
			skipto(F);
			return;
		}
		sb->SetReference(RF_DEFINED);
		BlockSubstructureBody(sb,F);
		expect(T_ENDSUBSTRUCTURE);
		CheckClosingIdent(&id, "Block substructure", Q_SUBSTRUCTURE, Combine(F, T_COMMENT, T_SEMICOLON, -1));
	}
	End(F);
}

/*********************/
/* Block Definitions */
/*********************/

/*
 * A block definition is a container for one or more process definitions,
 * and/or a block substructure. The purpose of the block definition is
 * to group processes that as a whole perform a certain function, either
 * directly or by a block substructure.
 *
 * A block definition provides a static communication interface by 
 * which its processes can communicate with other processes, and provides
 * the scope for process definitions.
 *
 * To interpret a block is to create the initial processes in the block.
 */

static void BlockDefinitionBody(block_def_t *bp, set_t F)
{
	Bool_t moreDefs = True;
	set_t F2 = Combine(F, T_PROCESS, T_SIGNALROUTE, T_CONNECT, T_SIGNAL,
			T_SIGNALLIST, T_NEWTYPE, T_SYNTYPE, T_SYNONYM,
			T_GENERATOR, T_SELECT, T_MACRODEFINITION, T_EOF,
			T_ENDSELECT, -1);
	if (!entrySync(F2, F))
		return;
	int selection = 0;
	while (moreDefs)
	{
		if (checkSelect(selection, F2)) continue;
		switch(symbol)
		{
		case T_PROCESS: /* Process definition or textual process reference */
			NEWNODE(process_def_t, p);
			if (ProcessDefinition(&p->info, F2))
				bp->processes.append(p);
			else delete p;
			break;
		case T_SIGNALROUTE:
			SignalRouteDefinition(bp->newroute(), F2);
			break;
		case T_CONNECT:
			ChannelToRouteConnection(bp->newc2r(), F2);
			break;
		case T_SIGNAL:
			SignalDefinitions(&bp->signals,F2);
			break;
		case T_SIGNALLIST:
			SignalListDefinition(bp->newsiglist(),F2);
			break;
		case T_NEWTYPE:
		case T_SYNTYPE:
		case T_SYNONYM:
		case T_GENERATOR:	
			DataDefinition(&bp->types,F2);
			break;
		case T_SELECT:
			continue;
		case T_ENDSELECT:
			if (selection<=0) SDLerror(ERR_NOSELECT);
			else selection--;
			nextsymbol();
			End(F2);
			break;
		case T_MACRODEFINITION:
			MacroDefinition(F2);
			break;
		default:
			moreDefs = False;
			break;
		}
	}
	exitSync(F);
}

static Bool_t BlockDefinition(block_def_t *bp, set_t F)
{
	ident_t id;
	Bool_t isRemote = ( (bp == NULL) ? True : False ), rtn = True;
	set_t F2 = Combine(F, T_SUBSTRUCTURE, T_PROCESS, T_SIGNALROUTE, T_CONNECT,
			T_SIGNAL, T_SIGNALLIST, T_NEWTYPE, T_SYNTYPE, T_SYNONYM,
			T_GENERATOR, T_SELECT, T_MACRODEFINITION, T_EOF, -1);
	if (!entrySync(Set(T_BLOCK,-1), F))
		return False;
	expect(T_BLOCK);
 	if (isRemote)
	{
		(void)Identifier(&id,Combine(F, T_SEMICOLON, -1));
		bp = (block_def_t *)ScopeStack->qualify(&id, Q_BLOCK);
		if (bp == NULL || bp->IsReference(RF_DEFINED))
		{
			if (bp) SDLerror(ERR_DUPREMDEF, bp->name());
			else SDLerror(ERR_QUALIFY);
			bp = new block_def_t;
			assert(bp);
		}
		bp->name(id.name());
	}
	else
	{
		bp->name(Name(Combine(F, T_REFERENCED, T_SEMICOLON, -1))->index());
		if (symbol == T_REFERENCED) /* Textual block reference */
		{
			bp->SetReference(RF_REMOTE);
			nextsymbol();
			End(F);
			return True;
		}
		// See if already defined via a reference. If so
		// we look it up and set rtn to False to force the
		// allocated block to be deleted
		typeclass_t t = Q_BLOCK;
		int lvl;
		heap_ptr_t olddef = ScopeStack->findNodeInScope(bp->nm, &t, &lvl);
		if (olddef && ScopeStack->RelLvl(lvl)==0)
		{
			rtn = False;
			bp = GetBlkDefP(olddef);
		}
	}
	ScopeStack->EnterScope(Q_BLOCK, bp);
	bp->SetReference(RF_DEFINED);
	End(F2);
	if (symbol != T_SUBSTRUCTURE)
		BlockDefinitionBody(bp, Combine(F, T_SUBSTRUCTURE, T_ENDBLOCK,-1));
	if (symbol == T_SUBSTRUCTURE)
		BlockSubstructureDefinition(&bp->substructure, Combine(F, T_ENDBLOCK, -1));
	expect(T_ENDBLOCK);
	if (isRemote)
		CheckClosingIdent(&id, "Block", Q_BLOCK, Combine(F, T_COMMENT, T_SEMICOLON, -1));
	else
		CheckClosingName(bp->name(), "Block");
	End(F);
	ScopeStack->ExitScope();
	return isRemote ? False : rtn;
}

/*********************/
/* System Definition */
/*********************/

/*
 * A system definition is the SDL representation of a specification
 * or description of a system. Before interpreting a system definition,
 * a consistent subset is chosen (the instance of the definition). The
 * interpretation of this instance is performed by an abstract SDL 
 * machine which thereby gives semantics to the SDL concepts. To
 * interpret an instance of a system definition is to:
 *
 * i) initiate the system time
 * ii) interpret the blocks and their connected channels
 */

static void DefineBasicTypes(system_def_t *sp)
{
	NEWNODE(data_def_t, iddp);
	iddp->info.name("integer");
	iddp->info.tag = INTEGER_TYP;
	intType = &iddp->info;
	sp->types.append(iddp);
	NEWNODE(data_def_t, nddp);
	nddp->info.name("natural");
	nddp->info.tag = NATURAL_TYP;
	naturalType = &nddp->info;
	sp->types.append(nddp);
	NEWNODE(data_def_t, cddp);
	cddp->info.name("character");
	cddp->info.tag = CHARACTER_TYP;
	charType = &cddp->info;
	sp->types.append(cddp);
	NEWNODE(data_def_t, bddp);
	bddp->info.name("boolean");
	bddp->info.tag = BOOLEAN_TYP;
	boolType = &bddp->info;
	sp->types.append(bddp);
	NEWNODE(data_def_t, pddp);
	pddp->info.name("pid");
	pddp->info.tag = PID_TYP;
	pidType = &pddp->info;
	sp->types.append(pddp);
	NEWNODE(data_def_t, rddp);
	rddp->info.name("real");
	rddp->info.tag = REAL_TYP;
	realType = &rddp->info;
	sp->types.append(rddp);
	NEWNODE(data_def_t, tddp);
	tddp->info.name("time");
	tddp->info.tag = TIME_TYP;
	timeType = &tddp->info;
	sp->types.append(tddp);
	NEWNODE(data_def_t, dddp);
	dddp->info.name("duration");
	dddp->info.tag = DURATION_TYP;
	durType = &dddp->info;
	sp->types.append(dddp);
}

static void SystemDefinition(system_def_t *sp, set_t F)
{
	int moreDefs = 1, skipRest=0;
	set_t F2 = Combine(F, T_BLOCK, T_CHANNEL, T_SIGNAL, T_SIGNALLIST,
					T_NEWTYPE, T_SYNTYPE, T_SYNONYM,
					T_GENERATOR, T_MACRODEFINITION,
					T_ENDSYSTEM, T_EOF, T_SELECT,
					T_ENDSELECT, -1);
	if (symbol!=T_SYSTEM) skipRest = 1;
	expect(T_SYSTEM);
	sp->nm = *Name(Combine(F, T_SEMICOLON, -1));
	End(F2);
	ScopeStack->EnterScope(Q_SYSTEM, sp);
	DefineBasicTypes(sp); // Define the predefined type identifiers
	lookupPredefinedTypes();

	int selection = 0;
	while (moreDefs)
	{
		if (checkSelect(selection, F2)) continue;
		switch(symbol)
		{
		case T_BLOCK: /* Block definition or textual block reference */
			NEWNODE(block_def_t, b);
			if (BlockDefinition(&b->info, F2))
				sp->blocks.append(b);
			else delete b;
			break;
		case T_CHANNEL:
			ChannelDefinition(sp->newchannel(),F2);
			break;
		case T_SIGNAL:
			SignalDefinitions(&sp->signals,F2);
			break;
		case T_SIGNALLIST:
			SignalListDefinition(sp->newsiglist(), F2);
			break;
		case T_NEWTYPE:
		case T_SYNTYPE:
		case T_SYNONYM:
		case T_GENERATOR:
			DataDefinition(&sp->types,F2);
			break;
		case T_MACRODEFINITION:
			MacroDefinition(F2);
			break;
		case T_SELECT:
			continue;
		case T_ENDSELECT:
			if (selection<=0) SDLerror(ERR_NOSELECT);
			else selection--;
			nextsymbol();
			End(F2);
			break;
		default:
			moreDefs = 0;
			break;
		}
	}
	if (skipRest) skipto(Set(T_ENDSYSTEM,-1)); 
	expect(T_ENDSYSTEM);
	CheckClosingName(sp->name(), "System");
	ScopeStack->ExitScope();
	End(F);
}

static void System(system_def_t *sys)
{
	set_t F = Set(T_BLOCK, T_PROCESS, T_SERVICE, T_PROCEDURE,
				T_SUBSTRUCTURE, T_MACRODEFINITION, T_EOF, -1);
	envName = new name_t("ENV");
	assert(envName);
	envIdent = new ident_t(*envName);
	assert(envIdent);

	SystemDefinition(sys, F);
	Bool_t moredefs = True;
	while (moredefs)
	{
		/* For each remote definition, there must exist one reference
			to that definition in the system or in other remote definitions.
			(Exception is macros which need not have any references).
			The reference is replaced by the definition.
			Note that remote definitions have identifiers rather than
			names. If the identifier is a name with no qualifier,
			the name must be unique within the entity class in the system.
			In replacing a reference with a definition the identifier
			must be replaced by the name.
		*/
		if (symbol == T_EOF && *fileList)
		{
			lineNum = 1;
			inStr.close();
			inStr.open(*fileList);
			if (!inStr)
			{
				char fname[120];
				strcpy(fname,*fileList);
				strcat(fname,".sdl");
				inStr.open(fname);
				if (!inStr) 
					SDLfatal(ERR_NOFILE, *fileList);
				fileTable->addName(fname);
				cerr << "File: " << fname << endl;
			}
			else
			{
				fileTable->addName(*fileList);
				cerr << "File: " << *fileList << endl;
			}
			fileList++;
			nextchar(); nextsymbol();
		}
		switch (symbol)
		{
		case T_BLOCK:
			(void)BlockDefinition(NULL, F);
			break;
		case T_PROCESS:
			(void)ProcessDefinition(NULL, F);
			break;
		case T_PROCEDURE:
			(void)ProcedureDefinition(NULL, F);
			break;
#if 0
		case T_SERVICE:
			RemoteServiceDefinition(System, F);
			break;
#endif
		case T_MACRODEFINITION:
#if 0
			RemoteMacroDefinition(System, F);
#else
			SDLfatal(ERR_NOMACROS,NULL);
#endif
			break;
		case T_SUBSTRUCTURE:
			RemoteSubstructure(F);
			break;
		case T_EOF:
			moredefs = False;
			break;
		default:
			SDLerror(ERR_NOREMOTE, NULL);
		}
	}
	/* Make sure there are no references with no corresponding
		remote definitions, no remote definitions (other
		than macros) without corresponding references, and
		that there is at least one block definition in the 
		system. */
}

void SDLparse(void)
{
	FirstID = Set(T_SYSTEM, T_BLOCK, T_PROCESS, T_SERVICE, T_SIGNAL,
		T_PROCEDURE, T_SUBSTRUCTURE, T_TYPE, T_NAME, -1);
	FirstTransition = Set(T_NAME, T_TASK, T_OUTPUT, T_PRIORITY, T_CREATE,
			T_DECISION, T_ALTERNATIVE, T_SET, T_RESET, T_EXPORT,
			T_CALL, T_RETURN, T_STOP, T_JOIN, T_NEXTSTATE,
			T_READ, T_WRITE, -1);
	nextsymbol();
	sys = new system_def_t;
	assert(sys);
	System(sys);
//	cout << *sys; // test decompiler; take out at end
	SaveAST("ast.out");
	nameStore->status();
}

