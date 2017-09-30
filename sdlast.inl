//---------------------------------------------------------------------
// inline methods for namestore_t

INLINE namestore_t::namestore_t()
{
//	cout << "Making namestore" << endl;
	top = 0;
	next = NUM_RESERVED_WORDS;
}

INLINE void namestore_t::status()
{
	cout << top << '/' << MAXCHAR << " bytes of name store used (";
	cout << ((top*100l)/MAXCHAR) << "%)" << endl;
}

INLINE const char *namestore_t::name(const s_code_word_t nm) const
{
	register const char *rtn = store+map[nm - NUM_RESERVED_WORDS];
	return rtn;
}

INLINE void namestore_t::MarkName(s_code_word_t nm)
{
	marks[nm/32] |= (1l << (nm%32));
}

INLINE int namestore_t::IsNameMarked(s_code_word_t nm)
{
	return ( (marks[nm/32] & (1l << (nm%32)) ) != 0l);
}

INLINE void namestore_t::ClearNameMarks()
{
	memset((void *)marks, 0, MARKSIZE*sizeof(marks[0]));
}

//---------------------------------------------------------------------
// inline methods for name_t

INLINE name_t::operator s_code_word_t()
{
	return nm; 
}

INLINE name_t::operator int()
{
	return (int)nm; 
}

INLINE name_t& name_t::operator=(const s_code_word_t &rhs)
{
	nm = rhs; 
	return *this; 
}

INLINE name_t& name_t::operator=(const char* &rhs)
{
	nm = DEFINE(rhs); 
	return *this; 
}

INLINE name_t& name_t::operator=(const name_t &rhs)
{
	nm = rhs.nm;
	return *this; 
}

INLINE name_t::name_t(char *n)
{
	nm = DEFINE(n);			
}

INLINE name_t::name_t(const name_t &n)
{
	nm = n.nm;				
}

INLINE void name_t::name(char *n)
{
	nm = DEFINE(n);			
}

INLINE void name_t::name(const char *n)
{
	nm = DEFINE(n);			
}

INLINE void name_t::name(s_code_word_t n)
{
	nm = n;					
}

INLINE const s_code_word_t name_t::index()	const
{
	return nm;				
}

INLINE const char *name_t::name() const
{
	return idx2name(nm);	
}

//---------------------------------------------------------------------
// inline methods for qualifier_elt_t

INLINE qualifier_elt_t::qualifier_elt_t()
{
	nm = *noName; 
}

INLINE qualifier_elt_t::qualifier_elt_t(typeclass_t t, name_t n)
	: nm(n)
{
	qt = t; 
}

INLINE void qualifier_elt_t::qualify(typeclass_t t, name_t n)
{
	qt = t;
	nm = n;
}

INLINE typeclass_t qualifier_elt_t::type()
{
	return qt; 
}

INLINE const char *qualifier_elt_t::name() const
{
	return nm.name(); 
}

//---------------------------------------------------------------------
// inline methods for identifier_t

INLINE ident_t::ident_t(name_t n)
	: nm(n)
{
}

INLINE const char *ident_t::name() const
{
	return nm.name(); 
}

INLINE void ident_t::name(name_t n)
{
	nm = n; 
}

INLINE void ident_t::qualify(typeclass_t t, name_t n)
{
	qual.append(new lh_list_node_t<qualifier_elt_t>(qualifier_elt_t(t,n)));	
}

//---------------------------------------------------------------------
// inline methods for signallist_t

INLINE Bool_t signallist_t::isEmpty() const
{
	return (signallists.isEmpty() && signals.isEmpty()) ? True : False; 
}

//---------------------------------------------------------------------
// inline methods for state_node_t

INLINE input_part_t *state_node_t::newinput()
{
	NEWNODE(input_part_t, i);
	inputs.append(i);
	return &i->info;
}

#if 0
INLINE input_part_t *state_node_t::newpinput()
{
	NEWNODE(input_part_t, i);
	pinputs.append(i);
	return &i->info;
}
#endif

INLINE save_part_t *state_node_t::newsave()
{
	NEWNODE(save_part_t, s);
	saves.append(s);
	return &s->info;
}

INLINE continuous_signal_t *state_node_t::newcsig()
{
	NEWNODE(continuous_signal_t, cs);
	csigs.append(cs);
	return &cs->info;
}

//---------------------------------------------------------------------
// inline methods for procedure_def_t

INLINE procedure_def_t::procedure_def_t(name_t n)
{
	nm = n;
	reference = False;
	procedures = localHeap->offset( new lh_list_t<procedure_def_t> );
}

INLINE const s_code_word_t procedure_def_t::index() const
{
	return nm.index(); 
}

INLINE void procedure_def_t::SetReference(unsigned char v)
{
	reference |= v; 
}

INLINE void procedure_def_t::ClearReference(unsigned char v)
{
	reference &= ~v; 
}

INLINE Bool_t procedure_def_t::IsReference(unsigned char v)
{
	return (Bool_t)((v & reference) == v);
}

INLINE void procedure_def_t::name(char *n)
{
	nm.name(n); 
}

INLINE void procedure_def_t::name(s_code_word_t n)
{
	nm.name(n); 
}

INLINE const char *procedure_def_t::name() const
{
	return nm.name(); 
}

INLINE variable_def_t *procedure_def_t::newvariable()
{
	NEWNODE(variable_def_t, v);
	variables.append(v);
	return &v->info;
}

//---------------------------------------------------------------------
// inline methods for process_def_t

INLINE process_def_t::process_def_t(name_t n)
{
	nm = n; 
	reference = False; 
	initInst = maxInst = 0;
	initval = 0;
	maxval = 9999; // big enuf
}

INLINE void process_def_t::SetReference(unsigned char v)
{
	reference |= v; 
}

INLINE void process_def_t::ClearReference(unsigned char v)
{
	reference &= ~v; 
}

INLINE Bool_t process_def_t::IsReference(unsigned char v)
{
	return (Bool_t)((v & reference) == v);
}

INLINE void process_def_t::name(char *n)
{
	nm.name(n); 
}

INLINE void process_def_t::name(s_code_word_t n)
{
	nm.name(n); 
}

INLINE const char *process_def_t::name() const
{
	return nm.name(); 
}

INLINE const s_code_word_t process_def_t::index() const
{
	return nm.index(); 
}

INLINE siglist_def_t *process_def_t::newsiglist()
{
	NEWNODE(siglist_def_t, s);
	siglists.append(s);
	return &s->info;
}

INLINE variable_def_t *process_def_t::newvariable()
{
	NEWNODE(variable_def_t, v);
	variables.append(v);
	return &v->info;
}

//---------------------------------------------------------------------
// inline methods for subblock_def_t

INLINE void subblock_def_t::SetReference(unsigned char v)
{
	reference |= v; 
}

INLINE void subblock_def_t::ClearReference(unsigned char v)
{
	reference &= ~v; 
}

INLINE Bool_t subblock_def_t::IsReference(unsigned char v)
{
	return (Bool_t)((v & reference) == v);
}

INLINE void subblock_def_t::name(const char *n)
{
	nm.name(n); 
}

INLINE void subblock_def_t::name(s_code_word_t n)
{
	nm.name(n); 
}

INLINE const char *subblock_def_t::name() const
{
	return nm.name(); 
}

INLINE channel_def_t *subblock_def_t::newchannel()
{
	NEWNODE(channel_def_t, c);
	channels.append(c);
	return &c->info;
}

INLINE connect_def_t *subblock_def_t::newchanconn()
{
	NEWNODE(connect_def_t, c);
	connects.append(c);
	return &c->info;
}

INLINE siglist_def_t *subblock_def_t::newsiglist()
{
	NEWNODE(siglist_def_t, s);
	siglists.append(s);
	return &s->info;
}

//---------------------------------------------------------------------
// inline methods for block_def_t

INLINE block_def_t::block_def_t(name_t n)
{
	reference = False; 
	nm = n; 
}

INLINE void block_def_t::name(const char *n)
{
	nm.name(n); 
}

INLINE void block_def_t::name(s_code_word_t n)
{
	nm.name(n); 
}

INLINE const char *block_def_t::name() const
{
	return nm.name(); 
}

INLINE const s_code_word_t block_def_t::index() const
{
	return nm.index(); 
}

INLINE void block_def_t::SetReference(unsigned char v)
{
	reference |= v; 
}

INLINE void block_def_t::ClearReference(unsigned char v)
{
	reference &= ~v; 
}

INLINE Bool_t block_def_t::IsReference(unsigned char v)
{
	return (Bool_t)((v & reference) == v);
}

INLINE siglist_def_t *block_def_t::newsiglist()
{
	NEWNODE(siglist_def_t, s);
	siglists.append(s);
	return &s->info;
}

INLINE c2r_def_t *block_def_t::newc2r()
{
	NEWNODE(c2r_def_t, c);
	connections.append(c);
	return &c->info;
}

INLINE route_def_t *block_def_t::newroute()
{
	NEWNODE(route_def_t, r);
	routes.append(r);
	return &r->info;
}

//---------------------------------------------------------------------
// inline methods for system_def_t

INLINE system_def_t::system_def_t(name_t n)
{
	nm = n; 
}

INLINE void system_def_t::name(char *n)
{
	nm.name(n); 
}

INLINE const char *system_def_t::name() const
{
	return nm.name(); 
}

INLINE block_def_t *system_def_t::newblock()
{
	NEWNODE(block_def_t, b);
	blocks.append(b);
	return &b->info;
}

INLINE channel_def_t *system_def_t::newchannel()
{
	NEWNODE(channel_def_t, c);
	channels.append(c);
	return &c->info;
}

INLINE siglist_def_t *system_def_t::newsiglist()
{
	NEWNODE(siglist_def_t, s);
	siglists.append(s);
	return &s->info;
}

