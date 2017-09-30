#ifndef _SDLRTE_H
#define _SDLRTE_H

#include <stdio.h>
#include <mem.h>
#include <assert.h>

#define MAX_PROCESSES	32

// SDL predefined types

typedef int			integer_typ;
typedef unsigned int	natural_typ;
typedef unsigned int	pid_typ;
typedef float			real_typ;
typedef float			duration_typ;
typedef float			time_typ;

// result of a schedule

typedef enum
{
	DONENOTHING, DONETRANS, DONESTOP, DONERETURN
} exec_result_t;

//--------------------------------------------------------------------
// Boolean type

typedef enum
{
	FALSE, TRUE
} bool_t;

//--------------------------------------------------------------------
// System timer access

float Now();
void Now(float t);

//--------------------------------------------------------------------
// Channel table

typedef struct
{
	int	reliability;
	float	deelay;
} channel_info_t;

extern channel_info_t channelTbl[];

//--------------------------------------------------------------------
// Signals

class signal_t
{
public:
	int		isTimer;
	int		id;
	int		sender;
	float		timestamp;
	signal_t	*next;

	signal_t()
	{
		isTimer = 0;
	}
	signal_t *Next()
	{
		return next;
	}
	void Next(signal_t *n)
	{
		next = n;
	}
	float Time()
	{
		return timestamp;
	}
	void Stamp(float tm)
	{
		timestamp = tm;
	}
	void Sender(int sid)
	{
		sender = sid;
	}
};

//-------------------------------------------------------------------
// Transition table entry

struct transition_t
{
	int	priority;
	int	state;
	int	signal;
	int	guard;
	int	dequeue;
	int	action;
	int	save;
	struct transition_t *next;
};

//-------------------------------------------------------------------
// Common ancestor of process/procedure

class SDL_procedure_t; // forward decl

class SDL_FSM_t
{
	transition_t	*transTbl;	// transition table
	int				tcnt;		// number of transitions in table
	SDL_procedure_t *called;	// pointer to called procedure
	int				__resume_idx; // used for resume after procedure call

	friend class SDL_procedure_t;
	friend class SDL_process_t;

public:
	long	*firecnt;		// firing statistics
	int		state;			// current state
	SDL_FSM_t(int tcnt_in);
	virtual ~SDL_FSM_t();
	void MakeEntry(int pri, int state, int sig, int prov, int deq,
			int act, int sav);
	virtual char* StateName(int s);
	virtual int Provided(int p);
	virtual void Input(int d, int sigid);
	virtual exec_result_t Action(int a) = 0;
	exec_result_t Call(SDL_procedure_t *p, int reslbl);
};

//-------------------------------------------------------------------
// Generic process

class SDL_process_t : public SDL_FSM_t
{
	int		PId; // process (instance) id
	int		pPId;
	int		child;
	int		sender;
	int		id; // class id

	friend class SDL_procedure_t;

public:
	signal_t	*port;
	float	lasteventtime;
	SDL_process_t(int tcnt_in, int id_in, int pPId_in = 0);
	virtual ~SDL_process_t();
	void Enqueue(signal_t *s);
	signal_t *Dequeue(int sigid);
	void SetTimer(signal_t *s, float tm);
	void ResetTimer(int idx);
	int TestTimer(int idx);
	exec_result_t Schedule(float &mindelay);
//	virtual char* StateName(int s) = 0;
//	virtual int Provided(int p);
	virtual exec_result_t Action(int a) = 0;
	virtual int CompareTimer(int idx, signal_t *s)
	{
		return 0;
	}
	virtual signal_t *GetHead(int saveidx, int wanted)
	{
		(void)saveidx;
		return (port && port->id == wanted) ? port : NULL;
	}
	int Sender()
	{
		return sender;
	}
	int Self()
	{
		return PId;
	}
	int Parent()
	{
		return pPId;
	}
	int Offspring()
	{
		return child;
	}
	void Offspring(int child_in)
	{
		child = child_in;
	}
	int Type()
	{
		return id;
	}
};

//-------------------------------------------------------------------
// Generic procedure

class SDL_procedure_t : public SDL_FSM_t
{
public:
	SDL_procedure_t *caller;
	SDL_process_t *owner;
	SDL_procedure_t(int tcnt_in, SDL_process_t *owner_in, SDL_procedure_t *caller_in = NULL);
	virtual ~SDL_procedure_t();

	void Enqueue(signal_t *s)
		{ owner->Enqueue(s);			}
	signal_t *Dequeue(int sigid)
		{ return owner->Dequeue(sigid);		}
	void SetTimer(signal_t *s, float tm)
		{ owner->SetTimer(s, tm);	}
	void ResetTimer(int idx);
	int TestTimer(int idx);
	int Sender()
		{ return owner->Sender();	}
	int Self()
		{ return owner->Self();		}
	int Parent()
		{ return owner->Parent();	}
	int Offspring()
		{ return owner->Offspring();}
	void Offspring(int child)
		{ owner->Offspring(child);	}

	exec_result_t Schedule(float &mindelay);
//	virtual char* StateName(int s) = 0;
//	virtual int Provided(int p);
	virtual int CompareTimer(int idx, signal_t *s)
	{
		return 0;
	}
	virtual exec_result_t Action(int a) = 0;
	virtual signal_t *GetHead(int saveidx, int wanted)
	{
		(void)saveidx;
		return (owner->port && owner->port->id == wanted) ?
			owner->port : NULL;
	}
};

//--------------------------------------------------------------
// System

typedef struct
{
	int	channel;	// channel ID
	int	src;		// src block class ID
	int	dest;		// dest block class ID, or -1 for ENV
	unsigned long	send;	// sigs from src->dest
	unsigned long	recv;	// sigs from dest->src
} channel_entry_t;

extern int nchannels;
extern channel_entry_t Channels[];

typedef struct
{
	int	route;		// route ID
	int	channel;	// channel to which connected, if any
	int	src;		// source process class ID
	int	dest;		// dest process class ID or -1 for ENV
	unsigned long	send;	// signals from src->dest
	unsigned long	recv;	// signals from dest->src
} route_entry_t;

extern route_entry_t Routes[];
extern int nroutes;

extern char *process_names[];
extern char *signal_names[];

extern SDL_process_t	*Process[];

extern int FindDest(int &dest_PId, int process_type);
extern void Output(signal_t *s, int src_PId, int dest_PId, int chan_num);
extern int AddProcess(SDL_process_t *p);
extern void CreateInitial();
extern void GetChannelInfo(int ch, signal_t *s, float &dlay, int &rel);

//-----------------------------------------------------------
// Event logging

extern long tcounts[];

void LogState(int PId, char *from, char *to);
void LogInput(int PId, signal_t *sig);
void LogSet(int PId, int sig, float tm);
void LogReset(int PId, int sig, float tm);
void LogOutput(int sPId, int dPId, int channel, int sig);
void LogFire(int PId, int actnum);
void LogCreate(int PId);

//---------------------------------------------------------
// Process limits

extern int Instances[];
extern int MaxInstances[];

#endif

