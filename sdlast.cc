/*
 * SDL Abstract Syntax Tree definitions and set types
 *
 * Written by Graham Wheeler, February 1994
 * Data Network Architecture Laboratory
 * Department of Computer Science
 * University of Cape Town
 * 
 * (c) 1994 Graham Wheeler and the Data Network Architectures Laboratory
 * All Rights Reserved
 *
 * Last modified: 14-6-94
 *
 */

#pragma hdrfile "SDLAST.SYM"
#include "sdlast.h"

//#ifndef INLINE
//#define INLINE 
//#include "sdlast.inl"
//#endif

Bool_t	dc_paren = False;
char	dc_sep = '\0';
system_def_t	*sys;
scope_stack_t	*ScopeStack;
file_table_t	*fileTable;
route_manager_t	*Router;
int		lastIDlevel;
s_code_word_t	lastIDoffset;		// offset of var/param
heap_ptr_t	lastIDtype;	// offset in heap of var/param's type def

char *qualifier_elt_t::tn[] =
			{ "SYSTEM", "BLOCK", "PROCESS", "SIGNAL", "PROCEDURE" };

int ident_t::Size()
{
	data_def_t *rtn = (data_def_t *)ScopeStack->dequalify(this, Q_TYPE);
	if (rtn == NULL) return 0;
	else if (rtn->size) return rtn->size; // already computed
	else return rtn->Size();
}

int data_def_t::Size()
{
	if (size) return size;
	switch(tag)
	{
	case STRUCT_TYP:
		size = (GetStrucDefP(contents))->Size();
		break;
	case ARRAY_TYP:
		size = (GetArrDefP(contents))->Size();
		break;
	case NATURAL_TYP:
	case INTEGER_TYP:
	case CHARACTER_TYP:
	case BOOLEAN_TYP:
	case PID_TYP:
	case TIME_TYP:
	case DURATION_TYP:
	case ENUM_TYP:
	case REAL_TYP:
		size = 1;
		break;
	}
//cout << "Type " << nm.name() << " has size " << size << endl;
	return size;
}

int array_def_t::Size()
{
	return (dval == UNDEFINED) ? UNDEFINED : (dval * sort.Size());
}

int struct_def_t::Size()
{
	int rtn = 0;
	lh_list_node_t<fieldgrp_t> *fl = fieldgrps.front();
	while (fl)
	{
		int tmp = fl->info.Size();
		if (tmp) rtn += tmp;
		else return 0; // error
		fl = fl->next();
	}
	return rtn;
}

int fieldgrp_t::Size()
{
	int ncnt = 0;
	lh_list_node_t<field_name_t> *nl = names.front();
	while (nl)
	{
		ncnt++;
		nl = nl->next();
	}
	return ncnt * sort.Size();
}

int  data_def_t::CanCast(data_def_t *otherdd)
{
	if (otherdd == this) return 1;
	if ((this==timeType || this==durType) && otherdd == realType)
		return 1;
	if ((this==intType && otherdd == naturalType) ||
	    (otherdd==intType && this == naturalType))
	{
		// should issue a warning!!
		return 1;
	}
//	if ((this==realType && (otherdd == intType) || otherdd == naturalType))
//		return 1;
	return 0;
}

//------------------------------------------------------------------------

template<class T>
void *matchesNameL(lh_list_t<T> &l, s_code_word_t n)
{
	void *rtn;
	lh_list_node_t<T> *p = l.front();
	while (p)
	{
		rtn = p->info.matchesName(n);
		if (rtn) return (void *)&p->info;
		p = p->next();
	}
	return NULL;
}

void *matchesEltNameL(lh_list_t<data_def_t> &l, s_code_word_t n)
{
	void *rtn;
	lh_list_node_t<data_def_t> *p = l.front();
	while (p)
	{
		if (p->info.tag == ENUM_TYP)
		{
			enum_def_t *ed = GetEnumDefP(p->info.contents);
			lh_list_node_t<enum_elt_t> *e = ed->elts.front();
			while (e)
			{
				if (e->info.matchesName(n))
					return (void *)&p->info;
				e = e->next();
			}
		}
		p = p->next();
	}
	return NULL;
}

// Force the compiler to make the necessary template function instances

void *matchesNameL(lh_list_t<block_def_t>&,	s_code_word_t);
void *matchesNameL(lh_list_t<channel_def_t>&,	s_code_word_t);
void *matchesNameL(lh_list_t<data_def_t>&,	s_code_word_t);
void *matchesNameL(lh_list_t<name_t>&,		s_code_word_t);
void *matchesNameL(lh_list_t<procedure_def_t>&,	s_code_word_t);
void *matchesNameL(lh_list_t<procedure_formal_param_t>&,s_code_word_t);
void *matchesNameL(lh_list_t<process_def_t>&,	s_code_word_t);
void *matchesNameL(lh_list_t<process_formal_param_t>&,s_code_word_t);
void *matchesNameL(lh_list_t<route_def_t>&,	s_code_word_t);
void *matchesNameL(lh_list_t<siglist_def_t>&,	s_code_word_t);
void *matchesNameL(lh_list_t<signal_def_t>&,	s_code_word_t);
void *matchesNameL(lh_list_t<state_node_t>&,	s_code_word_t);
void *matchesNameL(lh_list_t<timer_def_t>&,	s_code_word_t);
void *matchesNameL(lh_list_t<variable_def_t>&,	s_code_word_t);

//------------------------------------------------------------------------

static int allocs = 0, deletes = 0; // should go in class, put here
// for faster recompile for now

heap_t::heap_t(long size)
{
//	cout << "Making heap of size " << size << endl;
	sz = size + (long)(NULL_PTR_SZ+sizeof(free_list_node_t));
#if defined(__MSDOS__) && !defined(W16)
	space = (char *)farcalloc(sz, sizeof(char));
#else
	space = (char *)calloc(sz, sizeof(char));
#endif
	firstfree = NULL_PTR_SZ;
	free_list_node_t *f = (free_list_node_t *)&space[NULL_PTR_SZ];
	f->next = 0;
	f->size = size;
	top = 0;
	debug = 0;
}

const s_code_word_t heap_t::avail()
{
	s_code_word_t t = 0;
	free_list_node_t *f = (free_list_node_t *)&space[firstfree];
	for (;;)
	{
		t += f->size;
		if (f->next == 0) break;
		f = (free_list_node_t *)&space[f->next];
	}
	return t;
}

void heap_t::coalesce(s_code_word_t off)
{
	free_list_node_t *f = (free_list_node_t *)GetVoidP(off);
	while ((off + sizeof(free_list_node_t) + f->size) == f->next)
	{
#ifdef DEBUG
		cout << "Coalescing " << off << " with " << f->next << endl;
#endif
		free_list_node_t *nf = (free_list_node_t *)&space[f->next];
		f->next = nf->next;
		f->size += nf->size + sizeof(free_list_node_t);
	}
}

void *heap_t::alloc(const size_t size)
{
	free_list_node_t *f = (free_list_node_t *)&space[firstfree],
		*prev = NULL;
	allocs++;
	while (size > f->size)
	{
		if (f->next == 0)
		{
			fprintf(stderr,"FATAL - heap exhausted!\n");
			exit(-1);
		}
		prev = f;
		f = (free_list_node_t *)&space[f->next];
	}
	void *rtn = (void *)f;
	free_list_node_t *newf = (free_list_node_t *)(size+(char *)f);
	s_code_word_t off = GetOffset(newf);
	if ((off+sizeof(free_list_node_t)) > top)
		top = off + sizeof(free_list_node_t);
	if (prev) prev->next = off;
	else firstfree = off;
	newf->next = f->next;
	newf->size = f->size - size;
	// Coalesce adjacent free list nodes 
	coalesce(off);
#ifdef DEBUG
	cout << "Allocating " << size << " bytes at offset " << 
		GetOffset(rtn) << endl;
#endif
	return rtn;
}

void heap_t::release(void *p, size_t size)
{
	s_code_word_t off = GetOffset(p); // this validates p as well
	free_list_node_t *newf = (free_list_node_t *)p;
	newf->size = size - sizeof(free_list_node_t);
	deletes++;
#ifdef DEBUG
	cout << "Releasing " << size << " bytes at offset " << off << endl;
#endif
	// find the preceding free block, if any
	if (off>firstfree)
	{
		free_list_node_t *f = (free_list_node_t *)&space[firstfree];
		for (;;)
		{
			if (f->next > off) break;
			if (f->next == 0) break;
			f = (free_list_node_t *)&space[f->next];
		}
#ifdef DEBUG
		cout << "Linking free block between " << GetOffset(f)
			<< " and " << f->next << endl;
#endif
		newf->next = f->next;
		f->next = off;
		coalesce(off);
		coalesce(GetOffset(f));
	}
	else
	{
#ifdef DEBUG
		cout << "Inserting free block at front of free list" << endl;
#endif
		newf->next = firstfree;
		firstfree = off;
		coalesce(off);
	}
}

void heap_t::Debug()
{
	if (debug>1)
	{
		cerr << "HEAP STATISTICS AND INFO" << endl;
		cerr << "Heap size is "<< sz<< " bytes" << endl;
		if (debug>2)
			cerr << "Free List:" << endl;
		free_list_node_t *f = (free_list_node_t *)&space[firstfree];
		long n = 0;
		for (;;)
		{
			if (debug>2)
				cerr << "Offset " << GetOffset(f) <<
					"   Size " << f->size <<
					"   Next " << f->next << endl;
			n++;
			if (f->next == 0) break;
			f = (free_list_node_t *)&space[f->next];
		}
		cerr << "Fragmentation: " << (n / (sz / sizeof(free_list_node_t)))
			<< '%' << endl;
	}
	if (debug)
	{
		cerr << used() << '/' << sz << " bytes of local heap used (";
		cerr << ((used()*100l)/sz) << "%)" << endl;
		cerr << allocs << " allocations and " << deletes << " deletes"<< endl;
	}
}

heap_t::~heap_t()
{
	Debug();
#if defined(__MSDOS__) && !defined(W16)
	farfree(space);
#else
	free(space);
#endif
}

void heap_t::Write(ostream &os)
{
	os.write((char *)&top, sizeof(top));
	os.write((char *)&firstfree, sizeof(firstfree));
#ifdef __MSDOS__
	long cnt = top;
	char *sp = space;
	while (cnt>0)
	{
		long chunk = 0x4000;
		if (chunk > cnt) chunk = cnt;
		os.write(sp, (int)chunk);
		cnt -= chunk;
		sp += chunk;
	}
#else
	os.write(space, top);
#endif
}

void heap_t::Read(istream &is)
{
	is.read((char *)&top, sizeof(top));
	is.read((char *)&firstfree, sizeof(firstfree));
#ifdef __MSDOS__
	long cnt = top, rd=0;
	char *sp = space;
	while (cnt>0)
	{
		long chunk = 0x4000;
		if (chunk > cnt) chunk = cnt;
		is.read(sp, (int)chunk);
		rd += is.gcount();
		cnt -= chunk;
		sp += chunk;
	}
	if (rd != top)
#else
	is.read(space, top);
	if (is.gcount() != top)
#endif
	{
		cerr << "Failed to restore heap!" << endl;
		exit(-1);
	}
}


//------------------------------------------------------------------------
// Hash table non-inline members

/*INLINE*/ void hash_t::insert(const char *name, const s_code_word_t index)
{
	lh_list_node_t<hash_entry_t> *he = 
		new lh_list_node_t<hash_entry_t>(index);
	assert(he);
	hash[key(name)].prepend(he);
}

const s_code_word_t hash_t::key(const char *name)
{
	s_code_word_t sum=0;
	while (*name)
	   sum += *name++;
	return sum % MAXKEY;
}

// Name store non-inline members

const s_code_word_t namestore_t::index(const char *nm)
{
	s_code_word_t k = ht.key(nm);
	lh_list_node_t<hash_entry_t> *e = ht.hash[k].front();
	while (e)
	{
		s_code_word_t i = e->info.index;
		if (strcmp(nm, name(i)) == 0)
			return i;
		e = e->next();
	}
	return 0;
}

s_code_word_t namestore_t::insert(const char *name)
{
	// First see if we have it...
	s_code_word_t rtn = index(name);
	if (rtn == 0)
	{
		if ((next-NUM_RESERVED_WORDS)>=MAXIDENTIFIERS ||
			(top + strlen(name) + 1) >= MAXCHAR)
		{
			fprintf(stderr,"Fatal error - namestore exhausted!\n");
			exit(-1);
		}
		// new name
		ht.insert(name, next);
		map[next-NUM_RESERVED_WORDS] = top;
		while (*name) store[top++] = *name++;
		store[top++] = '\0';
		rtn = next++;
	}
	return rtn;
}

//----------------------------------------------------------------------

int signalset_t::operator== (const signalset_t& s) const
{
	int i;
	for (i=0;i<SIGSET_SIZE;i++)
		if (s.bitmask[i] != bitmask[i])
			return 0;
	return 1;
}

int signalset_t::operator!= (const signalset_t& s) const
{
	int i;
	for (i=0;i<SIGSET_SIZE;i++)
		if (s.bitmask[i] != bitmask[i])
			return 1;
	return 0;
}

Bool_t signalset_t::hasSignal(int idx)
{
	if (bitmask[idx/SWORD_SIZE] & (1<<(idx%SWORD_SIZE)))
		return True;
	else return False;
}

Bool_t signallist_t::hasSignal(int idx)
{
	return sigset.hasSignal(idx);
}

void signalset_t::set()
{
	for (int i = 0; i<SIGSET_SIZE; i++) bitmask[i] = ~((s_code_word_t)0);
}

void signalset_t::clear()
{
	for (int i = 0; i<SIGSET_SIZE; i++) bitmask[i] = 0;
}

signalset_t::signalset_t()
{
	clear();
}

int signalset_t::disjoint(signalset_t &s2)
{
	for (int j=0; j<SIGSET_SIZE; j++)
	{
		if ((bitmask[j] & s2.bitmask[j]) != 0)
			return 0;
	}
	return 1;
}

void signalset_t::add(set_bitmask_t &b)
{
 	for (int i = 0; i<SIGSET_SIZE; i++) bitmask[i] |= b[i];
}

void signalset_t::add(signalset_t &s)
{
 	for (int i = 0; i<SIGSET_SIZE; i++) bitmask[i] |= s.bitmask[i];
}

//----------------------------------------------------------------------
// Signal Lookup Table

signal_tbl_t::signal_tbl_t()
{
	for (int i= 0; i<MAX_SIGNALS; i++)
		sigoff[i] = 0;
}

void signal_tbl_t::Enter(int idx, signal_def_t *sd)
{
	sigoff[idx] = GetOffset(sd);
	isTimer[idx] = 0;
}

void signal_tbl_t::Enter(int idx, timer_def_t *td)
{
	sigoff[idx] = GetOffset(td);
	isTimer[idx] = 1;
}

void *signal_tbl_t::Lookup(int idx, int &type)
{
	type = isTimer[idx];
	return (GetVoidP(sigoff[idx]));
}

char *signal_tbl_t::GetName(int idx)
{
	if (isTimer[idx])
		return (char *)(GetTimerDefP(sigoff[idx])->name());
	else
		return (char *)(GetSigDefP(sigoff[idx])->name());
}

signal_tbl_t *signalTbl;

//----------------------------------------------------------------------

void *variable_decl_t::matchesName(s_code_word_t n)
{
	return matchesNameL(names, n);
}

void *variable_def_t::matchesName(s_code_word_t n)
{
	return matchesNameL(decls, n);
}

void *state_node_t::matchesName(s_code_word_t n)
{
	return matchesNameL(states, n);
}

void *procedure_formal_param_t::matchesName(s_code_word_t n)
{
	return matchesNameL(names, n);
}

void *process_formal_param_t::matchesName(s_code_word_t n)
{
	return matchesNameL(names, n);
}

//--------------------------------------------------------------
// State name lookup

int StateName2Index(lh_list_t<name_entry_t> &l, s_code_word_t nm)
{
	int rtn = 0;
	lh_list_node_t<name_entry_t> *n = l.front();
	while (n)
	{
		if (n->info.nm == nm)
			return rtn;
		rtn++;
		n = n->next();
	}
	return -rtn-1;
}

char *StateIndex2Name(lh_list_t<name_entry_t> &l, s_code_word_t idx)
{
	if (idx==UNDEFINED) return "UNDEFINED";
	lh_list_node_t<name_entry_t> *n = l.front();
	while (idx--)
	{
		if (n) n = n->next();
		else return NULL;
	}
	return (char *)nameStore->name(n->info.nm);
}

int procedure_def_t::StateNm2Idx(s_code_word_t nm)
{
	return StateName2Index(stateTbl, nm);
}

char *procedure_def_t::StateIdx2Nm(s_code_word_t idx)
{
	return StateIndex2Name(stateTbl, idx);
}

int process_def_t::StateNm2Idx(s_code_word_t nm)
{
	return StateName2Index(stateTbl, nm);
}

char *process_def_t::StateIdx2Nm(s_code_word_t idx)
{
	return StateIndex2Name(stateTbl, idx);
}

//----------------------------------------------------------------------
		
subblock_def_t::subblock_def_t(name_t n)
{
	nm = n;
	reference = False;
	lh_list_t<block_def_t> *bd = new lh_list_t<block_def_t>;
	assert(bd);
	blocks = GetOffset(bd);
}

// Global objects - heap, namestore, and empty name/ident

heap_t *localHeap;
namestore_t *nameStore;
name_t *noName;
ident_t *noIdent;

void initGlobalObjects(void)
{
	localHeap = new heap_t;
	assert(localHeap);
	fileTable = new file_table_t;
	assert(fileTable);
	signalTbl = new signal_tbl_t;
	assert(signalTbl);
	nameStore = new namestore_t;
	assert(nameStore);
	ScopeStack = new scope_stack_t;
	assert(ScopeStack);
	Router = new route_manager_t;
	assert(Router);
	noName = new name_t("");
	assert(noName);
	noIdent = new ident_t(*noName);
	assert(noIdent);
}

void DeleteAST(void)
{
	delete localHeap;
	delete ScopeStack;
}

//-----------------------------------------------------------------
// This can't go in sdlcheck as the GNU linker then barfs

void name_t::Mark()
{
	nameStore->MarkName(index());
}
//-----------------------------------------------------------------
// Search for objects by name and type

heap_ptr_t system_def_t::findNodeByName(name_t nm, typeclass_t *typ)
{
	void *rtn = NULL;
	switch(*typ)
	{
	case Q_ANY:
		if ((rtn = matchesNameL(blocks,nm.index())) != NULL)
			*typ = Q_BLOCK;
		else if ((rtn = matchesNameL(channels,nm.index())) != NULL)
			*typ = Q_CHANNEL;
		else if ((rtn = matchesNameL(signals,nm.index())) != NULL)
			*typ = Q_SIGNAL;
		else if ((rtn = matchesNameL(siglists,nm.index())) != NULL)
			*typ = Q_SIGLIST;
		else if ((rtn = matchesNameL(types,nm.index())) != NULL)
			*typ = Q_TYPE;
		break;
	case Q_BLOCK:
		rtn = matchesNameL(blocks,nm.index());
		break;
	case Q_CHANNEL:
		rtn = matchesNameL(channels,nm.index());
		break;
	case Q_SIGNAL:
		rtn = matchesNameL(signals,nm.index());
		break;
	case Q_SIGLIST:
		rtn = matchesNameL(siglists,nm.index());
		break;
	case Q_TYPE:
		rtn = matchesNameL(types,nm.index());
		break;
	}
	return GetOffset(rtn);
}

heap_ptr_t block_def_t::findNodeByName(name_t nm, typeclass_t *typ)
{
	void *rtn = NULL;
	switch(*typ)
	{
	case Q_ANY:
		if ((rtn = matchesNameL(processes,nm.index())) != NULL)
			*typ = Q_PROCESS;
		else if ((rtn = matchesNameL(routes,nm.index())) != NULL)
			*typ = Q_SIGROUTE;
		else if ((rtn = matchesNameL(signals,nm.index())) != NULL)
			*typ = Q_SIGNAL;
		else if ((rtn = matchesNameL(siglists,nm.index())) != NULL)
			*typ = Q_SIGLIST;
		else if ((rtn = matchesNameL(types,nm.index())) != NULL)
			*typ = Q_TYPE;
		break;
	case Q_SUBSTRUCTURE:
		assert(0);
	case Q_PROCESS:
		rtn = matchesNameL(processes, nm.index());
		break;
	case Q_SIGROUTE:
		rtn = matchesNameL(routes, nm.index());
		break;
	case Q_SIGNAL:
		rtn = matchesNameL(signals, nm.index());
		break;
	case Q_SIGLIST:
		rtn = matchesNameL(siglists, nm.index());
		break;
	case Q_TYPE:
		rtn = matchesNameL(types, nm.index());
		break;
	}
	return GetOffset(rtn);
}

heap_ptr_t process_def_t::findNodeByName(name_t nm, typeclass_t *typ)
{
	void *rtn = NULL;
	switch(*typ)
	{
	case Q_ANY:
		if ((rtn = matchesNameL(params,nm.index())) != NULL)
			*typ = Q_PROCESSPAR;
		else if ((rtn = matchesNameL(procedures,nm.index())) != NULL)
			*typ = Q_PROCEDURE;
		else if ((rtn = matchesNameL(signals,nm.index())) != NULL)
			*typ = Q_SIGNAL;
		else if ((rtn = matchesNameL(siglists,nm.index())) != NULL)
			*typ = Q_SIGLIST;
		else if ((rtn = matchesNameL(timers,nm.index())) != NULL)
			*typ = Q_TIMER;
		else if ((rtn = matchesNameL(types,nm.index())) != NULL)
			*typ = Q_TYPE;
		else if ((rtn = matchesNameL(variables,nm.index())) != NULL)
			*typ = Q_VARDEF;
		else if ((rtn = matchesNameL(states,nm.index())) != NULL)
			*typ = Q_STATE;
		break;
	case Q_PROCESSPAR:
		rtn = matchesNameL(params, nm.index());
		break;
	case Q_PROCEDURE:
		rtn = matchesNameL(procedures, nm.index());
		break;
	case Q_SIGNAL:
		rtn = matchesNameL(signals, nm.index());
		break;
	case Q_SIGLIST:
		rtn = matchesNameL(siglists, nm.index());
		break;
	case Q_TIMER:
		rtn = matchesNameL(timers, nm.index());
		break;
	case Q_TYPE:
		rtn = matchesNameL(types, nm.index());
		break;
	case Q_ENUMELT:
		rtn = matchesEltNameL(types, nm.index());
		break;
	case Q_VARDEF:
		rtn = matchesNameL(variables, nm.index());
		break;
	case Q_STATE:
		rtn = matchesNameL(states, nm.index());
		break;
	case Q_LABEL:
		break;
	}
	return GetOffset(rtn);
}

heap_ptr_t procedure_def_t::findNodeByName(name_t nm, typeclass_t *typ)
{
	void *rtn = NULL;
	lh_list_t<procedure_def_t> *pl = 
		(lh_list_t<procedure_def_t> *)GetVoidP(procedures);
	switch(*typ)
	{
	case Q_ANY:
		if ((rtn = matchesNameL(params, nm.index())) != NULL)
			*typ = Q_PROCEDUREPAR;
		else if ((rtn = matchesNameL(*pl, nm.index())) != NULL)
			*typ = Q_PROCEDURE;
		else if ((rtn = matchesNameL(types, nm.index())) != NULL)
			*typ = Q_TYPE;
		else if ((rtn = matchesNameL(variables, nm.index())) != NULL)
			*typ = Q_VARDEF;
		else if ((rtn = matchesNameL(states, nm.index())) != NULL)
			*typ = Q_STATE;
		break;
	case Q_PROCEDUREPAR:
		rtn = matchesNameL(params, nm.index());
		break;
	case Q_TYPE:
		rtn = matchesNameL(types, nm.index());
		break;
	case Q_ENUMELT:
		rtn = matchesEltNameL(types, nm.index());
		break;
	case Q_PROCEDURE:
		rtn = matchesNameL(*pl, nm.index());
		break;
	case Q_VARDEF:
		rtn = matchesNameL(variables, nm.index());
		break;
	case Q_STATE:
		rtn = matchesNameL(states, nm.index());
		break;
	case Q_LABEL:
		break;
	}
	return GetOffset(rtn);
}

//-----------------------------------------------------------------
// Scope stack - makes AST work like a symbol table

void scope_stack_t::_EnterScope(const typeclass_t typ, const heap_ptr_t off)
{
	assert(level<MAX_NEST);
	type[level] = typ;
	offset[level] = off;
	level++;
}

void scope_stack_t::EnterScope(const typeclass_t typ, const heap_ptr_t off)
{
	_EnterScope(typ, off);
}

void scope_stack_t::EnterScope(const typeclass_t typ, const void *p)
{
	_EnterScope(typ, localHeap->offset(p));
}

void scope_stack_t::ExitScope()
{
	assert(level>0);
	level--;
}

int longIDs = 1;

void scope_stack_t::Mangle(char *buf, int lvl) // used in cpp translation for unique IDs
{
	if (lvl<0) lvl = level;
	buf[0] = 0;
	for (int i = 1; i < lvl; i++)
	{
		switch(type[i])
		{
		case Q_SYSTEM:
			strcat(buf, GetSysDefP(offset[i])->name());
			break;
		case Q_BLOCK:
			strcat(buf, GetBlkDefP(offset[i])->name());
			break;
		case Q_PROCESS:
			strcat(buf, GetProcsDefP(offset[i])->name());
			break;
		case Q_PROCEDURE:
			strcat(buf, GetProcdDefP(offset[i])->name());
			break;
		case Q_SERVICE:
			break;
		case Q_SUBSTRUCTURE:
			break;
		case Q_SIGNAL:
			break;
		case Q_TYPE:
			break;
		}
		if (longIDs) strcat(buf, "_");
		else strcat(buf, "@"); // gets replaced in sdl2cpp
	}
}

void scope_stack_t::SetScope(const typeclass_t *typ, const heap_ptr_t *off, int cnt)
{
	for (int i = 0; i < cnt; i++)
	{
		type[i] = typ[i];
		offset[i] = off[i];
	}
	level = cnt;
}

void *scope_stack_t::GetInfo(const ident_t &id, data_def_t* &dd, int &lvl, int &offset,
	typeclass_t &t)
{
	void *rtn = (void *)ScopeStack->findIdent(&id, t);
	if (rtn && (t==Q_VARDEF || t == Q_PROCESSPAR || t == Q_PROCEDUREPAR))
	{
		dd = GetDataDefP(lastIDtype);
		offset = lastIDoffset;
	}
	else 
	{
		dd = NULL;
		offset = 0;
	}
	lvl = lastIDlevel;
	return rtn;
}

// search for name nm of type *typ
// return 0 if not found
// else put scope level in lvl, type in typ, and return heap offset

heap_ptr_t scope_stack_t::findNodeInScope(name_t nm, typeclass_t *typ, int *lvl)
{
	heap_ptr_t rtn;
	int lv = level-1;
	while (lv>=0)
	{
		switch (type[lv])
		{
			case Q_SYSTEM:
			{
				system_def_t *s = GetSysDefP(offset[lv]);
				rtn = s->findNodeByName(nm, typ);
				break;
			}
			case Q_BLOCK:
			{
				block_def_t *b = GetBlkDefP(offset[lv]);
				rtn = b->findNodeByName(nm, typ);
				break;
			}
			case Q_PROCESS:
			{
				process_def_t *p = GetProcsDefP(offset[lv]);
				rtn = p->findNodeByName(nm, typ);
				break;
			}
			case Q_SIGNAL:
			{
				//signal_def_t *s = GetSigDefP(offset[lv]);
				// ??
				break;
			}
			case Q_PROCEDURE:
			{
				procedure_def_t *p = GetProcdDefP(offset[lv]);
				rtn = p->findNodeByName(nm, typ);
				break;
			}
			case Q_SERVICE:
				assert(0);
			case Q_SUBSTRUCTURE:
				assert(0);
			case Q_TYPE:
			{
				//data_def_t *f = GetDataDefP(offset[lv]);
				// ??
				break;
			}
		}
		if (rtn)
 		{
			if (lvl) *lvl = lv+1;
			return rtn;
		}
		lv--;
	}
	return 0; // fail
}

// Convert an ID to a pointer to the object it identifies.
// This is currently restricted to full scope qualifiers,
// which are required for remote definition names. In other
// case partial qualifiers are permitted; this must still be
// implemented by maintaining a stack of qualifiers as we
// parse. When parsing an identifier with a partial qualifier,
// we must substitute a full qualifier before adding to the
// AST. Note that when partial qualifiers are supported,
// this should automatically enforce the restriction that 
// remote definition names must have full qualifiers, as
// they exist in an empty scope stack.

// Note that view definitions are the single exception to
// an identifier having to be in the current scope.
	
void *__dequalify(const ident_t *id, typeclass_t &context,
	void *startptr, typeclass_t startclass, int &lvl, int setstack)
{
	s_code_word_t searchName;
	lh_list_node_t<qualifier_elt_t> *q = 
		(lh_list_node_t<qualifier_elt_t> *)id->qual.front();
	typeclass_t qt, qlast = startclass;
	void *rtn = startptr;
	assert(lvl == -1 || !setstack); // setstack only for remote refs
	for (;;)
	{
		if (q)
		{
			searchName = q->info.nm.index();
			qt = q->info.qt;
		}
		else
		{
			searchName = id->nm.index();
			qt = context;
		}
		lvl++;
		switch(qt)
		{
		case Q_SYSTEM:
			if (rtn == NULL) rtn = (void *)sys;
			else return NULL;
			// check names
			break;
		case Q_BLOCK:
			if (qlast == Q_SYSTEM)
				rtn = matchesNameL( ((system_def_t *)rtn)->blocks, searchName);
			else if (qlast == Q_SUBSTRUCTURE)
			{
				/* EXCEPTION - should fix these some
					time thru good methods */
				lh_list_t<block_def_t> *bl = 
					(lh_list_t<block_def_t> *)
					GetVoidP(((subblock_def_t *)rtn)->blocks);
				rtn = matchesNameL(*bl, searchName);
			}
			else return NULL;
			break;
		case Q_PROCESS:
			if (qlast == Q_BLOCK)
				rtn = matchesNameL( ((block_def_t *)rtn)->processes, searchName);
			else return NULL;
			break;
		case Q_PROCEDURE:
			switch(qlast)
			{
			case Q_PROCESS:
				rtn = matchesNameL( ((process_def_t *)rtn)->procedures, searchName);
				break;
			case Q_PROCEDURE:
		     		{
					/* EXCEPTION */
					lh_list_t<procedure_def_t> *pl = (lh_list_t<procedure_def_t> *)
						GetVoidP(((procedure_def_t *)rtn)->procedures);
					rtn = matchesNameL(*pl, searchName);
				}
				break;
#if 0 
			case Q_SERVICE:
				rtn = ((_def_t *)rtn)->procedures.matchesName(searchName);
				break;
#endif
			default:
				return NULL;
			}
			break;
#if 0 
		case Q_SERVICE:
			if (qlast == Q_PROCESS)
				rtn = ((process_def_t *)rtn)->services.matchesName(searchName);
			else return NULL;
			break;
#endif
		case Q_SUBSTRUCTURE:
			if (qlast == Q_SYSTEM)
			{
#if 0
				???
#endif
			}
			else if (qlast == Q_BLOCK)
			{
				// only one substructure allowed, just check the name.
				rtn = (void *)&(((block_def_t *)rtn)->substructure);
			}
			else return NULL;
			break;
		case Q_SIGNAL:
			switch(qlast)
			{
			case Q_SYSTEM:
				rtn = matchesNameL(((system_def_t *)rtn)->signals, searchName);
				break;
			case Q_BLOCK:
				rtn = matchesNameL(((block_def_t *)rtn)->signals, searchName);
				break;
			case Q_PROCESS:
				rtn = matchesNameL(((process_def_t *)rtn)->signals, searchName);
				break;
#if 0
			case Q_SUBSTRUCTURE:
				rtn = ((substruct_t *)rtn)->signals.matchesName(searchName);
				break;
			case Q_SIGNAL:
				rtn = ((signal_def_t *)rtn)->signals.matchesName(searchName);
				break;
#endif
			default:
				return NULL;
			}
			break;
		case Q_TYPE:
			switch(qlast)
			{
			case Q_SYSTEM:
				rtn = matchesNameL(((system_def_t *)rtn)->types, searchName);
				break;
			case Q_BLOCK:
				rtn = matchesNameL(((block_def_t *)rtn)->types, searchName);
				break;
			case Q_PROCESS:
				rtn = matchesNameL(((process_def_t *)rtn)->types, searchName);
				break;
			case Q_PROCEDURE:
				rtn = matchesNameL(((procedure_def_t *)rtn)->types, searchName);
				break;
#if 0
			case Q_SUBSTRUCTURE:
				rtn = ((_def_t *)rtn)->types.matchesName(searchName);
				break;
			case Q_SERVICE:
				rtn = ((_def_t *)rtn)->types.matchesName(searchName);
				break;
#endif
			default:
				return NULL;
			}
			break;

		// The remaining ones are not scope units so they have
		// to be the actual object type we want

      		case Q_CHANNEL:
			if (qlast != Q_SYSTEM) return NULL;
			rtn = matchesNameL(((system_def_t *)rtn)->channels, searchName);
			goto done;
		case Q_SIGLIST:
			if (qlast == Q_SYSTEM)
				rtn = matchesNameL(((system_def_t *)rtn)->siglists, searchName);
			else if (qlast == Q_BLOCK)
				rtn = matchesNameL(((block_def_t *)rtn)->siglists, searchName);
			else if (qlast == Q_PROCESS)
				rtn = matchesNameL(((process_def_t *)rtn)->siglists, searchName);
			// substructures??
			else rtn = NULL;
			goto done;
		case Q_SIGROUTE:
			if (qlast != Q_BLOCK) return NULL;
			rtn = matchesNameL(((block_def_t *)rtn)->routes, searchName);
			goto done;
		case Q_TIMER:
			if (qlast != Q_PROCESS) return NULL;
			rtn = matchesNameL(((process_def_t *)rtn)->timers, searchName);
			goto done;
		case Q_ENUMELT:
			if (qlast == Q_SYSTEM)
				rtn = matchesEltNameL(((system_def_t *)rtn)->types, searchName);
			else if (qlast == Q_BLOCK)
				rtn = matchesEltNameL(((block_def_t *)rtn)->types, searchName);
			else if (qlast == Q_PROCESS)
				rtn = matchesEltNameL(((process_def_t *)rtn)->types, searchName);
			else if (qlast == Q_PROCEDURE)
				rtn = matchesEltNameL(((procedure_def_t *)rtn)->types, searchName);
			else rtn = NULL;
			goto done;
		case Q_VARDEF:
			if (qlast == Q_PROCESS)
				rtn = matchesNameL(((process_def_t *)rtn)->variables, searchName);
			else if (qlast == Q_PROCEDURE)
				rtn = matchesNameL(((procedure_def_t *)rtn)->variables, searchName);
			else rtn = NULL;
			goto done;
		case Q_STATE:
			if (qlast == Q_PROCESS)
				rtn = matchesNameL(((process_def_t *)rtn)->states, searchName);
			else if (qlast == Q_PROCEDURE)
				rtn = matchesNameL(((procedure_def_t *)rtn)->states, searchName);
			else rtn = NULL;
			goto done;
		case Q_LABEL:
			return NULL; // for now
		case Q_PROCESSPAR:
			if (qlast != Q_PROCESS) return NULL;
			rtn = matchesNameL(((process_def_t *)rtn)->params, searchName);
			goto done;
		case Q_PROCEDUREPAR:
			if (qlast != Q_PROCEDURE) return NULL;
		       	rtn = matchesNameL(((procedure_def_t *)rtn)->params, searchName);
			goto done;
		case Q_ANY:
			// This is not comprehensive!!
			switch (qlast)
			{
			case Q_PROCEDURE:
			{
				procedure_def_t *pd = (procedure_def_t *)rtn;
				rtn = matchesNameL(pd->types, searchName);
				if (rtn)
				{
					context = Q_TYPE;
					break;
				}
				rtn = matchesNameL(pd->params, searchName);
				if (rtn)
				{
					context = Q_PROCEDUREPAR;
					break;
				}
				rtn = matchesNameL(pd->variables, searchName);
				if (rtn)
				{
					context = Q_VARDEF;
					break;
				}
				rtn = matchesNameL(pd->states, searchName);
				if (rtn)
				{
					context = Q_STATE;
					break;
				}
				break;
			}
			case Q_PROCESS:
			{
				process_def_t *pd = (process_def_t *)rtn;
				rtn = matchesNameL(pd->types, searchName);
				if (rtn)
				{
					context = Q_TYPE;
					break;
				}
				rtn = matchesNameL(pd->params, searchName);
				if (rtn)
				{
					context = Q_PROCESSPAR;
					break;
				}
				rtn = matchesNameL(pd->variables, searchName);
				if (rtn)
				{
					context = Q_VARDEF;
					break;
				}
				rtn = matchesNameL(pd->states, searchName);
				if (rtn)
				{
					context = Q_STATE;
					break;
				}
				rtn = matchesNameL(pd->signals, searchName);
				if (rtn)
				{
					context = Q_SIGNAL;
					break;
				}
				rtn = matchesNameL(pd->timers, searchName);
				if (rtn)
				{
					context = Q_TIMER;
					break;
				}
				break;
			}
			case Q_BLOCK:
			{
				block_def_t *bd = (block_def_t *)rtn;
				rtn = matchesNameL(bd->types, searchName);
				if (rtn)
				{
					context = Q_TYPE;
					break;
				}
				rtn = matchesNameL(bd->signals, searchName);
				if (rtn)
				{
					context = Q_SIGNAL;
					break;
				}
				break;
			}
			}
			if (rtn) goto done;
			break;
		}
		if (setstack)
			ScopeStack->SetScope(qt, GetOffset(rtn), lvl);
		qlast = qt;
		if (q) q = q->next();
		else break;
	}
done:
	return rtn;
}

void *scope_stack_t::_dequalify(const ident_t *id, typeclass_t &context)
{
	// We start at the current scope level, and back up...
	int lv = level-1, lvl;
	void *rtn = NULL;
	while (lv>=0 && rtn==NULL)
	{
		lvl = lv;
		rtn = __dequalify(id, context, GetVoidP(offset[lv]), type[lv], lvl, 0);
		lv--;
	}
	if (rtn==NULL)
		rtn = __dequalify(id, context, NULL, (typeclass_t)-1, lvl = -1, 0);
	if (rtn) lastIDlevel = lvl;
	return rtn;
}

void *scope_stack_t::dequalify(const ident_t *id, const typeclass_t context)
{
	typeclass_t t = context;
	return _dequalify(id, t);
}

void *scope_stack_t::qualify(const ident_t *id, const typeclass_t context)
{
	typeclass_t t = context;
	int lvl;
	return __dequalify(id, t, NULL, (typeclass_t)-1, lvl = -1, 1);
}

void *scope_stack_t::findIdent(const ident_t *id, typeclass_t &context)
{
	context = Q_ANY;
	return _dequalify(id, context);
}

s_code_word_t scope_stack_t::findLabel(name_t &nm)
{
	int lv = level-1;
	while (lv>0 && (lv != Q_PROCESS && lv != Q_PROCEDURE))
		lv--;
	if (lv==0) return 0;
	name_entry_t *Tbl;
	int Cnt;
	switch (type[lv])
	{
	case Q_PROCESS:
		process_def_t *ps = GetProcsDefP(offset[lv]);
		Cnt = ps->numLabels;
		Tbl = ps->labelTbl;
		break;
	case Q_PROCEDURE:
		procedure_def_t *pd = GetProcdDefP(offset[lv]);
		Cnt = pd->numLabels;
		Tbl = pd->labelTbl;
		break;
	}
	for (int i=0; i<Cnt; i++)
		if (Tbl[i].nm == nm.index())
			return Tbl[i].lbl;
	return -1;
}

s_code_word_t scope_stack_t::findState(name_t &nm)
{
	int lv = level-1;
	while (lv>0 && (type[lv] != Q_PROCESS && type[lv] != Q_PROCEDURE))
		lv--;
	if (lv==0) return 0;
	switch (type[lv])
	{
	case Q_PROCESS:
		process_def_t *ps = GetProcsDefP(offset[lv]);
		return ps->StateNm2Idx(nm.index());
	case Q_PROCEDURE:
		procedure_def_t *pd = GetProcdDefP(offset[lv]);
		return pd->StateNm2Idx(nm.index());
	}
	return -1;
}

void *scope_stack_t::GetContaining(const typeclass_t context)
{
	int lv = level;
	while (--lv >= 0 && type[lv] != context);
	return (lv < 0) ? NULL : GetVoidP(offset[lv]);
}

//------------------------------------------------------------------------
// Ground expression handling. These are the only nodes which currently
// get deleted in the AST, as we make temporary expression trees, 
// evaluate them, delete them, and store the value in the system tree.
// We first define destructors to delete expression trees, and then
// the expression evaluator.

cond_expr_t::~cond_expr_t()
{
	expression_t *ip = GetExprP(if_part);
	expression_t *tp = GetExprP(then_part);
	expression_t *ep = GetExprP(else_part);
	assert(ip);
	assert(tp);
	delete ip;
	delete tp;
	if (ep) delete ep;
}

timer_active_expr_t::~timer_active_expr_t()
{
	if (!params.isEmpty())
	{
		lh_list_node_t<heap_ptr_t> *p = params.front();
		while (p)
		{
			expression_t *e = GetExprP(p->info.v);
			assert(e);
			delete e;
			p = p->next();
		}
	}
}

view_expr_t::~view_expr_t()
{
	if (pid_expr)
	{
		expression_t *e = GetExprP(pid_expr);
		delete e;
	}
}

var_ref_t::~var_ref_t()
{
	if (!sel.isEmpty()) 
	{
		lh_list_node_t<selector_t> *p = sel.front();
		while (p)
		{
			if (p->info.type == SEL_ELT)
			{
				expression_t *e = GetExprP(p->info.val);
				assert(e);
				delete e;
			}
			p = p->next();
		}
	}
}


unop_t::~unop_t()
{
	void *p = GetVoidP(prim);
	switch (type)
	{
	case PE_NLITERAL:
	case PE_ILITERAL:
		int_literal_t *i = (int_literal_t *)p;
		assert(i);
		delete i;
		break;
	case PE_RLITERAL:
		real_literal_t *r = (real_literal_t *)p;
		assert(r);
		delete r;
		break;
	case PE_EXPR:
		expression_t *e = (expression_t *)p;
		assert(e);
		delete e;
		break;
	case PE_CONDEXPR:
		cond_expr_t *ce = (cond_expr_t *)p;
		assert(ce);
		delete ce;
		break;
	case PE_TIMERACTIV:
		timer_active_expr_t *ta = (timer_active_expr_t *)p;
		assert(ta);
		delete ta;
		break;
	case PE_IMPORT:
	case PE_VIEW:
		view_expr_t *ie = (view_expr_t *)p;
		assert(ie);
		delete ie;
		break;
	case PE_VARREF:
		var_ref_t *vr = (var_ref_t *)p;
		assert(vr);
		delete vr;
		break;
	default:
		break;
	}
	prim = 0;
}

expression_t::~expression_t()
{
	if (op == T_UNDEF) // primary?
	{
		unop_t *u = GetUnOpP(l_op);
		assert(u);
		delete u;
	}
	else
	{
		expression_t *l = GetExprP(l_op);
		expression_t *r = GetExprP(r_op);
		assert(l);
		assert(r);
		delete l;
		delete r;
	}
	l_op = r_op = 0;
}

s_code_word_t int_literal_t::EvalGround(data_def_t* &dd)
{
	dd = (value >= 0) ? naturalType : intType;
	return value;
}

s_code_word_t real_literal_t::EvalGround(data_def_t* &dd)
{
	dd = realType;
	return value;
}

s_code_word_t enum_ref_t::EvalGround(data_def_t* &dd)
{
	dd = GetDataDefP(type);
	return (s_code_word_t)value;
}

s_code_word_t synonym_ref_t::EvalGround(data_def_t* &dd)
{
	synonym_def_t *sd = GetSynonymDefP(defn);
	dd = (data_def_t *)ScopeStack->dequalify(&sd->sort, Q_TYPE);
	return (s_code_word_t)sd->value;
}

s_code_word_t cond_expr_t::EvalGround(data_def_t* &dd)
{
	data_def_t *dd2;
	s_code_word_t rtn;
	expression_t *ip = GetExprP(if_part);
	expression_t *tp = GetExprP(then_part);
	expression_t *ep = GetExprP(else_part);
	assert(ip);
	assert(tp);
	s_code_word_t iv = ip->EvalGround(dd);
	if (iv==UNDEFINED || dd != boolType) return UNDEFINED;
	else if (iv)
	{
		rtn = tp->EvalGround(dd);
		(void)ep->EvalGround(dd2);
	}
	else if (ep)
	{
		rtn = ep->EvalGround(dd);
		(void)tp->EvalGround(dd2);
	}
	else return UNDEFINED;
	return (dd == dd2) ? rtn : UNDEFINED;
}

s_code_word_t unop_t::EvalGround(data_def_t* &dd)
{
	s_code_word_t rtn;
	void *p = GetVoidP(prim);
	switch (type)
	{
	case PE_EMPTY:
	case PE_SLITERAL:
		assert(0);
	case PE_SYNONYM:
		synonym_ref_t *sr = (synonym_ref_t *)p;
		assert(sr);
		rtn = sr->EvalGround(dd);
		break;
	case PE_ENUMREF:
		enum_ref_t *er = (enum_ref_t *)p;
		assert(er);
		rtn = er->EvalGround(dd);
		break;
	case PE_NLITERAL:
	case PE_ILITERAL:
		int_literal_t *i = (int_literal_t *)p;
		assert(i);
		rtn = i->EvalGround(dd);
		break;
	case PE_BLITERAL:
		rtn = prim;
		dd = boolType;
		break;
	case PE_RLITERAL:
		real_literal_t *r = (real_literal_t *)p;
		assert(r);
		rtn = r->EvalGround(dd);
		break;
	case PE_EXPR:
		expression_t *e = (expression_t *)p;
		assert(e);
		rtn = e->EvalGround(dd);
		break;
	case PE_FIX:
		e = (expression_t *)p;
		assert(e);
		rtn = e->EvalGround(dd);
		rtn = (s_code_word_t)(*(s_code_real_t *)&rtn);
		dd = intType;
		break;
	case PE_FLOAT:
		e = (expression_t *)p;
		assert(e);
		{
			float f = (float)e->EvalGround(dd);
			rtn =*(s_code_word_t *)&f;
		}
		dd = realType;
		break;
	case PE_CONDEXPR:
		cond_expr_t *ce = (cond_expr_t *)p;
		assert(ce);
		rtn = ce->EvalGround(dd);
		break;
	case PE_NOW:
		dd = timeType;
		rtn = UNDEFINED;
		break;
	case PE_SELF:	
	case PE_PARENT:
	case PE_OFFSPRING:
	case PE_SENDER:
		dd = pidType;
		rtn = UNDEFINED;
		break;
	case PE_TIMERACTIV:
	case PE_IMPORT:
	case PE_VIEW:
	case PE_VARREF:
		rtn = UNDEFINED;
	}
	if (rtn != UNDEFINED)
	{
		if (op == T_MINUS)
			if (dd==intType || dd==realType || dd==naturalType)
				rtn = -rtn;
			else rtn = UNDEFINED;
		else if (op == T_NOT)
			if (dd == boolType) rtn = !rtn;
			else rtn = UNDEFINED;
	}
	return rtn;
}

s_code_word_t expression_t::EvalGround(data_def_t* &dd)
{
	if (op == T_UNDEF) // primary?
	{
		unop_t *u = GetUnOpP(l_op);
		return u->EvalGround(dd);
	}
	else
	{
		expression_t *l = GetExprP(l_op);
		expression_t *r = GetExprP(r_op);
		assert(l);
		assert(r);
		s_code_real_t rlv, rrv;
		s_code_word_t lv = l->EvalGround(dd);
		data_def_t *dd2;
		s_code_word_t rv = r->EvalGround(dd2);
		if (dd==naturalType && dd2==intType)
			dd = intType;
		else if (dd2==naturalType && dd==intType)
			dd2 = intType;
		if (dd != dd2) return UNDEFINED;
		int isReal = 0;
		if (dd == realType || dd == durType || dd == timeType)
		{
			rlv = *((s_code_real_t *)&lv);
			rrv = *((s_code_real_t *)&rv);
			isReal = 1;
		}
		dd2 = boolType;
		if (lv!=UNDEFINED && rv!=UNDEFINED) switch(op)
		{
		case T_BIMP:
			assert(0);
		case T_OR:
			return (lv | rv);
		case T_XOR:
			return (lv ^ rv);
		case T_AND:
			return (lv & rv);
		case T_EQ:
			return isReal ? (rlv == rrv ) : (lv == rv);
		case T_NE:
			return isReal ? (rlv != rrv ) : (lv != rv);
		case T_GT:
			return isReal ? (rlv > rrv ) : (lv > rv);
		case T_GE:
			return isReal ? (rlv >= rrv ) : (lv >= rv);
		case T_LESS:
			return isReal ? (rlv < rrv ) : (lv < rv);
		case T_LE:
			return isReal ? (rlv <= rrv ) : (lv <= rv);
		case T_IN:
		case T_CONCAT:
		case T_EQUALS:
			assert(0);
		case T_MINUS:
			dd2 = dd;
			if (isReal)
			{
				rlv -= rrv;
				return *((s_code_word_t*)&rlv);
			}
			else return (lv - rv);
		case T_PLUS:
			dd2 = dd;
			if (isReal)
			{
				rlv += rrv;
				return *((s_code_word_t*)&rlv);
			}
			else return (lv + rv);
		case T_ASTERISK:
			dd2 = dd;
			if (isReal)
			{
				rlv *= rrv;
				return *((s_code_word_t*)&rlv);
			}
			else return (lv * rv);
		case T_SLASH:
			dd2 = dd;
			if (isReal)
			{
				if (rrv==0.) return UNDEFINED;
				rlv /= rrv;
				return *((s_code_word_t*)&rlv);
			}
			else if (rv==0) return UNDEFINED;
			else return (lv / rv);
		case T_MOD:
			dd2 = dd;
			return (lv % rv); // ??
		case T_REM:
			dd2 = dd;
			return (lv % rv); // ??
		}
		dd = dd2;
	}
	return UNDEFINED;
}

//------------------------------------------------------------------------

void file_table_t::Write(ofstream *os)
{
	int i;
	for (i=0; i<MAX_FILES;i++)
		os->write(fnames[i], MAX_FNAME_LEN);
}

void SaveAST(char *fname)
{
#ifdef __MSDOS__
	ofstream outFile(fname, ios::binary);
#else
	ofstream outFile(fname);
#endif
	assert(outFile);
	heap_ptr_t root = GetOffset(sys);
	outFile.write((char *)&root, sizeof(root));
	localHeap->Write(outFile);
}

void RestoreAST(char *fname)
{
	initGlobalObjects();
	heap_ptr_t root;
#ifdef __MSDOS__
	ifstream inFile(fname, ios::binary);
#else
	ifstream inFile(fname);
#endif
	assert(inFile);
	inFile.read((char *)&root, sizeof(root));
	localHeap->Read(inFile);
	sys = GetSysDefP(root);
	lookupPredefinedTypes();
}

//---------------------------------------------------

char *Id2Str(ident_t &id)
{
	static char buf[100];
	lh_list_node_t<qualifier_elt_t> *node =
		(lh_list_node_t<qualifier_elt_t> *)id.qual.front();
	buf[0] = '\0';
	while (node)
	{
		strcat(buf, node->info.tn[node->info.qt]);
		strcat(buf, " ");
		strcat(buf, node->info.nm.name());
		node = node->next();
		if (node) strcat(buf, "/");
		else strcat(buf, " ");
	}
	strcat(buf, id.name());
	return buf;
}

//-----------------------------------------------------------------

data_def_t *intType, *naturalType, *boolType, *charType, *realType,
	*pidType, *durType, *timeType;

void lookupPredefinedTypes()
{
	// set up predefined type pointers for convenience
	typeclass_t t = Q_TYPE;
	intType = GetDataDefP(sys->findNodeByName(name_t("integer"), &t));
	assert(intType);
	naturalType = GetDataDefP(sys->findNodeByName(name_t("natural"), &t));
	assert(naturalType);
	charType = GetDataDefP(sys->findNodeByName(name_t("character"), &t));
	assert(charType);
	boolType = GetDataDefP(sys->findNodeByName(name_t("boolean"), &t));
	assert(boolType);
	pidType = GetDataDefP(sys->findNodeByName(name_t("pid"), &t));
	assert(pidType);
	realType = GetDataDefP(sys->findNodeByName(name_t("real"), &t));
	assert(realType);
	durType = GetDataDefP(sys->findNodeByName(name_t("duration"), &t));
	assert(durType);
	timeType = GetDataDefP(sys->findNodeByName(name_t("time"), &t));
	assert(timeType);
}

//------------------------------------------------------------------------
// Routines to get type of expressions

data_def_t *GetType(view_expr_t *v)
{
	data_def_t *dd;
	int off, lvl;
	typeclass_t t;
	ScopeStack->GetInfo(v->var, dd, lvl, off, t);
	return dd;
}

data_def_t *GetType(int_literal_t *i)
{
	return (i->value >= 0) ? naturalType : intType;
}

data_def_t *GetType(var_ref_t *v)
{
	data_def_t *dd;
	int off, lvl;
	typeclass_t t;
	ident_t *idp = GetIdentP(v->id);
	ScopeStack->GetInfo(*idp, dd, lvl, off, t);
	if (dd)
	{
		lh_list_node_t<selector_t> *sl = v->sel.front();
		while (sl)
		{
			switch (sl->info.type)
			{
			case SEL_ELT:
				array_def_t *ad = GetArrDefP(dd->contents);
				dd = (data_def_t *)ScopeStack->dequalify(&ad->sort, Q_TYPE);
				break;
			case SEL_FIELD:
				name_t *n = GetNameP(sl->info.val);
				struct_def_t *sd = GetStrucDefP(dd->contents);
				lh_list_node_t<fieldgrp_t> *f = sd->fieldgrps.front();
				while (f)
				{
					lh_list_node_t<field_name_t> *fn = f->info.names.front();
					while (fn)
					{
						if (n->index() == fn->info.index())
						{
							dd = (data_def_t *)ScopeStack->dequalify(&f->info.sort, Q_TYPE);
							goto nextsel;
						}
						fn = fn->next();
					}
					f = f->next();
				}
				break;
			}
		nextsel:
			sl = sl->next();
		}
	}
	return dd;
}

data_def_t *GetType(cond_expr_t *ce)
{
	return GetType(GetExprP(ce->then_part));
}

data_def_t *GetType(unop_t *op)
{
	void *p = GetVoidP(op->prim);
	switch (op->type)
	{
	case PE_EMPTY:		return NULL;
	case PE_SLITERAL:	assert(0);
	case PE_ENUMREF:	return GetDataDefP(((enum_ref_t *)p)->type);
	case PE_SYNONYM:
	{
		data_def_t *dd = GetDataDefP(((synonym_ref_t *)p)->defn);
		synonym_def_t *sd = GetSynonymDefP(dd->contents);
		return GetDataDefP(sd->type);
	}
	case PE_NLITERAL:
	case PE_ILITERAL:	return GetType((int_literal_t *)p);
	case PE_RLITERAL:	return realType;
	case PE_BLITERAL:	return boolType;
	case PE_EXPR:		return GetType((expression_t *)p);
	case PE_FIX:		return intType;
	case PE_FLOAT:		return realType;
	case PE_CONDEXPR:	return GetType((cond_expr_t *)p);
	case PE_NOW:		return timeType;
	case PE_SELF:	
	case PE_PARENT:
	case PE_OFFSPRING:
	case PE_SENDER:		return pidType;
	case PE_TIMERACTIV:	return boolType;
	case PE_IMPORT:
	case PE_VIEW:		return GetType((view_expr_t *)p);
	case PE_VARREF:		return GetType((var_ref_t *)p);
	}
	return NULL;
}

data_def_t *GetType(expression_t *e)
{
	if (e->op == T_UNDEF)
		return GetType(GetUnOpP(e->l_op));
	else
	{
		expression_t *l = GetExprP(e->l_op);
		expression_t *r = GetExprP(e->r_op);
		data_def_t *ldd = GetType(l), *rdd = GetType(r);
		if (ldd == naturalType && (rdd == intType || rdd == naturalType))
			ldd = intType;
		if (rdd == naturalType && ldd == intType) rdd = intType;
		if (ldd && rdd) switch (e->op)
		{
		case T_OR:
		case T_XOR:
		case T_AND:
		case T_NOT:
		case T_NE:
		case T_EQ:
		case T_GT:
		case T_GE:
		case T_LESS:
		case T_LE:		return boolType;
		case T_REM:
		case T_MOD:		return intType;
		case T_MINUS: // int
			if (ldd == timeType && (rdd == timeType ||
				rdd == durType || rdd == realType))
					return durType;
			break;
		case T_PLUS: // int
			if ((ldd == timeType && (rdd == durType || rdd == realType))
			 || (rdd == timeType && (ldd == durType || ldd == realType)))
					return durType;
			break;
		}
		return ldd;
	}
}

//------------------------------------------------------------

void route_manager_t::Print(FILE *fp)
{
	fprintf(fp, "\n\nCHANNEL TABLE\n");
	for (int i=0; i<channelTblCnt; i++)
	{
		fprintf(fp, "\n%d: %s -> %s with ", i,
			channelTbl[i].src ? ChanSourceP(i)->name() : "ENV",
			channelTbl[i].dest ? ChanDestP(i)->name() : "ENV");
#if 0
		printSigList(channelTbl[i].sigs);
#endif
		fprintf(fp, " offset %ld", (long)channelTbl[i].self);
	}
	fprintf(fp, "\n\n\nROUTE TABLE\n");
	for (i=0;i<routeTblCnt;i++)
	{
		fprintf(fp, "\n%d: %s -> %s with ", i,
			routeTbl[i].src ? RouteSourceP(i)->name() : "ENV",
			routeTbl[i].dest ? RouteDestP(i)->name() : "ENV");
#if 0
		printSigList(routeTbl[i].sigs);
#endif
		fprintf(fp, " chan entry %ld", (long)routeTbl[i].ch);
	}
	fprintf(fp, "\n\n\nPROCESS TABLE\n");
	for (i=0;i<processTblCnt;i++)
	{
		fprintf(fp, "\n%d: %s with ", i, ProcessDefP(i)->name());
#if 0
		printSigSet(processTbl[i].sigs);
#endif
	}
	fprintf(fp, "\n\n\n");
}

//-------------------------------------------------------
// Determine the possible destinations for an output.
// We assume no block substructures, thus the via list
// contains signalroutes. A table of (process type, channel)
// pairs is built;

heap_ptr_t destinations[MAX_DEST][2];

int GetOutputDest(signal_def_t *sig, lh_list_t<ident_t> &via)
{
	int cnt = 0; // number of possible destinations
	process_def_t
		*srcProc = (process_def_t *)ScopeStack->GetContaining(Q_PROCESS);
	int spi; // source process table entry
	for (spi = 0; spi < Router->NumProcesses(); spi++)
		if (Router->ProcessDef(spi) == GetOffset(srcProc))
			break;
	assert(spi < Router->NumProcesses());
	if (via.isEmpty() && Router->ProcessSigs(spi)->hasSignal(sig->idx))
	{
		destinations[cnt][0] = GetOffset(srcProc);
		destinations[cnt][1] = 0;
		cnt++;
	}
	int sri, dri;
	for (sri = 0; sri < Router->NumRoutes(); sri++)
	{
		if (Router->RouteSource(sri) == GetOffset(srcProc) &&
		    Router->RouteSigsP(sri)->hasSignal(sig->idx))
		{
			if (!via.isEmpty())
			{
				lh_list_node_t<ident_t> *i = via.front();
				while (i)
				{
					route_def_t *rt = (route_def_t *)ScopeStack->dequalify(&i->info, Q_SIGROUTE);
					if (rt==NULL)
						return -1;
					else
					{
						// is this the same route?
						if (rt->paths[0].idx == sri ||
						    rt->paths[1].idx == sri)
							goto OK;
					}
					i = i->next();
				}
				continue; // not in via list
			}
		OK:
			if (Router->RouteDest(sri))
			{
				destinations[cnt][0] = Router->RouteDest(sri);
				destinations[cnt][1] = 0;
				cnt++;
			}
			else
			{
				// destination is in a different block
				int ce = Router->RouteChan(sri);
				// Search the route table for a process
				// that has the same channel entry, with
				// ENV as the source, and can carry the
				// signal.
				for (dri = 0; dri < Router->NumRoutes(); dri++)
				{
					if (Router->RouteChan(dri) == ce &&
					    Router->RouteSource(dri) == 0 &&
					    Router->RouteSigsP(dri)->hasSignal(sig->idx))
					{
						destinations[cnt][0] = Router->RouteDest(dri);
						destinations[cnt][1] = Router->ChanDef(ce);
						cnt++;
					}
				}
			}
		}
	}
	return cnt;
}


int route_manager_t::ChanIdx(heap_ptr_t p)
{
	for (int i = 0; i < channelTblCnt; i++)
		if (channelTbl[i].self == p)
			break;
	assert(i < channelTblCnt);
	return i;
}
