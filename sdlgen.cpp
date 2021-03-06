//---------------------------------------------------------------
// Generated by SDL*Designer on Sun Apr 23 20:00:44 1995
// from AST form of source files:
//	abs.sdl
// SDL*Designer written by Graham Wheeler
// (c) 1994 by Graham Wheeler and the DNA Laboratory
//---------------------------------------------------------------

#include "sdlrte.h"
#include "sdlgen.h"

//---------------------------------------------------------
// Process name table

char *process_names[] =
{
	"provider_provcontrol",
	"protocol_protcontrol",
	"user_usercontrol"
};

//---------------------------------------------------------
// Signal name table

char *signal_names[] =
{
	"u_data_req",
	"u_data_rsp",
	"u_data_ind",
	"p_data_req",
	"p_data_ind",
	"p_ack_req",
	"p_ack_ind",
	"protocol_protcontrol_timeout",
	"user_usercontrol_senddelay"
};

//---------------------------------------------------------
// Methods for process class provider_provcontrol

char *provider_provcontrol::StateName(int s)
{
	switch(s)
	{
		case idle_ID:
			return "idle";
		default:
			return "Unknown";
	}
}

void provider_provcontrol::Input(int d, int sigid)
{
	signal_t *s = Dequeue(sigid);
	switch(d)
	{
		case 0:
			provider_provcontrol_seq = ((p_data_req_t *)s)->arg_1;
			memcpy(&provider_provcontrol_data, &((p_data_req_t *)s)->arg_2, sizeof(provider_provcontrol_data));
			break;
		case 1:
			provider_provcontrol_seq = ((p_ack_req_t *)s)->arg_1;
			break;
		default:
			break;
	}
	LogInput(Self(), s);
	delete s;
}

exec_result_t provider_provcontrol::Action(int a)
{
	exec_result_t rtn = DONETRANS;
	switch(a)
	{
		case 0: // start transition
			LogCreate(Self());
			LogState(Self(), StateName(state), StateName(idle_ID));
			state = idle_ID;
			return DONETRANS;
		case 1:
			{
				p_data_ind_t *__sig_1 = new p_data_ind_t;
				__sig_1->arg_1 = provider_provcontrol_seq;
				memcpy(&__sig_1->arg_2, &provider_provcontrol_data, sizeof(__sig_1->arg_2));
				int pid_1 = 0;
				if (FindDest(pid_1, (int)protocol_protcontrol_ID))
					Output((signal_t *)__sig_1, Self(), pid_1, 1);
				else assert(0); // no dest found
			}
			return DONETRANS;
		case 2:
			{
				p_ack_ind_t *__sig_2 = new p_ack_ind_t;
				__sig_2->arg_1 = provider_provcontrol_seq;
				int pid_2 = 0;
				if (FindDest(pid_2, (int)protocol_protcontrol_ID))
					Output((signal_t *)__sig_2, Self(), pid_2, 1);
				else assert(0); // no dest found
			}
			return DONETRANS;
	}
	assert(0);
	return rtn;
}

provider_provcontrol::provider_provcontrol(int ppid_in)
	: SDL_process_t(2, (int)provider_provcontrol_ID, ppid_in)
{
	MakeEntry(-1, (int)idle_ID, (int)p_data_req_ID, -1, 0, 1, -1);
	MakeEntry(-1, (int)idle_ID, (int)p_ack_req_ID, -1, 1, 2, -1);
}

provider_provcontrol::~provider_provcontrol()
{
	printf("Time %-5f Process provider_provcontrol (%d) terminated (state %s, last event time %f)\n",
		Now(), Self(), StateName(state), lasteventtime);
}


//---------------------------------------------------------
// Methods for process class protocol_protcontrol

char *protocol_protcontrol::StateName(int s)
{
	switch(s)
	{
		case idle_ID:
			return "idle";
		case wait_ID:
			return "wait";
		default:
			return "Unknown";
	}
}

void protocol_protcontrol::Input(int d, int sigid)
{
	signal_t *s = Dequeue(sigid);
	switch(d)
	{
		case 0:
			memcpy(&protocol_protcontrol_data, &((u_data_req_t *)s)->arg_1, sizeof(protocol_protcontrol_data));
			break;
		case 1:
			protocol_protcontrol_seq = ((p_ack_ind_t *)s)->arg_1;
			break;
		case 2:
			protocol_protcontrol_seq = ((p_ack_ind_t *)s)->arg_1;
			break;
		case 4:
			protocol_protcontrol_seq = ((p_data_ind_t *)s)->arg_1;
			memcpy(&protocol_protcontrol_data, &((p_data_ind_t *)s)->arg_2, sizeof(protocol_protcontrol_data));
			break;
		default:
			break;
	}
	LogInput(Self(), s);
	delete s;
}

exec_result_t protocol_protcontrol::Action(int a)
{
	exec_result_t rtn = DONETRANS;
	switch(a)
	{
		case 0: // start transition
			LogCreate(Self());
			protocol_protcontrol_sn = 0;
			protocol_protcontrol_rn = 0;
			LogState(Self(), StateName(state), StateName(idle_ID));
			state = idle_ID;
			return DONETRANS;
		case 1:
			printf("%ld\n", (long)(protocol_protcontrol_sn));
			{
				p_data_req_t *__sig_3 = new p_data_req_t;
				__sig_3->arg_1 = protocol_protcontrol_sn;
				memcpy(&__sig_3->arg_2, &protocol_protcontrol_data, sizeof(__sig_3->arg_2));
				int pid_3 = 0;
				if (FindDest(pid_3, (int)provider_provcontrol_ID))
					Output((signal_t *)__sig_3, Self(), pid_3, 1);
				else assert(0); // no dest found
			}
			{
				signal_t *__timer_1 = (signal_t *)new protocol_protcontrol_timeout_t;
				ResetTimer(0);
				SetTimer(__timer_1, Now() + protocol_protcontrol_tm);
			}
			LogState(Self(), StateName(state), StateName(wait_ID));
			state = wait_ID;
			return DONETRANS;
		case 2:
			return DONETRANS;
		case 3:
			if (!(protocol_protcontrol_seq == protocol_protcontrol_sn))
			{
				printf("%ld\n", (long)((2)));
				return DONETRANS;
			}
			else if ((protocol_protcontrol_seq == protocol_protcontrol_sn))
			{
				protocol_protcontrol_sn = (protocol_protcontrol_sn + 1) 2;
				{
					u_data_rsp_t *__sig_4 = new u_data_rsp_t;
					int pid_4 = 0;
					if (FindDest(pid_4, (int)user_usercontrol_ID))
						Output((signal_t *)__sig_4, Self(), pid_4, 0);
					else assert(0); // no dest found
				}
				ResetTimer(1);
				LogState(Self(), StateName(state), StateName(idle_ID));
				state = idle_ID;
				return DONETRANS;
			}
		case 4:
			{
				p_data_req_t *__sig_5 = new p_data_req_t;
				__sig_5->arg_1 = protocol_protcontrol_sn;
				memcpy(&__sig_5->arg_2, &protocol_protcontrol_data, sizeof(__sig_5->arg_2));
				int pid_5 = 0;
				if (FindDest(pid_5, (int)provider_provcontrol_ID))
					Output((signal_t *)__sig_5, Self(), pid_5, 1);
				else assert(0); // no dest found
			}
			{
				signal_t *__timer_2 = (signal_t *)new protocol_protcontrol_timeout_t;
				ResetTimer(2);
				SetTimer(__timer_2, Now() + protocol_protcontrol_tm);
			}
			return DONETRANS;
		case 5:
			if (!(protocol_protcontrol_seq == protocol_protcontrol_rn))
			{
			}
			else if ((protocol_protcontrol_seq == protocol_protcontrol_rn))
			{
				{
					u_data_ind_t *__sig_6 = new u_data_ind_t;
					memcpy(&__sig_6->arg_1, &protocol_protcontrol_data, sizeof(__sig_6->arg_1));
					int pid_6 = 0;
					if (FindDest(pid_6, (int)user_usercontrol_ID))
						Output((signal_t *)__sig_6, Self(), pid_6, 0);
					else assert(0); // no dest found
				}
				{
					p_ack_req_t *__sig_7 = new p_ack_req_t;
					__sig_7->arg_1 = protocol_protcontrol_rn;
					int pid_7 = 0;
					if (FindDest(pid_7, (int)provider_provcontrol_ID))
						Output((signal_t *)__sig_7, Self(), pid_7, 1);
					else assert(0); // no dest found
				}
				protocol_protcontrol_rn = (protocol_protcontrol_rn + 1) 2;
			}
			return DONETRANS;
	}
	assert(0);
	return rtn;
}

protocol_protcontrol::protocol_protcontrol(int ppid_in)
	: SDL_process_t(5, (int)protocol_protcontrol_ID, ppid_in)
{
	protocol_protcontrol_tm = 10.000000;
	MakeEntry(-1, (int)idle_ID, (int)u_data_req_ID, -1, 0, 1, -1);
	MakeEntry(-1, (int)idle_ID, (int)p_ack_ind_ID, -1, 1, 2, -1);
	MakeEntry(-1, (int)wait_ID, (int)p_ack_ind_ID, -1, 2, 3, -1);
	MakeEntry(-1, (int)wait_ID, (int)protocol_protcontrol_timeout_ID, -1, 3, 4, -1);
	MakeEntry(-1, -1, (int)p_data_ind_ID, -1, 4, 5, -1);
}

protocol_protcontrol::~protocol_protcontrol()
{
	printf("Time %-5f Process protocol_protcontrol (%d) terminated (state %s, last event time %f)\n",
		Now(), Self(), StateName(state), lasteventtime);
}


//---------------------------------------------------------
// Methods for process class user_usercontrol

char *user_usercontrol::StateName(int s)
{
	switch(s)
	{
		case sendfirst_ID:
			return "sendfirst";
		case idle_ID:
			return "idle";
		default:
			return "Unknown";
	}
}

void user_usercontrol::Input(int d, int sigid)
{
	signal_t *s = Dequeue(sigid);
	switch(d)
	{
		case 1:
			memcpy(&user_usercontrol_data, &((u_data_ind_t *)s)->arg_1, sizeof(user_usercontrol_data));
			break;
		default:
			break;
	}
	LogInput(Self(), s);
	delete s;
}

exec_result_t user_usercontrol::Action(int a)
{
	exec_result_t rtn = DONETRANS;
	switch(a)
	{
		case 0: // start transition
			LogCreate(Self());
			LogState(Self(), StateName(state), StateName(sendfirst_ID));
			state = sendfirst_ID;
			return DONETRANS;
		case 1:
			user_usercontrol_data[0] = user_usercontrol_counter;
			user_usercontrol_data[1] = user_usercontrol_counter + 1;
			{
				u_data_req_t *__sig_8 = new u_data_req_t;
				memcpy(&__sig_8->arg_1, &user_usercontrol_data, sizeof(__sig_8->arg_1));
				int pid_8 = 0;
				if (FindDest(pid_8, (int)protocol_protcontrol_ID))
					Output((signal_t *)__sig_8, Self(), pid_8, 0);
				else assert(0); // no dest found
			}
			LogState(Self(), StateName(state), StateName(idle_ID));
			state = idle_ID;
			return DONETRANS;
		case 2:
			{
				signal_t *__timer_3 = (signal_t *)new user_usercontrol_senddelay_t;
				ResetTimer(0);
				SetTimer(__timer_3, Now() + user_usercontrol_delaywait);
			}
			return DONETRANS;
		case 3:
			return DONETRANS;
		case 4:
			user_usercontrol_counter = user_usercontrol_counter + 1;
			user_usercontrol_data[0] = user_usercontrol_counter;
			user_usercontrol_data[1] = user_usercontrol_counter + 1;
			{
				u_data_req_t *__sig_9 = new u_data_req_t;
				memcpy(&__sig_9->arg_1, &user_usercontrol_data, sizeof(__sig_9->arg_1));
				int pid_9 = 0;
				if (FindDest(pid_9, (int)protocol_protcontrol_ID))
					Output((signal_t *)__sig_9, Self(), pid_9, 0);
				else assert(0); // no dest found
			}
			return DONETRANS;
	}
	assert(0);
	return rtn;
}

user_usercontrol::user_usercontrol(int ppid_in)
	: SDL_process_t(4, (int)user_usercontrol_ID, ppid_in)
{
	user_usercontrol_delaywait = 3.000000;
	user_usercontrol_counter = 0;
	MakeEntry(-1, (int)sendfirst_ID, -1, -1, -1, 1, -1);
	MakeEntry(-1, (int)idle_ID, (int)u_data_rsp_ID, -1, 0, 2, -1);
	MakeEntry(-1, (int)idle_ID, (int)u_data_ind_ID, -1, 1, 3, -1);
	MakeEntry(-1, (int)idle_ID, (int)user_usercontrol_senddelay_ID, -1, 2, 4, -1);
}

user_usercontrol::~user_usercontrol()
{
	printf("Time %-5f Process user_usercontrol (%d) terminated (state %s, last event time %f)\n",
		Now(), Self(), StateName(state), lasteventtime);
}


//---------------------------------------------------------
void CreateInitial()
{
	MaxInstances[(int)provider_provcontrol_ID] = 1;
	AddProcess(new provider_provcontrol);
	MaxInstances[(int)protocol_protcontrol_ID] = 1;
	AddProcess(new protocol_protcontrol);
	MaxInstances[(int)user_usercontrol_ID] = 1;
	AddProcess(new user_usercontrol);
	for (int i = 0; i < 3; i++) Instances[i] = 0;
}


//---------------------------------------------------------

int Instances[3];
int MaxInstances[3];

//---------------------------------------------------------
// Channel table

channel_info_t channelTbl[] =
{
	{ 100, 1. },
	{ 100, 1. }
};

// Modify this as necessary

void GetChannelInfo(int ch, signal_t *s, float &dlay, int &rel)
{
	(void)s;
	dlay = channelTbl[ch].deelay;
	rel = channelTbl[ch].reliability;
}

//---------------------------------------------------------

int protocol_protcontrol::CompareTimer(int idx, signal_t *s)
{
	switch(idx)
	{
		case 0:
		{
			protocol_protcontrol_timeout_t *sg = (protocol_protcontrol_timeout_t *)s;
			if (!sg->isTimer || sg->id != protocol_protcontrol_timeout_ID) return 0;
			return 1;
		}
		case 1:
		{
			protocol_protcontrol_timeout_t *sg = (protocol_protcontrol_timeout_t *)s;
			if (!sg->isTimer || sg->id != protocol_protcontrol_timeout_ID) return 0;
			return 1;
		}
		case 2:
		{
			protocol_protcontrol_timeout_t *sg = (protocol_protcontrol_timeout_t *)s;
			if (!sg->isTimer || sg->id != protocol_protcontrol_timeout_ID) return 0;
			return 1;
		}
	}
	return 0;
}

int user_usercontrol::CompareTimer(int idx, signal_t *s)
{
	switch(idx)
	{
		case 0:
		{
			user_usercontrol_senddelay_t *sg = (user_usercontrol_senddelay_t *)s;
			if (!sg->isTimer || sg->id != user_usercontrol_senddelay_ID) return 0;
			return 1;
		}
	}
	return 0;
}
