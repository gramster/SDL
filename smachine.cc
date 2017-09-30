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
 * Last modified: 12-9-94
 *
 */

#pragma hdrfile "RUN.SYM"
#include "sdlrun.h"

#ifndef UNDEFINED
#define UNDEFINED (32766)
#endif

#define DEBUG

#ifdef DEBUG
int debug = 0;
#endif

scheduler_t *Scheduler = NULL;
channel_mgr_t ChanMgr;
static int verbose = 0, Abort = 0;

//---------------------------------------------------------------
// Supplementary AST routines 

static void Indent(char *buf, int indent)
{
	buf[0] = 0;
	for (int i=0;i<indent;i++) strcat(buf,"    ");
}

void array_def_t::PrintValue(int indent, s_code_word_t* &ptr)
{
	data_def_t *dd = (data_def_t *)ScopeStack->dequalify(&sort, Q_TYPE);
	for (int i = 0; i < dval; i++)
	{
		char tbuf[10];
		sprintf(tbuf, "%3d : ", i);
		dd->PrintValue(indent, tbuf, ptr);
	}
}

void fieldgrp_t::PrintValue(int indent, s_code_word_t* &ptr)
{
	data_def_t *dd = (data_def_t *)ScopeStack->dequalify(&sort, Q_TYPE);
	lh_list_node_t<field_name_t> *fn = names.front();
	while (fn)
	{
		dd->PrintValue(indent, (char *)fn->info.name(), ptr);
		fn = fn->next();
	}
}

void struct_def_t::PrintValue(int indent, s_code_word_t* &ptr)
{
	lh_list_node_t<fieldgrp_t> *fg = fieldgrps.front();
	while (fg)
	{
		fg->info.PrintValue(indent, ptr);
		fg = fg->next();
	}
}

void data_def_t::PrintValue(int indent, char *name, s_code_word_t* &ptr)
{
	char tbuf[100], *tp;
	int i;
	Indent(tbuf, indent);
	tp = tbuf+strlen(tbuf);
	if (name)
	{
		sprintf(tp,"%-10.10s : \0",name);
		tp = tbuf+strlen(tbuf);
	}
	switch(tag)
	{
	case STRUCT_TYP:
		Scheduler->Write(tbuf);
		indent++;
		Indent(tbuf, indent); strcat(tbuf,"{");
		Scheduler->Write(tbuf);
		(GetStrucDefP(contents))->PrintValue(indent+1, ptr);
		Indent(tbuf, indent); strcat(tbuf,"}");
		Scheduler->Write(tbuf);
		indent--;
		break;
	case ARRAY_TYP:
		Scheduler->Write(tbuf);
		indent++;
		Indent(tbuf, indent); strcat(tbuf,"(");
		Scheduler->Write(tbuf);
		(GetArrDefP(contents))->PrintValue(indent+1, ptr);
		Indent(tbuf, indent); strcat(tbuf,")");
		Scheduler->Write(tbuf);
		indent--;
		break;
	default:
		if (*ptr == UNDEFINED) strcpy(tp,"UNDEFINED");
		else switch(tag)
		{
		case REAL_TYP:
		case TIME_TYP:
		case DURATION_TYP:
			sprintf(tp, "%lf", (double)(*((s_code_real_t *)ptr)));
			break;
		case ENUM_TYP:
		{
			enum_def_t *ed = GetEnumDefP(contents);
			int cnt = (int) *ptr;
			lh_list_node_t<enum_elt_t> *el = ed->elts.front();
			while (--cnt)
			{
				el = el->next();
				assert(el);
			}
			sprintf(tp, "%s", el->info.name());
			break;
		}
		case INTEGER_TYP:
		case NATURAL_TYP:
		case PID_TYP:
			sprintf(tp, "%ld", (long) *ptr);
			break;
		case CHARACTER_TYP:
			sprintf(tp, "%c", (char) *ptr);
			break;
		case BOOLEAN_TYP:
			sprintf(tp, "%s", (*ptr) ? "True" : "False" );
			break;
		}
		Scheduler->Write(tbuf);
		ptr++;
		break;
	}
}

static void PrintValue(data_def_t *dd, char *name, s_code_word_t *ptr)
{
	dd->PrintValue(0, name, ptr);
}

//---------------------------------------------------------------

int channel_mgr_t::Find(s_code_word_t ch)
{
	for (int i = 0; i < cnt; i++)
		if (chans[i] == ch) return i;
	return -1;
}

void channel_mgr_t::Enter(s_code_word_t ch, s_code_real_t d, int r)
{
	int i = Find(ch);
	if (i<0)
	{
		assert(cnt<MAX_CHANNELS);
		i = cnt++;
	}
	chans[i] = ch;
	delays[i] = d;
	reliability[i] = r;
}

void channel_mgr_t::Edit(int idx, s_code_real_t d, int r)
{
	delays[idx] = d;
	reliability[idx] = r;
}

s_code_real_t channel_mgr_t::GetDelay(s_code_word_t ch)
{
	int i = Find(ch);
	if (i>=0)
	{
#if __MSDOS__
		if (random(100) < reliability[i])
#else
		if ((random()%100) < reliability[i])
#endif
			return delays[i];
		else return -1.0; // lose it!
	}
	else
	{
		Enter(ch);
		return 1.0;
	}
}

void channel_mgr_t::Show()
{
	for (int i = 0; i < cnt; i++)
	{
		char buff[80];
		channel_def_t *cd = GetChanDefP(chans[i]);
		sprintf(buff,"C%-2d %-3f %-3d %-12s", i+1,
			(float)delays[i], (int)reliability[i], cd->name());
		Scheduler->Write(buff);
	}
}

//-----------------------------------------------------------------

void filter_t::Print(char *buf)
{
	if (PId==-1)
		sprintf(buf,"%s (%3d) %c * %s %s %s",
		typ==ACCEPT_FILTER? "Accept" : "Reject", cnt,
		fclassCodes[fclass], flds[0], flds[1], flds[2]);
	else
		sprintf(buf,"%s (%3d) %c %d %s %s %s",
		typ==ACCEPT_FILTER? "Accept" : "Reject", cnt,
		fclassCodes[fclass], PId, flds[0], flds[1], flds[2]);
}

int filter_t::operator== (const filter_t& f) const
{
	return (PId == f.PId && fclass == f.fclass && typ == f.typ &&
		strcmp(flds[0], f.flds[0])==0 &&
		strcmp(flds[1], f.flds[1])==0 &&
		strcmp(flds[2], f.flds[2])==0);
}

void filter_t::parse(char *pat_in, filter_class_t &class_out, int &PId_out,
	char flds_out[][20])
{
	char pidStr[10], tc;
	flds_out[0][0] = flds_out[1][0] = flds_out[2][0] = '\0';
	PId_out = -1;
	class_out = UNKNOWN_FILTER;
	if (pat_in)
	{
		if (sscanf(pat_in, "%d", &cnt)==0) cnt = 0;
		while (*pat_in && *pat_in < 'A') pat_in++;
		if (sscanf(pat_in, "%c%s%s%s%s", &tc, pidStr,
			flds_out[0], flds_out[1], flds_out[2])>=2)
				if (strcmp(pidStr, "*") != 0)
					PId_out = atol(pidStr);
		if (tc>'Z') tc -= 'z'-'Z';
		switch (tc)
		{
		case 'F':	class_out = FIRE_FILTER;	break;
		case 'O':	class_out = OUTPUT_FILTER;	break;
		case 'U':	class_out = LOSS_FILTER;	break;
		case 'A':	class_out = ARRIVE_FILTER;	break;
		case 'I':	class_out = INPUT_FILTER;	break;
		case 'C':	class_out = CLAUSE_FILTER;	break;
		case 'L':	class_out = LINE_FILTER;	break;
		case 'P':	class_out = PROCESS_FILTER;	break;
		case 'S':	class_out = STATE_FILTER;	break;
		}
	}
}

filter_t::filter_t(char *pat_in, filter_type_t type_in,	int cnt_in)
	: cnt(cnt_in)
	, typ(type_in)
{
	parse(pat_in, fclass, PId, flds);
	if (cnt==0) active = 1;
	else if (typ == ACCEPT_FILTER) active = 0;
	else active = 1;
}

int filter_t::Apply(char *input)
{
	// break up the input

	char F[4][50], tc, *itmp = input;
	while (*itmp<'A') itmp++;
	int cc = sscanf(itmp,"%c%s%s%s%s", &tc, F[0], F[1], F[2], F[3]);
	if (cc<1) cc=1;
	while (cc<5)
	{
		F[cc-1][0]='\0';
		cc++;
	}

	// Check the filter class against the event type

	if (tc>'Z') tc -= 'z'-'Z';
	switch (tc)
	{
	case 'S':
		if (fclass != STATE_FILTER) return 0;
		break;
	case 'F':
		if (fclass != FIRE_FILTER) return 0;
		break;
	case 'O':
		if (fclass != OUTPUT_FILTER) return 0;
		break;
	case 'A':
		if (fclass != ARRIVE_FILTER) return 0;
		break;
	case 'I':
		if (fclass != INPUT_FILTER) return 0;
		break;
	case 'L':
		if (fclass != LINE_FILTER) return 0;
		break;
	case 'C':
	case 'D':
		if (fclass != PROCESS_FILTER) return 0;
		break;
	}

	// Check the PID

	if (fclass != LINE_FILTER && PId>=0)
	{
		long pid = atol(F[0]);
		if (pid != PId) return 0;
	}

	// Check the fields

	for (int i=0; i<3; i++)
	{
		if (flds[i][0]!=0 && strcmp(flds[i],"*")!=0 &&
			strcmp(flds[i], F[i+1])!=0)
				return 0;
	}

	// should we fire this time?
	if (cnt>0)
	{
		if (--cnt==0)
			if (typ==ACCEPT_FILTER)
				active = 1;
			else
				active = 0;
	}
	return active;
}

//--------------------------------------------------------------------

trace_mgr_t::trace_mgr_t()
{
	for (int i = 0; i< MAX_FILTERS; i++)
	{
		accFilters[i] = NULL;
		rejFilters[i] = NULL;
	}
}

void trace_mgr_t::Show(filter_type_t typ)
{
	filter_t **f = (typ==ACCEPT_FILTER) ? accFilters : rejFilters;
	for (int i = 0; i < MAX_FILTERS; i++)
	{
		if (f[i])
		{
			char buff[80];
			f[i]->Print(buff);
			Scheduler->Write(buff);
		}
	}
}

void trace_mgr_t::Add(filter_type_t typ, char *ftxt, int cnt)
{
	filter_t **f = (typ==ACCEPT_FILTER) ? accFilters : rejFilters;
	for (int i = 0; i < MAX_FILTERS; i++)
		if (f[i]==NULL || (typ==REJECT_FILTER && f[i]->active==0))
		{
			f[i] = new filter_t(ftxt, typ, cnt);
			assert(f[i]);
			break;
		}
}

void trace_mgr_t::Delete(filter_type_t typ, char *ftxt)
{
	filter_t **f = (typ==ACCEPT_FILTER) ? accFilters : rejFilters;
	filter_t tmpF(ftxt, typ);
	for (int i = 0; i < MAX_FILTERS; i++)
	{
		if (f[i] && *f[i]==tmpF)
		{
			delete f[i];
			f[i] = NULL;
		}
	}
}

void trace_mgr_t::Filter(char *msg)
{
	for (int i = 0; i < MAX_FILTERS; i++)
	{
		if (accFilters[i] && accFilters[i]->Apply(msg)==1)
			goto OK;
	}
	for (i = 0; i < MAX_FILTERS; i++)
	{
		if (rejFilters[i] && rejFilters[i]->Apply(msg)==1)
			return; // rejected
	}
	// everything else is accepted
OK:
	Scheduler->Trace(msg); 
}

void trace_mgr_t::LogFire(s_process_t *p)
{
	if (flags & TRC_FIRE)
	{
		char buff[80];
		sprintf(buff,"%-6.f F %s %-2d",
			(float)Scheduler->GetTime(), p->Identity(), p->selected);
		Filter(buff);
	}
}

void trace_mgr_t::LogImplicit(s_process_t *p, signal_t *s)
{
	if (flags & TRC_IMPLICIT)
	{
		char sbuff[80];
		char buff[80];
		sbuff[0] = 0;
		s->Print(sbuff);
		sprintf(buff, "%-6.f T %s %s",
			(float)s->timestamp, p->Identity(), sbuff);
		Filter(buff);
	}
}

void trace_mgr_t::LogInput(s_process_t *p, signal_t *s)
{
	if (flags & TRC_INPUT)
	{
		char sbuff[80];
		char buff[80];
		sbuff[0] = 0;
		s->Print(sbuff);
		sprintf(buff, "%-6.f I %s %s",
			(float)s->timestamp, p->Identity(), sbuff);
		Filter(buff);
	}
}

void trace_mgr_t::LogLoss(s_process_t *p, signal_t *s, int chan)
{
	if (flags & TRC_LOSS)
	{
		char buff[120];
		char sbuff[80];
		sbuff[0] = 0;
		s->Print(sbuff);
		sprintf(buff,"%-6.f U %s %s %-5d",
			(float)Scheduler->GetTime(), p->Identity(), sbuff, chan);
		Filter(buff);
	}
}

void trace_mgr_t::LogOutput(s_process_t *src, s_process_t *dest, signal_t *s)
{
	if (flags & TRC_OUTPUT)
	{
		char buff[120];
		char sbuff[80];
		sbuff[0] = 0;
		s->Print(sbuff);
		sprintf(buff,"%-6.f O %s %s %s",
			(float)Scheduler->GetTime(), src->Identity(), sbuff,
			dest->Identity());
		Filter(buff);
	}
}

void trace_mgr_t::LogLine(int filenum, int linenum)
{
	if (flags & TRC_LINE)
	{
		char buff[80];
		sprintf(buff,"%-6.f L %-15.15s %-4d",
			(float)Scheduler->GetTime(),
			fname[filenum], linenum);
		Filter(buff);
	}
}

void trace_mgr_t::LogCreate(s_process_t *p)
{
	if (flags & TRC_PROCESS)
	{
		char buff[80];
		sprintf(buff,"%-6.f C %s",
			(float)Scheduler->GetTime(),
		        p->Identity());
		Filter(buff);
	}
}

void trace_mgr_t::LogState(s_process_t *p, s_code_word_t fstate,
	s_code_word_t tstate, char *fnm, char *tnm)
{
	if (flags & TRC_STATE)
	{
		char buff[100];
		sprintf(buff,"%-6.f S %s %-2d %-2d %.10s %.10s",
			(float)Scheduler->GetTime(), p->Identity(),
			(int)fstate, (int)tstate, fnm, tnm);
		Filter(buff);
	}
}

void trace_mgr_t::LogDestroy(s_process_t *p)
{
	if (flags & TRC_PROCESS)
	{
		char buff[80];
		sprintf(buff,"%-6.f D %s",
			(float)Scheduler->GetTime(),
		        p->Identity());
		Filter(buff);
	}
}

void trace_mgr_t::LogSCode(int sp, s_code_word_t tos, int p)
{
	if (flags & TRC_SCODE)
	{
		char buff[100];
		sprintf(buff,"%-6.f X [sp %3d:%5ld] %s",
			(float)Scheduler->GetTime(),
			sp, (long)tos, Code->DisassembleOp(p));
		Filter(buff);
	}
}

void trace_mgr_t::LogArrive(s_process_t *p, signal_t *s)
{
	if (flags & TRC_ARRIVE)
	{
		char sbuff[80];
		char buff[80];
		sbuff[0] = 0;
		s->Print(sbuff);
		sprintf(buff, "%-6.f A %s %s",
			(float)s->timestamp, p->Identity(), sbuff);
		Filter(buff);
	}
}

trace_mgr_t TraceMgr;

//---------------------------------------------------------------------
// Signals

signal_t::signal_t(int idx_in, int arglen_in, s_code_word_t *argptr,
		int sender_in, s_code_real_t timestamp_in, signal_t *next_in)
{
	idx = idx_in;
	arglen = arglen_in;
	sender = sender_in;
	next = next_in;
	timestamp = timestamp_in;
	args = new s_code_word_t[arglen];
	assert(args);
	int i = arglen;
	for (i=0; i < arglen; i++)
		args[i] = argptr[i];
	hasArrived = 0;
	canHandle = 0;
}

signal_t::~signal_t()
{
	delete [] args;
}

void signal_t::Print(char *buff)
{
	strcat(buff, signalTbl->GetName(idx));
	buff[12] = 0; // limit length
	if (arglen > 0)
	{
		strcat(buff,"(");
		for (int i = 0; i < 6 && i < arglen; i++)
		{
			if (i) strcat(buff, ", ");
			sprintf(buff+strlen(buff), "%d\0", args[i]);
		}
		if (i<arglen) strcat(buff, "...");
		strcat(buff,")");
	}
	sprintf(buff+strlen(buff), " [%.f]\0", (float)timestamp);
}

//---------------------------------------------------------------------
// SDL Process transitions

void s_transition_t::Init(int idx_in, int p_in)
{
	idx = idx_in;
	first = last = -1;
	fired = 0;
	s_code_word_t *argTbl_in = Code->GetAddr(p_in+1);
	// Convert relative offsets to absolute offsets so that we
	// don't need to know where the OP_SDLTRANS was
#ifdef W16
	for (int i = 0; i<10;i++)
#else
	for (int i = 0; i<8;i++)
#endif
		argTbl[i] = argTbl_in[i];
#ifdef W16
	for (i = 5; i<9;i++)
#else
	for (i = 3; i<7;i++)
#endif
		if (argTbl[i]!=-1)
			argTbl[i] += p_in+1+Code->GetNumArgs(OP_SDLTRANS);
}

void s_transition_t::ShowInfo()
{
	char buff[80];
	if (first>=0.)
		sprintf(buff,"T%-3d %-8ld %-8.f %-8.f",
			idx, fired, (float)first, (float)last);
	else
		sprintf(buff,"T%-3d %-8ld -        -", idx, fired);
	Scheduler->Write(buff);
}

void s_transition_t::UpdateInfo()
{
	last = Scheduler->GetTime();
	if (first < 0) first = last;
	fired++;
}

//---------------------------------------------------------------------
// SDL Processes

s_code_word_t s_process_t::vTOS()
{
	if (TOS == UNDEFINED) SDLerror(ERR_UNDEFVAL);
	return TOS;
}

s_code_word_t s_process_t::vTOS1()
{
	if (TOS1 == UNDEFINED) SDLerror(ERR_UNDEFVAL);
	return TOS1;
}

s_code_word_t s_process_t::Chain(s_code_word_t level) // follow activation chain
{
	s_code_word_t v = b;
	assert(level>=0);
	while (level--) v = Abs(v);
	return v;
}

void s_process_t::Export(s_code_word_t offset, s_code_word_t size,
	s_code_word_t *vptr)
{
	int i;
	for (i = 0; i < MAX_EXPORTS; i++)
		if (exports[i].offset == offset) break;
		else if (exports[i].offset == 0)
		{
			exports[i].offset = offset;
			exports[i].size = size;
			break;
		}
	if (i==MAX_EXPORTS) SDLerror(ERR_MAXEXPORT);
	else
	{
		if (exports[i].valptr)
			delete [] exports[i].valptr;
		s_code_word_t *vp = new s_code_word_t[size];
		assert(vp);
		exports[i].valptr = vp;
		while (size--) *vp++ = *vptr++;
	}
}

void s_process_t::Import(s_code_word_t AST, s_code_word_t pid,
	s_code_word_t offset, s_code_word_t size)
{
	s_code_word_t *vptr = Scheduler->FindExported(AST, pid, offset, size);
	while (size--) Push(*vptr++);
}

exec_result_t s_process_t::ExecOp()
{
	exec_result_t rtn = DONE_OP; // default return
	TraceMgr.LogSCode(SP(), TOS, p);
	s_code_op_t op = (s_code_op_t)Code->Get(p++);
	s_code_word_t *args = Code->GetAddr(p);
	p += Code->GetNumArgs(op);
	switch (op)
	{
	// Basic arithmetic ops (all OK)

	case OP_ADD:	Drop();	Top(vTOS() + vTOS1());		break;
	case OP_AND:	Drop();	Top(vTOS() & vTOS1());		break;
	case OP_ARGASSIGN:
	{
		s_code_word_t  addr = Pop();		// pop address
		int l = args[0];
		while (l--) Abs(addr+l, Pop());
		break;
	}
	case OP_ASSIGN:
	{
		s_offset_t  val		= Drop(args[0]);	// get start of value
		s_code_word_t  addr	= Pop();		// pop address
		memcpy(Addr(addr), Addr(val), args[0]*sizeof(s_code_word_t));
		break;
	}
	case OP_CHANNEL: // create an entry for the channel 
		(void)ChanMgr.Enter(args[0]);
		break;
	case OP_CONSTANT:	Push(args[0]);		break;
	case OP_DEFADDR:
	case OP_DEFARG:		assert(0);
	case OP_DIV:	
		if (TOS==0) SDLerror(ERR_ZERODIV);
		else { Drop(); Top(vTOS() / vTOS1()); }
		break;
	case OP_DO:	if (!Pop()) p += args[0];		break;
	case OP_ENDCLAUSE: rtn = DONE_CLAUSE;			break;
	case OP_ENDTRANS: rtn = DONE_TRANS;			break;
	case OP_EQUAL:	Drop();	TOS = (TOS == TOS1);	break;
	// floating point ops
	case OP_FCONST:	Push(args[0]);			break;
	case OP_FIX:
	{
		// Check that this works!!
		s_code_cast_t cv;
		cv.iv = (s_code_word_t)vTOS();
		TOS = (s_code_word_t)cv.rv;
		break;
	}
	case OP_FLOAT:
	{
		s_code_cast_t cv;
		cv.rv = (s_code_real_t)vTOS();
		TOS = cv.iv;
		break;
	}
	case OP_FMINUS:
	{
		s_code_word_t v = vTOS();
		s_code_real_t rv = *((s_code_real_t *)&v);
		rv = -rv;
		TOS = *((s_code_word_t *)&rv);
		break;
	}
	case OP_FADD:
	case OP_FDIV:
	case OP_FEQU:
	case OP_FGEQ:
	case OP_FGTR:
	case OP_FLEQ:
	case OP_FLES:
	case OP_FMUL:
	case OP_FNEQ:
	case OP_FSUB:
	{
		Drop();
		s_code_word_t lv = vTOS(), rv = vTOS1(), bres = -1;
		s_code_real_t rlv = *((s_code_real_t *)&lv), res,
			      rrv = *((s_code_real_t *)&rv);
		switch (op)
		{
		case OP_FADD:
			res = rlv + rrv;	break;
		case OP_FDIV:
			if (rrv == 0.) SDLerror(ERR_ZERODIV);
			else res = rlv / rrv;
			break;
		case OP_FEQU:
			bres = (rlv == rrv);	break;
		case OP_FGEQ:
			bres = (rlv >= rrv);	break;
		case OP_FGTR:
			bres = (rlv > rrv);	break;
		case OP_FLEQ:
			bres = (rlv <= rrv);	break;
		case OP_FLES:
			bres = (rlv < rrv);	break;
		case OP_FMUL:
			res = rlv * rrv;	break;
		case OP_FNEQ:
			bres = (rlv != rrv);	break;
		case OP_FSUB:
			res = rlv - rrv;	break;
		}
		if (bres>=0) TOS = bres;
		else TOS = *((s_code_word_t *)&res);
		break;
	}
	case OP_FIELD:	TOS += args[0];				break;
	case OP_GLOBALTIME:
		s_code_cast_t cv;
		cv.rv = Scheduler->GetTime();
		Push(cv.iv);
		break;
	case OP_GLOBALVALUE:	Push(Abs(Abs(b) + args[0]));	break;
	case OP_GLOBALVAR:	Push(Abs(b) + args[0]);		break;
	case OP_GOTO:		p += args[0];			break;
	case OP_GREATER:Drop();	Top(vTOS() > vTOS1());		break;
	case OP_INDEX:
	{
		s_code_word_t ix = vTOS();
		Drop();
		if (ix < args[0] || ix > args[1])
		{
			char buf[80];
			printf(buf, "Value %d not in range [%d,%d]",ix,
				args[0], args[1]);
			SDLerror(ERR_CUSTOM,buf);
		}
		else Top(vTOS() + (ix-args[0]) * args[2]);
		break;
	}
	case OP_LESS:	Drop();	Top(vTOS() < vTOS1());	break;
	case OP_LOCALVALUE: Push(Abs(b + args[0]));	break;
	case OP_LOCALVAR: Push(b+args[0]);		break;
	case OP_MINUS:	TOS = -vTOS(); 				break;
	case OP_MODULO:	 Drop(); 	Top(vTOS() % vTOS1());	break;
	case OP_MULTIPLY:Drop();	Top(vTOS() * vTOS1());	break;
	case OP_NATASSIGN:
	{
		s_code_word_t val = Pop();
		if (val < 0 && val != UNDEFINED)
			SDLerror(ERR_UNNATURAL, (char *)val);
		else
			Abs(Pop(), val);
		break;
	}
	case OP_NEWLINE:
		TraceMgr.LogLine(args[0], args[1]);
		rtn = DONE_NEWLINE;
		break;
	case OP_NOT:	TOS = !vTOS();				break;
	case OP_NOTEQUAL:Drop();	TOS = (TOS != TOS1);	break;
	case OP_NOTGREATER:Drop();	Top(vTOS() <= vTOS1());	break;
	case OP_NOTLESS:Drop();		Top(vTOS() >= vTOS1());	break;
	case OP_OR:	Drop();		Top(vTOS() | vTOS1());	break;
	case OP_POP:	Drop(args[0]);			break;
	case OP_PREDEFVAR:
	{
		s_process_t *pr = this;
		while (pr->caller) pr = pr->caller;
		switch(args[0])
		{
		case SELF:
			Push(pr->PId);
			break;
		case SENDER:
			Push(pr->Sender);
			break;
		case PARENT:
			Push(pr->PPId);
			break;
		case OFFSPRING:
			Push(pr->Offspring);
			break;
		}
		break;
	}
	case OP_READ:
		/* Read a value and push on stack */
		printf("?");
		fflush(stdout);
		{
			long v;
			while (scanf("%ld", &v)!=1);
			Push((s_code_word_t)v);
		}
		break;
	case OP_REM:	Drop(); 	Top(vTOS() % vTOS1());	break;
	case OP_RESETTIMER:
		(void)GetActive(1, args[0], Addr(SP()), args[1]);
		Drop(args[1]);
		break;
	case OP_SDLCALL:
	{
		calls = Scheduler->NewProcedure(this, p+args[0], args[2]);
		if (calls != NULL) rtn = DONE_CALL;
		break;
	}
	case OP_SDLEXPORT:
		Export(args[0], args[1], Addr(SP() - args[1]));
		break;
	case OP_SDLIMPORT:
		Import(args[0], Pop(), args[1], args[2]);
		break;
	case OP_SDLINIT:
		(void)Scheduler->NewProcess(p+args[0], Addr(SP()-args[1]+1),
						args[2], args[3]);
		Drop(args[1]);
		break;
	case OP_SDLINPUT:
		TestSignal(args[0], &args[1]);
		break;
	case OP_SDLMODULE:
		assert(0);
	case OP_SDLNEXT:
		if (args[0]>=0 && state != args[0])
		{
			TraceMgr.LogState(this, (s_code_word_t)state, args[0],
				StateIndex2Name(*stateTbl, state),
				StateIndex2Name(*stateTbl, args[0]));
			state = args[0];
		}
		rtn = DONE_TRANS;
		break;
	case OP_SDLOUTPUT:
		// Stack has the arguments, a set of channel and
		// process offset pairs, and the PId expression
		// args are the sig idx, the number of ch,pr tuples,
		// and the length of the arguments
		// Find the target - if the PId is given, we use
		// that; else we pop off the channel/process pairs
		// one at a time and look for processes of that 
		// type. If there are more than one, this is a
		// run-time error. Else we call the target process
		// to enqueue the message, after calling the delay
		// handler to generate a delay for the channel.
		{
			s_process_t *destP;
			s_code_word_t destPId, chan;
			Pop(destPId);
			int tups = args[1], cnt, tot=0;
			while (tups--)
			{
				s_code_word_t pType, cType;
				Pop(pType);
				Pop(cType);
				cnt = Scheduler->findProcess(destPId, pType, destP);
				if (cnt) chan = cType;
				tot += cnt;
			}
			if (tot==0) SDLfatal(ERR_NODEST, signalTbl->GetName(args[0]));
			else if (tot>1) SDLfatal(ERR_MULTIDEST);
			else
			{
				signal_t *sig = new signal_t((int)args[0], (int)args[2],
						Addr(SP()-args[2]+1), PId, 0., NULL);
				assert(sig);
				s_code_real_t delay = ChanMgr.GetDelay(chan);
				TraceMgr.LogOutput(this, destP, sig);
				if (delay>=0.)
				{
					sig->timestamp = Scheduler->GetTime() + delay;
					destP->Enqueue(sig);
				}
				else
				{
					TraceMgr.LogLoss(this, sig, chan);
					delete sig;
				}
			}
			Drop(args[2]);
			break;
		}
	case OP_SDLRANGE:
	{
		s_code_word_t v = Rel(-1);
		if (v == UNDEFINED) SDLerror(ERR_UNDEFVAL);
		switch (args[0])
		{
		case RO_IN:
			TOS = (v>=args[1] && v<=args[2]);
			break;
		case RO_NONE:
		case RO_EQU:
			TOS = (v==args[2]);
			break;
		case RO_NEQ:
			TOS = (v!=args[2]);
			break;
		case RO_LE:
			TOS = (v<args[2]);
			break;
		case RO_LEQ:
			TOS = (v<=args[2]);
			break;
		case RO_GT:
			TOS = (v>args[2]);
			break;
		case RO_GTQ:
			TOS = (v>=args[2]);
			break;
		}
		break;
	}
	case OP_SDLRETURN:
		TraceMgr.LogDestroy(this);
		rtn = DONE_RETURN;
		break;
	case OP_SDLSTOP:
		TraceMgr.LogDestroy(this);
		rtn = DONE_STOP;
		break;
	case OP_SDLTRANS: assert(0);				break;
	case OP_SETTIMER:
		{
			s_code_cast_t cv;
			Pop(cv.iv);
			(void)GetActive(1, args[0], Addr(SP()), args[1]);
			(void)Enqueue(args[0], Addr(SP()-args[1]+1), args[1],
					PId, cv.rv);
			Drop(args[1]);
			break;
		}
	case OP_SIMPLEASSIGN:
	{
		s_code_word_t val = Pop();
		Abs(Pop(), val);
		break;
	}
	case OP_SIMPLEVALUE:Push(Abs(Pop()));break;
	case OP_SUBTRACT: Drop();	Top(vTOS() - vTOS1());	break;
	case OP_TESTRANGE:
		if (!TOS) p += args[0];
		else Drop(2);				break;
	case OP_TESTTIMER:
	{
		int rtn = GetActive(0, args[0], Addr(SP()), args[1]);
		Drop(args[1]);
		Push(rtn);
		break;
	}
	case OP_VALUE:
	{
		s_code_word_t length 	= args[0];
		s_code_word_t addr	= Pop();
		while (length--)
			Push(Abs(addr++));
		break;
	}
	case OP_VARIABLE:
		// get address of variable
		Push(args[1]+Chain(args[0]));
		break;
	case OP_VARPARAM:
		/* Get address of bound variable */
		Push(Abs(args[1]+Chain(args[0])));
		break;
	case OP_WRITE:
	{
		s_code_word_t v = Pop();
		if (v==UNDEFINED)
			fprintf(stderr,"### UNDEFINED\n");
		else if (args[0])
			fprintf(stderr,"### %f\n", *((float *)&v));
		else
			fprintf(stderr,"### %ld\n", (long)v);
		break;
	}
	case OP_XOR:	Drop(); 	Top(vTOS() ^ vTOS1());	break;
	}
	return rtn;
}

void s_process_t::Enqueue(signal_t *sig)
{
	if (port==NULL || port->timestamp > sig->timestamp) // put on front
	{
		if (port) sig->next = port;
		port = sig;
	}
	else
	{
		// insert in queue according to timestamp
		signal_t *s = port, *last = NULL;
		while (s && s->timestamp <= sig->timestamp)
		{
			last = s;
			s  = s->next;
		}
		sig->next = last->next;
		last->next = sig;
	}
}

signal_t *s_process_t::Enqueue(s_code_word_t sig, s_code_word_t *args,
		s_code_word_t argLen, int sender, s_code_real_t timestamp)
{
	signal_t *rtn = new signal_t(sig, argLen, args, sender, timestamp, NULL);
	assert(rtn);
	Enqueue(rtn);
	return rtn;
}

int s_process_t::GetActive(int mustDel, s_code_word_t sig,
		s_code_word_t *args, s_code_word_t argLen)
{
	signal_t *s = port, *last = NULL;
	while (s)
	{
		if (s->idx == sig && s->arglen==argLen &&
			memcmp(s->args, args, argLen*sizeof(s_code_word_t))==0)
		{
			if (mustDel)
			{
				if (last) last->next = s->next;
				else port = s->next;
				delete s;
			}
			return 1;
		}
		last = s;
		s = s->next;
	}
	return 0;
}

void s_process_t::TestSignal(s_code_word_t sig, s_code_word_t *save)
{
	s_code_real_t now = Scheduler->GetTime();
	signal_t *s = port, *last = NULL;
	int pos = 1;
	while (s)
	{
		int idx = s->idx;
		if (idx == sig)
		{
			s->canHandle = 1;
			if (Scheduler->Evaluating())
			{
				if (s->timestamp > now)
				{
					s_code_cast_t cv;
					cv.rv = s->timestamp - now;
					Push(cv.iv);
				}
				else Push(pos);
			}
			else
			{
				assert (s->timestamp <= now);
				TraceMgr.LogInput(this, s);
				for (int i=0; i<s->arglen; i++)
					Push(s->args[i]);
				if (last)
					last->next = s->next;
				else
					port = s->next;
				s_process_t *pr = this;
				while (pr->caller) pr = pr->caller;
				pr->Sender = s->sender;
				delete s; // WOT ABOUT SENDER??
			}
			return;
		}
		else if (!IsElt(save, idx)) break;
		s->canHandle = 1;
		last = s;
		s = s->next;
		pos++;
	}
	assert (Scheduler->Evaluating());
	Push(0);
}

void s_process_t::ShowInfo()
{
	char buff[100];

	SetScope();

	// Show process identity

	sprintf(buff,"P%s %12s (%-2d)", Identity(),
		StateIndex2Name(*stateTbl, state), state);
	Scheduler->Write(buff);

	// show input port queue contents

	if (port)
	{
		strcpy(buff, "Port: ");
		port->Print(buff+strlen(buff));
		Scheduler->Write(buff);
		signal_t *sig = port->next;
		while (sig)
		{
			strcpy(buff, "      ");
			sig->Print(buff+strlen(buff));
			Scheduler->Write(buff);
			sig = sig->next;
		}
	}

	// show parameters

	if (_class == Q_PROCESS)
	{
		lh_list_node_t<process_formal_param_t> *pdef =
			GetProcsDefP(ASTloc)->params.front();
		while (pdef)
		{
			data_def_t *dd = (data_def_t *)ScopeStack->dequalify(&pdef->info.sort, Q_TYPE);
			lh_list_node_t<process_param_name_t> *pn
				= pdef->info.names.front();
			while (pn)
			{
				PrintValue(dd, (char *)pn->info.name(),
					Addr(b + pn->info.offset));
				pn = pn->next();
			}
			pdef = pdef->next();
		}
	}
	else
	{
		lh_list_node_t<procedure_formal_param_t> *pdef =
			GetProcdDefP(ASTloc)->params.front();
		while (pdef)
		{
			data_def_t *dd = (data_def_t *)ScopeStack->dequalify(&pdef->info.sort, Q_TYPE);
			lh_list_node_t<procedure_param_name_t> *pn
				= pdef->info.names.front();
			while (pn)
			{
				if (pdef->info.isValue)
					PrintValue(dd, (char *)pn->info.name(),
						Addr(b + pn->info.offset));
				else
					PrintValue(dd, (char *)pn->info.name(),
						Addr(Abs(b + pn->info.offset)));
				pn = pn->next();
			}
			pdef = pdef->next();
		}
	}

	// show variables

	lh_list_node_t<variable_def_t> *vdef =
		_class == Q_PROCESS ?
			GetProcsDefP(ASTloc)->variables.front() :
			GetProcdDefP(ASTloc)->variables.front();
	while (vdef)
	{
		lh_list_node_t<variable_decl_t> *vdec = vdef->info.decls.front();
		while (vdec)
		{
			data_def_t *dd = (data_def_t *)ScopeStack->dequalify(&vdec->info.sort, Q_TYPE);
			lh_list_node_t<variable_name_t> *vnm = vdec->info.names.front();
			while (vnm)
			{
				PrintValue(dd, (char *)vnm->info.name(),
					Addr(b + vnm->info.offset));
				vnm = vnm->next();
			}
			vdec = vdec->next();
		}
		vdef = vdef->next();
	}

	// show transition table

	for (int i = 1; i <= transCnt; i++)
		transTbl[i].ShowInfo();
}

void s_process_t::UpdateInfo()
{
	transTbl[selected].UpdateInfo();
}

s_process_t::s_process_t(s_code_word_t p_in, s_code_word_t *args_in,
			exec_result_t &xu, long PId_in, int level,
			s_process_t *parent, s_process_stack_t *stk_in,
			s_code_word_t s_in, s_code_word_t b_in,
			signal_t *port_in)
{
	char buff[80];
	int i;
	// save the useful arguments of the OP_SDLMODULE instruction

	state = 9999;
	startp = p_in; // used to uniquely identify process type
	p = p_in + 1 + Code->GetNumArgs(OP_SDLMODULE);
	s_code_word_t *args = Code->GetAddr(p_in+1);
	ASTloc	= args[0];
	_class	= args[1];
	vars	= args[2];
	params	= args[3];
	transCnt= (args[4]>=0) ? args[4] : 0;
	PId = PId_in;
	calls = NULL;

	if (_class == Q_PROCEDURE)
	{
		assert(stk_in && parent);
		caller = parent;
		stack = stk_in;
		assert(s_in > 0);
		port = port_in;
		Set(s_in);
		// Make activation record
		b = b_in;
		Push(Chain(level)); // static link
		Push(b_in);
		Push(p);
		b = SP()-2;
	}
	else
	{
		assert(stk_in == NULL && port_in==NULL && s_in == -1);
		port = NULL;
		caller = NULL;
		Sender = UNDEFINED;
		if (parent)
		{
			s_process_t *pr = parent;
			while (pr->caller) pr = pr->caller;
			PPId = pr->PId;
			pr->Offspring = PId;
		}
		else PPId = UNDEFINED;
		Offspring = UNDEFINED;
		stack = new s_process_stack_t;
		assert(stack);
		i = params;
		while (i--) Abs(i, args_in[i]); /* get init params */
		Set(params + (s_code_word_t)vars + 2);
		Abs(b = params, params); /* set stack[b] to point to itself */
	}
	
	// make scope table

	scopeLvl = args[7];
	scopeTypes = new typeclass_t[scopeLvl];
	assert(scopeTypes);
	scopeOffsets = new heap_ptr_t[scopeLvl];
	assert(scopeOffsets);
	for (i = 0; i < scopeLvl; i++)
	{
		assert(args[8+3*i] == OP_SETSCOPE);
		scopeTypes[i] = (typeclass_t)args[9+3*i];
		scopeOffsets[i] = args[10+3*i];
	}

	// Get the name and state name table.

	if (_class == Q_PROCESS)
	{
		process_def_t *p = GetProcsDefP(ASTloc);
		name = (char *)(p->name());
		numStates = p->numStates;
		stateTbl = &p->stateTbl;
	}
	else if (_class == Q_PROCEDURE)
	{
		procedure_def_t *p = GetProcdDefP(ASTloc);
		name = (char *)(p->name());
		numStates = p->numStates;
		stateTbl = &p->stateTbl;
	}
	else if (_class == Q_BLOCK)
	{
		name = (char *)GetBlkDefP(ASTloc)->name();
		numStates = 0;
	}
	else
	{
		assert(_class == Q_SYSTEM);
		name = (char *)GetSysDefP(ASTloc)->name();
		numStates = 0;
	}
	TraceMgr.LogCreate(this);

	// Build the trans table; entry 0 is the init trans

	transTbl = new s_transition_t[transCnt+1];
	assert(transTbl);
	transTbl[0].Init(0, p+args[5]);
	if (transCnt) transTbl[1].Init(1, p+args[6]);

	// skip thru the code to get the rest...

	for (int t = 2; t <= transCnt; t++)
		transTbl[t].Init(t, transTbl[t-1].Next());

	// clear the export table

	for (i = 0; i < MAX_EXPORTS; i++)
	{
		exports[i].offset = 0;
		exports[i].size = 0;
		exports[i].valptr = NULL;
	}

	// Execute the initial transition

	p = transTbl[0].Start();
	do xu = ExecOp();
	while (xu != DONE_TRANS && xu != DONE_STOP && xu != DONE_RETURN);
	selected = -1;
}

int s_process_t::EvalClauses(s_code_real_t &minDelay)
{
#ifdef DEBUG
	char buff[80];
	if (debug)
	{
		sprintf(buff, "  Evaluating clauses in process %s %d (%d)",
				name, PId, ASTloc);
		Scheduler->Write(buff);
	}
#endif
	signal_t *sig = port;
retry:
	while (sig)
	{
		if (!sig->hasArrived && sig->timestamp <= Scheduler->GetTime())
		{
			sig->hasArrived = 1;
			TraceMgr.LogArrive(this, sig);
		}
		sig->canHandle = 0;
		sig = sig->next;
		// If a signal that has arrived is saved or mentioned
		// in an input in the current state, we set canHandle.
		// At the end, if no transition is found, we see if there
		// are any signals with canHandle==0, trash them, and
		// try again (this is the SDL implicit transition).
	}
	for (int t=1; t <= transCnt; t++)
	{
		int s = SP();
		// if we have already selected a transition of 
		// a higher priority, ignore this
		if (selected>=0 && selPri <= transTbl[t].Priority())
			continue;

#ifdef DEBUG
		sprintf(buff, "    Transition %d \0", t);
#endif

		// are we in the right state?

		if (!IsElt(transTbl[t].FromStates(), state))
		{
#ifdef DEBUG
			sprintf(buff+strlen(buff), "doesn't admit current state %s",
				StateIndex2Name(*stateTbl, state));
#endif
			goto done;
		}

		// evaluate the INPUT
		p = transTbl[t].When();
		if (p>=0)
		{
			while (ExecOp() != DONE_CLAUSE);
			s_code_word_t result;
			Pop(result);
			if (result < 0 || result > 999)	// hack!!
			{
				s_code_cast_t cv;
				cv.iv = result;
				// only in the future!
				if (minDelay<0 || minDelay > cv.rv)
					minDelay = cv.rv;
#ifdef DEBUG
				sprintf(buff+strlen(buff), "can only happen in %f time units",
					 (float)cv.rv);
#endif
				goto done;
			}
			if (result==0 || (selected>=0 && selPos < result))
			{
#ifdef DEBUG
				if (result==0)
					strcat(buff, "fails its input test");
				else
					strcat(buff, "is further down queue than selected");
#endif
				goto done;
			}
			selPos = result;
		}
		else selPos = 0;

		// evaluate the PROVIDED - note not in scope of
		// input, unlike Estelle
		assert(s == SP());
		p = transTbl[t].Prov();
		if (p>=0)
		{
			while (ExecOp() != DONE_CLAUSE);
			s_code_word_t result;
			Pop(result);
			if (result == 0)
			{
#ifdef DEBUG
				strcat(buff, "fails its provided clause");
#endif
				goto done;
			}
		}
#ifdef DEBUG
		strcat(buff, "is admissible!");
#endif
		selected = t;
		selPri = transTbl[t].Priority();
		selP = transTbl[t].When();
		if (selP==-1) selP = transTbl[t].Start();
	done:
#ifdef DEBUG
		if (debug) Scheduler->Write(buff);
#endif
		assert(s == SP());
	}
	if (selected < 0/* && minDelay < 0.*/)
	{
		// See if we can discard a signal and retry
		signal_t *last = NULL;
		sig = port;
		while (sig)
		{
			if (!sig->hasArrived && minDelay>=0.) break;
			if (sig->canHandle == 0)
			{
				if (last==NULL) port = port->next;
				else last->next = sig->next;
				TraceMgr.LogImplicit(this, sig);
				delete sig;
				goto retry;
			}
			last = sig;
			sig = sig->next;
		}
	}
	return (selected>=0) ? 1 : 0;
}

void s_process_t::SetScope()
{
	ScopeStack->SetScope(scopeTypes, scopeOffsets, scopeLvl);
}

void s_process_t::ShowTrans(int tr)
{
	if (tr>0 && tr<=transCnt)
	{
		transition_t *tp = GetTransP(transTbl[tr].ASTloc());
		cout << *tp;
	}
}

s_process_t::~s_process_t()
{
	if (_class == Q_PROCESS)
	{
		while (port)
		{
			signal_t *s = port;
			port = port->next;
			delete s;
		}
	}
	if (_class == Q_PROCEDURE)
	{
		caller->Drop(params);
		caller->port = port;
	}
	else delete stack;
	delete [] scopeTypes;
	delete [] scopeOffsets;
	delete [] transTbl;
}

//----------------------------------------------------------
// Scheduler

static void showHelp()
{
	cout << "Command groups are:\n\n";
	cout << "T - Transition code\n";
	cout << "P - Process table\n";
	cout << "C - Channel table\n";
	cout << "M - Trace message type selection\n";
	cout << "A - Accept filters\n";
	cout << "R - Reject filters\n";
	cout << "X - Execute\n";
	cout << "V - Toggle verbose process identifiers on/off\n";
	cout << "D - Toggle clause evaluation trace on/off\n\n";
	cout << "! - Execute an external DOS/UNIX command\n";
	cout << "Q - Quit\n\n";
	cout << "Enter ?? followed by the appropriate letter for more help" << endl;
}

static void showTHelp()
{
	cout << "The T command lets you see the body code of a transition\n";
	cout << "Enter ?T followed by the process ID and transition index" << endl;
}

static void showCHelp()
{
	cout << "The ?C command shows the channel table.\n";
	cout << "Use `C <chan num> <delay> <reliability>' to change the\n";
	cout << "attributes of a channel." << endl;
}

static void showPHelp()
{
	cout << "The ?P command shows the process IDs of currently active processes.\n";
	cout << "Enter ?P followed by a process ID for info on a specific process." << endl;
}

static void showMHelp()
{
	cout << "?M shows the current enables message types.\n";
	cout << "M<types> enables the specified types.\n";
	cout << "-M<types> disables the specified types.\n";
	cout << "The type codes are:\n";
	cout << "\tT - implicit transition executions\n";
	cout << "\tS - state changes\n";
	cout << "\tX - s-code instructions\n";
	cout << "\tF - transition firings/executions\n";
	cout << "\tO - signal outputs\n";
	cout << "\tA - signal arrivals\n";
	cout << "\tI - signal inputs\n";
	cout << "\tL - line number changes\n";
	cout << "\tC - clause state changes (not implemented yet)\n";
	cout << "\tU - signal losses\n";
	cout << "\tP - process creation/deletion" << endl;
}

static void showAHelp()
{
	cout << "?A shows the current accept filters.\n";
	cout << "[<count>] A [<time> [<class> [<Pid> [<fld1> [<fld2>]]]]] adds a filter\n";
	cout << "-A [<time> [<class> [<Pid> [<fld1> [<fld2>]]]]] deletes a filter\n";
	cout << "Where the class specifies the message type that the filter applies\n";
	cout << "to and is one of 'F', 'O', 'U', 'A', 'I', 'C', 'L', 'P', 'S', or 'T'" << endl;
}

static void showRHelp()
{
	cout << "?R shows the current reject filters.\n";
	cout << "[<count>] R [<time> [<class> [<Pid> [<fld1> [<fld2>]]]]] adds a filter\n";
	cout << "-R [<time> [<class> [<Pid> [<fld1> [<fld2>]]]]] deletes a filter\n";
	cout << "Where the class specifies the message type that the filter applies\n";
	cout << "to and is one of 'F', 'O', 'U', 'A', 'I', 'C', 'L', 'P', 'S', or 'T'" << endl;
}

static void showXHelp()
{
	cout << "[<count>] X [<unit>] executes the system for the specified\n";
	cout << "number of units. If the count is omitted, the last count is\n";
	cout << "used; similarly for the execution unit, which can be any one of:\n";
	cout << "\tF - transition firings         I - scheduler iterations\n";
	cout << "\tT - time units                 C - scheduler choices\n";
	cout << "\tX - s-code ops                 L - line number changes\n";
	cout << "\tC - scheduler choices (N/I)    M - trace messages\n";
	cout << endl;
}

int scheduler_t::MyReader(char *buf)
{
	static int first = 1;
	if (first)
	{
		cout << "Enter `q' to quit" << endl;
		first = 0;
	}
	cout << '\n' << Scheduler->GetTime() << '>' << ' ' << flush;
	//cin.get(buf, 80, '\n');
	//cin.ignore();
   fgets(buf, 80, stdin); buf[strlen(buf)-1] = 0; // strip \n
	cout << endl;
	if ((buf[0]=='q' || buf[0]=='Q') && buf[1]<32)
	{
		Abort = 1;
		return -1;
	}
	else if (strcmp(buf,"?")==0)
	{
		buf[0] = 0;
		showHelp();
		return 0;
	}
	else return strlen(buf);
}

void scheduler_t::MyWriter(char *buf)
{
	cout << buf << endl;
}

void scheduler_t::Trace(char *msg)
{
	(*writer)(msg);
	if (execType==X_TRACE) CheckExecCount(1);
}

void scheduler_t::Write(char *msg)
{
	(*writer)(msg);
}

scheduler_t::scheduler_t(read_cmd_fn_t reader_in, write_cmd_fn_t writer_in)
	: shows(0)
{
 	reader = (reader_in == NULL) ? MyReader : reader_in;
 	writer = (writer_in == NULL) ? MyWriter : writer_in;
	for (int i = 0; i< MAX_INSTANCES; i++)
		processes[i] = NULL;
	PId = 1;
	globalTime = 0.;
	busyEvaluating = 0;
	active = 0;
}

s_process_t *scheduler_t::NewProcess(s_code_word_t p_in,
	s_code_word_t *params_in, int level, int maxinst)
{
	int cnt = 0;
	for (int i = 0; i< MAX_INSTANCES; i++)
	{
		if (processes[i] && processes[i]->startp == p_in)
			cnt++;
	}
	if (cnt>=maxinst) return NULL;
	exec_result_t xu;
	s_process_t *p = new s_process_t(p_in, params_in, xu, PId++, level);
	assert(p);
	if (xu == DONE_STOP)
	{
		Terminate(p);
		return NULL;
	}
	for (i = 0; i< MAX_INSTANCES; i++)
	{
		if (processes[i] == NULL)
			return processes[i] = p;
	}
	SDLfatal(ERR_PROCTBLFULL);
	return NULL;
}

s_process_t *scheduler_t::NewProcedure(s_process_t *caller,
	s_code_word_t p_in, int level)
{
	for (int i = 0; i< MAX_INSTANCES; i++)
	{
		if (processes[i] == NULL)
		{
			exec_result_t xu;
			s_process_t *p = new s_process_t(
				p_in, NULL, xu, PId++, level,
				caller, caller->Stack(), caller->SP(),
				caller->Link(), caller->Port());
			assert(p);
			if (xu != DONE_RETURN)
				processes[i] = p;
			else
				Terminate(p);
			return processes[i];
		}
	}
	SDLfatal(ERR_PROCTBLFULL);
	return NULL;
}

scheduler_t::~scheduler_t()
{
	for (int i = 0; i< MAX_INSTANCES; i++)
		if (processes[i])
			delete processes[i];
}

int scheduler_t::findProcess(s_code_word_t destPId, s_code_word_t pType,
			s_process_t* &destP)
{
	int i = 0, cnt = 0;
	while (i < MAX_INSTANCES)
	{
		if (processes[i] &&
		    (pType==-1 || processes[i]->ASTloc == pType) &&
		    (destPId == UNDEFINED || destPId == processes[i]->PId))
		{
			destP = processes[i];
			cnt++;
		}
		i++;
	}
	return cnt;
}

s_code_word_t *scheduler_t::FindExported(s_code_word_t AST,
	s_code_word_t pid, s_code_word_t offset, s_code_word_t size)
{
	for (int p = 0; p<MAX_INSTANCES; p++)
	{
		if (processes[p]==NULL) continue;
		if (processes[p]->ASTloc != AST ||
		    processes[p]->PId != pid) continue;
		// got process; find export
		for (int e = 0; e < MAX_EXPORTS; e++)
		{
			if (processes[p]->exports[e].valptr == NULL)
				break;
			if (processes[p]->exports[e].offset != offset)
				continue;
			if (processes[p]->exports[e].size != size)
				SDLerror(ERR_IMPORTSZ);
			// got it!
			assert(processes[p]->exports[e].valptr);
			return processes[p]->exports[e].valptr;
		}
	}
	SDLerror(ERR_NOEXPORT);
	return NULL;
}

int scheduler_t::EvalClauses(s_code_real_t &minDelay)
{
	int cnt = 0;
	busyEvaluating = 1;
#ifdef DEBUG
	if (debug) Write("\nClause evaluation:\n");
#endif
	for (int p = 0; p<MAX_INSTANCES; p++)
	{
		if (processes[p]==NULL) continue;
		processes[p]->selected = -1;
		if (processes[p]->calls) continue; // in a procedure
		cnt += processes[p]->EvalClauses(minDelay);
	}
#ifdef DEBUG
	if (debug) Write("Finished clause evaluation:\n");
#endif
	busyEvaluating = 0;
	return cnt;
}

// loop reading commands and performing actions till Quit

int scheduler_t::HandleCommand()
{
	static int lastCnt=1;
	char cmdBuf[80];
	unsigned short tmp_shows;
	if (Abort) return -1;
	for (;;)
	{
	 	if ( ((*reader)(cmdBuf)) < 0)
		{
			execCnt = 1;
			execType = X_TRANS; // force quit if not called
					// from Scheduler->Execute
			return -1;
		}
		int list = 0, del = 0, cnt = 0;
		int pos = 0;
		switch (cmdBuf[0])
		{
		case '\0':
			execCnt = lastCnt;
			return 0;
		case '!':
			system(cmdBuf+1);
			continue;
		case '?':
			list = 1;
			break;
		case '-':
			del = 1;
			break;
		}
		if (list || del) pos++;
		if (cmdBuf[pos]>='0' && cmdBuf[pos]<='9')
		{
			while (cmdBuf[pos]>='0' && cmdBuf[pos]<='9')
			{
				cnt = 10*cnt + cmdBuf[pos] - '0';
				pos++;
			}
		}
		switch (cmdBuf[pos])
		{
		case '\0':
			if (list) showHelp();
			break;
		case '?':
			switch(toupper(cmdBuf[pos+1]))
			{
			case 'T': showTHelp();	break;
			case 'C': showCHelp();	break;
			case 'P': showPHelp();	break;
			case 'M': showMHelp();	break;
			case 'A': showAHelp();	break;
			case 'R': showRHelp();	break;
			case 'X': showXHelp();	break;
			}
			break;
#ifdef DEBUG
		case 'd': // debug toggle
		case 'D':
			debug = 1-debug;
			break;
#endif
		case 'p':
		case 'P': // list processes
			if (!list) break;
			do pos++; while (cmdBuf[pos]==' ');
			if (cmdBuf[pos]>'0' && cmdBuf[pos]<'9')
			{
				s_process_t *p;
				int destPId = atoi(cmdBuf+pos);
				if (findProcess(destPId, -1, p)==1)
				{
					p->ShowInfo();
					if (processes[active])
						processes[active]->SetScope();
				}
				else Write("No such process!");
			}
			else for (int i=0;i<MAX_INSTANCES;i++)
			{
				if (processes[i])
				{
					char buff[80];
					sprintf(buff,"P%s",
						processes[i]->Identity());
					Write(buff);
				}
			}
			break;
		case 'c':
		case 'C': // channels
			if (list)
			{
				int c;
				if (sscanf(cmdBuf+pos+1,"%d", &c)==1)
					ChanMgr.ShowCode(c);
				else
					ChanMgr.Show();
			}
			else
			{
				int c, d, r;
				if (sscanf(cmdBuf+pos+1,"%d %d %d", &c, &d, &r)==3)
					ChanMgr.Edit(c-1,d,r);
				else
					ChanMgr.Show();
			}
			break;
		case 'x': // execute
		case 'X': // execute
			do pos++; while (cmdBuf[pos]==' ');
			execCnt = 0;
			while (cmdBuf[pos]>='0' && cmdBuf[pos]<='9')
				execCnt = execCnt * 10 + cmdBuf[pos++]-'0';
			if (execCnt==0) execCnt = lastCnt;
			else lastCnt = execCnt;
			if (cmdBuf[pos]==' ')
				do pos++; while (cmdBuf[pos]==' ');
			switch(cmdBuf[pos])
			{
			case 'f': 
			case 'F':
				execType = X_TRANS;
				break;
			case 'x': 
			case 'X':
				execType = X_SCODE_OP;
				break;
			case 'l': 
			case 'L':
				execType = X_LINE;
				break;
			case 'i': 
			case 'I':
				execType = X_ITER;
				break;
			case 't': 
			case 'T':
				execType = X_TICK;
				break;
			case 'c': 
			case 'C':
				execType = X_CHOICE;
				break;
			case 'm': 
			case 'M': // trace message
				execType = X_TRACE;
				break;
			default:
				Write("Bad execution unit");
				continue;
			case '\0':
				break; // no change
			}
			return 0; // the only way out!
		case 't': // display transition code, unfortunately
		case 'T': // directly to an ostream...
			if (!list) break;
			do pos++; while (cmdBuf[pos]==' ');
			int pi = 0, tr = 0;
			while (cmdBuf[pos]>='0' && cmdBuf[pos]<='9')
				pi = pi * 10 + cmdBuf[pos++]-'0';
			do pos++; while (cmdBuf[pos]==' ');
			while (cmdBuf[pos]>='0' && cmdBuf[pos]<='9')
				tr = tr * 10 + cmdBuf[pos++]-'0';
			if (pi>=0 && pi < MAX_INSTANCES)
			{
				s_process_t *p;
				if (findProcess(pi, -1, p)==1)
					p->ShowTrans(tr);
			}
			break;
		case 'm': // message traces
		case 'M':
			tmp_shows = 0;
			while (cmdBuf[++pos])
			{
				switch(cmdBuf[pos])
				{
				case 't':
				case 'T':
					tmp_shows |= TRC_IMPLICIT;
					break;
				case 's':
				case 'S':
					tmp_shows |= TRC_STATE;
					break;
				case 'x':
				case 'X':
					tmp_shows |= TRC_SCODE;
					break;
				case 'f':
				case 'F':
					tmp_shows |= TRC_FIRE;
					break;
				case 'o':
				case 'O':
					tmp_shows |= TRC_OUTPUT;
					break;
				case 'a':
				case 'A':
					tmp_shows |= TRC_ARRIVE;
					break;
				case 'i':
				case 'I':
					tmp_shows |= TRC_INPUT;
					break;
				case 'c':
				case 'C':
					tmp_shows |= TRC_CLAUSE;
					break;
				case 'l':
				case 'L':
					tmp_shows |= TRC_LINE;
					break;
				case 'u':
				case 'U':
					tmp_shows |= TRC_LOSS;
					break;
				case 'p':
				case 'P':
					tmp_shows |= TRC_PROCESS;
					break;
				}
			}
			if (tmp_shows == 0)
				tmp_shows = TRC_ALL;
			if (del)
				shows &= ~tmp_shows;
			else if (list)
			{
				int p = 5;
				char buff[80];
				strcpy(buff,"Show ");
				if (shows & TRC_FIRE)	buff[p++] = 'f';
				if (shows & TRC_OUTPUT)	buff[p++] = 'o';
				if (shows & TRC_ARRIVE)	buff[p++] = 'a';
				if (shows & TRC_INPUT)	buff[p++] = 'i';
				if (shows & TRC_CLAUSE)	buff[p++] = 'c';
				if (shows & TRC_LINE)	buff[p++] = 'l';
				if (shows & TRC_PROCESS)buff[p++] = 'p';
				if (shows & TRC_SCODE)  buff[p++] = 'x';
				if (shows & TRC_STATE)  buff[p++] = 's';
				if (shows & TRC_LOSS)   buff[p++] = 'u';
				if (shows & TRC_IMPLICIT)buff[p++] = 't';
				buff[p] = '\0';
				Write(buff);
			}
			else shows |= tmp_shows;
			TraceMgr.Set(shows);
			break;
		case 'a': // accept filter
		case 'A': // accept filter
			if (list) TraceMgr.Show(ACCEPT_FILTER);
			else if (del) TraceMgr.Delete(ACCEPT_FILTER, cmdBuf+pos+1);
			else TraceMgr.Add(ACCEPT_FILTER, cmdBuf+pos+1, cnt);
			break;
		case 'r': // reject filter
		case 'R': // reject filter
			if (list) TraceMgr.Show(REJECT_FILTER);
			else if (del) TraceMgr.Delete(REJECT_FILTER, cmdBuf+pos+1);
			else TraceMgr.Add(REJECT_FILTER, cmdBuf+pos+1, cnt);
			break;
		case 'v':
		case 'V':
			verbose = 1 - verbose;
			break;
		}
	}
}

int scheduler_t::CheckExecCount(int cnt)
{
	execCnt -= cnt;
	if (execCnt <= 0)
	 	return HandleCommand();
	else return 0;
}

// hackish but simple common code to get process identity for messages

static char pIdentBuf[2][40];
static int pIdBufNow = 0;

char *s_process_t::Identity()
{
	char *rtn, tmpBuf[80];
	if (verbose)
	{
		sprintf(tmpBuf, "%-3ld(%6ld:%.14s)", PId, (long)ASTloc, name);
		sprintf(rtn = pIdentBuf[pIdBufNow],"%-26.26s", tmpBuf);
	}
	else sprintf(rtn = pIdentBuf[pIdBufNow], "%-3ld", PId);
	pIdBufNow = 1 - pIdBufNow; // use other buffer next; allows 2
					// concurrent uses
	return rtn;
}

#ifdef __MSDOS__
#include <conio.h>
#endif

void scheduler_t::Terminate(s_process_t *p)
{
	for (int i = 0; i < MAX_INSTANCES; i++)
	{
		if (processes[i] && processes[i]->PPId == p->PId)
			processes[i]->PPId = UNDEFINED;
	}
	delete p;
}

void scheduler_t::Terminate(int pi)
{
	Terminate(processes[pi]);
	processes[pi] = NULL;
}

void scheduler_t::Execute()
{
	char cmdBuf[80];
	unsigned short tmp_shows;
	s_code_real_t minDelay;
#ifdef __MSDOS__
	randomize();
#endif
	execType = X_ITER;
	execCnt = 1;
	TraceMgr.Set(shows = TRC_PROCESS);

	(void)NewProcess(0,NULL,0,1); // create initial processes

 	if (HandleCommand()<0) goto abort; // get first command

	for (;;)
	{
		minDelay = -1.;
 		if (!verbose) TraceMgr.Set(shows & ~TRC_SCODE);
		while (EvalClauses(minDelay)==0)
		{
			if (minDelay<0.)
			{
				SDLerror(ERR_DEADLOCK);
				if (CheckExecCount(execCnt)<0)
						goto abort;
				break;
			}
			globalTime+=minDelay;
			if (execType==X_TICK)
				if (CheckExecCount((int)minDelay)<0)
					goto abort;
		}
 		TraceMgr.Set(shows);
		for (int i = 0; i < MAX_INSTANCES; i++)
		{
			if (processes[i] && processes[i]->selected>=0)
			{
				exec_result_t xu;
				processes[i]->p = processes[i]->selP;
				TraceMgr.LogFire(processes[i]);
				processes[i]->UpdateInfo();
				int s = processes[i]->SP();
				active = i;
				processes[i]->SetScope();
				do
				{
					xu = processes[i]->ExecOp();
					if (execType==X_SCODE_OP)
						if (CheckExecCount(1) < 0)
							goto abort;
					if (xu == DONE_NEWLINE && execType==X_LINE)
						if (CheckExecCount(1) < 0)
							goto abort;
					if (xu == DONE_RETURN)
					{
						// switch to parent to
						// complete transition
						s_process_t *p = processes[i]->caller;
						assert(p);
						// delete procedure
						Terminate(i);
						// activate caller
						p->calls = NULL;
						for (i = 0; i < MAX_INSTANCES; i++)
							if (processes[i] == p)
								break;
						assert(i < MAX_INSTANCES);
						active = i;
						processes[i]->SetScope();
					        s = processes[i]->SP();
					}
				}
				while (xu != DONE_TRANS && xu != DONE_STOP &&
					xu != DONE_CALL);
				if (xu == DONE_STOP)
					Terminate(i);
				else if (xu == DONE_TRANS)
					assert(s == processes[i]->SP());
#ifdef __MSDOS__
				if (kbhit())
					if (getch()==27)
						if (CheckExecCount(execCnt) < 0)
							goto abort;
#endif
				if (execType==X_TRANS)
					if (CheckExecCount(1) < 0)
						goto abort;
			}
		}
		if (execType==X_ITER)
			if (CheckExecCount(1) < 0)
				goto abort;
	}
abort:
	Write("Bye!");
}

