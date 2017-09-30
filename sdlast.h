/*
 * SDL Abstract Syntax Tree and set definitions
 *
 * Written by Graham Wheeler, 1994
 * Data Network Architecture Laboratory
 * Department of Computer Science
 * University of Cape Town
 * 
 * (c) 1994 Graham Wheeler and the Data Network Architectures Laboratory
 * All Rights Reserved
 *
 * Last modified: 14/6/94
 *
 * Notes:
 *      * Signal lists are intended to be expanded (much like macros)
 *		before the AST is built, but this is quite wasteful of
 *		memory. Keeping them in the AST reduces memory requirements,
 *		and has the added benefit of allowing them to be recreated
 *		in the AST->SDL decompiler.
 */

#ifndef _SDLAST_H
#define _SDLAST_H

#include <stdlib.h>
#include <iostream.h>
#include <fstream.h>
#ifdef __BORLANDC__
#include <alloc.h>
#endif
#include "sdl.h"

class data_def_t; // useful forward declaration

/*
 * This class allows objects to be allocated contiguously 
 * from an array of characters. No deallocation is supported.
 * We also provide a special pointer template class for 
 * `relocatable' objects (in the sense that the heap can 
 * be relocated).
 */

typedef struct
{
	s_code_word_t size;
	s_code_word_t next;
} free_list_node_t;

class heap_t
{
private:
	char	*space;
	s_code_word_t	firstfree, top, sz;
	int Validate(const s_offset_t v)
	{
		return (v < sz);
	}
	void coalesce(s_code_word_t);
public:
	int debug;
	heap_t(s_code_word_t sz_in = DEFAULT_HEAP_SIZE);
	const s_code_word_t size()
	{
		return sz;	
	}
	const s_code_word_t avail();
	const s_code_word_t used()
	{
		return sz-avail();
	}
	~heap_t();
	const void *address(const s_offset_t p)
	{
		assert(Validate(p));
		return (void *)( p ? (space+p) : NULL);
	}
	const s_offset_t offset(const void *p)
	{
		s_offset_t rtn = (s_offset_t)( (p!=NULL) ? (((char *)p) - space) : 0);
		assert(Validate(rtn));
		return rtn;
	}
	void *alloc(const size_t size);
	void release(void *p, size_t size);
	void Debug();
	void Read(istream &is);
	void Write(ostream &os);
};

extern heap_t *localHeap;

// To create classes that are allocated from the local
// heap, we define the heap_object_t class with overloaded
// new and delete operators. Any class that should be
// allocated on the local heap must inherit this class.

class heap_object_t
{
public:
	void *operator new(size_t s)
	{
		return localHeap->alloc(s);
	}
	void operator delete(void *p, size_t size)
	{
		localHeap->release(p, size);
	}
};

class heap_ptr_t
{
public:
	u_s_code_word_t	v;
	heap_ptr_t(u_s_code_word_t v_in = 0)
	{
		v = v_in;
	}
#ifndef __MSDOS__
	heap_ptr_t& operator=(const s_code_word_t &rhs) // Assign
	{
		v = (u_s_code_word_t)rhs; 
		return *this; 
	}
#endif
	heap_ptr_t& operator=(const u_s_code_word_t &rhs) // Assign
	{
		v = rhs; 
		return *this; 
	}
	heap_ptr_t& operator=(const heap_ptr_t &rhs) // Assign
	{
		v = rhs.v; 
		return *this; 
	}
	operator u_s_code_word_t() const
	{
		return v; 
	}
};

#define NULL_PTR_SZ	sizeof(heap_ptr_t)

/*
 * Linked list template class
 */

// Linked list node. This is a base class that is used in the
// template defined afterwards.

class lh_list_node_base_t : public heap_object_t
{
protected:
  heap_ptr_t	nextp;
	lh_list_node_base_t()
	{
		nextp = (heap_ptr_t)0;
	}
  void		link(lh_list_node_base_t* n)
	{
		n->nextp = nextp;
		nextp = localHeap->offset(n);
	}
  friend		class lh_list_base_t;
};

// Template for a list node containing info of type TYPE

template<class TYPE>
class lh_list_node_t : public lh_list_node_base_t
{
public:
  TYPE	info;
  lh_list_node_t(const TYPE &x)
		: info(x)
	{
	}
  lh_list_node_t()
	{
	}
  lh_list_node_t<TYPE> *next()
 	{
		return (lh_list_node_t<TYPE> *)(localHeap->address(nextp));
	}
  const lh_list_node_t<TYPE>* next() const
 	{
		return (lh_list_node_t<TYPE> *)(localHeap->address(nextp));
	}
};

// Base class for linked list. This inherits a next `pointer'
// from the node base class, which points to the front of the
// list.

class lh_list_base_t : public lh_list_node_base_t
{
protected:
  heap_ptr_t	lastp;

  lh_list_base_t()
	{
		lastp = (heap_ptr_t)0;
	}
  lh_list_node_base_t* header()
	{
		return this;
	}
  const lh_list_node_base_t* header() const
	{
		return this;
	}
  lh_list_node_base_t* front()
	{
		return (lh_list_node_base_t*)localHeap->address(nextp); 
	}	
  const lh_list_node_base_t* front() const
	{
		return (lh_list_node_base_t*)localHeap->address(nextp); 
	}
  lh_list_node_base_t* back()
	{
		return (lh_list_node_base_t*)localHeap->address(lastp); 
	}
  const lh_list_node_base_t* back() const
	{
		return (lh_list_node_base_t*)localHeap->address(lastp); 
	}
  void link(lh_list_node_base_t* a, lh_list_node_base_t* b)
	{
		if (a)
		{
			a->link(b);
			if (localHeap->offset(a) == lastp)
				lastp = localHeap->offset(b);
		} else lastp = nextp = localHeap->offset(b);
	}
  void prepend(lh_list_node_base_t* n)
	{
		link(this, n); 
	}
  void append(lh_list_node_base_t* n)
	{
		link((lh_list_node_base_t*)localHeap->address(lastp), n); 
	}
public:
  int isEmpty() const
	{
		return (lastp==0 && nextp==0);	
	}
  int isHead(const lh_list_node_base_t* n) const
	{
		return (n == this);
	}
};

// Class template for list with nodes of type list_node<TYPE>.

template<class TYPE>
class lh_list_t : public lh_list_base_t
{
public:
 	lh_list_node_t<TYPE>* header()
 	{
		return (lh_list_node_t<TYPE>*)this;
	}
 	const lh_list_node_t<TYPE>* header() const
 	{
		return (lh_list_node_t<TYPE>*)this;
	}
 	lh_list_node_t<TYPE>* front()
	{
		return (lh_list_node_t<TYPE>*)lh_list_base_t::front();
	}
 	const lh_list_node_t<TYPE>* front() const
	{
		return (lh_list_node_t<TYPE>*)lh_list_base_t::front();
	}
 	lh_list_node_t<TYPE>* back()
	{
		return (lh_list_node_t<TYPE>*)lh_list_base_t::back();
	}
 	const lh_list_node_t<TYPE>* back() const
	{
		return (lh_list_node_t<TYPE>*)lh_list_base_t::back();
	}
	TYPE *head()
	{
		lh_list_node_t<TYPE>* p = (lh_list_node_t<TYPE>*)lh_list_base_t::front();
		return ( isHead(p) ? 0 : &(p->info) );
	}
 	const TYPE* head() const
	{
		lh_list_node_t<TYPE>* p = (lh_list_node_t<TYPE>*)lh_list_base_t::front();
		return ( isHead(p) ? 0 : &(p->info) );
	}
	TYPE* tail()
	{
	  lh_list_node_t<TYPE>* p = (lh_list_node_t<TYPE>*)lh_list_base_t::back();
	  return ( isHead(p) ? 0 : &(p->info) );
	}
	const TYPE* tail() const
	{
	  lh_list_node_t<TYPE>* p = (lh_list_node_t<TYPE>*)lh_list_base_t::back();
	  return ( isHead(p) ? 0 : &(p->info) );
	}
 	void link(lh_list_node_t<TYPE>* a, lh_list_node_t<TYPE>* b)
	{
		lh_list_base_t::link(a, b);
	}
	void prepend(lh_list_node_t<TYPE>* n)
	{
		lh_list_base_t::prepend(n);
	}
	void append(lh_list_node_t<TYPE>* n)
	{
		lh_list_base_t::append(n);
	}
#ifdef PRINT
	friend ostream& operator<<(ostream& os, lh_list_t<TYPE>& l);
#endif
};

extern Bool_t	dc_paren;
extern char	dc_sep;

#ifdef PRINT
template<class TYPE>
ostream& operator<<(ostream& os, lh_list_t<TYPE>& l)
{
	lh_list_node_t<TYPE> *p = (lh_list_node_t<TYPE> *)l.front();
	while (p)
	{
		if (dc_paren) os << '(';
		os << (p->info);
		if (dc_paren) os << ')';
		p = p->next();
		if (dc_sep && p)
			os << dc_sep;
	}
	return os;
}
#endif

#define NEWNODE(Typ, var)	lh_list_node_t<Typ> *var = new lh_list_node_t<Typ>; assert(var)
#define NEWLIST(Typ, var)	lh_list_t<Typ> *var = new lh_list_t<Typ>; assert(var)

//------------------------------------------------------------------------

#ifdef PRINT
#define DECOMPILE(typ)	friend ostream& operator<<(ostream &, typ &);
#else
#define DECOMPILE(typ)
#endif

// common method for nodes containing names

#define MATCHES()				void *matchesName(s_code_word_t n)\
								{ return (nm.index()==n) ? this : NULL; }

// Lexical analyser tokens

typedef enum
{
	/* Reserved words */

	T_ACTIVE, T_ADDING, T_ALL, T_ALTERNATIVE, T_AND, T_ARRAY,
	T_AXIOMS, T_BLOCK, T_CALL, T_CHANNEL, T_COMMENT, T_CONNECT,
	T_CONSTANT, T_CONSTANTS, T_CREATE, T_DCL, T_DECISION, T_DEFAULT,
	T_ELSE, T_ENDALTERNATIVE, T_ENDBLOCK, T_ENDCHANNEL, T_ENDDECISION,
	T_ENDGENERATOR, T_ENDMACRO, T_ENDNEWTYPE, T_ENDPROCEDURE,
	T_ENDPROCESS, T_ENDREFINEMENT, T_ENDSELECT, T_ENDSERVICE,
	T_ENDSTATE, T_ENDSUBSTRUCTURE, T_ENDSYNTYPE, T_ENDSYSTEM,
	T_ENV, T_ERROR, T_EXPORT, T_EXPORTED, T_EXTERNAL, T_FALSE, T_FI,
	T_FIX, T_FLOAT, T_FOR,
	T_FPAR, T_FROM, T_GENERATOR, T_IF, T_IMPORT, T_IMPORTED, T_IN,
	T_INHERITS, T_INPUT, T_JOIN, T_LITERAL, T_LITERALS, T_MACRO,
	T_MACRODEFINITION, T_MACROID, T_MAP, T_MOD, T_NAMECLASS, T_NEWTYPE,
	T_NEXTSTATE, T_NOT, T_NOW, T_OF, T_OFFSPRING, T_OPERATOR, T_OPERATORS,
	T_OR, T_ORDERING, T_OUT, T_OUTPUT, T_PARENT, T_PRIORITY,
	T_PROCEDURE, T_PROCESS, T_PROVIDED, T_READ, T_REFERENCED, T_REFINEMENT,
	T_REM, T_RESET, T_RETURN, T_REVEALED, T_REVERSE, T_SAVE, T_SELECT,
	T_SELF, T_SENDER, T_SERVICE, T_SET, T_SIGNAL, T_SIGNALLIST,
	T_SIGNALROUTE, T_SIGNALSET, T_SPELLING, T_START, T_STATE, T_STOP,
	T_STRUCT, T_SUBSTRUCTURE, T_SYNONYM, T_SYNTYPE, T_SYSTEM, T_TASK,
	T_THEN, T_TIMER, T_TO, T_TRUE, T_TYPE, T_VIA, T_VIEW, T_VIEWED, T_WITH,
	T_WRITE, T_XOR,

	/* single character punctuation */

	T_SEMICOLON, T_COMMA, T_LEFTPAR, T_RIGHTPAR, T_MINUS, T_PLUS,
	T_ASTERISK, T_SLASH, T_EQUALS, T_COLON, T_LESS, T_GT, T_BANG, T_QUOTE,
	T_PERCENT,

	/* punctuation token > 1 char */

	T_EQ, T_IMPLIES, T_NE, T_LE, T_GE, T_CONCAT, T_ASGN, T_BIMP,
	T_RET, T_SLST, T_ELST,

	/* Basic tokens - names, strings and integers */

	T_NAME, T_STRING, T_INTEGER, T_NATURAL, T_REAL,

	/* Special tokens */

	T_NEWLINE, T_UNDEF, T_EOF
} SDL_token_t;

// The next class is inherited by all AST nodes. It
// provides support for tracking objects in the original
// source by storing the line number of the source the
// parser was busy with when the AST node was created.
// It is also reused later to store the stack offset of
// variables, structure type fields, and parameters.

extern int  getLineNumber();

class file_table_t: public heap_object_t
{
private:
	char	fnames[MAX_FILES][MAX_FNAME_LEN];
	int     cnt;
public:
	file_table_t()
	{
		cnt = 0;
	}
	char *getName(int idx = -1)
	{
		return fnames[(idx<0) ? (cnt-1) : idx];
	}
	int getIndex()
	{
		return cnt-1;
	}
	int addName(char *name)
	{
		if (cnt>=10) return -1;
		strcpy(fnames[cnt], name);
		return cnt++;
	}
	void Write(ofstream *os);
};

extern file_table_t *fileTable;

class heap_node_t: public heap_object_t
{
public:
	int place, file;
	heap_node_t()
	{
		place = getLineNumber();
		file = fileTable->getIndex();
	}
};

//--------------------------------------------------------------
// SDL NAMES
//
// All names in the AST are represented by numeric indices,
// to decouple name management from the tree.
//--------------------------------------------------------------

#define NUM_RESERVED_WORDS	256	// First 256 name indices are reserved
#define MAXKEY		631				// Number of hash table entries
#define MAXIDENTIFIERS  500      // Maximum number of distinct id names
#define MAXCHAR         5000     // Size of the name store
#define MARKSIZE  	((MAXIDENTIFIERS+31)/32)

class hash_entry_t: public heap_node_t
{
public:
	s_code_word_t index;
	hash_entry_t(const s_code_word_t i = 0)
	{ index = i; }
	DECOMPILE(hash_entry_t)
};

class hash_t: public heap_node_t // Hash table class
{
	public:
		lh_list_t<hash_entry_t> hash[MAXKEY]; // public for now for debugging
		const s_code_word_t key(const char *name);
		friend class namestore_t;
		hash_t() { }
		void insert(const char *name, const s_code_word_t index);
		DECOMPILE(hash_t)
};

class namestore_t: public heap_node_t	// Name store class
{
	public:
		// Everything public for now for debugging
		hash_t	ht;	// hash table
		s_code_word_t		map[MAXIDENTIFIERS]; // maps indices to name store offsets
		unsigned long marks[MARKSIZE];
		char		store[MAXCHAR];	// holds the actual names
		s_code_word_t		top;					// top of store
		s_code_word_t		next;					// next index to use
		friend class hash_t;

						namestore_t();
				void	status();
				s_code_word_t	insert(const char *name);
		const	char   *name(const s_code_word_t nm)		const;
		const	s_code_word_t	index(const char *nm);
				void	MarkName(s_code_word_t nm);
				int		IsNameMarked(s_code_word_t nm);
				void	ClearNameMarks();
		DECOMPILE(namestore_t)
};

extern namestore_t *nameStore;

#define DEFINE(n)		nameStore->insert(n)
#define idx2name(i)	nameStore->name((s_code_word_t)i)
#define name2idx(n)	nameStore->index(n)

class name_t: public heap_node_t
{
private:
	s_code_word_t	nm;
public:
	operator s_code_word_t();
	operator int();
	name_t& name_t::operator=(const s_code_word_t &rhs);
	name_t& name_t::operator=(const char* &rhs);
	name_t& name_t::operator=(const name_t &rhs);
	name_t(char *n = "");
	name_t(const name_t &n);
	void name(char *n);
	void name(const char *n);
	void name(s_code_word_t n);
	const s_code_word_t index()	const;
	const char *name() const;
	DECOMPILE(name_t)
	void Mark();
	void *matchesName(s_code_word_t n)
	{
		return (nm==n) ? this : NULL;
	}
};

// hackish variables set as a side-effect of matchesName when the
// name is a variable or parameter

extern int lastIDlevel;
extern s_code_word_t lastIDoffset;		// offset of var/param
extern heap_ptr_t lastIDtype;	// offset in heap of var/param's type def

// States and labels aren't declared anywhere, so processes/procedures 
// have lookup tables in the second pass that are filled in by Address() 
// and used by Emit(). These tables have entries of the following type:

class name_entry_t : public heap_object_t
{
public:
	s_code_word_t	nm;		// label or state name index
	s_code_word_t	lbl;	// label number
	// the corresponding state index is implicit in the location of the
	// name_entry_t in the stateTbl
	DECOMPILE(name_entry_t)
};

// Predefined empty name

extern name_t *noName;

//--------------------------------------------------------------
// SDL IDENTIFIERS
//
// These consist of a name and a list of qualifiers
//
// Here is an example of how they may be used:
//
//	ident_t *id = new ident_t(name_t("myID"));
//	id->qualify(Q_SYSTEM, name_t("sysID"));
//	id->qualify(Q_BLOCK, name_t("blkID"));
//	id->qualify(Q_PROCESS, name_t("procID"));
// cout << *id; // decompile the above definition
//--------------------------------------------------------------

typedef enum
{
	Q_SYSTEM, Q_BLOCK, Q_PROCESS, Q_SIGNAL, Q_PROCEDURE,
	Q_SERVICE, Q_SUBSTRUCTURE, Q_TYPE,
	// We also use the qualifiers as a basis to search for objects
	// of various types. The following represent types rather
	// than qualifiers (the above can be either types, qualifiers,
	// or scopeunits).
	Q_CHANNEL, Q_SIGLIST, Q_SIGROUTE, Q_TIMER, Q_VARDEF,
	Q_STATE, Q_LABEL, Q_PROCESSPAR, Q_PROCEDUREPAR, Q_ENUMELT,
	Q_ANY
} typeclass_t;

class qualifier_elt_t: public heap_node_t
{
	public:
		typeclass_t	qt;
		name_t				nm;
		static char			*tn[];
		qualifier_elt_t();
		qualifier_elt_t(typeclass_t t, name_t n);
		void qualify(typeclass_t t, name_t n);
		typeclass_t type();
		const char *name() const;
		DECOMPILE(qualifier_elt_t)
		void Mark() { nm.Mark(); }
		MATCHES()
};

class ident_t: public heap_node_t
{
public:
	lh_list_t<qualifier_elt_t>	qual;
	name_t			nm;
	ident_t(name_t n = *noName);
	const char *name() const;
	void name(name_t n);
	void qualify(typeclass_t t, name_t n);
	int  Size();
	DECOMPILE(ident_t)
};

extern ident_t *noIdent;
extern char *Id2Str(ident_t &id); // return printable identifier string

//-----------------------------------------------------
// Data type definitions - simplified from standard
//-----------------------------------------------------

class array_def_t: public heap_node_t
{
public:
	heap_ptr_t			dimension;
	s_code_word_t		dval;
	ident_t				sort;
	array_def_t()
	{
		dval = UNDEFINED;
	}
	DECOMPILE(array_def_t)
	int  Size();
	void Check();
	int Address(int n);
	void PrintValue(int indent, s_code_word_t* &ptr);
};

class field_name_t: public name_t
{
public:
	s_code_word_t offset; // used in 2nd pass
	void Mark();
};

class fieldgrp_t: public heap_node_t
{
public:
	lh_list_t<field_name_t>	names;
	ident_t				sort;
	DECOMPILE(fieldgrp_t)
	void Mark();
	void Check();
	int  Address(int n);
	int  Size();
	void PrintValue(int indent, s_code_word_t* &ptr);
};

class struct_def_t: public heap_node_t
{
public:
	lh_list_t<fieldgrp_t>	fieldgrps;
	DECOMPILE(struct_def_t)
	void Check();
	int  Address(int n);
	int  Size();
	void PrintValue(int indent, s_code_word_t* &ptr);
};

class enum_elt_t: public name_t
{
public:
	int value; // used in 2nd pass
	void Mark();
};

class enum_def_t: public heap_node_t
{
public:
	int nelts;
	lh_list_t<enum_elt_t>	elts;
	DECOMPILE(enum_def_t)
	void Check();
	int  Address(int n);
	int  Size();
};

typedef enum
{
	STRUCT_TYP, ARRAY_TYP, ENUM_TYP, SYNONYM_TYP,
	// The predefined types are set up at system level
	INTEGER_TYP, NATURAL_TYP, BOOLEAN_TYP, CHARACTER_TYP, REAL_TYP,
	PID_TYP, TIME_TYP, DURATION_TYP, NO_TYP
} typedef_class_t;

class synonym_def_t: public heap_node_t
{
public:
	int					has_sort;
	ident_t				sort; // defined type if present
	s_code_word_t		value;// actual value
	heap_ptr_t			expr; // expression offset
	heap_ptr_t			type; // expression value type def offset
	DECOMPILE(synonym_def_t)
	int  Size() { return 1; }
	void Check();
};

class data_def_t: public heap_node_t
{
public:
	name_t				nm;
	heap_ptr_t			contents;
	typedef_class_t		tag;
	int					size; // size in words of an object of the type (pass 2)
	data_def_t()
	{
		size = 0;
		contents = (u_s_code_word_t)0;
		tag = NO_TYP;
	}
	void name(char *n)
		{ nm.name(n); }
	const char *name() const
		{ return nm.name(); }
	DECOMPILE(data_def_t)
	void Mark() { nm.Mark(); }
	void Check();
	MATCHES()
	int  Address(int n);
	int  Size();
	int  CanCast(data_def_t *otherdd);
	void PrintValue(int indent, char *name, s_code_word_t* &ptr);
};

//--------------
// Expressions 
//--------------

// Variable reference

typedef enum
{
	SEL_ELT,
	SEL_FIELD
} selector_type_t;

class selector_t: public heap_node_t
{
public:
	selector_type_t	type;
	heap_ptr_t		val;	/* name or expression */
	DECOMPILE(selector_t)
};

class var_ref_t: public heap_node_t
{
public:
	heap_ptr_t				id;
	lh_list_t<selector_t>	sel;
	~var_ref_t();
	DECOMPILE(var_ref_t)
	void Check(data_def_t* &);
	Bool_t Emit(int &sz, int &off);
};

// Conditional expression

class cond_expr_t: public heap_node_t
{
public:
	heap_ptr_t	if_part;
	heap_ptr_t	then_part;
	heap_ptr_t	else_part;
	~cond_expr_t();
	s_code_word_t EvalGround(data_def_t* &dd);
	DECOMPILE(cond_expr_t)
	void Check(data_def_t* &, Bool_t isRefParam = False);
	int Emit();
};

// Integer Literal

class int_literal_t: public heap_node_t
{
public:
	s_code_word_t	value;
	int_literal_t(s_code_word_t v_in)
		{ value = v_in; }
	DECOMPILE(int_literal_t)
	s_code_word_t EvalGround(data_def_t* &dd);
	void Check(data_def_t* &);
	void Emit();
};

class real_literal_t: public heap_node_t
{
public:
	s_code_word_t value;
	real_literal_t(s_code_word_t v_in)
		{ value = v_in; }
	DECOMPILE(real_literal_t)
	s_code_word_t EvalGround(data_def_t* &dd);
	void Emit();
};

class enum_ref_t: public heap_node_t
{
public:
	int			value;
	int			index;
	heap_ptr_t	type;
	enum_ref_t(int i_in)
		{ index = i_in; }
	s_code_word_t EvalGround(data_def_t* &dd);
	void Emit();
};

class synonym_ref_t: public heap_node_t
{
public:
	int			index;
	heap_ptr_t	defn;
	synonym_ref_t(int i_in)
		{ index = i_in; }
	s_code_word_t EvalGround(data_def_t* &dd);
	void Emit();
};

// Import/View expressions

class view_expr_t: public heap_node_t
{
public:
	ident_t var;
	heap_ptr_t	pid_expr;
	~view_expr_t();
	DECOMPILE(view_expr_t)
};

// Timer active expressions

class timer_active_expr_t: public heap_node_t
{
public:
	ident_t					timer;
	lh_list_t<heap_ptr_t>	params;
	~timer_active_expr_t();
	DECOMPILE(timer_active_expr_t)
	void Check();
	void Emit();
};

// Primary expression types

typedef enum
{
	PE_EMPTY,		// empty expression
	PE_SYNONYM,		// synonym reference
	PE_SLITERAL,	// string literal
	PE_ILITERAL,	// integer literal
	PE_NLITERAL,	// natural literal
	PE_BLITERAL,	// boolean literal
	PE_RLITERAL,	// real literal
	PE_EXPR,		// parenthesised expression or cast
	PE_CONDEXPR,	// conditional IF-THEN-ELSE expression
	PE_NOW,			// now imperative operator
	PE_SELF,		// PId imperative operator (self)
	PE_PARENT,		// PId imperative operator (parent)
	PE_OFFSPRING,	// PId imperative operator (offspring)
	PE_SENDER,		// PId imperative operator (sender)
	PE_TIMERACTIV, 	// timer active expression
	PE_IMPORT,	 	// import expression
	PE_VIEW,	 	// view expression
	PE_VARREF,		// variable reference
	PE_ENUMREF,		// enumeration element
	PE_FIX,			// real->int cast of expression
	PE_FLOAT		// int->real cast of expression
} primexp_type_t;

class unop_t: public heap_node_t
{
public:
	SDL_token_t		op;	  // operation token
	primexp_type_t	type; // type of primary
	heap_ptr_t		prim; // `pointer' to primary
	~unop_t();
	s_code_word_t EvalGround(data_def_t* &dd);
	DECOMPILE(unop_t)
	void Check(data_def_t* &, Bool_t isRefParam = False);
	int Emit(Bool_t isRefPar = False);
};

class expression_t: public heap_node_t
{
public:
	heap_ptr_t	l_op; // `pointer' to left operand
	SDL_token_t	op;	  // operation token
	heap_ptr_t	r_op; // `pointer' to right operand
	heap_ptr_t	etype;// type of the expression; used in 2nd pass
	~expression_t();
	DECOMPILE(expression_t)
	s_code_word_t EvalGround(data_def_t* &dd);
	void Check(data_def_t* &, Bool_t isRefParam = False);
	int Emit(Bool_t isRefPar = False);
};

//------------------------------
// Transition body statements
//------------------------------

class assignment_t: public heap_node_t
{
public:
	var_ref_t		lval;
	heap_ptr_t		rval; // expression
	DECOMPILE(assignment_t)
	void Check();
	void Emit();
};

class output_arg_t: public heap_node_t
{
public:
	ident_t					signal;		/* Signal to output				*/
	lh_list_t<heap_ptr_t>	args;			/* Signal arguments				*/
	DECOMPILE(output_arg_t)
	void Check(Bool_t hasPId, lh_list_t<ident_t> &via);
};

class output_t: public heap_node_t
{
public:
	lh_list_t<output_arg_t>	signals;		/* Signals to output			*/
	Bool_t				hasDest;
	heap_ptr_t			dest;			/* Destination expression		*/
	lh_list_t<ident_t>	via;			/* Direct-via list				*/
	DECOMPILE(output_t)
	void Check();
	void Emit();
};

class pri_output_t: public heap_node_t
{
public:
	lh_list_t<output_arg_t>	signals;	/* Signals to output			*/
	DECOMPILE(pri_output_t)
	void Check();
	void Emit();
};

class invoke_node_t: public heap_node_t
{
public:
	ident_t					ident;	// Process or procedure ID
	lh_list_t<heap_ptr_t>	args;	// Actual parameters
	SDL_token_t				tag;	// call or create
	DECOMPILE(invoke_node_t)
	void Check();
	void Emit();
};

class read_node_t: public heap_node_t
{
public:
	var_ref_t					varref;
	DECOMPILE(read_node_t)
	void Check();
	void Emit();
};

class write_node_t: public heap_node_t
{
public:
	lh_list_t<heap_ptr_t>	exprs;
	DECOMPILE(write_node_t)
	void Check();
	void Emit();
};

class timer_set_t: public heap_node_t
{
public:
	ident_t					timer;
	heap_ptr_t				time;
	lh_list_t<heap_ptr_t>	args;
	DECOMPILE(timer_set_t)
	void Check();
	void Emit();
};

class timer_reset_t: public heap_node_t
{
public:
	ident_t					timer;
	lh_list_t<heap_ptr_t>	args;
	DECOMPILE(timer_reset_t)
	void Check();
	void Emit();
};

typedef enum
{
	N_TASK, N_OUTPUT, N_PRI_OUTPUT, N_CREATE, N_CALL,
	N_SET, N_RESET, N_EXPORT, N_DECISION, N_READ, N_WRITE
} gnode_type_t;

typedef enum
{
	NEXTSTATE, STOP, RETURN, JOIN, EMPTY
} terminator_type_t;

class gnode_t: public heap_node_t
{
public:
	name_t				label;
	gnode_type_t		type;
	heap_ptr_t			node;
	DECOMPILE(gnode_t)
	int Check(Bool_t isInitial, Bool_t isProcess);
	void Emit();
};

class transition_t: public heap_node_t
{
public:
	lh_list_t<gnode_t>	nodes;
	name_t				label;
	terminator_type_t	type;
	name_t				next; // nextstate state or join label
	transition_t() { type = EMPTY; }
	DECOMPILE(transition_t)
	int Check(Bool_t isInitial, Bool_t mustTerminate, Bool_t isProcess);
	void Emit(int lbl);
};

typedef enum
{
	RO_IN, RO_EQU, RO_NEQ, RO_LE,
	RO_GT, RO_LEQ, RO_GTQ, RO_NONE, RO_EMPTY
} range_op_t;

class range_condition_t: public heap_node_t
{
public:
	heap_ptr_t		lower;
	heap_ptr_t		upper;
	s_code_word_t	lval;
	s_code_word_t	uval;
	range_op_t		op;
	DECOMPILE(range_condition_t)
	range_condition_t()
	{
		lower = upper = (u_s_code_word_t)0;
		lval = uval = UNDEFINED;
	}
	void Emit();
};

class subdecision_node_t: public heap_node_t
{
public:
	lh_list_t<range_condition_t>	answer; // Empty => ELSE
	transition_t					transition;
	DECOMPILE(subdecision_node_t)
	void Emit();
};

class decision_node_t: public heap_node_t
{
public:
	heap_ptr_t						question;
	lh_list_t<subdecision_node_t>	answers;
	DECOMPILE(decision_node_t)
	void Emit();
	int Check(Bool_t isInitial, Bool_t isProcess);
};

//------------------------------------------------------------------
// VARIABLE DEFINITIONS
//------------------------------------------------------------------

class variable_name_t: public name_t
{
public:
	heap_ptr_t		type;	// offset of data_def_t filled in in second pass
	s_code_word_t			offset; // used in 2nd pass for address 
	void Mark();
	void *matchesName(s_code_word_t n)
	{
		if (index()==n)
		{
			lastIDtype = type;
			lastIDoffset = offset;
			return this;
		}
		else return NULL;
	}
};

class variable_decl_t: public heap_node_t
{
public:
	lh_list_t<variable_name_t>	names;
	ident_t				sort;
	heap_ptr_t			value; // initial value ground expression
	heap_ptr_t			vtype; // initial value type
	s_code_word_t		ival;  // value of initial value ground expression
	DECOMPILE(variable_decl_t)
	variable_decl_t()
	{
		value = (u_s_code_word_t)0;
		ival = UNDEFINED;
	}
	void Mark();
	void Check();
	int  Address(int n);
	void Emit();
	void *matchesName(s_code_word_t n);
};

class variable_def_t: public heap_node_t
{
public:
	Bool_t		isRevealed;
	Bool_t		isExported;
	lh_list_t<variable_decl_t> decls;
	DECOMPILE(variable_def_t)
	variable_def_t()
		{ isExported = isRevealed = False; }
	void Mark();
	void Check();
	int  Address(int n);
	void Emit();
	void *matchesName(s_code_word_t n);
};

//------------------------------------------------------------
// CHANNELS AND SIGNALS
//------------------------------------------------------------

typedef u_s_code_word_t set_bitmask_t[SIGSET_SIZE];

// signalsets are used by me to do internal representation
// of signal lists and sets

class signalset_t
{
public:
	set_bitmask_t bitmask;
	signalset_t();
	void clear();
	void set();
	int disjoint(signalset_t &s2);
	void add(int elt)
	{
		bitmask[elt/SWORD_SIZE] |= 1 << (elt%SWORD_SIZE);
	}
	void remove(int elt)
	{
		bitmask[elt/SWORD_SIZE] &= ~(1 << (elt%SWORD_SIZE));
	}
	void add(set_bitmask_t &b);
	void add(signalset_t &s);
	Bool_t hasSignal(int idx); // used in 2nd pass
	int operator==(const signalset_t&) const;
	int operator!=(const signalset_t&) const;
};

class signal_def_t;
class timer_def_t;

// The next class is a lookup table mapping signal
// indices (2nd pass) with signal definitions, for
// use by the interpreter.

class signal_tbl_t : public heap_object_t
{
private:
	heap_ptr_t	sigoff[MAX_SIGNALS];
	char		isTimer[MAX_SIGNALS];
public:
	signal_tbl_t();
	void Enter(int idx, timer_def_t *sd);
	void Enter(int idx, signal_def_t *sd);
	void *Lookup(int idx, int &type);
	char *GetName(int idx);
};

extern signal_tbl_t *signalTbl;

class signallist_t: public heap_node_t
{
public:
	lh_list_t<ident_t>	signallists;// Signal list ID list (non-standard) 
	lh_list_t<ident_t>	signals;	// Signal ID list				
	signalset_t			sigset;		// Signal set created in 2nd pass	

	Bool_t isEmpty() const;
	DECOMPILE(signallist_t)
	int  Address(int n);
	Bool_t hasSignal(int idx); // used in 2nd pass
	Bool_t hasSignal(signal_def_t *sig);
	void add(int elt)			{ sigset.add(elt); }
	void add(signalset_t &s)	{ sigset.add(s); }
};

class path_t: public heap_node_t
{
public:
	ident_t				originator;	// Originating block/process
	ident_t				destination;// Destination block/process
	signallist_t		s;
	int					idx;		// route or channel table entry (2nd pass)
	path_t() : originator(*noName) , destination(*noName)
		{ }
	void Originator(ident_t v)
		{ originator = v; }
	void Destination(ident_t v)
		{ destination = v; }
	const ident_t Originator() const
		{ return originator; }
	const ident_t Destination() const
		{ return destination; }
	DECOMPILE(path_t)
	int Address(int n);
};

/* Channel and signal route definitions */

typedef struct route_struct *route_ptr_t;

class route_def_t : public heap_node_t
{
public:
	name_t				nm;
	path_t				paths[2];
	int					endpt[2]; // used by sdlgr
	route_def_t(name_t n = *noName)
		{ nm = n; }
	const char *name() const
		{ return nm.name(); }
	DECOMPILE(route_def_t)
	MATCHES()
	void Check();
	void Address2();
	void Mark() { nm.Mark(); }
};

class signal_def_t: public heap_node_t
{
public:
	name_t				nm;
	lh_list_t<ident_t>	sortrefs;	// sort or syntype idents
	s_code_word_t				idx;		// globally-unique index used in 2nd pass
	signal_def_t(name_t n = *noName)
		{ nm = n; }
	const char *name() const
		{ return nm.name(); }
	const s_code_word_t index() const
		{ return nm.index(); }
	DECOMPILE(signal_def_t)
	void Mark() { nm.Mark(); }
	void Check();
	int  Address(int n);
	MATCHES()
};

class siglist_def_t: public heap_node_t
{
public:
	name_t			nm;
	signallist_t	s;
	siglist_def_t(name_t n = *noName)
		{ nm = n; }
	const char *name() const
		{ return nm.name(); }
	DECOMPILE(siglist_def_t)
	void Mark() { nm.Mark(); }
	void Check();
	MATCHES()
	int  Address(int n);
};

/* Channel connection definitions */

class connect_def_t: public heap_node_t
{
public:
	ident_t				channel;	/* Channel identifier			*/
	lh_list_t<ident_t>	subchannels;/* Subchannel identifiers	*/
	DECOMPILE(connect_def_t)
	void Check();
};

/* Channel-to-route connection definitions */

class c2r_def_t: public heap_node_t
{
public:
	ident_t				channel;	/* Channel identifier			*/
	lh_list_t<ident_t>	routes;		/* Signal route identifiers	*/
	DECOMPILE(c2r_def_t)
	void Check();
	void Address2();
};

// import definition

class import_def_t: public heap_node_t
{
public:
	lh_list_t<name_t>	names;		/* Variable names			*/
	ident_t				sort;		/* variable sort ID			*/
	DECOMPILE(import_def_t)
	void Mark();
};

// view definition

class view_def_t: public heap_node_t
{
public:
	lh_list_t<ident_t>	variables;	/* Variable IDs				*/
	ident_t				sort;		/* variable sort ID			*/
	DECOMPILE(view_def_t)
	void Check();
};

/* Timer definitions */

class timer_def_t: public heap_node_t
{
public:
	name_t				nm;
	lh_list_t<ident_t>	paramtypes;	// argument sorts
	s_code_word_t				idx;		// globally-unique index used in 2nd pass
	timer_def_t(name_t n = *noName)
		{ nm = n; }
	const char *name() const
		{ return nm.name(); }
	DECOMPILE(timer_def_t)
	MATCHES()
	void Check();
	void Mark() { nm.Mark(); }
	int  Address(int n);
};

//---------------------------------------
// State Nodes
//---------------------------------------

class stimulus_t: public heap_node_t
{
public:
	ident_t				signal;	// timer or signal
	lh_list_t<ident_t>	variables;	// var params
	DECOMPILE(stimulus_t)
};

class input_part_t: public heap_node_t
{
public:
	Bool_t					isAsterisk;	// input list is an asterisk
	lh_list_t<stimulus_t>	stimuli;	// not asterisk but a stimulus list
	heap_ptr_t				enabler;	// optional enabling condition
	transition_t			transition;	// action
	DECOMPILE(input_part_t)
};

class save_part_t: public heap_node_t
{
public:
	Bool_t			isAsterisk;	// Save all
	signallist_t 	savesigs;	// Save these
	DECOMPILE(save_part_t)
};

class continuous_signal_t: public heap_node_t
{
public:
	s_code_word_t	priority;
	heap_ptr_t		condition;	// PROVIDED condition
	transition_t	transition;
	DECOMPILE(continuous_signal_t)
};

class state_node_t: public heap_node_t
{
public:
	Bool_t							isAsterisk;
	u_s_code_word_t					from[MAX_STATES/SWORD_SIZE];
	signalset_t						save;
	s_code_word_t					nextstate;
	lh_list_t<name_t>				states;
	lh_list_t<input_part_t>			inputs;
//lh_list_t<input_part_t>			pinput; // priority inputs
	lh_list_t<save_part_t>			saves;
	lh_list_t<continuous_signal_t>	csigs;
	input_part_t *newinput();
#if 0
	input_part_t *newpinput();
#endif
	save_part_t *newsave();
	continuous_signal_t *newcsig();
	DECOMPILE(state_node_t)
	void Check(Bool_t isProcess, Bool_t &hasNonAstrsk);
	int Emit(int &lbl);
	void *matchesName(s_code_word_t n);
	int Address(int n, lh_list_t<name_entry_t> &nTbl, int &nCnt,
			name_entry_t* lTbl, int &lCnt);
};

//------------------------------------------------------------------
// REFERENCE FLAGS FOR PROCEDURES, PROCESSES, SUBBLOCKS AND BLOCKS
//------------------------------------------------------------------

#define RF_REMOTE		1
#define RF_DEFINED	2
#define RF_PRINTREF	4

//------------------------------------------------------------------
// PROCEDURE DEFINITIONS
//------------------------------------------------------------------

class procedure_param_name_t: public name_t
{
public:
	heap_ptr_t		type;	// offset of type_def_t filled in in second pass
	s_code_word_t offset; // offset of param used in 2nd pass
	void Mark();
	void *matchesName(s_code_word_t n)
	{
		if (index()==n)
		{
			lastIDtype = type;
			lastIDoffset = offset;
			return this;
		}
		else return NULL;
	}
};

class procedure_formal_param_t: public heap_node_t
{
public:
	Bool_t				isValue;
	lh_list_t<procedure_param_name_t>	names;
	ident_t				sort;
	DECOMPILE(procedure_formal_param_t)
	void Mark();
	void Check();
	void *matchesName(s_code_word_t n);
};

class procedure_def_t: public heap_node_t
{
public:
	name_t						nm;
	unsigned char				reference;
	lh_list_t<data_def_t>		types;	// Data type definitions
	lh_list_t<procedure_formal_param_t>	params;		// process formal parameters	*/
	lh_list_t<variable_def_t> 	variables;	// Variables
	heap_ptr_t					procedures; // offset on heap to linked list
	transition_t				start;		// Start node
	lh_list_t<state_node_t> 	states;		// Body states
	lh_list_t<name_entry_t>		stateTbl;	// Maps state names to indices
	int							numStates;	// number of entries in above table
	name_entry_t				*labelTbl;	// Maps label names to indices
	int							numLabels;	// number of entries in above table
	int							startLbl;	// label of start of code (2nd pass)
	int							level;		// scope level
	// Things used for code generation
	s_code_word_t						varSize;
	s_code_word_t						parSize;
	s_code_word_t						tmpSpace;

	procedure_def_t(name_t n = *noName);
	const s_code_word_t index() const;
	void SetReference(unsigned char v);
	void ClearReference(unsigned char v);
	Bool_t IsReference(unsigned char v);
	void name(char *n);
	void name(s_code_word_t n);
	const char *name() const;
	variable_def_t *newvariable();
	heap_ptr_t findNodeByName(name_t nm, typeclass_t *typ);
	DECOMPILE(procedure_def_t)
	void Check();
	void CheckRemote();
	void Emit();
	int  Address(int n);
	MATCHES()
	void Mark() { nm.Mark(); }
	int StateNm2Idx(s_code_word_t nm);
	char *StateIdx2Nm(s_code_word_t idx);
};

/* Process definition */

class process_param_name_t: public name_t
{
public:
	heap_ptr_t		type;	// offset of type_def_t filled in in second pass
	s_code_word_t offset; // offset of param used in 2nd pass
	void Mark();
	void *matchesName(s_code_word_t n)
	{
		if (index()==n)
		{
			lastIDtype = type;
			lastIDoffset = offset;
			return this;
		}
		else return NULL;
	}
};

class process_formal_param_t: public heap_node_t
{
public:
	lh_list_t<process_param_name_t>	names;
	ident_t				sort;
	DECOMPILE(process_formal_param_t)
	void Mark();
	void Check();
	void *matchesName(s_code_word_t n);
};

class process_def_t: public heap_node_t
{
public:
	name_t						nm;
	unsigned char				reference;
	heap_ptr_t					initInst;	// Initial number of instances expr
	heap_ptr_t					maxInst;	// Maximum number of instances expr
	s_code_word_t						initval;	// Initial number of instances val
	s_code_word_t						maxval;		// Maximum number of instances val
	lh_list_t<siglist_def_t> 	siglists;	// Signal lists**
	lh_list_t<procedure_def_t>	procedures;	// Procedures
	transition_t				start;		// Start node
	lh_list_t<state_node_t> 	states;		// Body states
	lh_list_t<data_def_t>		types;		// Data types definition
	lh_list_t<process_formal_param_t>	params;		// process formal parameters	*/
	signallist_t			 	validsig;	// Valid input signal list**
	lh_list_t<variable_def_t> 	variables;	// Variables
	lh_list_t<view_def_t>  		views;		// Views
	lh_list_t<import_def_t>		imports;	// Imports
	lh_list_t<signal_def_t>  	signals;   	// Signals
	lh_list_t<timer_def_t>  	timers;		// Timers
	lh_list_t<name_entry_t>		stateTbl;	// Maps state names to indices
	int							numStates;	// number of entries in above table
	name_entry_t				*labelTbl;	// Maps label names to indices
	int							numLabels;	// number of entries in above table
	int							startLbl;	// label of start of code (2nd pass)
	int							level;		// scope level
	// Things used for code generation
	s_code_word_t						varSize;
	s_code_word_t						parSize;
	s_code_word_t						tmpSpace;

	int							num;		// position in list; used by sdlgr

	process_def_t(name_t n = *noName);
	void SetReference(unsigned char v);
	void ClearReference(unsigned char v);
	Bool_t IsReference(unsigned char v);
	void name(char *n);
	void name(s_code_word_t n);
	const char *name() const;
	const s_code_word_t index() const;
	siglist_def_t *newsiglist();
	variable_def_t *newvariable();
	heap_ptr_t findNodeByName(name_t nm, typeclass_t *typ);
	DECOMPILE(process_def_t)
	void Mark() { nm.Mark(); }
	MATCHES()
	void Check(Bool_t mustHaveVSig);
	void CheckRemote();
	void Emit();
	int  Address(int n);
	void Address2();
	int findVar(lh_list_node_t<qualifier_elt_t> *q, name_t &n,
		    data_def_t* &typ,
			Bool_t isExported, Bool_t isRevealed,
			int &idx, process_def_t* &p, Bool_t matchQ);
	int StateNm2Idx(s_code_word_t nm);
	char *StateIdx2Nm(s_code_word_t idx);
};

class channel_def_t : public heap_node_t
{
public:
	name_t				nm;
	path_t				paths[2];
	int					endpt[2]; // used by sdlgr
	channel_def_t(name_t n = *noName)
		{ nm = n; }
	const char *name() const
		{ return nm.name(); }
	DECOMPILE(channel_def_t)
	void Mark() { nm.Mark(); }
	void Address2();
	MATCHES()
	void Check();
};

// Block substructure definition

class subblock_def_t : public heap_node_t
{
public:
	name_t			nm;
	unsigned char	reference;
	heap_ptr_t		blocks;		// offset to list of blocks
	lh_list_t<channel_def_t>	channels;// channel definitions
	lh_list_t<connect_def_t>	connects;	// channel connections (48)
	lh_list_t<signal_def_t>		signals;	// signal definitions
	lh_list_t<siglist_def_t>	siglists;// signal list definitions **
	lh_list_t<data_def_t>		types;	// data type definitions
	subblock_def_t(name_t n = *noName);
	void SetReference(unsigned char v);
	void ClearReference(unsigned char v);
	Bool_t IsReference(unsigned char v);
	void name(const char *n);
	void name(s_code_word_t n);
	const char *name() const;
	channel_def_t *newchannel();
	connect_def_t *newchanconn();
	siglist_def_t *newsiglist();
	void Mark() { nm.Mark(); }
	MATCHES()
	DECOMPILE(subblock_def_t)
	void Check();
	void CheckRemote();
	void Emit();
	int  Address(int n);
};

/* Block definitions */

class block_def_t : public heap_node_t
{
public:
	name_t						nm;
	unsigned char						reference;
	lh_list_t<signal_def_t>  	signals;		// Signals
	lh_list_t<process_def_t> 	processes;		// Processes
	lh_list_t<data_def_t>		types;			// Data types definition
	lh_list_t<c2r_def_t>		connections;	// Channel to route connections
	lh_list_t<route_def_t>		routes;			// Signal routes
	lh_list_t<siglist_def_t> 	siglists;		// Signal lists**
	subblock_def_t				substructure;	// optional block substruct
	int							startLbl;	// label of start of code (2nd pass)
	int							num;		// position in list; used by sdlgr

	block_def_t(name_t n = *noName);
	void name(const char *n);
	void name(s_code_word_t n);
	const char *name() const;
	const s_code_word_t index() const;
	void SetReference(unsigned char v);
	void ClearReference(unsigned char v);
	Bool_t IsReference(unsigned char v);
	siglist_def_t *newsiglist();
	c2r_def_t *newc2r();
	route_def_t *newroute();
	heap_ptr_t findNodeByName(name_t nm, typeclass_t *typ);
	MATCHES()
	DECOMPILE(block_def_t)
	void Mark() { nm.Mark(); }
	void Check();
	void CheckRemote();
	void Emit();
	int  Address(int n);
	void Address2();
	int findVar(lh_list_node_t<qualifier_elt_t> *q, name_t &n,
		    data_def_t* &typ,
			Bool_t isExported, Bool_t isRevealed,
			int &idx, process_def_t* &p, Bool_t matchQ);
};
	
/* System definition */

class system_def_t : public heap_node_t
{
public:
	name_t						nm;
	lh_list_t<block_def_t>		blocks;	// block definitions
	lh_list_t<channel_def_t>	channels;// channel definitions
	lh_list_t<signal_def_t>		signals;	// signal definitions
	lh_list_t<siglist_def_t>	siglists;// signal list definitions **
	lh_list_t<data_def_t>		types;	// data type definitions

	system_def_t(name_t n = *noName);
	void name(char *n);
	const char *name() const;
	block_def_t *newblock();
	channel_def_t *newchannel();
	siglist_def_t *newsiglist();
	heap_ptr_t findNodeByName(name_t nm, typeclass_t *typ);
	DECOMPILE(system_def_t)
	MATCHES()
	void Check();
	void Emit(int doLink = 1);
	int  Address(int n);
	int findVar(lh_list_node_t<qualifier_elt_t> *q, name_t &n,
		    data_def_t* &typ,
			Bool_t isExported, Bool_t isRevealed,
			int &idx, process_def_t* &p);
};

//----------------------------------------------
// Scope stack class
//----------------------------------------------

class scope_stack_t
{
private:
	int level;
	typeclass_t  type[MAX_NEST];
	heap_ptr_t   offset[MAX_NEST];
	void _EnterScope(const typeclass_t typ, const heap_ptr_t off);
	void *_dequalify(const ident_t *id, typeclass_t &context);
public:
	scope_stack_t()
	{
		level = 0;
	}
	void EnterScope(const typeclass_t typ, const heap_ptr_t off);
	void EnterScope(const typeclass_t typ, const void *p);
	void SetScope(const typeclass_t *typ, const heap_ptr_t *off, int cnt);
	void SetScope(const typeclass_t typ, const heap_ptr_t off, int lvl)
		{ type[lvl] = typ; offset[lvl] = off; level = lvl+1; }
	int  Depth() { return level; }
	void Emit();
	void ExitScope();
	// Find an identifier given the type in context
	void *dequalify(const ident_t *id, const typeclass_t context);
	// As above, but for remote definitions with absolute qualifiers.
	// This has the side effect of setting the scope stack.
	void *qualify(const ident_t *id, const typeclass_t context);
	// Find an identifier and return the type in context
	void *findIdent(const ident_t *id, typeclass_t &context);
	int RelLvl(int lv)
	{
		return level - lv;
	}
	void *GetInfo(const ident_t &id, data_def_t* &dd, int &lvl, int &offset,
		typeclass_t &t);

	// search for name nm of type *typ
	// return 0 if not found
	// else put scope level in lvl, type in typ, and return heap offset
	heap_ptr_t findNodeInScope(name_t nm, typeclass_t *typ, int *lvl);

	// Similarly we must do one to find view identifiers. It should
	// save the scope stack and build a new one for the identifier
	// (via a modified dequalify() )
	// before searching and restoring the old stack

	s_code_word_t findLabel(name_t &nm);
	s_code_word_t findState(name_t &nm);
	void *GetContaining(const typeclass_t context);
	void Mangle(char *buf, int lvl=-1); // used in cpp translation for unique IDs
};

extern scope_stack_t *ScopeStack;

//#ifdef INLINE
#undef INLINE
#define INLINE inline
#include "sdlast.inl"
//#endif

int StateName2Index(lh_list_t<name_entry_t> &l, s_code_word_t nm);
char *StateIndex2Name(lh_list_t<name_entry_t> &l, s_code_word_t idx);

extern system_def_t *sys;	// root pointer of AST

// Predefined type definitions

extern data_def_t *intType, *naturalType, *boolType, *charType, *realType,
	*pidType, *durType, *timeType;

extern void lookupPredefinedTypes();

// Useful lookup macros for heap_ptr_t references in nodes

#define GetNameP(ho)		((name_t *)localHeap->address(ho))
#define GetIdentP(ho)		((ident_t *)localHeap->address(ho))
#define GetUnOpP(ho)		((unop_t *)localHeap->address(ho))
#define GetExprP(ho)		((expression_t *)localHeap->address(ho))
#define GetTransP(ho)		((transition_t *)localHeap->address(ho))
#define GetArrDefP(ho)	((array_def_t *)localHeap->address(ho))
#define GetSynonymDefP(ho) ((synonym_def_t *)localHeap->address(ho))
#define GetStrucDefP(ho)	((struct_def_t *)localHeap->address(ho))
#define GetEnumDefP(ho)	((enum_def_t *)localHeap->address(ho))
#define GetDataDefP(ho)	((data_def_t *)localHeap->address(ho))
#define GetBlkDefP(ho)	((block_def_t *)localHeap->address(ho))
#define GetSigDefP(ho)	((signal_def_t *)localHeap->address(ho))
#define GetSigLstP(ho)	((signallist_t *)localHeap->address(ho))
#define GetSigSetP(ho)	((signalset_t *)localHeap->address(ho))
#define GetTimerDefP(ho)	((timer_def_t *)localHeap->address(ho))
#define GetChanDefP(ho)	((channel_def_t *)localHeap->address(ho))
#define GetSysDefP(ho)	((system_def_t *)localHeap->address(ho))
#define GetProcdDefP(ho)	((procedure_def_t *)localHeap->address(ho))
#define GetProcsDefP(ho)	((process_def_t *)localHeap->address(ho))
#define GetNameTblP(ho)	((name_entry_t *)localHeap->address(ho))
#define GetVoidP(ho)		((void *)localHeap->address(ho))
#define GetOffset(p)		(localHeap->offset(p))

//---------------------------------------------------------------------
// Route Manager. Used to generate s-code or C++ code for outputs
// Built by second pass of compiler.

typedef struct
{
	heap_ptr_t	src;	// source block def
	heap_ptr_t	dest;	// destination block def
	heap_ptr_t	sigs;	// signal list
	heap_ptr_t	self;	// the channel definition
} channel_tbl_entry_t;

typedef struct
{
	heap_ptr_t	src;	// source process def
	heap_ptr_t	dest;	// destination process def
	heap_ptr_t	blk;	// containing block def
	heap_ptr_t	sigs;	// signal list
	int			ch;		// channel table entry if connected to ENV
} route_tbl_entry_t;

typedef struct
{
	heap_ptr_t		self;
	signalset_t		sigs;
} process_tbl_entry_t;

class route_manager_t : public heap_node_t
{
	channel_tbl_entry_t channelTbl[MAX_CHANNELS];
	int channelTblCnt;
	route_tbl_entry_t routeTbl[MAX_ROUTES];
	int routeTblCnt;
	process_tbl_entry_t processTbl[MAX_PROCESSES];
	int processTblCnt;
public:
	route_manager_t()
	{
		channelTblCnt = 0;
		routeTblCnt = 0;
		processTblCnt = 0;
	}
	int ChanIdx(heap_ptr_t p);

	// Channel table access

	int NumChans()					{ return channelTblCnt; }
	heap_ptr_t ChanSource(int c)	{ return channelTbl[c].src;	}
	block_def_t *ChanSourceP(int c)	{ return GetBlkDefP(channelTbl[c].src);	}
	heap_ptr_t ChanDest(int c)		{ return channelTbl[c].dest; }
	block_def_t *ChanDestP(int c)	{ return GetBlkDefP(channelTbl[c].dest); }
	heap_ptr_t ChanSigs(int c)		{ return channelTbl[c].sigs; }
	signallist_t *ChanSigsP(int c)	{ return GetSigLstP(channelTbl[c].sigs); }
	heap_ptr_t ChanDef(int c)		{ return channelTbl[c].self; }
	channel_def_t *ChanDefP(int c)	{ return GetChanDefP(channelTbl[c].self); }

	void ChanSource(int c, block_def_t *src)
		{ channelTbl[c].src = GetOffset(src); }
	void ChanDest(int c, block_def_t *dest)
		{ channelTbl[c].dest = GetOffset(dest); }
	void ChanSigs(int c, signallist_t *sigs)
		{ channelTbl[c].sigs = GetOffset(sigs); }
	void ChanDef(int c, channel_def_t *d)
		{ channelTbl[c].self = GetOffset(d); }

	int AddChannel(block_def_t *src, block_def_t *dest, signallist_t *sigs,
		channel_def_t *ch)
	{
		if (channelTblCnt >= MAX_CHANNELS) return -1;
		int c = channelTblCnt++;
		ChanSource(c, src);
		ChanDest(c, dest);
		ChanSigs(c, sigs);
		ChanDef(c, ch);
		return c;
	}

	// Route table access

	int NumRoutes()						{ return routeTblCnt; }
	heap_ptr_t RouteSource(int r)		{ return routeTbl[r].src;	}
	process_def_t *RouteSourceP(int r)	{ return GetProcsDefP(routeTbl[r].src);	}
	heap_ptr_t RouteDest(int r)			{ return routeTbl[r].dest; }
	process_def_t *RouteDestP(int r)	{ return GetProcsDefP(routeTbl[r].dest); }
	heap_ptr_t RouteBlock(int r)		{ return routeTbl[r].blk; }
	block_def_t *RouteBlockP(int r)		{ return GetBlkDefP(routeTbl[r].blk); }
	heap_ptr_t RouteSigs(int r)		{ return routeTbl[r].sigs; }
	signallist_t *RouteSigsP(int r)		{ return GetSigLstP(routeTbl[r].sigs); }
	heap_ptr_t RouteChan(int r)			{ return routeTbl[r].ch; }

	void RouteSource(int r, process_def_t *src)
		{ routeTbl[r].src = GetOffset(src); }
	void RouteDest(int r, process_def_t *dest)
		{ routeTbl[r].dest = GetOffset(dest); }
	void RouteBlock(int r, block_def_t *blk)
		{ routeTbl[r].blk = GetOffset(blk); }
	void RouteSigs(int r, signallist_t *sigs)
		{ routeTbl[r].sigs = GetOffset(sigs); }
	void RouteChan(int r, int chan)
		{ routeTbl[r].ch = chan; }

	int AddRoute(process_def_t *src, process_def_t *dest, block_def_t *block,
		signallist_t *sigs, int ch)
	{
		if (routeTblCnt >= MAX_ROUTES) return -1;
		int r = routeTblCnt++;
		RouteSource(r, src);
		RouteDest(r, dest);
		RouteBlock(r, block);
		RouteSigs(r, sigs);
		RouteChan(r, ch);
		return r;
	}

	// Process table access

	int NumProcesses()					{ return processTblCnt; }
	heap_ptr_t ProcessDef(int p)		{ return processTbl[p].self;	}
	process_def_t *ProcessDefP(int p)	{ return GetProcsDefP(processTbl[p].self);	}
	signalset_t *ProcessSigs(int p)		{ return &processTbl[p].sigs; }

	void ProcessDef(int p, process_def_t *def)
		{ processTbl[p].self = GetOffset(def); }
	void ProcessSigs(int p, signalset_t &s)
		{ processTbl[p].sigs = s; }

	int AddProcess(process_def_t *prc, signalset_t &sigs)
	{
		if (processTblCnt >= MAX_PROCESSES) return -1;
		int p = processTblCnt++;
		ProcessDef(p, prc);
		ProcessSigs(p, sigs);
		return p;
	}

	// Print tables
	void Print(FILE *fp);
};

extern route_manager_t *Router;

extern heap_ptr_t destinations[][2];

extern int GetOutputDest(signal_def_t *sig, lh_list_t<ident_t> &via);

//---------------------------------------------------------------------
// Various driver init/shutdown entry points

extern void initGlobalObjects(); // entry for sdlpp and sdlparse
extern void SaveAST(char *fname); // used at end of 1st and 2nd pass
extern void RestoreAST(char *fname); // entry for other drivers
extern void DeleteAST(); // exit for drivers

//------------------------------------------------------------------------
// Routines to get type of expressions

data_def_t *GetType(view_expr_t *v);
data_def_t *GetType(int_literal_t *i);
data_def_t *GetType(var_ref_t *v);
data_def_t *GetType(cond_expr_t *ce);
data_def_t *GetType(unop_t *op);
data_def_t *GetType(expression_t *e);

#endif // _SDLAST_H

