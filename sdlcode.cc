#pragma hdrfile "CODE.SYM"
#include "sdlcode.h"
#pragma hdrstop
#include <string.h>
#include <assert.h>

class codespace_t *Code;

static short op_argcnt_tbl[(int)NUM_OPS+1] = 
{
/* Add              */ 0,
/* And              */ 0,
/* ArgAssign        */ 1,
/* Assign           */ 1,
/* Channel          */ 1,
/* Constant         */ 1,
/* DefAddr          */ 1,
/* DefArg           */ 2,
/* Div              */ 0,
/* Do               */ 1,
/* EndClause	    */ 0,
/* EndTrans         */ 0,
/* Equal            */ 0,
/* FAdd		    */ 0,
/* FConst	    */ 1,
/* FDiv	            */ 0,
/* FEqu		    */ 0,
/* FGEq		    */ 0,
/* FGtr		    */ 0,
/* Field            */ 1,
/* Fix              */ 0,
/* FLEq		    */ 0,
/* FLes		    */ 0,
/* Float	    */ 0,
/* FMinus	    */ 0,
/* FMul		    */ 0,
/* FNeq		    */ 0,
/* FSub		    */ 0,
/* GlobalTime	    */ 0,
/* GlobalValue	    */ 1,
/* GlobalVar	    */ 1,
/* Goto             */ 1,
/* Greater          */ 0,
/* Index            */ 3,
/* Less             */ 0,
/* LocalValue       */ 1,
/* LocalVar         */ 1,
/* Minus            */ 0,
/* Modulo           */ 0,
/* Multiply         */ 0,
/* NatAssign        */ 0,
/* NewLine          */ 2,
/* Not              */ 0,
/* NotEqual         */ 0,
/* NotGreater       */ 0,
/* NotLess          */ 0,
/* Or               */ 0,
/* Pop              */ 1,
/* PredefVar        */ 1,
/* Read		    */ 0,
/* Rem		    */ 0,
/* ResetTimer	    */ 2,
/* SDLCall	    */ 3,
/* SDLExport	    */ 2,
/* SDLImport	    */ 3,
/* SDLInit	    */ 4,
/* SDLInput	    */ 1+SIGSET_SIZE,
/* SDLModule	    */ 8,
/* SDLNext	    */ 1,
/* SDLOutput	    */ 3,
/* SDLRange	    */ 3,
/* SDLReturn	    */ 0,
/* SDLStop	    */ 0,
/* SDLTrans	    */ 6+(MAX_STATES/SWORD_SIZE),
/* SetScope	    */ 2,
/* SetTimer	    */ 2,
/* SimpleAssign	    */ 0,
/* SimpleValue	    */ 0,
/* Subtract         */ 0,
/* TestRange	    */ 1,
/* TestTimer	    */ 2,
/* Value            */ 1,
/* Variable         */ 2,
/* VarParam         */ 2, 
/* Write	    */ 1,
/* XOr		    */ 0,
/* Consistency check*/ 666
};

static char *op_name_tbl[(int)NUM_OPS+1]=
{
	"Add",
	"And",
	"ArgAssign",
	"Assign",
	"Channel",
	"Constant",
	"DefAddr",
	"DefArg",
	"Div",
	"Do",
	"EndClause",
	"EndTrans",
	"Equal",
	"FAdd",
	"FConst",
	"FDiv",
	"FEqu",
	"FGEq",
	"FGtr",
	"Field",
	"Fix",
	"FLEq",
	"FLes",
	"Float",
	"FMinus",
	"FMul",
	"FNeq",
	"FSub",
	"GlobalTime",
	"GlobalValue",
	"GlobalVar",
	"Goto",
	"Greater",
	"Index",
	"Less",
	"LocalValue",
	"LocalVar",
	"Minus",
	"Modulo",
	"Multiply",
	"NatAssign",
	"NewLine",
	"Not",
	"NotEqual",
	"NotGreater",
	"NotLess",
	"Or",
	"Pop",
	"PredefVar",
	"Read",
	"Rem",
	"ResetTimer",
	"SDLCall",
	"SDLExport",
	"SDLImport",
	"SDLInit",
	"SDLInput",
	"SDLModule",
	"SDLNext",
	"SDLOutput",
	"SDLRange",
	"SDLReturn",
	"SDLStop",
	"SDLTrans",
	"SetScope",
	"SetTimer",
	"SimpleAssign",
	"SimpleValue",
	"Subtract",
	"TestRange",
	"TestTimer",
	"Value",
	"Variable",
	"VarParam",
	"Write",
	"XOr",
	NULL
};

//----------------------------------------------------------
// Constructor 

codespace_t::codespace_t()
{
	top = 0;
	emitcode = 1;
	verbose = 0;
	// do some consistency checks
	assert(op_argcnt_tbl[NUM_OPS] == 666);
	assert(op_name_tbl[NUM_OPS] == NULL);
}

//----------------------------------------------------------
// Utilities

int codespace_t::GetNumArgs(s_code_op_t op)
{
	assert(op>=0 && op<NUM_OPS);
	return op_argcnt_tbl[op];
}

char *codespace_t::GetName(s_code_op_t op)
{
	if (op>=0 && op<NUM_OPS)
		return op_name_tbl[op];
	else return "???";
}

//----------------------------------------------------------
// Code generation

void codespace_t::_Emit(s_code_op_t op, ...)
{
	int n;
	va_list argptr;
	s_code_word_t arg;
	va_start(argptr, op);
	n = op_argcnt_tbl[(int)op];
	assert((top+n+1)<=MAX_CODE_SIZE);
	if (emitcode)
	{
		space[top] = (s_code_word_t)op;
		if (verbose)
			cout << endl << top << '\t' << op_name_tbl[op] << " ";
	}
	top++;
	while (n--)
	{
		if (emitcode)
		{
			space[top] = va_arg(argptr, s_code_word_t);
			if (verbose)
			{	
				cout << space[top];
				if (n) cout << ", ";
			}
		}
		top++;
	}
	va_end(argptr);
}

//--------------------------------------------------------------
// Linking

void codespace_t::NextInstruction()
{
	int i, cnt;
	op = (s_code_op_t)space[in];
	assert(op>=0 && op<NUM_OPS);
	cnt = op_argcnt_tbl[(int)op]+1;
	assert(cnt<MAX_OP_ARGS);
	for (i=0;i<cnt;i++)
		arg[i] = space[in++];
}

s_code_word_t codespace_t::Displ(int argNum)
{
	if (arg[argNum] == -1 || table[arg[argNum]] == -1)
		return (s_code_word_t)-1;
	else return (s_code_word_t)(table[arg[argNum]]-nextP);
}

s_code_word_t codespace_t::Value(int argNum)
{
	if (arg[argNum] == -1) return (s_code_word_t)-1;
	else return table[arg[argNum]];
}

void codespace_t::Variable(s_code_word_t level, s_code_word_t disp)
{
	NextInstruction();
	while (op == OP_FIELD)
	{
		disp += arg[1];
		NextInstruction();
	}
	if (level == 0)
	{
		if (op == OP_VALUE && arg[1] == 1)
		{
			Emit(OP_LOCALVALUE, disp);
			NextInstruction();
		}
		else Emit(OP_LOCALVAR, disp);
	}
	else if (level == 1)
	{
		if (op == OP_VALUE && arg[1] == 1)
		{
			Emit(OP_GLOBALVALUE, disp);
			NextInstruction();
		}
		else Emit(OP_GLOBALVAR, disp);
	}
	else Emit(OP_VARIABLE, level, disp);
}

void codespace_t::Process(s_code_word_t pass, s_code_word_t codetop)
{
	int sourceline = 0, sourcefile = 0;
	emitcode = (pass==1);
	top = in = 0;
	while (in<codetop)
	{
		NextInstruction();
	next: // OP_NEWLINE and OP_VARIABLE use this
		nextP = top + 1 + op_argcnt_tbl[op];
		switch (op)
		{
		// Zero-argument instructions are echoed...

		case OP_ADD:
		case OP_AND:
		case OP_DIV:
		case OP_ENDCLAUSE:
		case OP_ENDTRANS:
		case OP_EQUAL:
		case OP_FADD:
		case OP_FDIV:
		case OP_FEQU:
		case OP_FGTR:
		case OP_FIX:
		case OP_FLES:
		case OP_FLOAT:
		case OP_FMINUS:
		case OP_FMUL:
		case OP_FNEQ:
		case OP_FLEQ:
		case OP_FGEQ:
		case OP_FSUB:
		case OP_GLOBALTIME:
		case OP_GREATER:
		case OP_LESS:
		case OP_MINUS:
		case OP_MODULO:
		case OP_MULTIPLY:
		case OP_NATASSIGN:
		case OP_NOT:
		case OP_NOTEQUAL:
		case OP_NOTGREATER:
		case OP_NOTLESS:
		case OP_OR:
		case OP_READ:
		case OP_REM:
		case OP_SDLRETURN:
		case OP_SDLSTOP:
		case OP_SIMPLEASSIGN:
		case OP_SIMPLEVALUE:
		case OP_SUBTRACT:
		case OP_XOR:
			Emit(op);
			break;

		// Single argument ops that require no action:

		case OP_ARGASSIGN:
		case OP_CHANNEL:
		case OP_CONSTANT:
		case OP_FCONST:
		case OP_GLOBALVALUE:
		case OP_GLOBALVAR:
		case OP_LOCALVALUE:
		case OP_LOCALVAR:
		case OP_POP:
		case OP_PREDEFVAR:
		case OP_SDLNEXT:
		case OP_WRITE:
			Emit(op, arg[1]);
			break;

		// Two argument ops that require no action:

		case OP_RESETTIMER:
		case OP_SDLEXPORT:
		case OP_SETTIMER:
		case OP_SETSCOPE:
		case OP_TESTTIMER:
		case OP_VARPARAM:
			Emit(op, arg[1], arg[2]);
			break;

		// Three argument ops that require no action:

		case OP_INDEX:
		case OP_SDLIMPORT:
		case OP_SDLOUTPUT:
		case OP_SDLRANGE:
			Emit(op, arg[1], arg[2], arg[3]);
			break;

		// Other ops that require no action:

		case OP_SDLINPUT:
#ifdef W16
			Emit(op, arg[1], arg[2], arg[3], arg[4], arg[5]);
#else
			Emit(op, arg[1], arg[2], arg[3]);
#endif
			break;

		// The remaining ops all have link actions:

		case OP_ASSIGN:
			if (arg[1]==1)
				Emit(OP_SIMPLEASSIGN);
			else
				Emit(OP_ASSIGN, arg[1]);
			break;
		case OP_DEFADDR:
//cout << "Setting table[" << arg[1] << "] to be " << top << endl;
			table[arg[1]] = top;
			break;
		case OP_DEFARG:
//cout << "Setting table[" << arg[1] << "] to be " << arg[2] << endl;
			table[arg[1]] = arg[2];
			break;
		case OP_DO:
			Emit(op,Displ(1));
			break;
		case OP_FIELD:
			if (arg[1] != 0)
				Emit(OP_FIELD, arg[1]);
			break;
		case OP_GOTO:
			Emit(op,Displ(1));
			break;
		case OP_NEWLINE:
			do
			{
				sourcefile = arg[1];
				sourceline = arg[2];
				NextInstruction();
			}
			while (op==OP_NEWLINE);
			Emit(OP_NEWLINE, sourcefile, sourceline);
			goto next;
		case OP_SDLCALL:
			Emit(OP_SDLCALL, Displ(1), arg[2], arg[3]);
			break;
		case OP_SDLINIT:
			Emit(OP_SDLINIT, Displ(1), arg[2], arg[3], arg[4]);
			break;
		case OP_SDLMODULE:
			Emit(OP_SDLMODULE, arg[1], arg[2], arg[3], arg[4],
				Value(5), Displ(6), Displ(7), arg[8]);
			break;
		case OP_SDLTRANS:
#ifdef W16
			Emit(OP_SDLTRANS, arg[1], arg[2], arg[3], arg[4],
				arg[5], Displ(6), Displ(7), Displ(8),Displ(9),
				arg[10]);
#else
			Emit(OP_SDLTRANS, arg[1], arg[2], arg[3], Displ(4),
				Displ(5), Displ(6), Displ(7), arg[8]);
#endif
			break;
		case OP_TESTRANGE:
			Emit(op,Displ(1));
			break;
		case OP_VALUE:
			if (arg[1]==1)
				Emit(OP_SIMPLEVALUE);
			else
				Emit(OP_VALUE,arg[1]);
			break;
		case OP_VARIABLE:
			Variable(arg[1],arg[2]);
			goto next;
		default:
			assert(0); // should deal with each case above
		}
	}
}

int codespace_t::Link()
{
	if (verbose)
		cout << endl << "Linking..." << endl;
	int codetop = top;
	Process(0,codetop);
	Process(1,codetop);
//	Process(2,codetop);
	return top;
}

//--------------------------------------------------------------
// Save/load to/from disk

int codespace_t::Write(char *base, ofstream *os)
{
	int mustDel = 0, rtn = -1;
	if (os == NULL)
	{
		char targetname[MAX_FNAME_LEN];
		strcpy(targetname, base);
		strcat(targetname,".cod");
#ifdef __MSDOS__
		os = new ofstream(targetname, ios::binary);
#else
		os = new ofstream(targetname);
#endif
		assert(os);
		mustDel = 1;
		if (verbose)
			cout << endl << "Writing code to file " << targetname << endl;
	}
	if (os)
	{
		os->write((char *)&top, sizeof(top));
		os->write((char *)space, top*sizeof(s_code_word_t));
		rtn = 0;
	}
	if (mustDel) delete os;
	return rtn;
}

int codespace_t::Read(char *base, ifstream *is)
{
	int mustDel = 0, rtn = -1;
	if (is == NULL)
	{
		char targetname[MAX_FNAME_LEN];
		strcpy(targetname, base);
		strcat(targetname,".cod");
#ifdef __MSDOS__
		is = new ifstream(targetname, ios::binary);
#else
		is = new ifstream(targetname);
#endif
		assert(is);
		mustDel = 1;
		if (verbose)
			cout << endl << "Reading code from file " << targetname << endl;
	}
	if (is)
	{
		is->read((char *)&top, sizeof(top));
		is->read((char *)space, top*sizeof(s_code_word_t));
		rtn = 0;
	}
	if (mustDel) delete is;
	return rtn;
}

//-----------------------------------------------------------------
// Disassembler

#include <stdio.h>

char *codespace_t::DisassembleOp(int pos)
{
	static char buf[80];
	in = pos; NextInstruction();
	int cnt = op_argcnt_tbl[op];
	sprintf(buf,"%d\t%s", pos, op_name_tbl[op]);
	char *fmt = (sizeof(s_code_word_t)==sizeof(int)) ? " %X%c" : " %lX%c";
	for (int i = 0; cnt; )
	{
		if (arg[++i] == UNDEFINED)
			sprintf(buf+strlen(buf), " UNDEFINED%c", (--cnt) ? ',' : '\0');
		else
			sprintf(buf+strlen(buf), fmt, arg[i], (--cnt) ? ',' : '\0');
	}
	return buf;
}

void codespace_t::Disassemble(disFilterFn fn)
{
	in = 0;
	for (int pos = 0; pos<top; pos = in)
	{
		NextInstruction();
		if (fn==NULL || (*fn)(arg)==0)
			cout << DisassembleOp(pos) << endl;
	}
}

