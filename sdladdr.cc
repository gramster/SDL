/*
 * SDL Abstract Syntax Tree to E-Code translator
 *
 * Written by Graham Wheeler
 * Data Network Architecture Laboratory
 * Department of Computer Science
 * University of Cape Town
 * 
 * (c) 1994 Graham Wheeler and the Data Network Architectures Laboratory
 * All Rights Reserved
 *
 * Last modified: 8-6-94
 *
 */

#pragma hdrfile "PASS2.SYM"
#include "sdlast.h"
#include "sdlc2.h"

static void printSigSet(signalset_t &s)
{
	int i, j=-1;
	cout << "{ ";
	for (i=0;i<MAX_SIGNALS;i++)
	{
		if ((s.bitmask[i/SWORD_SIZE] & (1<<(i%SWORD_SIZE))) != 0)
		{
			if (j>=0) cout << ',';
			j = i;
			cout << i;
		}
	}
	cout << " }";
}

static void printSigList(signallist_t* &s)
{
	printSigSet(s->sigset);
}

void printTables()
{
	Router->Print(stdout);
}
//---------------------------------------------------------------------
// Allocation of addresses/offsets to objects. In addition, signals
// are alocated system-wide unique integer IDs, and signallists are
// allocated values representing sets of these.
//---------------------------------------------------------------------

template<class T>
int AddressL(lh_list_t<T> &l, int n)
{
	lh_list_node_t<T> *p = l.front();
	while (p)
	{
		n = p->info.Address(n);
		p = p->next();
	}
	return n;
}

template<class T>
void Address2L(lh_list_t<T> &l)
{
	lh_list_node_t<T> *p = l.front();
	while (p)
	{
		p->info.Address2();
		p = p->next();
	}
}

int AddressL(lh_list_t<variable_decl_t>&, int);
int AddressL(lh_list_t<variable_def_t>&, int);
int AddressL(lh_list_t<procedure_def_t>&, int);
int AddressL(lh_list_t<process_def_t>&, int);
int AddressL(lh_list_t<block_def_t>&, int);
int AddressL(lh_list_t<fieldgrp_t>&, int);
int AddressL(lh_list_t<data_def_t>&, int);
int AddressL(lh_list_t<signal_def_t>&, int);
int AddressL(lh_list_t<timer_def_t>&, int);
int AddressL(lh_list_t<siglist_def_t>&, int);

static int nextSigIdx = 0;

int signal_def_t::Address(int n)
{
	idx = nextSigIdx++;
	signalTbl->Enter(idx, this);
	if (nextSigIdx==MAX_SIGNALS) 
		SDLerror(file, place,ERR_MAXSIGS);
	if (verbose)
		cout << "Allocated index " << idx << " to signal " << nm.name() << endl;
	return n;
}

int timer_def_t::Address(int n)
{
	idx = nextSigIdx++;
	signalTbl->Enter(idx, this);
	if (nextSigIdx==MAX_SIGNALS) 
		SDLerror(file, place,ERR_MAXSIGS);
	return n;
}

int signallist_t::Address(int n)
{
	lh_list_node_t<ident_t> *il = signals.front();
	while (il)
	{
		signal_def_t *sd = (signal_def_t *)ScopeStack->dequalify(&il->info, Q_SIGNAL);
		if (sd)
		{
			sigset.add(sd->idx);
//cout << "Adding " << sd->idx << " to signal list" << endl;
		}
		else SDLerror(il->info.file, il->info.place, ERR_BADSIGNAL, Id2Str(il->info));
		il = il->next();
	}
	il = signallists.front();
	while (il)
	{
		siglist_def_t *sd = (siglist_def_t *)ScopeStack->dequalify(&il->info, Q_SIGLIST);
		if (sd) sigset.add(sd->s.sigset);
		il = il->next();
	}
	return n;
}

int siglist_def_t::Address(int n)
{
	s.Address(0);
	return n;
}

int path_t::Address(int n)
{
	s.Address(0);
	return n;
}

int variable_decl_t::Address(int n)
{
	int typsz = sort.Size();
	lh_list_node_t<variable_name_t> *nl = names.front();
	while (nl)
	{
		if (verbose)
			cout << "Allocated address " << n << " to variable " << nl->info.name() << endl;
		nl->info.offset = n;
		n += typsz;
		nl = nl->next();
	}
	return n;
}

int variable_def_t::Address(int n)
{
	(void)n;
	return AddressL(decls, n);
}

int array_def_t::Address(int n)
{
	// Compute dimension and check that this is an integer
	expression_t *e = GetExprP(dimension);
	data_def_t *dd;
	if ((dval = e->EvalGround(dd))==UNDEFINED)
		SDLerror(file, place, ERR_GROUNDEXPR);
	if (dd != naturalType)
		SDLerror(file, place, ERR_NATEXPR);
	return n;
}

int fieldgrp_t::Address(int n)
{
	int typsz = sort.Size();
	lh_list_node_t<field_name_t> *nl = names.front();
	while (nl)
	{
		if (verbose)
			cout << "Allocated address " << n << " to field " << nl->info.name() << endl;
		nl->info.offset = n;
		n += typsz;
		nl = nl->next();
	}
	return n;
}

int struct_def_t::Address(int n)
{
	(void)AddressL(fieldgrps, 0);
	return n;
}

int data_def_t::Address(int n)
{
	ScopeStack->EnterScope(Q_TYPE,this);
	if (tag == STRUCT_TYP)
	{
		struct_def_t *sd = GetStrucDefP(contents);
		(void)sd->Address(0);
	}
	else if (tag == ARRAY_TYP)
	{
		array_def_t *ad = GetArrDefP(contents);
		(void)ad->Address(0);
	}
	ScopeStack->ExitScope();
	return n;
}

static void addLabel(name_t &nm, name_entry_t* Tbl, int &Cnt)
{
	if (nm.name()[0])
	{
		s_code_word_t idx = nm.index(), j;
		for (j=0;j<Cnt;j++)
			if (Tbl[j].nm == idx) break;
		if (j==Cnt)
		{
			// new label
			Tbl[j].nm = idx;
			Tbl[j].lbl = newlabel();
			Cnt++;
			assert(Cnt<=32);
		}
		else SDLerror(nm.file, nm.place,ERR_DUPLABEL, nm.name());
	}
}

static void addLabels(transition_t &t, name_entry_t* Tbl, int &Cnt)
{
	addLabel(t.label, Tbl, Cnt);
	lh_list_node_t<gnode_t> *gn = t.nodes.front();
	while (gn)
	{
		addLabel(gn->info.label, Tbl, Cnt);
		if (gn->info.type == N_DECISION)
		{
			decision_node_t *dn = (decision_node_t *)GetVoidP(gn->info.node);
			lh_list_node_t<subdecision_node_t> *sn = dn->answers.front();
			while (sn)
			{
				addLabels(sn->info.transition, Tbl, Cnt);
				sn = sn->next();
			}
		}
		gn = gn->next();
	}
}

int state_node_t::Address(int n, lh_list_t<name_entry_t> &nTbl, int &nCnt,
	name_entry_t* lTbl, int &lCnt)
{
	for (int i = 0;i<(MAX_STATES/SWORD_SIZE);i++)
		from[i] = isAsterisk ? ((u_s_code_word_t)~0) : 0;
	lh_list_node_t<name_t> *nm = states.front();
	while (nm)
	{
		int j, idx = nm->info.index();
		if ((j = StateName2Index(nTbl, idx)) < 0)
		{
			// we use a list as we want to allocate
			// from local heap (unlike label table)
			NEWNODE(name_entry_t, ne);
			ne->info.nm = idx;
			nTbl.append(ne);
			assert((nCnt == (-j-1)));
			j = nCnt++;
			assert(nCnt<=MAX_STATES); // size of set
		}
		if (isAsterisk)
			from[j/SWORD_SIZE] &= ~(1l << (j%SWORD_SIZE));
		else
			from[j/SWORD_SIZE] |= 1l << (j%SWORD_SIZE);
		nm = nm->next();
	}
	// we don't work out nextstates as a transition can have
	// different nextstates in a decision or join.

	// work out the save states
	lh_list_node_t<save_part_t> *sv = saves.front();
	save.clear();
	while (sv)
	{
		if (sv->info.isAsterisk)
			save.set();
		else
			save.add(sv->info.savesigs.sigset);
		sv = sv->next();
	}
	// Work out the labels 
	lh_list_node_t<input_part_t> *ip = inputs.front();
	while (ip)
	{
		addLabels(ip->info.transition, lTbl, lCnt);
		ip = ip->next();
	}
	lh_list_node_t<continuous_signal_t> *cs = csigs.front();
	while (cs)
	{
		addLabels(cs->info.transition, lTbl, lCnt);
		cs = cs->next();
	}
	return n;
}

int PdrParAddr2(int n, int sz, lh_list_node_t<procedure_param_name_t> *param)
{
	if (param->next())
		n = PdrParAddr2(n, sz, param->next());
	return param->info.offset = n-sz;
}

int PdrParAddr(int n, lh_list_node_t<procedure_formal_param_t>	*params)
{
	if (params->next())
		n = PdrParAddr(n, params->next());
	int typsz = params->info.isValue ? params->info.sort.Size() : 1;
	return PdrParAddr2(n, typsz, params->info.names.front());
}

int procedure_def_t::Address(int n)
{
	ScopeStack->EnterScope(Q_PROCEDURE,this);
	startLbl = newlabel(); // offset of start
	AddressL(types, 0);
	if (params.front())
		parSize = -PdrParAddr(0, params.front());
	else parSize = 0;
	varSize = AddressL(variables, VAR_START)-3;
	if (procedures)
	{
		lh_list_t<procedure_def_t> *pl = 
			(lh_list_t<procedure_def_t> *)GetVoidP(procedures);
		AddressL(*pl, 0);
	}
	nameStore->ClearNameMarks();
	lh_list_node_t<state_node_t> *sn = states.front();
	numStates = numLabels = 0;
	labelTbl = new name_entry_t[32];
	assert(labelTbl);
	while (sn)
	{
		sn->info.Address(0, stateTbl, numStates, labelTbl, numLabels);
		sn = sn->next();
	}
	ScopeStack->ExitScope();
	return n;
}

int PrcParAddr2(int n, int sz, lh_list_node_t<process_param_name_t> *param)
{
	if (param->next())
		n = PrcParAddr2(n, sz, param->next());
	return param->info.offset = n-sz;
}

int PrcParAddr(int n, lh_list_node_t<process_formal_param_t> *params)
{
	if (params->next())
		n = PrcParAddr(n, params->next());
	return PrcParAddr2(n, params->info.sort.Size(),
		params->info.names.front());
}

int process_def_t::Address(int n)
{
	ScopeStack->EnterScope(Q_PROCESS,this);
	startLbl = newlabel(); // offset of MODULE instruction
	AddressL(types, 0);
	if (params.front())
		parSize = -PrcParAddr(0, params.front());
	else parSize = 0;
	varSize = AddressL(variables, VAR_START)-3;
	AddressL(signals, 0);
	AddressL(timers, 0);
	AddressL(procedures, 0);
	AddressL(siglists, 0);

	nameStore->ClearNameMarks();
	lh_list_node_t<state_node_t> *sn = states.front();
	numStates = numLabels = 0;
	labelTbl = new name_entry_t[32];
	assert(labelTbl);
	while (sn)
	{
		sn->info.Address(0, stateTbl, numStates, labelTbl, numLabels);
		sn = sn->next();
	}
	ScopeStack->ExitScope();
	return n;
}

void process_def_t::Address2()
{
	ScopeStack->EnterScope(Q_PROCESS,this);
	validsig.Address(0);
	signalset_t ss;

	// We now add all signals in sigroutes connected to the
	// process to the valid input set, plus any locally defined
	// ones, and make a process table entry with this info.

	ss.add(validsig.sigset);
	lh_list_node_t<signal_def_t> *s = signals.front();
	while (s)
	{
		ss.add(s->info.idx);
		s = s->next();
	}
	block_def_t *b = (block_def_t *)ScopeStack->GetContaining(Q_BLOCK);
	lh_list_node_t<route_def_t> *r = b->routes.front();
	while (r)
	{
		for (int i = 0; i<2; i++)
		{
			if (this == ((process_def_t *)ScopeStack->dequalify(&r->info.paths[i].destination, Q_PROCESS)))
			{
				ss.add(r->info.paths[i].s.sigset);
			}
		}
		r = r->next();
	}
	int pi = Router->AddProcess(this, ss);
	assert(pi>=0); // should be fatal error
	ScopeStack->ExitScope();
}

int subblock_def_t::Address(int n)
{
	ScopeStack->EnterScope(Q_SUBSTRUCTURE,this);
	AddressL(types, 0);
	AddressL(signals, 0);
	AddressL(siglists, 0);
	ScopeStack->ExitScope();
	return n;
}

void route_def_t::Address2()
{
	for (int i=0;i < 2; i++)
	{
		if (paths[i].originator.name()[0]=='\0') continue;
		paths[i].Address(0);
		int ri = Router->AddRoute(
			(process_def_t *)ScopeStack->dequalify(&paths[i].originator, Q_PROCESS),
			(process_def_t *)ScopeStack->dequalify(&paths[i].destination, Q_PROCESS),
			(block_def_t *)ScopeStack->GetContaining(Q_BLOCK),
			&paths[i].s,
			 -1);
		if (ri < 0) SDLfatal(file, place, ERR_ROUTETBLFULL);
		paths[i].idx = ri;
	}
}

void c2r_def_t::Address2()
{
	// This routine sets the ch field of route table
	// entries to be the index of a channel table entry
	// connected to the route, if any
	block_def_t *b = (block_def_t *)ScopeStack->GetContaining(Q_BLOCK);
	channel_def_t *c = (channel_def_t *)ScopeStack->dequalify(&channel, Q_CHANNEL);
	if (c==NULL)
	{
		SDLerror(file, place,ERR_CONNCH, Id2Str(channel));
		return;
	}
	// Get the outgoing and incoming channel table entries
	int inChEnt, outChEnt;
	block_def_t *b0 = (block_def_t *)ScopeStack->dequalify(&c->paths[0].originator,Q_BLOCK);
	if (b0==b)
	{
		outChEnt = c->paths[0].idx;
		inChEnt = c->paths[1].originator.name()[0] ? c->paths[1].idx : -1;
	}
	else
	{
		// at least one endpoint of the channel must be the
		// containing block
		if (b != (block_def_t *)ScopeStack->dequalify(&c->paths[0].destination,Q_BLOCK))
			SDLerror(file, place,ERR_C2RBLK, b->name());
		else
		{
			inChEnt = c->paths[0].idx;
			outChEnt = c->paths[1].originator.name()[0] ? c->paths[1].idx : -1;
		}
	}
	lh_list_node_t<ident_t> *r = routes.front();
	// process each route in turn...
	for ( ; r ; r = r->next())
	{
		route_def_t *rd = (route_def_t *)ScopeStack->dequalify(&r->info, Q_SIGROUTE);
		if (rd==NULL)
		{
			SDLerror(file, place, ERR_BADROUTE, Id2Str(r->info));
			continue;
		}
		// Make sure that the sigroute is defined in the
		// containing block
		lh_list_node_t<route_def_t> *br = b->routes.front();
		while (br)
		{
			if (&br->info == rd) break;
			br = br->next();
		}
		if (br==NULL)
		{
			SDLerror(file, place, ERR_BADROUTE, Id2Str(r->info));
			continue;
		}
		// Get the route table entry
		int re = rd->paths[0].idx;
		// at least one endpoint must be ENV...
		if (Router->RouteSource(re) && Router->RouteDest(re))
			SDLerror(file, place, ERR_C2RROUTE);
		// set the first entry. If dest of route is ENV then
		// this is the outgoing route (from process), else incoming
		if (Router->RouteDest(re) == 0)
			Router->RouteChan(re, outChEnt);
		else
			Router->RouteChan(re, inChEnt);
		// if bidirectional route, do second entry
		if (rd->paths[1].originator.name()[0])
		{
			re++;
			if (Router->RouteDest(re) == 0)
				Router->RouteChan(re, outChEnt);
			else
				Router->RouteChan(re, inChEnt);
		}
	}
}

int block_def_t::Address(int n)
{
	ScopeStack->EnterScope(Q_BLOCK,this);
	startLbl = newlabel(); // offset of MODULE instruction
	// The order is important here!
	AddressL(types, 0);
	AddressL(signals, 0);
	AddressL(processes, 0);
	AddressL(siglists, 0);
	ScopeStack->ExitScope();
	return n;
}

void block_def_t::Address2()
{
	ScopeStack->EnterScope(Q_BLOCK,this);
	Address2L(routes);
	Address2L(connections);
	Address2L(processes);
	ScopeStack->ExitScope();
}

void channel_def_t::Address2()
{
	for (int i=0; i < 2; i++)
	{
		if (paths[i].originator.name()[0]=='\0') continue;
		paths[i].Address(0);
		int ce = Router->AddChannel(
				(block_def_t *)ScopeStack->dequalify(&paths[i].originator, Q_BLOCK),
				(block_def_t *)ScopeStack->dequalify(&paths[i].destination, Q_BLOCK),
				&paths[i].s,
				this);
		if (ce < 0) SDLfatal(file, place, ERR_CHTBLFULL);
		paths[i].idx = ce;
	}
}

int system_def_t::Address(int n)
{
	ScopeStack->EnterScope(Q_SYSTEM,this);
	// The order is important here!
	AddressL(types, 0);
	AddressL(signals, 0);
	AddressL(blocks, 0);
	AddressL(siglists, 0);
	Address2L(channels);
	Address2L(blocks);
	ScopeStack->ExitScope();
	return n;
}

