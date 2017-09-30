#include <stdlib.h>
#include "sdlrte.h"

// trace classes

#define TRACEFIRE	1
#define TRACEOUTPUT	2
#define TRACEINPUT	4
#define TRACESTATE	8
#define TRACESET	16
#define TRACERESET	32
#define TRACECREATE	64
#define TRACEALL	0xFFFF

int tron;

//--------------------------------------------------------------------
// System timer access

float now = 0.;
float timelimit = 0.;

float Now()
{
	return now;
}

void Now(float t)
{
	now = t;
}

//--------------------------------------------------------------------
// Signals

/* now done in each individual subclass after a memset

signal_t::signal_t(int id_in)
{
	isTimer = 0;
	timestamp = Now();
	next = NULL;
	id = id_in;
}
*/

//-------------------------------------------------------------------
// Transition table entry routine

void MakeTransEntry(transition_t* &transTbl, int pri, int state, int sig,
	int prov, int deq, int act, int sav)
{
	struct transition_t *t = new struct transition_t;
	t->priority = pri;
	t->state = state;
	t->signal = sig;
	t->guard = prov;
	t->dequeue = deq;
	t->action = act;
	t->save = sav;
	t->next = NULL;
	if (transTbl == NULL)
		transTbl = t;
	else
	{
		struct transition_t *tp = transTbl, *last = NULL;
		while (tp && tp->priority <= pri)
		{
			last = tp;
			tp = tp->next;
		}
		if (last)
		{
			t->next = last->next;
			last->next = t;
		}
		else
		{
			t->next = transTbl->next;
			transTbl = t;
		}
	}
}

//-------------------------------------------------------------------
// Common ancestor of process/procedure

SDL_FSM_t::SDL_FSM_t(int tcnt_in)
{
	transTbl = NULL;
	state = -1;
	tcnt = tcnt_in;
	firecnt = new long[tcnt+1]; // one extra for start trans
	for (int t = 0; t <= tcnt; t++) firecnt[t] = 0l;
	called = NULL;
}

SDL_FSM_t::~SDL_FSM_t()
{
	if (tron)
	{
		if (firecnt[0])
			printf("Implicit transition executed %-6ld times\n",
				firecnt[0]);
		for (int t = 1; t <= tcnt; t++)
			if (firecnt[t])
				printf("Transition %-2d executed %-6ld times\n",
					t, firecnt[t]);
	}
	while (transTbl)
	{
		struct transition_t *tp = transTbl;
		transTbl = transTbl->next;
		delete tp;
	}
	delete [] firecnt;
}

void SDL_FSM_t::MakeEntry(int pri, int state, int sig, int prov,
	int deq, int act, int sav)
{
	MakeTransEntry(transTbl, pri, state, sig, prov, deq, act, sav);
}

char* SDL_FSM_t::StateName(int s)
{
	(void)s;
	return "Unknown";
}

int SDL_FSM_t::Provided(int p)
{
	(void)p;
	assert(0);
	return 1;
}

void SDL_FSM_t::Input(int d, int sigid)
{
	(void)d;
	(void)sigid;
	assert(0);
}

exec_result_t SDL_FSM_t::Call(SDL_procedure_t *p, int reslbl)
{
	called = p;
	__resume_idx = reslbl;
	exec_result_t rtn = p->Action(0);
	if (rtn == DONERETURN)
	{
		called = NULL;
		delete p;
		return Action(-1);
	}
	else return rtn;
}

//-------------------------------------------------------------------
// Generic procedure

SDL_procedure_t::SDL_procedure_t(int tcnt_in, SDL_process_t *owner_in,
	SDL_procedure_t *caller_in)
	: SDL_FSM_t(tcnt_in)
{
	owner = owner_in;
	caller = caller_in;
}

SDL_procedure_t::~SDL_procedure_t()
{
	printf("Procedure exits at time %f\n", Self(), Now());
}

exec_result_t SDL_procedure_t::Schedule(float &mindelay)
{
	if (called)
	{
		exec_result_t xr = called->Schedule(mindelay);
		if (xr == DONERETURN)
		{
			delete called;
			called = NULL;
			return Action(-1); // resume caller
		}
		return xr;
	}
	int cando = 0;
retry:
	for (struct transition_t *tp = transTbl; tp; tp=tp->next)
	{
		// check state
		if (tp->state>=0 && state != tp->state)
			continue;
		// check input
		signal_t *hd = GetHead(tp->save, tp->signal);
		if (tp->signal>=0)
			if (hd==NULL)
				continue;
		// check provided
		if (tp->guard>=0 && !Provided(tp->guard))
			continue;
		if (tp->signal>=0)
		{
			if (hd->Time() > Now())
			{
				float deelay = hd->Time() - Now();
				if (deelay < mindelay)
					mindelay = deelay;
				cando = 1;
				continue;
			}
			Input(tp->dequeue, tp->signal);
		}
		if (tron)
		{
			LogFire(Self(), tp->action);
			firecnt[tp->action]++;
		}
		return Action(tp->action);
	}
	if (!cando && owner->port)
	{
		// implicit transition
		LogFire(Self(), 0);
		signal_t *s = owner->port;
		owner->port = s->Next();
		delete s;
		if (owner->port) goto retry;
	}
	return DONENOTHING;
}

void SDL_procedure_t::ResetTimer(int idx)
{
	signal_t *s = owner->port, *last = NULL;
	while (s)
	{
		if (CompareTimer(idx, s))
		{
			if (last) last->Next(s->Next());
			else owner->port = s->Next();
			LogReset(Self(), s->id, s->Time());
			delete s;
			if (last) s = last->Next();
			else s = owner->port;
		}
		else
		{
			last = s;
			s = s->Next();
		}
	}
}

int SDL_procedure_t::TestTimer(int idx)
{
	signal_t *s = owner->port;
	while (s)
	{
		if (CompareTimer(idx, s))
			return 1;
		s = s->Next();
	}
	return 0;
}

//-------------------------------------------------------------------
// Generic process

int _PId = 1;

SDL_process_t::SDL_process_t(int tcnt_in, int id_in, int pPId_in)
	: SDL_FSM_t(tcnt_in)
{
	port = NULL;
	PId = _PId++;
	pPId = pPId_in;
	id = id_in;
	lasteventtime = 0.;
}

void SDL_process_t::Enqueue(signal_t *s)
{
	if (port==NULL)
	{
		port = s;
		s->Next(NULL);
	}
	else
	{
		signal_t *p = port, *last = NULL;
		while (p && p->Time() <= s->Time())
		{
			last = p;
			p = p->Next();
		}
		if (last==NULL)
		{
			s->Next(port);
			port = s;
		}
		else
		{
			s->Next(p);
			last->Next(s);
		}
	}
}

SDL_process_t::~SDL_process_t()
{
//	printf("Time %-5f Process %d terminated (state %s, last event time %f)\n",
//		Now(), Self(), StateName(state), lasteventtime);
	while (port)
	{
		signal_t *p = port;
		port = port->Next();
		printf("Destroying signal %s (timestamp %f)\n",
			signal_names[p->id], p->Time());
		delete p; // must use virtual destructors!!
	}
}

signal_t* SDL_process_t::Dequeue(int sigid)
{
	signal_t *p = port, *l = NULL;
	while (p)
	{
		if (p->id == sigid) break;
		l = p;
		p = p->Next();
	}
	assert(p && p->Time() <= Now());
	if (l)
		l->Next(p->Next());
	else port = p->Next();
	sender = p->sender;
	return p;
}

void SDL_process_t::ResetTimer(int idx)
{
	signal_t *s = port, *last = NULL;
	while (s)
	{
		if (CompareTimer(idx, s))
		{
			if (last) last->Next(s->Next());
			else port = port->Next();
			LogReset(Self(), s->id, s->Time());
			delete s;
			if (last) s = last->Next();
			else s = port;
		}
		else
		{
			last = s;
			s = s->Next();
		}
	}
}

void SDL_process_t::SetTimer(signal_t *s, float tm)
{
	s->Sender(Self());
	LogSet(Self(), s->id, tm);
	s->Stamp(tm);
	Enqueue(s);
}

exec_result_t SDL_process_t::Schedule(float &mindelay)
{
	// assumes transitions sorted by priority
	if (called)
	{
		exec_result_t xr = called->Schedule(mindelay);
		if (xr == DONERETURN)
		{
			delete called;
			called = NULL;
			return Action(-1); // resume caller
		}
		if (xr != DONENOTHING)
			lasteventtime = Now();
		return xr;
	}
	int cando = 0;
retry:
	for (struct transition_t *tp = transTbl; tp; tp=tp->next)
	{
		// check state
		if (tp->state>=0 && state != tp->state)
			continue;
		// check input
		signal_t *hd = GetHead(tp->save, tp->signal);
		if (tp->signal>=0)
			if (hd==NULL)
				continue;
		// check provided
		if (tp->guard>=0 && !Provided(tp->guard))
			continue;
		if (tp->signal>=0)
		{
			if (hd->Time() > Now())
			{
				float deelay = hd->Time() - Now();
				if (deelay < mindelay)
					mindelay = deelay;
				cando = 1;
				continue;
			}
			Input(tp->dequeue, tp->signal);
		}
		if (tron)
		{
			LogFire(Self(), tp->action);
			firecnt[tp->action]++;
		}
		lasteventtime = Now();
		return Action(tp->action);
	}
	if (!cando && port)
	{
		// implicit transition
		LogFire(Self(), 0);
		signal_t *s = port;
		port = port->Next();
		delete s;
		if (port) goto retry;
	}
	return DONENOTHING;
}

int SDL_process_t::TestTimer(int idx)
{
	signal_t *s = port;
	while (s)
	{
		if (CompareTimer(idx, s))
			return 1;
		s = s->Next();
	}
	return 0;
}

//----------------------------------------------------------------

SDL_process_t	*Process[MAX_PROCESSES];

void Schedule()
{
	int change = 1;
	Now(0.);
	while (change)
	{
		float mindelay = 1.e10;
		change = 0;
		for (int p = 0; p < MAX_PROCESSES; p++)
		{
			if (Process[p] == NULL) continue;
			exec_result_t xr = Process[p]->Schedule(mindelay);
			if (xr == DONESTOP)
			{
				delete Process[p];
				Process[p] = NULL;
			}
			else if (xr != DONENOTHING)
				change = 1;
		}
		if (!change && mindelay < 1.e10)
		{
			Now(Now()+mindelay);
			if (timelimit > 0. && Now() >= timelimit)
				break;
			change = 1;
		}
	}
}

int FindDest(int &dest_PId, int process_type)
{
	if (dest_PId!=0)
	{
		for (int p = 0; p < MAX_PROCESSES; p++)
		{
			if (Process[p] && Process[p]->Self() == dest_PId)
			{
				assert(Process[p]->Type() == process_type);
				return 1;
			}
		}
	}
	int cnt = 0;
	for (int p = 0; p < MAX_PROCESSES; p++)
	{
		if (Process[p] && Process[p]->Type() == process_type)
		{
			cnt++;
			dest_PId = Process[p]->Self();
		}
	}
	assert(cnt<=1);
	return cnt;
}

void Output(signal_t *s, int src_PId, int dest_PId, int chan_num)
{
	LogOutput(src_PId, dest_PId, chan_num, s->id);
	float d; int r;
	GetChannelInfo(chan_num, s, d, r);
	s->Sender(src_PId);
	s->Stamp(Now() + d);

	for (int p = 0; p < MAX_PROCESSES; p++)
		if (Process[p] && Process[p]->Self() == dest_PId)
		{
			Process[p]->Enqueue(s);
			return;
		}
	assert(0);
}

int AddProcess(SDL_process_t *p)
{
	// add to process table
	if (Instances[p->Type()] >= MaxInstances[p->Type()])
	{
		delete p;
		printf("\tdue to too many instances of this type\n");
		return 0;
	}
	for (int i = 0; i < MAX_PROCESSES; i++)
	{
		if (Process[i]==NULL)
		{
			Process[i] = p;
			Instances[p->Type()]++;
			// do initial transition
			if (p->Action(0) == DONESTOP)
			{
				Instances[p->Type()]--;
				Process[i] = NULL;
				delete p;
			}
			return 1;
		}
	}
	fprintf(stderr, "Process table overflow!\n");
	delete p;
	return 0;
}

//-------------------------------------------------------------------
// Event logging

char *GetProcName(int PId)
{
	for (int p = 0; p < MAX_PROCESSES; p++)
		if (Process[p] && Process[p]->Self()==PId)
			return process_names[Process[p]->Type()];
	assert(0);
	return NULL;
}

void LogState(int PId, char *from, char *to)
{
	if (tron & TRACESTATE)
		printf("Time %-6f %s[%d] changes state from %s to %s\n",
			Now(), GetProcName(PId), PId, from, to);
}

void LogInput(int PId, signal_t *sig)
{
	if (tron & TRACEINPUT)
		printf("Time %-6f %s[%d] inputs signal %s (timestamp %f)\n",
			Now(), GetProcName(PId), PId,
			signal_names[sig->id],
			sig->Time());
}

void LogOutput(int sPId, int dPId, int channel, int sig)
{
	if (tron & TRACEOUTPUT)
		printf("Time %-6f %s[%d] outputs signal %s to %s[%d] via chan %d\n",
			Now(), GetProcName(sPId), sPId, signal_names[sig],
			GetProcName(dPId), dPId, channel);
}

void LogFire(int PId, int actnum)
{
	if (tron & TRACEFIRE)
		printf("Time %-6f %s[%d] executes transition %d\n",
			Now(), GetProcName(PId), PId, actnum);
}

void LogCreate(int PId)
{
	if (tron & TRACECREATE)
		printf("Time %-6f Created %s[%d]\n",
			Now(), GetProcName(PId), PId);
}

void LogSet(int PId, int sig, float tm)
{
	if (tron & TRACESET)
		printf("Time %-6f %s[%d] sets timer %s for time %f\n",
			Now(), GetProcName(PId), PId, signal_names[sig], tm);
}

void LogReset(int PId, int sig, float tm)
{
	if (tron & TRACERESET)
		printf("Time %-6f %s[%d] resets timer %s scheduled for time %f\n",
			Now(), GetProcName(PId), PId, signal_names[sig], tm);
}

//-------------------------------------------------------------------

static void useage(char *cmd)
{
	fprintf(stderr, "Useage: %s [flags] [-T <timelimit>]\nwhere flags are:", cmd);
	fprintf(stderr, "\t-Q disables all trace logging\n");
	fprintf(stderr, "\t-F disables logging of firings\n");
	fprintf(stderr, "\t+F enables logging of firings\n");
	fprintf(stderr, "\t-O disables logging of outputs\n");
	fprintf(stderr, "\t+O enables logging of outputs\n");
	fprintf(stderr, "\t-I disables logging of inputs\n");
	fprintf(stderr, "\t+I enables logging of inputs\n");
	fprintf(stderr, "\t-C disables logging of state changes\n");
	fprintf(stderr, "\t+C enables logging of state changes\n");
	fprintf(stderr, "\t-S disables logging of timer sets\n");
	fprintf(stderr, "\t+S enables logging of timer sets\n");
	fprintf(stderr, "\t-R disables logging of timer resets\n");
	fprintf(stderr, "\t+R enables logging of timer resets\n");
	fprintf(stderr, "\t-T exits after the specified time has elapsed\n");
	exit(-1);
}

int main(int argc, char *argv[])
{
	tron = TRACEALL;
	for (int i=1;i<argc;i++)
	{
		if (argv[i][0]=='-' || argv[i][0]=='+')
		{
			int set = (argv[i][0]=='+');
			switch(argv[i][1])
			{
			case 'Q':
				tron = 0;
				break;
			case 'F':
				if (set) tron |= TRACEFIRE;
				else tron &= ~TRACEFIRE;
				break;
			case 'O':
				if (set) tron |= TRACEOUTPUT;
				else tron &= ~TRACEOUTPUT;
				break;
			case 'I':
				if (set) tron |= TRACEINPUT;
				else tron &= ~TRACEINPUT;
				break;
			case 'C':
				if (set) tron |= TRACESTATE;
				else tron &= ~TRACESTATE;
				break;
			case 'S':
				if (set) tron |= TRACESET;
				else tron &= ~TRACESET;
				break;
			case 'R':
				if (set) tron |= TRACERESET;
				else tron &= ~TRACERESET;
				break;
			case 'T':
				if (argv[i][2])
					timelimit = atof(argv[i]+2);
				else
				{
					i++;
					if (i==argc) useage(argv[0]);
					timelimit = atof(argv[i]);
				}
				break;
			default:
				useage(argv[0]);
			}
		}
		else useage(argv[0]);
	}
	CreateInitial();
	Schedule();
	if (timelimit > Now())
		printf("System has deadlocked\n");
	for (int p = 0; p < MAX_PROCESSES; p++)
		if (Process[p])
			delete Process[p];
	return 0;
}

