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

#pragma hdrfile "PRINT.SYM"
#define PRINT
#include "sdlast.h"

// Function templates for linked list iterators

template<class T>
void SetReferenceL(lh_list_t<T> &l)
{
	lh_list_node_t<T> *p = l.front();
	while (p)
	{
		p->info.SetReference(RF_PRINTREF);
		p = p->next();
	}
}

template<class T>
void ClearReferenceL(lh_list_t<T> &l)
{
	lh_list_node_t<T> *p = l.front();
	while (p)
	{
		p->info.ClearReference(RF_PRINTREF);
		p = p->next();
	}
}

// Force compiler to generate needed template functions 

void SetReferenceL(lh_list_t<block_def_t>&);
void ClearReferenceL(lh_list_t<block_def_t>&);
void SetReferenceL(lh_list_t<process_def_t>&);
void ClearReferenceL(lh_list_t<process_def_t>&);

#ifndef __MSDOS__
//-------------------------------------------------------------------
// Gnu compiler is dumb and doesn't make template-based ostream
// operators. We thus define them here with the preprocessor.
//-------------------------------------------------------------------

#define LSTDECOMPILE(Typ)\
	ostream& operator<<(ostream& os, lh_list_t<Typ>&l) \
	{ \
		lh_list_node_t<Typ> *p = (lh_list_node_t<Typ> *)l.front(); \
		while (p) \
		{ \
			if (dc_paren) os << '('; \
			os << (p->info); \
			if (dc_paren) os << ')'; \
			p = p->next(); \
			if (dc_sep && p) \
				os << dc_sep; \
		} \
		return os; \
	}

LSTDECOMPILE(connect_def_t)
LSTDECOMPILE(c2r_def_t)
LSTDECOMPILE(process_formal_param_t)
LSTDECOMPILE(variable_def_t)
LSTDECOMPILE(process_param_name_t)
LSTDECOMPILE(hash_entry_t)
LSTDECOMPILE(continuous_signal_t)
LSTDECOMPILE(field_name_t)
LSTDECOMPILE(import_def_t)
LSTDECOMPILE(input_part_t)
LSTDECOMPILE(subdecision_node_t)
LSTDECOMPILE(ident_t)
LSTDECOMPILE(process_def_t)
LSTDECOMPILE(procedure_formal_param_t)
LSTDECOMPILE(save_part_t)
LSTDECOMPILE(selector_t)
LSTDECOMPILE(view_def_t)
LSTDECOMPILE(procedure_def_t)
LSTDECOMPILE(timer_set_t)
LSTDECOMPILE(name_t)
LSTDECOMPILE(siglist_def_t)
LSTDECOMPILE(block_def_t)
LSTDECOMPILE(timer_reset_t)
LSTDECOMPILE(variable_name_t)
LSTDECOMPILE(range_condition_t)
LSTDECOMPILE(gnode_t)
LSTDECOMPILE(state_node_t)
LSTDECOMPILE(enum_elt_t)
LSTDECOMPILE(variable_decl_t)
LSTDECOMPILE(signal_def_t)
LSTDECOMPILE(timer_def_t)
LSTDECOMPILE(data_def_t)
LSTDECOMPILE(procedure_param_name_t)
LSTDECOMPILE(route_def_t)
LSTDECOMPILE(channel_def_t)
LSTDECOMPILE(assignment_t)
LSTDECOMPILE(output_arg_t)
LSTDECOMPILE(fieldgrp_t)
LSTDECOMPILE(stimulus_t)
#endif

//-------------------------------------------------------------------
// Decompilation support
//-------------------------------------------------------------------

#ifdef DECOMPILE
#undef DECOMPILE
#endif

#define DECOMPILE(Typ, v)	ostream& operator<<(ostream& os, Typ &v)

int indentLevel = 0;

char *indentation()
{
	static char buf[80];
	sprintf(buf, "%*c", 2*indentLevel, ' ');
	return buf;
}

DECOMPILE(name_t, n)
{
	os << n.name();
	return os;
}

DECOMPILE(name_entry_t, n)
{
	(void)n;
	return os;
}

DECOMPILE(hash_entry_t, h)
{
	os << h.index;
	return os;
}

DECOMPILE(hash_t, ht)
{
	int i;
	char oldsep = dc_sep;
	dc_sep = ',';
	for (i=0;i<MAXKEY;i++)
		if (!ht.hash[i].isEmpty())
			os << "Hash entry " << i << " is (" 
 			   << ht.hash[i] << ")" << endl;
	dc_sep = oldsep;
	return os;
}

DECOMPILE(namestore_t, ns)
{
	for (s_code_word_t i=NUM_RESERVED_WORDS;i<ns.next;i++)
	{
		const char *name = ns.name(i);
		os << "ID " << i << " is " << name
			<< " with key " << ns.ht.key(name) << endl;
	}
	os << ns.ht;
	return os;
}

DECOMPILE(system_def_t, s)
{
	os << "SYSTEM " << s.name() << ';';
	indentLevel++;
	os << s.types;
	SetReferenceL(s.blocks);
	os << s.blocks;
	ClearReferenceL(s.blocks);
	dc_sep = ',';
	if (!s.signals.isEmpty())
	{
		os << endl << indentation() << "SIGNAL";
		indentLevel++;
		os << s.signals << ';';
		indentLevel--;
	}
	dc_sep = '\0';
	os << s.siglists;
	os << s.channels;
	os << s.blocks;
	indentLevel--;
	os << endl << "ENDSYSTEM " << s.name() << ';' << endl ;
	return os;
}

DECOMPILE(signallist_t, s)
{
	char sep = dc_sep;
	dc_sep = ',';
	if (!s.signallists.isEmpty())
	{
		dc_paren = True;
		os << s.signallists;
		if (!s.signals.isEmpty()) os << ',';
		dc_paren = False;
	}
	os << s.signals;
	dc_sep = sep;
	return os;
}

DECOMPILE(siglist_def_t, s)
{
	os << endl << indentation() << "SIGNALLIST " << s.name() << '='
		<< s.s << ';';
	return os;
}

DECOMPILE(signal_def_t, s)
{
	os << endl << indentation() << s.name();
	if (!s.sortrefs.isEmpty())
	{
		char sep = dc_sep; dc_sep = ',';
		os << '(' << s.sortrefs << ')';
		dc_sep = sep;
	}
	return os;
}

DECOMPILE(connect_def_t, con)
{
	os << endl << indentation() << "CONNECT " <<
		con.channel << " AND " << con.subchannels << ';';
	return os;
}

DECOMPILE(c2r_def_t, c2r)
{
	os << endl << indentation() << "CONNECT " <<
		c2r.channel << " AND " << c2r.routes << ';';
	return os;
}

DECOMPILE(path_t, p)
{
	os << endl << indentation() << "FROM " << p.Originator() << " TO " 
		<< p.Destination();
	indentLevel++;
	os << endl << indentation() << "WITH ";
	os << p.s << ';';
	indentLevel--;
	return os;
}

DECOMPILE(channel_def_t, c)
{
	os << endl << indentation() << "CHANNEL " << c.name();
	indentLevel++;
	for (int i = 0; i < 2 ; i++)
	{
		if (i && c.paths[1].Originator().name()[0]=='\0')
			break;
		os << c.paths[i];
	}
	indentLevel--;
	os << endl << indentation() << "ENDCHANNEL " << c.name() << ';';
	return os;
}

DECOMPILE(route_def_t, r)
{
	os << endl << indentation() << "SIGNALROUTE " << r.name();
	indentLevel++;
	for (int i = 0; i < 2 ; i++)
	{
		if (i && r.paths[1].Originator().name()[0]=='\0')
			break;
		os << r.paths[i];
	}
	indentLevel--;
	return os;
}

DECOMPILE(subblock_def_t, s)
{
	lh_list_t<block_def_t> *bl
		= (lh_list_t<block_def_t> *)GetVoidP(s.blocks);
	os << endl << indentation() << "SUBSTRUCTURE " << s.name() << ';';
	indentLevel++;
	os << s.types;
	// Print block references
	if (s.blocks)
	{
		SetReferenceL(*bl);
		os << *bl;
		ClearReferenceL(*bl);
	}
	dc_sep = ',';
	if (!s.signals.isEmpty())
		os << endl << indentation() << "SIGNAL " << s.signals << ';';
	dc_sep = '\0';
	os << s.siglists;
	os << s.channels;
	os << s.connects;
	if (s.blocks) os << *bl;
	indentLevel--;
	os << endl << indentation() << "ENDSUBSTRUCTURE " << s.name() << ';';
	return os;
}

DECOMPILE(block_def_t, b)
{
	if (b.IsReference(RF_PRINTREF))
	{
		if (b.IsReference(RF_REMOTE))
			os << endl << indentation() << "BLOCK " << b.name() << " REFERENCED;";
		return os;
	}
	os << endl << indentation() << "BLOCK " << b.name() << ';';
	indentLevel++;
	char oldsep = dc_sep;
	os << b.types;
	// Print process references
	SetReferenceL(b.processes);
	os << b.processes;
	ClearReferenceL(b.processes);
	dc_sep = ',';
	if (!b.signals.isEmpty())
		os << endl << indentation() << "SIGNAL " << b.signals << ';';
	dc_sep = '\0';
	os << b.siglists;
	dc_sep = oldsep;
	os << b.routes;
	os << b.connections;
	os << b.processes;
	if (b.substructure.name()[0])
		os << b.substructure;
	indentLevel--;
	os << endl << indentation() << "ENDBLOCK " << b.name() << ';';
	return os;
}

DECOMPILE(qualifier_elt_t, q)
{
	assert(0);
	(void)q;
	return os;
}

DECOMPILE(ident_t, i)
{
	if (i.name()[0])
		os << Id2Str(i);
	return os;
}

DECOMPILE(array_def_t, a)
{
	expression_t *e = GetExprP(a.dimension);
	indentLevel++;
	os << endl << indentation() << "ARRAY (" << *e << ") OF "
		<< a.sort << ';';
	indentLevel--;
	return os;
}

DECOMPILE(synonym_def_t, s)
{
	if (s.has_sort) os << Id2Str(s.sort);
	os << " = ";
	if (s.expr)
	{
		expression_t *e = GetExprP(s.expr);
		os << *e;
	}
	else os << "EXTERNAL";
	os << ';';
	return os;
}

DECOMPILE(fieldgrp_t, f)
{
	char oldsep = dc_sep;
	os << endl << indentation();
	dc_sep = ',';
	os << f.names << ' ' << f.sort;
	dc_sep = oldsep;
	return os;
}

DECOMPILE(struct_def_t, s)
{
	char oldsep = dc_sep;
	os << " STRUCT";
	indentLevel++;
	dc_sep = ';';
	os << s.fieldgrps << ';';
	dc_sep = oldsep;
	indentLevel--;
	return os;
}

DECOMPILE(enum_def_t, e)
{
	char oldsep = dc_sep;
	indentLevel++;
	dc_sep = ',';
	os << endl << indentation() << "LITERALS " << e.elts << ';';
	dc_sep = oldsep;
	indentLevel--;
	return os;
}

DECOMPILE(data_def_t, d)
{
	if (&d == intType || &d == boolType || &d == charType ||
		&d == pidType || &d == durType || &d == timeType ||
		&d == realType || &d == naturalType)
			return os;
	if (d.tag==SYNONYM_TYP)
	{
		os << endl << indentation() << "SYNONYM " << d.name() << ' ';
		os << *GetSynonymDefP(d.contents);
	}
	else
	{
		os << endl << indentation() << "NEWTYPE " << d.name();
		switch (d.tag)
		{
		case ARRAY_TYP:
			os << *GetArrDefP(d.contents);
			break;
		case STRUCT_TYP:
			os << *GetStrucDefP(d.contents);
			break;
		case ENUM_TYP:
			os << *GetEnumDefP(d.contents);
			break;
		}
		os << endl << indentation() << "ENDNEWTYPE " << d.name();
	}
	return os;
}

DECOMPILE(import_def_t, i)
{
	char oldsep = dc_sep;
	dc_sep = ',';
	os << endl << indentation() << "IMPORTED " << i.names << ' '
		<< i.sort << ';' ;
	dc_sep = oldsep;
	return os;
}

DECOMPILE(timer_def_t, t)
{
	os << endl << indentation() << "TIMER " << t.name();
	if (!t.paramtypes.isEmpty())
	{
		char sep = dc_sep; dc_sep = ',';
		os << '(' << t.paramtypes << ')';
		dc_sep = sep;
	}
	os << ';';
	return os;
}

DECOMPILE(variable_decl_t, v)
{
	os << v.names << ' ' << v.sort;
	if (v.value)
	{
		expression_t *e = GetExprP(v.value);
		os << " := " << *e;
	}
	return os;
}

DECOMPILE(variable_def_t, v)
{
	char oldsep = dc_sep;
	dc_sep = ',';
	os << endl << indentation() << "DCL ";
	if (v.isExported) os << "EXPORTED ";
	if (v.isRevealed) os << "REVEALED ";
	os << v.decls << ';';
	dc_sep = oldsep;
	return os;
}

DECOMPILE(view_def_t, v)
{
	char oldsep = dc_sep;
	dc_sep = ',';
	os << endl << indentation() << "VIEWED " << v.variables << ' '
		<< v.sort << ';' ;
	dc_sep = oldsep;
	return os;
}

DECOMPILE(procedure_formal_param_t, p)
{
	char oldsep = dc_sep; dc_sep = ',';
	os << "IN";
	if (!p.isValue) os << "/OUT";
	os << ' ' << p.names << ' ' << p.sort;
	dc_sep = oldsep;
	return os;
}

//---------------------------------------------------

static void printArgs(ostream &os, lh_list_t<heap_ptr_t> &l)
{
	if (l.isEmpty()) return;
	lh_list_node_t<heap_ptr_t> *al = l.front();
	os << '(';
	for (;;)
	{
		expression_t *e = GetExprP(al->info);
		if (e) os << *e;
		al = al->next();
		if (al) os << ',';
		else break;
	}
	os << ')';
}

//---------------------------------------------------
// Procedure and process body statements
//---------------------------------------------------

DECOMPILE(selector_t, t)
{
	switch(t.type)
	{
	case SEL_FIELD:
		name_t *n = GetNameP(t.val);
		os << '!' << *n;
		break;
	case SEL_ELT:
		expression_t *e = GetExprP(t.val);
		os << '(' << *e << ')';
		break;
	}
	return os;
}

DECOMPILE(var_ref_t, v)
{
	ident_t *id = GetIdentP(v.id);
	os << *id;
	if (!v.sel.isEmpty()) os << v.sel;
	return os;
}

DECOMPILE(cond_expr_t, ce)
{
	expression_t *ip = GetExprP(ce.if_part);
	expression_t *tp = GetExprP(ce.then_part);
	expression_t *ep = GetExprP(ce.else_part);
	os << "IF " << *ip << " THEN " << *tp;
	if (ep) os << " ELSE " << *ep;
	return os;
}

DECOMPILE(real_literal_t, rl)
{
	os << *((s_code_real_t *)&rl.value);
	return os;
}

DECOMPILE(int_literal_t, il)
{
	os << il.value;
	return os;
}

DECOMPILE(timer_active_expr_t, tae)
{
	os << "ACTIVE (" << tae.timer;
	printArgs(os, tae.params);
	return os;
}

DECOMPILE(view_expr_t, ve)
{
	os << ve.var << ", ";
	if (ve.pid_expr)
	{
		expression_t *e = GetExprP(ve.pid_expr);
		os << *e;
	}
	return os;
}

DECOMPILE(unop_t, uo)
{
	void *p = GetVoidP(uo.prim);
	if (uo.op == T_MINUS) os << "-";
	else if (uo.op == T_NOT) os << "NOT ";
	switch (uo.type)
	{
	case PE_EMPTY:
		break;
	case PE_SLITERAL:
		assert(0);
	case PE_NLITERAL:
	case PE_ILITERAL:
		int_literal_t *i = (int_literal_t *)p;
		os << *i; 
		break;
	case PE_RLITERAL:
		real_literal_t *r = (real_literal_t *)p;
		os << *r; 
		break;
	case PE_BLITERAL:
		if (uo.prim) os << "True";
		else os << "False";
		break;
	case PE_EXPR:
		expression_t *e = (expression_t *)p;
		os << "( " << *e << " )"; 
		break;
	case PE_FIX:
		e = (expression_t *)p;
		os << "FIX(" << *e << ")"; 
		break;
	case PE_FLOAT:
		e = (expression_t *)p;
		os << "FLOAT(" << *e << ")"; 
		break;
	case PE_CONDEXPR:
		cond_expr_t *ce = (cond_expr_t *)p;
		os << *ce;
		break;
	case PE_NOW:
		os << "NOW"; 
		break;
	case PE_SELF:	
		os << "SELF"; 
		break;
	case PE_PARENT:
		os << "PARENT"; 
		break;
	case PE_OFFSPRING:
		os << "OFFSPRING"; 
		break;
	case PE_SENDER:
		os << "SENDER"; 
		break;
	case PE_TIMERACTIV:
		timer_active_expr_t *ta = (timer_active_expr_t *)p;
		os << *ta;
		break;
	case PE_IMPORT:
		os << "IMPORT ";
		goto view;
	case PE_VIEW:
		os << "VIEW ";
	view:
		view_expr_t *ie = (view_expr_t *)p;
		os << "(" << *ie << ")";
		break;
	case PE_VARREF:
		var_ref_t *vr = (var_ref_t *)p;
		os << *vr;
		break;
	case PE_ENUMREF:
		enum_ref_t *er = (enum_ref_t *)p;
		os << nameStore->name(er->index); 
		break;
	case PE_SYNONYM:
		synonym_ref_t *sr = (synonym_ref_t *)p;
		os << nameStore->name(sr->index);
		break;
	}
	return os;
}

DECOMPILE(expression_t, e)
{
	if (e.op == T_UNDEF) // primary?
	{
		unop_t *u = GetUnOpP(e.l_op);
		os << *u;
	}
	else
	{
		expression_t *l = GetExprP(e.l_op);
		expression_t *r = GetExprP(e.r_op);
		os << *l;
		switch(e.op)
		{
		case T_BIMP:
			os << " => "; break;
		case T_OR:
			os << " or "; break;
		case T_XOR:
			os << " xor "; break;
		case T_AND:
			os << " and "; break;
		case T_NOT:
			assert(0);
		case T_EQ:
			os << " = "; break;
		case T_NE:
			os << " /= "; break;
		case T_GT:
			os << " > "; break;
		case T_GE:
			os << " >= "; break;
		case T_LESS:
			os << " < "; break;
		case T_LE:
			os << " <= "; break;
		case T_IN:
			os << "=>"; break;
		case T_EQUALS:
			os << " == "; break;
		case T_MINUS:
			os << "-"; break;
		case T_CONCAT:
			os << "//"; break;
		case T_PLUS:
			os << "+"; break;
		case T_ASTERISK:
			os << "*"; break;
		case T_SLASH:
			os << "/"; break;
		case T_MOD:
			os << " mod "; break;
		case T_REM:
			os << " rem "; break;
		}
		os << *r;
	}
	return os;
}

DECOMPILE(assignment_t, a)
{
	os << endl << indentation() << "TASK " << a.lval << " := " ;
	expression_t *e = GetExprP(a.rval);
	os << *e << ';';
	return os;
}

DECOMPILE(output_arg_t, a)
{
	os << a.signal;
	printArgs(os, a.args);
	return os;
}

DECOMPILE(output_t, o)
{
	char oldsep = dc_sep; dc_sep = ',';
	os << "OUTPUT " << o.signals;
	dc_sep = oldsep;
	if (o.dest)
	{
		expression_t *e = GetExprP(o.dest);
		os << " TO " << *e;
	}
	if (!o.via.isEmpty())
	{
		char oldsep = dc_sep; dc_sep = ',';
		os << " VIA " << o.via ;
		dc_sep = oldsep;
	}
	os << ';';
	return os;
}

//DECOMPILE(pri_output_t, o)
//{
//	char oldsep = dc_sep; dc_sep = ',';
//	os << o.signals;
//	dc_sep = oldsep;
//	return os;
//}

DECOMPILE(timer_set_t, t)
{
	expression_t *e = GetExprP(t.time);
	os << '(' << *e << ',' << t.timer;
	printArgs(os, t.args);
	os << ')';
	return os;
}

DECOMPILE(timer_reset_t, t)
{
	os << '(' << t.timer;
	printArgs(os, t.args);
	os << ')';
	return os;
}

DECOMPILE(invoke_node_t, i)
{
	if (i.tag == T_CALL)
		os << "CALL ";
	else
		os << "CREATE ";
	os << i.ident;
	printArgs(os, i.args);
	os << ';';
	return os;
}

DECOMPILE(range_condition_t, rc)
{
	char *opS = "";
	if  (rc.op == RO_IN)
	{
		expression_t *e = GetExprP(rc.lower);
		os << *e;
	}
	switch (rc.op)
	{
	case RO_EQU:
		opS = "="; 
		break;
	case RO_NEQ:
		opS = "!="; 
		break;
	case RO_LE:
		opS = "<"; 
		break;
	case RO_LEQ:
		opS = "<="; 
		break;
	case RO_GT:
		opS = ">"; 
		break;
	case RO_GTQ:
		opS = ">="; 
		break;
	case RO_IN:
		opS = " : "; 
		break;
	default:
		break;
	}
	expression_t *e = GetExprP(rc.upper);
	os << opS << *e;
	return os;
}

DECOMPILE(subdecision_node_t, sd)
{
	if (sd.answer.isEmpty())
		os << endl << indentation() << "ELSE: ";
	else
	{
		char oldsep = dc_sep; dc_sep = ',';
		os << endl << indentation() << '(' << sd.answer << ')' << ':';
		dc_sep = oldsep;
	}
	indentLevel++;
	os << sd.transition;
	indentLevel--;
	return os;
}

DECOMPILE(decision_node_t, d)
{
	expression_t *e = GetExprP(d.question);
	os << "DECISION " << *e << ';';
	if (!d.answers.isEmpty())
	{
		char oldsep = dc_sep; dc_sep = 0;
		os << d.answers;
		dc_sep = oldsep;
	}
	os << endl << indentation() << "ENDDECISION;";
	return os;
}

DECOMPILE(read_node_t, r)
{
	os << "READ " << r.varref << ';';
	return os;
}

DECOMPILE(write_node_t, w)
{
	os << "WRITE " ;
	lh_list_node_t<heap_ptr_t> *e = w.exprs.front();
	assert(e);
	for (;;)
	{
		os << *(GetExprP(e->info));
		e = e->next();
		if (e) os << ',';
		else break;

	}
	os << ';';
	return os;
}

DECOMPILE(gnode_t, n)
{
	if (n.label.name()[0] != '\0')
		os << endl << indentation() << n.label.name() << ':';
	void *node = GetVoidP(n.node);
	if (n.type != N_TASK)
		os << endl << indentation();
	char oldsep = dc_sep;
	switch (n.type)
	{
	case N_TASK:
		dc_sep = 0;
		os << *( (lh_list_t<assignment_t> *)node );
		break;
	case N_EXPORT:
		dc_sep = ',';
		os << "EXPORT " <<  (*((lh_list_t<ident_t> *)node)) << ';';
		break;
	case N_OUTPUT:
		os << (*((output_t *)node));
		break;
	case N_CREATE:
		os << (*((invoke_node_t *)node));
		break;
	case N_CALL:
		os << (*((invoke_node_t *)node));
		break;
	case N_SET:
		dc_sep = ',';
		os << "SET " << (*((lh_list_t<timer_set_t> *)node)) << ';';
		break;
	case N_RESET:
		dc_sep = ',';
		os << "RESET " << (*((lh_list_t<timer_reset_t> *)node)) << ';';
		break;
	case N_READ:
		os << (*((read_node_t *)node));
		break;
	case N_WRITE:
		os << (*((write_node_t *)node));
		break;
	case N_DECISION:
		os << (*((decision_node_t *)node));
	}
	dc_sep = oldsep;
	return os;
}

DECOMPILE(transition_t, t)
{
	os << t.nodes;
	if (t.label.name()[0] != '\0')
	{
		indentLevel--;
		os << endl << indentation() << t.label.name() << ':';
		indentLevel++;
	}
	if (t.type != EMPTY)
	{
		os << endl << indentation();
		switch(t.type)
		{
		case NEXTSTATE:
			os << "NEXTSTATE ";
			if (t.next.name()[0] == '\0')
				os << "-;";
			else
				os << t.next.name() << ';';
			break;
		case JOIN:
			os << "JOIN " << t.next.name() << ';';
			break;
		case STOP:
			os << "STOP;";
			break;
		case RETURN:
			os << "RETURN;";
			break;
		}
	}
	return os;
}

DECOMPILE(stimulus_t, s)
{
	os << s.signal;
	if (!s.variables.isEmpty())
	{
		char oldsep = dc_sep; dc_sep = ',';
		os << '(' << s.variables << ')';
		dc_sep = oldsep;
	}
	return os;
}

DECOMPILE(input_part_t, i)
{
	os << endl << indentation() << "INPUT ";
	if (i.isAsterisk) os << '*' << ';';
	else os << i.stimuli << ';';
	if (i.enabler)
	{
		expression_t *e = GetExprP(i.enabler);
		os << endl << indentation() << "PROVIDED" << *e << ';'; // non-priority only!
	}
	indentLevel++;
	os << i.transition; 
	indentLevel--;
	return os;
}

DECOMPILE(save_part_t, s)
{
	if (s.isAsterisk)
		os << endl << indentation() << "SAVE *";
	else if (!s.savesigs.isEmpty())
		os << endl << indentation() << "SAVE " << s.savesigs;
	return os;
}

DECOMPILE(continuous_signal_t, s)
{
	os << endl << indentation() << "PROVIDED " << *GetExprP(s.condition) << ';';
	if (s.priority!=UNDEFINED)
		os << endl << indentation() << "PRIORITY " << s.priority << ';';

	indentLevel++;
	os << s.transition;
	indentLevel--;
	return os;
}

DECOMPILE(state_node_t, s)
{
	os << endl << indentation() << "STATE ";
	if (s.isAsterisk)
	{
		os << '*';
		if (!s.states.isEmpty())
			os << '(' << s.states << ')';
	}
	else os << s.states;
	os << ';';
	indentLevel++;
	char oldsep = dc_sep; dc_sep = 0;
	if (!s.saves.isEmpty())
		os << s.saves <<';';
//	if (!s.pinput.isEmpty())
//		os << endl << indentation() << "PRIORITY " << s.pinput <<';';
	if (!s.inputs.isEmpty())
		os << s.inputs;
	if (!s.csigs.isEmpty())
		os << s.csigs;
	indentLevel--;
	os << endl << indentation() << "ENDSTATE;";
	dc_sep = oldsep;
	return os;
}

DECOMPILE(procedure_def_t, p)
{
	char oldsep = dc_sep;
	os << endl << indentation() << "PROCEDURE " << p.name();
	if (p.IsReference(RF_PRINTREF|RF_REMOTE))
	{
		os << " REFERENCED;";
		return os;
	}
	else os<<';';
	if (!p.params.isEmpty())
	{
		dc_sep = ',';
		os << endl << indentation() << "FPAR " << p.params << ';';
		dc_sep = '\0';
	}
	indentLevel++;
	os << p.types << p.variables;
	if (p.procedures)
	{
		lh_list_t<procedure_def_t> *pl = 
			(lh_list_t<procedure_def_t> *)GetVoidP(p.procedures);
		os << *pl;
	}
	// procedure body
	os << endl << indentation() << "START;";
	indentLevel++;
	os << p.start;
	indentLevel--;
	os << p.states;
	indentLevel--;
	cout << endl << indentation() << "ENDPROCEDURE " << p.name() << ';';
	dc_sep = oldsep;
	return os;
}

DECOMPILE(process_formal_param_t, p)
{
	char oldsep = dc_sep; dc_sep = ',';
	os << p.names << ' ' << p.sort;
	dc_sep = oldsep;
	return os;
}

DECOMPILE(process_def_t, p)
{
	char oldsep = dc_sep;
	if (p.IsReference(RF_PRINTREF))
	{
		if (p.IsReference(RF_REMOTE))
			os << endl << indentation() << "PROCESS " << p.name() << " REFERENCED;";
		return os;
	}
	os << endl << indentation() << "PROCESS " << p.name() << '(';
	if (p.initInst!=0)
	{
		expression_t *e = GetExprP(p.initInst);
		os << *e;
	}
	os << ',';
	if (p.maxInst!=0)
	{
		expression_t *e = GetExprP(p.maxInst);
		os << *e;
	}
	os << ')' << ';';
	dc_sep = ',';
	if (!p.params.isEmpty())
		os << endl << indentation() << "FPAR " << p.params << ';';
	if (!p.validsig.isEmpty())
		os << endl << indentation() << "SIGNALSET " << p.validsig << ';';
	indentLevel++;
	if (!p.signals.isEmpty())
		os << endl << indentation() << "SIGNAL " << p.signals << ';';
	dc_sep = '\0';
	if (!p.timers.isEmpty())
		os << p.timers;
	os << p.types << p.siglists << p.views << p.imports << p.variables << p.procedures;
	// process body or service decomposition
	os << endl << indentation() << "START;";
	indentLevel++;
	os << p.start;
	indentLevel--;
	os << p.states;
	indentLevel--;
	cout << endl << indentation() << "ENDPROCESS " << p.name() << ';';
	dc_sep = oldsep;
	return os;
}


