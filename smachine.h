/*
 * SDL S-machine
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

#ifndef _SMACHINE_H

#define _SMACHINE_H

#ifndef constructor
#define constructor // useful for searching for constructors
#endif

#include <string.h>
#include <assert.h>
#include <stdlib.h>

// Things that can be traced

#define TRC_FIRE		1
#define TRC_ARRIVE	2
#define TRC_INPUT		4
#define TRC_OUTPUT	8
#define TRC_CLAUSE	16
#define TRC_LINE		32
#define TRC_PROCESS	64
#define TRC_LOSS		128
#define TRC_STATE		256
#define TRC_IMPLICIT	512
#define TRC_SCODE		1024
#define TRC_ALL		(TRC_SCODE-1) // doesn't turn disassembly on/off

// Predefined `variables' (duplicated in sdlc2.h)

#define PARENT	0
#define SELF		1
#define OFFSPRING	2
#define SENDER	3

// To execute an instruction the scheduler calls the ExecOp
// method of a process class. That method returns a code
// which identifies the `unit' that was executed. The unit
// types are given by exec_result_t:

typedef enum
{
	DONE_CLAUSE,
	DONE_TRANS,
	DONE_NEWLINE,
	DONE_CALL,
	DONE_RETURN,
	DONE_STOP,
	DONE_OP		// all other cases return this
} exec_result_t;

// The scheduler in turn can be instructed to run until 
// a given event. These can be:

typedef enum
{
	X_SCODE_OP,
	X_LINE,
	X_TRANS,
	X_ITER,
	X_TICK,
	X_TRACE,
	X_CHOICE
} exec_unit_t;

//----------------------------------------------------------------------
// Useful stack macros

#define IsElt(set, v) ( ((set)[(v)/(8*sizeof(s_code_word_t))] & \
				(1 << ((v) % (8*sizeof(s_code_word_t))))) != 0)


//----------------------------------------------------------------------
// Forward declaration of classes

class s_process_stack_t;
class s_transition_t;
class signal_t;
class s_process_t;
class scheduler_t;

//---------------------------------------------------------------------
// Channel Manager handles channel delays

class channel_mgr_t
{
private:
	s_code_word_t chans[MAX_CHANNELS];
	s_code_real_t delays[MAX_CHANNELS];
	int reliability[MAX_CHANNELS];
	int cnt;
	int Find(s_code_word_t ch);
public:
	channel_mgr_t()
	{
		cnt = 0;
	}
	s_code_real_t GetDelay(s_code_word_t ch);
	void Enter(s_code_word_t ch, s_code_real_t d=1.0, int r=100);
	void Edit(int idx, s_code_real_t d=1.0, int r=100);
	void Show();
	void ShowCode(int c)
	{
		cout << *(GetChanDefP(chans[c-1]));
	}
};

extern channel_mgr_t ChanMgr;

//----------------------------------------------------------------------
// Filters
//
// The possible trace outputs are:
//
// <Time> T <PId> <Trans>
// <Time> O <PId> <DestPId> <Sig> <Args?>
// <Time> A <PId> <Sig> <Args?>
// <Time> I <PId> <Sig> <Args?>
// <Time> C <PId> <Clause> [+|-]
// <Time> L <PId> <File> <Line>
// <Time> P <PId> + <PPId> 
// <Time> P <PId> -

typedef enum
{
	ACCEPT_FILTER, REJECT_FILTER
} filter_type_t;

typedef enum
{
	FIRE_FILTER,
	OUTPUT_FILTER,
	ARRIVE_FILTER,
	LOSS_FILTER,
	INPUT_FILTER,
	CLAUSE_FILTER,
	LINE_FILTER,
	PROCESS_FILTER,
	STATE_FILTER,
	IMPLICIT_FILTER,
	UNKNOWN_FILTER
} filter_class_t;

static char fclassCodes[] =
{
	'F', 'O', 'U', 'A', 'I', 'C', 'L', 'P', 'S', 'T', '?'
};

class filter_t
{
private:
	filter_type_t typ;
	int cnt;
	filter_class_t fclass;
	int PId;
	char flds[3][20];
public:
	int active;
	void parse(char *pat_in, filter_class_t &class_out, int &PId_out,
		char flds_out[][20]);
	filter_t(char *pat_in = NULL, filter_type_t type_in = REJECT_FILTER,
		int cnt_in = 0);
	~filter_t()
	{ }
	int isActive() { return active; }
	int Apply(char *input);
	void Print(char *buf);
	int operator==(const filter_t&) const;
};

//----------------------------------------------------------
// Trace manager

class trace_mgr_t
{
private:
	filter_t *accFilters[MAX_FILTERS];
	filter_t *rejFilters[MAX_FILTERS];
	unsigned flags;
public:
	trace_mgr_t();
	void Show(filter_type_t typ);
	void Delete(filter_type_t typ, char *ftxt);
	void Add(filter_type_t typ, char *ftxt, int cnt=0);
	void Set(unsigned flags_in)
	{
		flags = flags_in;
	}
	void LogFire(s_process_t *p);
	void LogImplicit(s_process_t *p, signal_t *s);
	void LogInput(s_process_t *p, signal_t *s);
	void LogOutput(s_process_t *src, s_process_t *dest, signal_t *s);
	void LogLine(int filenum, int linenum);
	void LogClause() {} // unimplemented as yet
	void LogDestroy(s_process_t *p);
	void LogCreate(s_process_t *p);
	void LogSCode(int sp, s_code_word_t tos, int p);
	void LogArrive(s_process_t *p, signal_t *s);
	void LogLoss(s_process_t *p, signal_t *s, int chan);
	void LogState(s_process_t *p, s_code_word_t fstate, s_code_word_t tstate,
					char *fnm, char *tnm);
	void Filter(char *msg);
};

extern trace_mgr_t TraceMgr;

//----------------------------------------------------------------------
// Process stack class (includes heap)

class s_process_stack_t
{
	friend class s_process_t;
private:
	s_code_word_t	*ptr;
	size_t		size;
	s_offset_t	s, firstb;
public:
	constructor s_process_stack_t()
	{
		ptr = new s_code_word_t[STACKSIZE];
		assert(ptr);
		size = ptr ? STACKSIZE : 0;
		s = 0;
	}
	~s_process_stack_t()
	{
		assert(ptr);
		if (ptr && size) delete [] ptr;
	}
	void		Drop()
	{
		assert(s>0);
		s--;
	}
	s_offset_t	Drop(size_t n)
	{
		assert(s>=n);
		s -= n;
		return s;
	}
	s_code_word_t	Pop()
	{
		assert(s>0);
		return ptr[s--];
	}
	void Pop(s_code_word_t &v)
	{
		assert(s>0);
		v = ptr[s--];
	}
	void Pop(s_code_word_t &v1, s_code_word_t &v2)
	{
		assert(s>0);
		v1 = ptr[s--];
		v2 = ptr[s--];
	}
	void Pop(s_code_word_t &v1, s_code_word_t &v2, s_code_word_t &v3)
	{
		assert(s>0);
		v1 = ptr[s--];
		v2 = ptr[s--];
		v3 = ptr[s--];
	}
	s_offset_t Push(const s_code_word_t v)
	{
		assert((s+1) < size);
		ptr[++s] = v;
		return s;
	}
	s_offset_t Push(s_code_word_t &v1, s_code_word_t &v2)
	{
		assert((s+2) < size);
		ptr[++s] = v1;
		ptr[++s] = v2;
		return s;
	}
	s_offset_t Push(s_code_word_t &v1, s_code_word_t &v2, s_code_word_t &v3)
	{
		assert((s+3) < size);
		ptr[++s] = v1;
		ptr[++s] = v2;
		ptr[++s] = v3;
		return s;
	}
	s_offset_t	Now()
	{
		return s;
	}
	void		Set(s_offset_t p)
	{
		s = p;
	}
	s_code_word_t	Rel(s_code_word_t off)
	{
		return ptr[s+off]; 
	}
	void		Rel(s_code_word_t off, s_code_word_t v)
	{
		ptr[s+off] = v; 
	}
	s_code_word_t	Abs(s_offset_t off)
	{
		return ptr[off]; 
	}
	void		Abs(s_offset_t off, s_code_word_t v)
	{
		ptr[off] = v; 
	}
};

class s_transition_t
{
private:
	s_code_word_t *argTbl;
	int		idx;
	long	fired;
	s_code_real_t first, last;
public:
	s_transition_t()
	{
#ifdef W16
		argTbl = new s_code_word_t[10];
#else
		argTbl = new s_code_word_t[8];
#endif
		assert(argTbl);
	}
	~s_transition_t()
	{
		delete [] argTbl;
	}
	void Init(int idx_in, int p_in);
	s_code_word_t *FromStates()	{ return &argTbl[0];}
#ifdef W16
	s_code_word_t Priority()	{ return argTbl[4];	}
	s_code_word_t Prov()		{ return argTbl[5];	}
	s_code_word_t When()		{ return argTbl[6];	}
	s_code_word_t Next()		{ return argTbl[7];	}
	s_code_word_t Start()		{ return argTbl[8];	}
	s_code_word_t ASTloc()		{ return argTbl[9];	}
#else
	s_code_word_t Priority()	{ return argTbl[2];	}
	s_code_word_t Prov()		{ return argTbl[3];	}
	s_code_word_t When()		{ return argTbl[4];	}
	s_code_word_t Next()		{ return argTbl[5];	}
	s_code_word_t Start()		{ return argTbl[6];	}
	s_code_word_t ASTloc()		{ return argTbl[7];	}
#endif
	void ShowInfo();
	void UpdateInfo(); // logs the firing of the transition
};

class signal_t
{
private:
	int idx;
	long sender;
	s_code_real_t timestamp;
	int arglen;
	s_code_word_t *args;
	signal_t	*next;
	char		canHandle, 	// flag used for doing implicit transitions
				hasArrived;
public:
	friend class s_process_t;
	friend class trace_mgr_t;
	signal_t(int idx_in, int arglen_in, s_code_word_t *argptr,
		int sender_in, s_code_real_t timestamp_in, signal_t *next_in = NULL);
	~signal_t();
	void Print(char *buf);
};

typedef struct
{
	s_code_word_t offset;
	s_code_word_t size;
	s_code_word_t *valptr;
} export_t;

// Common process/procedure stuff

class s_process_t
{
private:
	s_process_stack_t	*stack;		// process' stack
	s_code_word_t		b, p,		// registers
						startp,		// initial p; used to identify process type
						ASTloc,		// offset in local heap of AST node
						_class,  	// Q_PROCESS or Q_PROCEDURE
						vars,   	// stack space used for vars
						params, 	// stack space used for params
						transCnt; 	// number of transitions
	s_transition_t		*transTbl;	// transition table
	int					state;		// current state
	int					selected;	// next transition to fire
	int					selPri;		// priority of selected transition
	int					selPos;		// position in port queue of signal
	int					selP;		// code offset of selected trans
	long				PId;		// Process ID
	long				PPId;		// Parent process ID
	long				Offspring;	// Offspring process ID
	long				Sender;		// Signal sender process ID
	signal_t			*port;		// Input signal port
	char				*name;		// name of process/procedure
	s_process_t			*caller;	// Caller of this procedure
	s_process_t			*calls;		// Procedure called by this
	export_t			exports[MAX_EXPORTS];
	lh_list_t<name_entry_t> *stateTbl;
	int					numStates;
	int					scopeLvl;
	typeclass_t  		*scopeTypes;
	heap_ptr_t   		*scopeOffsets;
	void Export(s_code_word_t offset, s_code_word_t size, s_code_word_t *vptr);
	void Import(s_code_word_t AST, s_code_word_t pid, s_code_word_t offset,
				s_code_word_t size);
public:
	friend class scheduler_t;
	friend class trace_mgr_t;
	constructor s_process_t(s_code_word_t p_in, s_code_word_t *args_in,
			exec_result_t &xu, long PID_in, int level,
			s_process_t *parent = NULL,
			s_process_stack_t *stk_in = NULL, s_code_word_t s_in = -1,
			s_code_word_t b_in = -1, signal_t *port_in = NULL);
	~s_process_t();

	// Useful macros/functions for stack manipulation

#define TOS		stack->ptr[stack->Now()]
#define TOS1		stack->ptr[stack->Now() + 1]
#define DROP		stack->Drop()
#define POP(v)	v = stack->Pop()

	s_code_word_t	SP()				{ return stack->Now();	}
	s_code_word_t	*Addr(s_code_word_t off) { return &stack->ptr[off];	}
	s_code_word_t	Top()				{ return TOS;	}
	void		Top(s_code_word_t v)	{ TOS = v;		}

	s_code_word_t	vTOS();
	s_code_word_t	vTOS1();
	s_code_word_t	Pop()				{ return stack->Pop();	}
	void		Pop(s_code_word_t &v)	{ POP(v);				}
	void		Push(s_code_word_t v)	{ stack->Push(v);		}
	void		Set(s_code_word_t v)	{ stack->Set(v);		}
	void		Drop()					{ stack->Drop();		}
	s_offset_t	Drop(size_t n)			{ return stack->Drop(n);}

	s_code_word_t	Rel(s_code_word_t off)				{ return stack->Rel(off);}
	void		Rel(s_code_word_t off, s_code_word_t v){ stack->Rel(off, v); 	}

	s_code_word_t	Abs(s_offset_t off)				{ return stack->Abs(off);}
	void		Abs(s_offset_t off, s_code_word_t v)	{ stack->Abs(off, v); 	}

	s_code_word_t Chain(s_code_word_t level); // follow activation chain

	// Interpret the next instruction

	exec_result_t ExecOp();
	void Enqueue(signal_t *sig);
	signal_t *Enqueue(s_code_word_t sig, s_code_word_t *args,
		s_code_word_t argLen, int sender, s_code_real_t timestamp_in);
	int GetActive(int mustDel, s_code_word_t sig,
		s_code_word_t *args, s_code_word_t argLen);
	void TestSignal(s_code_word_t sig, s_code_word_t *save);
	int EvalClauses(s_code_real_t &minDelay);
	void ShowInfo();
	void UpdateInfo(); // logs the firing of selected transition
	char *Identity();
	void SetScope();
	void ShowTrans(int tr);
	signal_t *Port() { return port; }
	void Port(signal_t *pr) { port = pr; }
	s_code_word_t Link() { return b; }
	void Link(s_code_word_t b_in) { b = b_in; }
	s_process_stack_t *Stack() { return stack; }
};

//-----------------------------------------------------------------
// Scheduler
//
// The scheduler is constructed with a GetCommand argument.
// This is a function which it calls when it is passive,
// or when it is interrupted. It then executes the command.
// If the function is not specified, the scheduler reads
// from cin.

typedef int (*read_cmd_fn_t)(char *buf);
typedef void (*write_cmd_fn_t)(char *buf);

class scheduler_t
{
private:
	read_cmd_fn_t	reader;
	write_cmd_fn_t	writer;
	unsigned short	shows;
	s_process_t		*processes[MAX_INSTANCES];
	exec_unit_t		execType;
	int				execCnt;

	static int MyReader(char *buf);
	static void MyWriter(char *buf);
	int				busyEvaluating;
	long 			PId;
	s_code_real_t	globalTime;
	int				active;		// currently executing process (tbl index)
public:
	constructor scheduler_t(read_cmd_fn_t reader_in = (read_cmd_fn_t)NULL,
				write_cmd_fn_t writer_in = (write_cmd_fn_t)NULL);
	~scheduler_t();

	int HandleCommand();
	int CheckExecCount(int cnt);
	void Terminate(s_process_t *p);
	void Terminate(int pi);
	void Execute();
	s_process_t *NewProcess(s_code_word_t p, s_code_word_t *args, int level,
			int maxinst);
	s_process_t *NewProcedure(s_process_t *caller,
			s_code_word_t p, int level);
	s_code_real_t GetTime()
	{
		return globalTime;
	}
	int Evaluating()
	{
		return busyEvaluating;
	}
	int findProcess(s_code_word_t destPId, s_code_word_t pType,
			s_process_t* &destP);
	int EvalClauses(s_code_real_t &minDelay);
	void Write(char *msg);
	void Trace(char *msg);
	s_code_word_t *FindExported(s_code_word_t AST, s_code_word_t pid,
			s_code_word_t offset, s_code_word_t size);
};

extern scheduler_t *Scheduler;

#endif

