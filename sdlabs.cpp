#include <stdio.h>

typedef enum
{
	DONENOTHING, DONETRANS, DONESTOP, DONERETURN
} exec_result_t;

typedef enum
{
	FALSE, TRUE
} bool_t;

//--------------------------------------------------------------------
// System timer

float now = 0.;

float Now()
{
	return now;
}

void Now(float &t)
{
	now = t;
}

//--------------------------------------------------------------------
// Block type IDs

typedef enum
{
	_user_blk, _protocol_blk, _provider_blk
} block_type_t

//--------------------------------------------------------------------
// Process type IDs

typedef enum
{
	_user_usercontrol,
	_protocol_protcontrol,
	_provider_provcontrol
} process_type_t

//--------------------------------------------------------------------
// Data types

typedef int packet[2];

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

	signal_t(int id_in)
	{
		isTimer = 0;
		timestamp = Now();
		next = NULL;
		id = id_in;
	}
};

//-------------------------------------------------------------------
// Generic process

int _pid = 0;

class SDL_process_t
{
	signal_t	*port;
	int		pid; // process (instance) id
	int		id; // class id
	struct transition_t
	{
		int	priority;
		int	state;
		int	signal;
		int	guard;
		int	dequeue;
		int	action;
		struct transition_t *next;
	};
	transition_t	*transTbl;

public:
	SDL_process_t(int id_in)
	{
		port = NULL;
		transTbl = NULL;
		pid = _pid++;
		id = id_in;
	}
	void MakeEntry(int pri, int state, int sig, int prov, int deq, int act);
	void Output(signal_t *s, int dest_pid);
	void Enqueue(signal_t *s);
	signal_t* Dequeue();
	int Schedule(float &mindelay);
	virtual int Provided(int p) = 0;
};

void SDL_process_t::MakeEntry(int pri, int state, int sig, int prov,
	int deq, int act)
{
	struct transition_t *t = new struct transition_t;
	t->priority = pri;
	t->state = state;
	t->signal = signal;
	t->guard = prov;
	t->dequeue = deq;
	t->action = act;
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

void SDL_process_t::Enqueue(signal_t *s)
{
	if (port==NULL)
		port = s;
	else
	{
		signal_t *p = port, *last = NULL;
		while (p && p->timestamp < s->timestamp)
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

signal_t* SDL_process_t::Dequeue()
{
	assert(port && port->Time() <= Now());
	signal_t *rtn = port;
	port = port->Next();
	return rtn;
}

exec_result_t SDL_process_t::Schedule(float &mindelay)
{
	// assumes transitions sorted by priority
	for (struct transition_t *tp = transTbl; tp; tp=tp->next())
	{
		// check state
		if (state != tp->state) continue;
		// check input
		if (tp->signal>=0)
			if (port==NULL || port->id != tp->signal ||
				port->timestamp > Now())
					continue;
		// check provided
		if (tp->guard>=0 && !Provided(tp->guard))
			continue;
		return Action(tp->action);
	}
	if (port)
	{
		float deelay = port->timestamp - Now();
		if (deelay < mindelay)
			mindelay = deelay;
	}
	return DONENOTHING;
}

//----------------------------------------------------------------
// System signals

typedef enum
{
	u_data_req_ID,
	u_data_rsp_ID,
	u_data_ind_ID,
	p_data_req_ID,
	p_data_ind_ID,
	p_ack_req_ID,
	p_ack_ind_ID,
	senddelay_ID,
	timeout_ID
} signal_type_t;

class u_data_req : public signal_t
{
public:
	packet	arg_1;
	u_data_req(packet a1)
		: signal_t((int)u_data_req_ID)
	{
		arg1 = a1;
	}
};

class u_data_rsp : public signal_t
{
public:
	u_data_rsp()
		: signal_t((int)u_data_rsp_ID)
	{
	}
};

class u_data_ind : public signal_t
{
public:
	packet	arg_1;
	u_data_ind(packet a1)
		: signal_t((int)u_data_ind_ID)
	{
		arg1 = a1;
	}
};

class p_data_req : public signal_t
{
public:
	int	arg_1;
	packet	arg_2;
	p_data_req(int a1, packet a2)
		: signal_t((int)p_data_req_ID)
	{
		arg1 = a1;
		arg2 = a2;
	}
};

class p_data_ind : public signal_t
{
public:
	int	arg_1;
	packet	arg_2;
	p_data_ind(int a1, packet a2)
		: signal_t((int)p_data_ind_ID)
	{
		arg1 = a1;
		arg2 = a2;
	}
};

class p_ack_req	: public signal_t
{
public:
	int	arg1;
	p_ack_req(int a1)
		: signal_t((int)p_ack_req_ID)
	{
		arg1 = a1;
	}
};

class p_ack_ind	: public signal_t
{
public:
	int	arg1;
	p_ack_ind(int a1)
		: signal_t((int)p_ack_ind_ID)
	{
		arg1 = a1;
	}
};

class timeout : public signal_t
{
public:
	timeout()
		: signal_t((int)timeout_ID)
	{
		isTimer = 1;
	}
};

class senddelay : public signal_t
{
public:
	senddelay()
		: signal_t((int)senddelay_ID)
	{
		isTimer = 1;
	}
};

//-------------------------------------------------------------------
// Process class IDs

typedef enum
{
	provider_provcontrol_ID,
	protocol_protcontrol_ID,
	user_usercontrol_ID
} process_type_t;

//-------------------------------------------------------------------
// Provider control

class provcontrol_prc_t : public SDL_process_t
{
	int		seq;
	packet		data;

public:
	provcontrol_prc_t();

	bool_t		Provided(int p);
	void		Dequeue(int d);
	exec_result_t	Action(int a);
};

bool_t provcontrol_prc_t::Provided(int p)
{
	assert(0);
	return FALSE;
}

void provcontrol_prc_t::Dequeue(int d)
{
	signal_t *s = DequeueSignal();
	sender = s->sender;
	switch(d)
	{
		case 0:
			seq = (p_data_req *)s->arg_1;
			data = (p_data_req *)s->arg_2;
			delete (p_data_req *)s;
			break;
		case 1:
			seq = (p_ack_req *)s->arg_1;
			delete (p_ack_req *)s;
			break;
		default:
			assert(0);
	}
}

exec_result_t provcontrol_prc_t::Action(int a)
{
	exec_result_t rtn = DONETRANS;
	switch(a)
	{
		case 0:
			Output(new p_data_req(seq, data), -1);
			return rtn;
		case 1:
			Output(new p_ack_ind(seq), -1);
			return rtn;
	}
	assert(0);
	return rtn;
}

provcontrol_prc_t::provcontrol_prc_t()
	: SDL_process_t((int)provider_provcontrol_ID)
{
	state = idle;
	MakeEntry(0, (int)idle, (int)p_data_req_ID, -1, -1, 0, 0);
	MakeEntry(0, (int)idle, (int)p_ack_req_ID, -1, -1, 1, 1);
}

//----------------------------------------------------------------
// Provider control

class protcontrol_prc_t : public SDL_process_t
{
	int		sn;
	int		rn;
	int		seq;
	packet		data;
	duration	tm;
	timer		timeout;

public:
	protcontrol_prc_t();

	bool_t		Provided(int p);
	void		Dequeue(int d);
	exec_result_t	Action(int a);
};

bool_t protcontrol_prc_t::Provided(int p)
{
	switch(p)
	{
		case 0:	return TRUE;
	}
	assert(0);
	return FALSE;
}

void protcontrol_prc_t::Dequeue(int d)
{
	signal_t *s = DequeueSignal();
	sender = s->sender;
	switch(d)
	{
		case 0:
			data = (u_data_req *)s->arg_1;
			delete (u_data_req *)s;
			break;
		case 1:
			seq = (p_ack_ind *)s->arg_1;
			delete (p_ack_ind *)s;
			break;
		case 2:
			seq = (p_ack_ind *)s->arg_1;
			delete (p_ack_ind *)s;
			break;
		case 3:
			delete (timeout *)s;
			break;
		case 4:
			seq = (p_data_ind *)s->arg_1;
			data = (p_data_ind *)s->arg_2;
			delete (p_data_ind *)s;
			break;
		default: assert(0);
	}
}

exec_result_t protcontrol_prc_t::Action(int a)
{
	exec_result_t rtn = DONETRANS;
	switch(a)
	{
		case 0:
			printf("%d\n", data[0]);
			printf("%d\n", data[1]);
			Output(new p_data_req(sn, data), -1);
			Set(new timeout, Now()+tm);
			state = wait;
			return rtn;
		case 1:
			return rtn;
		case 2:
			if ( (seq == sn) == FALSE)
			{
				return rtn;
			}
			else if ( (seq == sn) == TRUE)
			{
				sn = (sn + 1) % 2;
				Output(new u_data_rsp, -1);
				state = idle;
				return rtn;
			}
		case 3:
			Output(new p_data_req(sn, data), -1);
			Set(new timeout, Now() + tm);
			return rtn;
		case 4:
			if ( (seq == rn) == FALSE)
			{
			}
			else if ( (seq == sn) == TRUE)
			{
				Output(new u_data_ind(data), -1);
				Output(new p_ack_req(rn), -1);
				rn = (rn + 1) % 2;
			}
			return rtn;
	}
	assert(0);
	return rtn;
}

protcontrol_prc_t::protcontrol_prc_t()
	: SDL_process_t((int)protocol_protcontrol_ID)
{
	tm = 10.;
	sn = 0;
	rn = 0;
	state = idle;
	MakeEntry(0,(int)idle, (int)u_data_req_ID, -1, 0, 0);
	MakeEntry(0,(int)idle, (int)p_ack_ind_ID, -1, 1, 1);
	MakeEntry(0,(int)wait, (int)p_ack_ind_ID, -1, 2, 2);
	MakeEntry(0,(int)wait, (int)timeout_ID, -1, 3, 3);
	MakeEntry(0,(int)-1, (int)p_data_ind_ID, -1, 4, 4);
}

//---------------------------------------------------------------
// User control

class usercontrol_prc_t : public SDL_process_t
{
	packet		data;
	duration	delaywait;
	int		counter;
	timer		senddelay;

public:
	usercontrol_prc_t();

	bool_t		Provided(int p);
	void		Dequeue(int d);
	exec_result_t	Action(int a);
};

bool_t usercontrol_prc_t::Provided(int p)
{
	switch(p)
	{
		case 0:	return TRUE;
	}
	assert(0);
	return FALSE;
}

void usercontrol_prc_t::Dequeue(int d)
{
	signal_t *s = DequeueSignal();
	sender = s->sender;
	switch(d)
	{
		case 0:
			delete (u_data_rsp *)s;
			break;
		case 1:
			data = (u_data_ind *)s->arg_1;
			delete (u_data_ind *)s;
			break;
		case 2:
			delete (senddelay *)s;
			break;
		default: assert(0);
	}
}

exec_result_t usercontrol_prc_t::Action(int a)
{
	exec_result_t rtn = DONETRANS;
	switch(a)
	{
		case 0:
			data[0] = counter;
			data[1] = counter+1;
			Output(new u_data_req(data), -1);
			state = idle;
			return rtn;
		case 1:
			Set(new senddelay, Now()+delaywait);
			return rtn;
		case 2:
			return rtn;
		case 3:
			counter= counter+1;
			data[0] = counter;
			data[1] = counter+1;
			Output(new u_data_req(data), -1);
			return rtn;
	}
	assert(0);
	return rtn;
}

usercontrol_prc_t::usercontrol_prc_t()
	: SDL_process_t((int)user_usercontrol_ID)
{
	delaywait = 3.0;
	counter = 0;
	state = sendfirst;
	// rather than storing priorities, arrange these in priority order...
	MakeEntry(0, (int)sendfirst, -1, 0, -1, 0); 
	MakeEntry(0, (int)idle, (int)u_data_rsp_ID, -1, 0, 1);
	MakeEntry(0, (int)idle, (int)u_data_ind_ID, -1, 1, 2);
	MakeEntry(0, (int)idle, (int)senddelay_ID, -1, 2, 3);
}

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

channel_entry_t Channels[] =
{
		// u_data_req, u_data_rsp/ind
	{ upper_ch, _user_blk, _protocol_blk, 0x??, 0x?? },
		// p_data/ack_req, u_data/ack_ind
	{ lower_ch, _protocol_blk, _provider_blk, 0x??, 0x?? }
};

typedef struct
{
	int	route;		// route ID
	int	channel;	// channel to which connected, if any
	int	src;		// source process class ID
	int	dest;		// dest process class ID or -1 for ENV
	unsigned long	send;	// signals from src->dest
	unsigned long	recv;	// signals from dest->src
} route_entry_t;

route_entry_t Routes[] =
{
		// p_data/ack_ind, p_data/ack_req
	{ _provider_comm, lower_ch, provider_provcontrol_ID, -1,
			0x??, 0x?? },
		// p_data/ack_req, p_data/ack_ind
	{ _protocol_pcomm, lower_ch, protocol_protcontrol_ID, -1,
			0x??, 0x?? },
		// u_data_ind/rsp, u_data_req;
	{ _protocol_ucomm, upper_ch, protocol_protcontrol_ID, -1,
			0x??, 0x?? },
		// u_data_req, u_data_ind/rsp
	{ _user_comm, upper_ch, user_usercontrol_ID, -1,
			0x??, 0x?? }
};

int nchannels = 2;
int nroutes = 4;

//----------------------------------------------------------------

SDL_process_t	*Process[MAX_PROCESSES];

void Create(int np)
{
}

void Schedule(int np)
{
	int change = 1;
	Now(0.);
	while (change)
	{
		float mindelay = 1.e10;
		change = 0;
		for (int p = 0; p < np; p++)
		{
			if (Process[p]->Schedule(mindelay))
				change = 1;
		}
		if (!change && mindelay < 1.e10)
		{
			Now(Now()+mindelay);
			change = 1;
		}
	}
}

int FindDestClass(int procid, int sigid, int dest_hint)
{
	int ce, re;
	for (int r = 0; r < nroutes; r++)
	{
		if (Routes[r].src = procid)
		{
			if ((1l << sigid) & Routes[r].send)
			{
				if (Routes[r].channel>=0)
				{
					for (int c = 0; c < nchans; c++)
					{
						if (Channels[c].id != Routes[r].channel)
							continue;

					}
				}
				else if (dest_hint < 0 || Routes[r].dest == dest_hint)
					return Routes[r].dest;
			}
		}
	}
	// try other direction
	for (int r = 0; r < nroutes; r++)
	{
		if (Routes[r].dest = procid)
		{
			if ((1l << sigid) & Routes[r].recv)
			{
			}
		}
	}
}

void SDL_process_t::Output(signal_t *s, int dest_pid)
{
	int dest_hint = -1;
	if (dest_pid>=0)
	{
		for (int p = 0; p < np; p++)
			if (Process[p]->pid == dest_pid)
			{
				dest_hint = Process[p]->id;
				break;
			}
	}
	int cid = FindDestClass(this_.id, s->id, dest_hint);
}

main()
{
	for (int i = 0; i < MAX_PROCESSES; i++)
		Process[i] = NULL;
	Process[0] = new provcontrol_t;
	Process[1] = new protcontrol_t;
	Process[2] = new usercontrol_t;
	Schedule(3);
}

