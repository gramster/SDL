#ifndef _SDLCODE_H
#define _SDLCODE_H

#include "sdl.h"
#include <iostream.h>
#include <fstream.h>
#include <stdarg.h>

#define MAX_OP_ARGS	16

//---------------------------------------------------------------------
// The code store class
//---------------------------------------------------------------------

typedef enum
{
	OP_ADD,			OP_AND,			OP_ARGASSIGN,	OP_ASSIGN,		OP_CHANNEL,
	OP_CONSTANT,	OP_DEFADDR,
	OP_DEFARG,		OP_DIV,			OP_DO,			OP_ENDCLAUSE,	OP_ENDTRANS,
	OP_EQUAL,		OP_FADD,		OP_FCONST,		OP_FDIV,		OP_FEQU,
	OP_FGEQ,		OP_FGTR,		OP_FIELD,		OP_FIX,
	OP_FLEQ,		OP_FLES,
	OP_FLOAT,		OP_FMINUS,		OP_FMUL,		OP_FNEQ,		OP_FSUB,
	OP_GLOBALTIME,	OP_GLOBALVALUE, OP_GLOBALVAR,
	OP_GOTO,		OP_GREATER,		OP_INDEX,		OP_LESS,		OP_LOCALVALUE,
	OP_LOCALVAR, 	OP_MINUS, 		OP_MODULO,		OP_MULTIPLY,
	OP_NATASSIGN,	OP_NEWLINE,
	OP_NOT,			OP_NOTEQUAL, 	OP_NOTGREATER,	OP_NOTLESS,		OP_OR,
	OP_POP,			OP_PREDEFVAR,
	OP_READ,		OP_REM,			OP_RESETTIMER,	OP_SDLCALL,
	OP_SDLEXPORT,
	OP_SDLIMPORT,	OP_SDLINIT,		OP_SDLINPUT,	OP_SDLMODULE,	OP_SDLNEXT,
	OP_SDLOUTPUT,	OP_SDLRANGE,	OP_SDLRETURN,	OP_SDLSTOP,		OP_SDLTRANS,
	OP_SETSCOPE,	OP_SETTIMER,	OP_SIMPLEASSIGN,OP_SIMPLEVALUE,	OP_SUBTRACT,
	OP_TESTRANGE, 	OP_TESTTIMER,	OP_VALUE,		OP_VARIABLE,	OP_VARPARAM,
	OP_WRITE,		OP_XOR,
	NUM_OPS
} s_code_op_t;

typedef int (*disFilterFn)(s_code_word_t *);

class codespace_t
{
private:
	s_code_word_t	space[MAX_CODE_SIZE];
	int				top, in, nextP;
	s_code_word_t 	arg[MAX_OP_ARGS];
	s_code_op_t		op;
	s_code_word_t 	table[1000];	/* Allocate dynamically of correct size! */
	short         	emitcode;
	void			NextInstruction();
	s_code_word_t	Displ(int argNum);
	s_code_word_t	Value(int argNum);
	void 			Variable(s_code_word_t level, s_code_word_t disp);
	void			Process(s_code_word_t pass, s_code_word_t top);
	void _Emit(s_code_op_t, ...);
public:
	int verbose;
	codespace_t();
	// These next methods are to ensure that the varargs are
	// of the right size
	void Emit(s_code_op_t op)
	{ _Emit(op); }	
	void Emit(s_code_op_t op, s_code_word_t a1)
	{ _Emit(op, a1); }	
	void Emit(s_code_op_t op, s_code_word_t a1, s_code_word_t a2)
	{ _Emit(op, a1, a2); }	
	void Emit(s_code_op_t op, s_code_word_t a1, s_code_word_t a2,
		s_code_word_t a3)
	{ _Emit(op, a1, a2, a3); }	
	void Emit(s_code_op_t op, s_code_word_t a1, s_code_word_t a2,
		s_code_word_t a3, s_code_word_t a4)
	{ _Emit(op, a1, a2, a3, a4); }	
	void Emit(s_code_op_t op, s_code_word_t a1, s_code_word_t a2,
		s_code_word_t a3, s_code_word_t a4, s_code_word_t a5)
	{ _Emit(op, a1, a2, a3, a4, a5); }	
	void Emit(s_code_op_t op, s_code_word_t a1, s_code_word_t a2,
		s_code_word_t a3, s_code_word_t a4, s_code_word_t a5,
		s_code_word_t a6)
	{ _Emit(op, a1, a2, a3, a4, a5, a6); }	
	void Emit(s_code_op_t op, s_code_word_t a1, s_code_word_t a2,
		s_code_word_t a3, s_code_word_t a4, s_code_word_t a5,
		s_code_word_t a6, s_code_word_t a7)
	{ _Emit(op, a1, a2, a3, a4, a5, a6, a7); }	
	void Emit(s_code_op_t op, s_code_word_t a1, s_code_word_t a2,
		s_code_word_t a3, s_code_word_t a4, s_code_word_t a5,
		s_code_word_t a6, s_code_word_t a7, s_code_word_t a8)
	{ _Emit(op, a1, a2, a3, a4, a5, a6, a7, a8); }	
	void Emit(s_code_op_t op, s_code_word_t a1, s_code_word_t a2,
		s_code_word_t a3, s_code_word_t a4, s_code_word_t a5,
		s_code_word_t a6, s_code_word_t a7, s_code_word_t a8,
		s_code_word_t a9)
	{ _Emit(op, a1, a2, a3, a4, a5, a6, a7, a8, a9); }	
	void Emit(s_code_op_t op, s_code_word_t a1, s_code_word_t a2,
		s_code_word_t a3, s_code_word_t a4, s_code_word_t a5,
		s_code_word_t a6, s_code_word_t a7, s_code_word_t a8,
		s_code_word_t a9, s_code_word_t a10)
	{ _Emit(op, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10); }	
	int Link();
	int Write(char *base = "sdl", ofstream *os = NULL);
	int Read(char *base = "sdl", ifstream *is = NULL);
	char *DisassembleOp(int pos);
	void Disassemble(disFilterFn fn = NULL);
	s_code_word_t Get(int addr)
	{
		return space[addr];
	}
	s_code_word_t *GetAddr(int addr)
	{
		return &space[addr];
	}
	int GetNumArgs(s_code_op_t op);
	char *GetName(s_code_op_t op);
};

extern codespace_t *Code;

#endif
