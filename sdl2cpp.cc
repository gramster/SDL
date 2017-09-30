/*
 * SDL AST to C++ convertor
 *
 * Written by Graham Wheeler, September/October 1994
 * Data Network Architecture Laboratory
 * Department of Computer Science
 * University of Cape Town
 * 
 * (c) 1994 Graham Wheeler and the Data Network Architectures Laboratory
 * All Rights Reserved
 *
 * Created 20-9-94
 * Last modified: 2-10-94
 *
 */

#pragma hdrfile "sdl2cpp.sym"
#include "sdlast.h"
#include <math.h>
#include <time.h>
#if __MSDOS__
#include <dos.h>
#include <conio.h>
#endif

#define max(a,b)		( ((a)>(b)) ? (a) : (b) )
#define min(a,b)		( ((a)>(b)) ? (b) : (a) )

#define MAX_RESUMES		64 // limit on procedure calls per process

// Output files

FILE *cfp, *hfp, *testfp;

// command-line flags

extern int longIDs; // defined in sdlast.cpp; used by this prog only

// state table (built for each process)

typedef struct
{
	int pri;
	char *state;
	char *input;
	int prov;
	int deq;
	int act;
	int sav;
} state_tbl_entry_t;

state_tbl_entry_t stateTable[256];
int inputnum, provnum, actnum, savenum, testnum,
	inputcnt, provcnt, statecnt, savecnt, testcnt;
int tcount = 0;
int hasViewed, hasExported;

int inProcedure = 0;

// hackish global variables used to simplify code 

char	Owner[128], Caller[128]; // Owning process and caling procedure
		// when in a procedure - used to generate the Owner()/
		// Caller() pointer casting methods

//-------------------------------------------------------------------------
// Useful iterator macros

#define ForAll(var, typ, init) \
		for (lh_list_node_t<typ> *var = init;\
			var;\
			var = var->next())

#define ForAny(var, typ, init) \
	if (init)\
		for (lh_list_node_t<typ> *var = init;\
			var;\
			var = var->next())

#define ApplyToAll(fn, var, typ, init) \
	for (lh_list_node_t<typ> *var = init;\
		var;\
		var = var->next())\
			fn(var->info)

//----------------------------------------------------------
// Basic formatting routines

int indent = 0, lbl = 0;

void Indent(FILE *fp = NULL)
{
	for (int i = 0; i < indent; i++)
		fputc('\t', fp ? fp : cfp);
}

void Strip(FILE *fp)
{
	fprintf(fp, "\n//---------------------------------------------------------\n");
}

//-----------------------------------------------------------
// Names and identifiers

static void Crunch(char *buf)
{
	char tbuf[80];
	char *p, *b = buf;
	tbuf[0] = 0;
	while ((p = strchr(b, '@')) != NULL)
	{
		*p = 0;
		sprintf(tbuf + strlen(tbuf), "_%d", 
			nameStore->index(b));
		b = p+1;
	}
	strcpy(buf, b);
	strcat(buf, tbuf);
}

char *Qualifier()
{
	static char buf[200];
	ScopeStack->Mangle(buf);
	return buf;
}

char *Name(const char *nm = NULL)
{
	static char buf[80];
	buf[0] = 0;
	strcpy(buf, Qualifier());
	if(nm && nm[0]) strcat(buf, nm);
	else buf[strlen(buf)-1] = 0; // strip trailing underscore
	if (!longIDs) Crunch(buf);
	return buf;
}

char *Ident(ident_t &id, typeclass_t hint = Q_ANY, int *level = NULL)
{
	static char buf[200];
	int lvl;
	void *rtn;
	if (hint == Q_ANY) // weakness of this is that GetInfo with Q_ANY
			// is not a comprehensive search
	{
		data_def_t *dd;
		int off;
		rtn = ScopeStack->GetInfo(id, dd, lvl, off, hint);
	}
	else // quicker and more reliable
	{
		rtn = ScopeStack->dequalify(&id, hint);
		if (rtn == NULL && hint == Q_SIGNAL)
			rtn = ScopeStack->dequalify(&id, Q_TIMER);
		lvl = lastIDlevel;
	}
	assert(rtn);
	ScopeStack->Mangle(buf, lvl);
	strcat(buf, id.name());
	if (!longIDs) Crunch(buf);
	if (level) *level = lvl;
	return buf;
}

char *BasicTypeName(data_def_t *dd)
{
	if (dd == intType)	return "integer";
	if (dd == naturalType)	return "natural";
	if (dd == boolType)	return "boolean";
	if (dd == realType)	return "real";
	if (dd == durType)	return "duration";
	if (dd == timeType)	return "time";
	if (dd == pidType)	return "pid";
	return "unknown"; // shouldn't happen
}

char *GetProcessID(heap_ptr_t po)
{
	static char buf[80];
	ForAny(b, block_def_t, sys->blocks.front())
	{
		ForAny(p, process_def_t, b->info.processes.front())
		{
			if (GetOffset(&p->info) == po)
			{
				if (!longIDs)
					sprintf(buf, "%s_%d_ID",
						p->info.name(),
						b->info.nm.index());
				else 
					sprintf(buf, "%s_%s_ID",
						b->info.name(),
						p->info.name());
				return buf;
			}
		}
	}
	assert(0);
	return NULL;
}

//----------------------------------------------------------------
// stub needed by sdlast

int getLineNumber()
{
	return 0;
}

//------------------------------------------------------------------------
// See if expression is just TRUE; in this case no provided entry
// is generated

int IsTrue(expression_t *e)
{
	if (e && e->op == T_UNDEF)
	{
		unop_t *u = GetUnOpP(e->l_op);
		return (u->type == PE_BLITERAL && u->prim);
	}
	return 0;
}

//------------------------------------------------------------------------
void Convert(expression_t *e, FILE *fp); // forward decl
//------------------------------------------------------------------------
// Timer test conversion for timer active expressions and timer resets

void ConvertTimerTest(int idx, ident_t &id, lh_list_t<heap_ptr_t> &args)
{
	char *in = Ident(id, Q_TIMER);
	fprintf(testfp, "\t\tcase %d:\n\t\t{\n\t\t\t%s_t *sg = (%s_t *)s;\n",
		idx, in, in);
	fprintf(testfp, "\t\t\tif (!sg->isTimer || sg->id != %s_ID) return 0;\n", 
			in);
	int acnt = 0;
	ForAny(ta, heap_ptr_t, args.front())
	{
		expression_t *e = GetExprP(ta->info);
		acnt++;
		if (e)
		{
			data_def_t *dd = GetType(e);
			fprintf(testfp, "\t\t\tif (");
			if (dd && dd->tag == ARRAY_TYP)
			{
				fprintf(testfp, "memcmp(&sg->arg_%d, &", acnt);
				Convert(e, testfp);
				fprintf(testfp, ", sizeof(sg->arg_%d)", acnt);
			}
			else
			{
				fprintf(testfp, "sg->arg_%d != ", acnt);
				Convert(e, testfp); // beware of nesting!!
			}
			fprintf(testfp, ") return 0;\n");
		}
	}
	fprintf(testfp, "\t\t\treturn 1;\n\t\t}\n");
}

//------------------------------------------------------------------------
// Expression conversion

void Convert(int_literal_t *u, FILE *fp)
{
	fprintf(fp, "%ld", (long)u->value);
}

void Convert(real_literal_t *u, FILE *fp)
{
	fprintf(fp, "%f", *((s_code_real_t *)&u->value));
}

void Convert(cond_expr_t *ce, FILE *fp)
{
	expression_t *ip = GetExprP(ce->if_part);
	expression_t *tp = GetExprP(ce->then_part);
	expression_t *ep = GetExprP(ce->else_part);
	fprintf(fp, "( (");
	Convert(ip, fp);
	fprintf(fp, ") ? (");
	Convert(tp, fp);
	fprintf(fp, ") : (");
	if (ep)
		Convert(ep, fp);
	else fprintf(fp, "UNDEFINED");
	fprintf(fp, ") )");
}

void Convert(timer_active_expr_t *u, FILE *fp)
{
	fprintf(fp, "TimerActive(%d)", testnum);
	ConvertTimerTest(testnum++, u->timer, u->params);
}

void Convert(view_expr_t *u, FILE *fp)
{
	fprintf(fp, Ident(u->var));
	if (u->pid_expr)
	{
		fprintf(fp, ", ");
		Convert(GetExprP(u->pid_expr), fp);
	}
}

void Convert(var_ref_t *v, FILE *fp)
{
	ident_t *id = GetIdentP(v->id);
	int off, vlvl, curlvl = ScopeStack->Depth();
	data_def_t *dd;
	typeclass_t t;
	void *vdef = ScopeStack->GetInfo(*id, dd, vlvl, off, t);
	char *idn = Ident(*id, t);
	int isRefP = (t==Q_PROCEDUREPAR && !((procedure_formal_param_t *)vdef)->isValue);
	if (isRefP) fprintf(fp, "(*(");
	if (vlvl != curlvl)
	{
		if (vlvl == 3) // Process level?
			fprintf(fp, "(Owner()->");
		else
		{
			assert(curlvl > vlvl);
			while (curlvl > vlvl)
			{
				curlvl--;
				fprintf(fp, "Caller()->");
			}
		}
		fprintf(fp, "%s)", idn);
	}
	else fprintf(fp, idn);
	if (isRefP) fprintf(fp, "))");
	ForAny(sel, selector_t, v->sel.front())
	{
		switch(sel->info.type)
		{
		case SEL_FIELD:
			fprintf(fp, ".");
			fprintf(fp, GetNameP(sel->info.val)->name());
			break;
		case SEL_ELT:
			fprintf(fp, "[");
			Convert(GetExprP(sel->info.val), fp);
			fprintf(fp, "]");
			break;
		}
	}
}

void Convert(unop_t *u, FILE *fp)
{
	void *p = GetVoidP(u->prim);
	if (u->op == T_MINUS)
		fprintf(fp, "-");
	else if (u->op == T_NOT)
		fprintf(fp, "!");
	switch (u->type)
	{
	case PE_EMPTY:
		break;
	case PE_SLITERAL:
		assert(0);
	case PE_NLITERAL:
	case PE_ILITERAL:
		int_literal_t *i = (int_literal_t *)p;
		Convert(i, fp);
		break;
	case PE_RLITERAL:
		real_literal_t *r = (real_literal_t *)p;
		Convert(r, fp);
		break;
	case PE_BLITERAL:
		if (u->prim)
			fprintf(fp, "TRUE");
		else
			fprintf(fp, "FALSE");
		break;
	case PE_EXPR:
		expression_t *e = (expression_t *)p;
		fprintf(fp, "(");
		Convert(e, fp);
		fprintf(fp, ")");
		break;
	case PE_FIX:
		e = (expression_t *)p;
		fprintf(fp, "( (int) (");
		Convert(e, fp);
		fprintf(fp, ") )");
		break;
	case PE_FLOAT:
		e = (expression_t *)p;
		fprintf(fp, "( (float) (");
		Convert(e, fp);
		fprintf(fp, ") )");
		break;
	case PE_CONDEXPR:
		cond_expr_t *ce = (cond_expr_t *)p;
		Convert(ce, fp);
		break;
	case PE_NOW:
		fprintf(fp, "Now()");
		break;
	case PE_SELF:	
		fprintf(fp, "Self()");
		break;
	case PE_PARENT:
		fprintf(fp, "Parent()");
		break;
	case PE_OFFSPRING:
		fprintf(fp, "Offspring()");
		break;
	case PE_SENDER:
		fprintf(fp, "Sender()");
		break;
	case PE_TIMERACTIV:
		timer_active_expr_t *ta = (timer_active_expr_t *)p;
		Convert(ta, fp);
		break;
	case PE_IMPORT:
		fprintf(fp, "Import(");
		goto view;
	case PE_VIEW:
		fprintf(fp, "View(");
	view:
		Convert((view_expr_t *)p, fp);
		fprintf(fp, ")");
		break;
	case PE_VARREF:
		Convert((var_ref_t *)p, fp);
		break;
	case PE_ENUMREF:
		enum_ref_t *er = (enum_ref_t *)p;
		fprintf(fp, Name(nameStore->name(er->index))); 
		break;
	case PE_SYNONYM:
		synonym_ref_t *sr = (synonym_ref_t *)p;
		fprintf(fp, Name(nameStore->name(sr->index))); 
		break;
	}
}

void Convert(expression_t *e, FILE *fp)
{
	if (e->op == T_UNDEF) // primary?
	{
		Convert(GetUnOpP(e->l_op), fp);
	}
	else
	{
		expression_t *l = GetExprP(e->l_op);
		expression_t *r = GetExprP(e->r_op);
		Convert(l, fp);
		switch(e->op)
		{
		case T_BIMP:
//			fprintf(fp, " => ");
			break;
		case T_OR:
			fprintf(fp, " || ");
			break;
		case T_XOR:
			fprintf(fp, " ^ ");
			break;
		case T_AND:
			fprintf(fp, " && ");
			break;
		case T_NOT:
			assert(0);
		case T_EQ:
			fprintf(fp, " == ");
			break;
		case T_NE:
			fprintf(fp, " != ");
			break;
		case T_GT:
			fprintf(fp, " > ");
			break;
		case T_GE:
			fprintf(fp, " >= ");
			break;
		case T_LESS:
			fprintf(fp, " < ");
			break;
		case T_LE:
			fprintf(fp, " <= ");
			break;
		case T_IN:
//			fprintf(fp, "=>");
			break;
		case T_EQUALS:
//			fprintf(fp, "==");
			break;
		case T_MINUS:
			fprintf(fp, " - ");
			break;
		case T_CONCAT:
//			fprintf(fp, "//");
			break;
		case T_PLUS:
			fprintf(fp, " + ");
			break;
		case T_ASTERISK:
			fprintf(fp, " * ");
			break;
		case T_SLASH:
			fprintf(fp, " / ");
			break;
		case T_MOD:
		case T_REM:
			fprintf(fp, " % ");
			break;
		}
		Convert(r, fp);
	}
}

//------------------------------------------------------------------------
// Data type conversion

void Convert(data_def_t &d)
{
	if (&d == intType || &d == boolType || &d == charType ||
		&d == pidType || &d == durType || &d == timeType ||
		&d == realType || &d == naturalType)
			return;
	if (d.tag==SYNONYM_TYP)
	{
		synonym_def_t *sd = GetSynonymDefP(d.contents);
		char *si;
		if (sd->has_sort)
			si = Ident(sd->sort, Q_TYPE);
		else
		{
			data_def_t *dd = GetType(GetExprP(sd->expr));
			si = BasicTypeName(dd);
		}
		fprintf(hfp, "const %s_typ", si);
		fprintf(hfp, " %s = ", Name(d.name()));
		if (sd->expr)
			Convert(GetExprP(sd->expr), hfp);
		else fprintf(hfp, "EXTERNAL");
	}
	else
	{
		fprintf(hfp, "typedef ");
		switch (d.tag)
		{
		case ARRAY_TYP:
			array_def_t *a = GetArrDefP(d.contents);
			fprintf(hfp, "%s_typ %s_typ[", Ident(a->sort, Q_TYPE), d.name());
			Convert(GetExprP(a->dimension), hfp);
			fprintf(hfp, "]");
			break;
		case STRUCT_TYP:
			fprintf(hfp, "struct\n{\n");
			ForAny(f, fieldgrp_t, 
				GetStrucDefP(d.contents)->fieldgrps.front())
			{
				fprintf(hfp, "\t%s_typ", Ident(f->info.sort, Q_TYPE));
				int ch = ' ';
				ForAny(n, field_name_t, f->info.names.front())
				{
					fprintf(hfp, "%c %s",	ch, n->info.name());
					ch = ',';
				}
				fprintf(hfp, ";\n");
			}
			fprintf(hfp, "} %s_typ", d.name());
			break;
		case ENUM_TYP:
			fprintf(hfp, "enum\n{\n");
			int ch = ' ';
			ForAny(ee, enum_elt_t,
				GetEnumDefP(d.contents)->elts.front())
			{
				fprintf(hfp, "%c %s",	ch, ee->info.name());
				ch = ',';
			}
			fprintf(hfp, "\n} %s_typ", d.name());
			break;
		}
	}
	fprintf(hfp, ";\n\n");
}

//---------------------------------------------------------------------
// Signal and siglist declaration conversion

void Convert(signal_def_t &sd)
{
	char *n = Name(sd.name());
	fprintf(hfp, "class %s_t : public signal_t\n{\n", n);
	int argcnt = 0;
	fprintf(hfp, "public:\n");
	ForAny(sr, ident_t, sd.sortrefs.front())
	{
		fprintf(hfp, "\t%s_typ arg_%d;\n", Ident(sr->info, Q_TYPE), ++argcnt);
	}
	//n = Name(sd.name());
	fprintf(hfp, "\t%s_t()\n\t{\n", n);
	if (sd.sortrefs.front())
		fprintf(hfp, "\t\tmemset(this, 0, sizeof(%s_t));\n", n);
	fprintf(hfp, "\t\tid = %s_ID;\n", n);
	fprintf(hfp, "\t}\n");
	fprintf(hfp, "};\n\n");
}

void Convert(timer_def_t &tm)
{
	char *n = Name(tm.name());
	fprintf(hfp, "class %s_t : public signal_t\n{\n", n);
	int argcnt = 0;
	fprintf(hfp, "public:\n");
	ForAny(tr, ident_t, tm.paramtypes.front())
	{
		fprintf(hfp, "\t%s_typ arg_%d;\n", Ident(tr->info, Q_TYPE), ++argcnt);
	}
	//n = Name(tm.name());
	fprintf(hfp, "\t%s_t()\n\t{\n", n, n);
	if (tm.paramtypes.front())
		fprintf(hfp, "\t\tmemset(this, 0, sizeof(%s));\n", n);
	fprintf(hfp, "\t\tisTimer = 1;\n");
	fprintf(hfp, "\t\tid = %s_ID;\n", n);
	fprintf(hfp, "\t}\n");
	fprintf(hfp, "};\n\n");
}

void Convert(siglist_def_t &sl)
{
	(void)sl;
}

void Convert(variable_decl_t &vdc)
{
	int ch = ' ';
	fprintf(hfp, "\t%s_typ\t", Ident(vdc.sort, Q_TYPE));
	ForAny(vn, variable_name_t, vdc.names.front())
	{
		fprintf(hfp, "%c %s", ch, Name(vn->info.name()));
		ch = ',';
	}
	fprintf(hfp, ";\n");
}

void ConvertCopy(variable_decl_t &vdc)
{
	int ch = ' ';
	fprintf(hfp, "\t%s_typ\t", Ident(vdc.sort, Q_TYPE));
	ForAny(vn, variable_name_t, vdc.names.front())
	{
		fprintf(hfp, "%c __%s", ch, Name(vn->info.name()));
		ch = ',';
	}
	fprintf(hfp, ";\n");
}

void Convert(variable_def_t &vd)
{
	if (vd.isExported)
		hasExported = 1;
	if (vd.isRevealed)
		hasViewed = 1;
	ForAny(vdc, variable_decl_t, vd.decls.front())
	{
		Convert(vdc->info);
		if (vd.isExported)
			ConvertCopy(vdc->info);
	}
}

void ConvertReveal(variable_decl_t &vdc)
{
	ForAny(vn, variable_name_t, vdc.names.front())
	{
		Indent();fprintf(cfp, "if (idx == %d)\n", 
				vn->info.index());
		Indent();fprintf(cfp, "\treturn (void *)&%s;\n",
				Name(vn->info.name()));
	}
}

void ConvertExport(variable_decl_t &vdc)
{
	ForAny(vn, variable_name_t, vdc.names.front())
	{
		Indent();fprintf(cfp, "if (idx == %d)\n", 
				vn->info.index());
		Indent();fprintf(cfp, "\treturn (void *)&__%s;\n",
				Name(vn->info.name()));
	}
}

void ConvertReveal(variable_def_t &vd)
{
	if (vd.isRevealed)
	{
		ForAny(vdc, variable_decl_t, vd.decls.front())
			ConvertReveal(vdc->info);
	}
}

void ConvertExport(variable_def_t &vd)
{
	if (vd.isExported)
	{
		ForAny(vdc, variable_decl_t, vd.decls.front())
			ConvertExport(vdc->info);
	}
}

void Convert2(variable_def_t &vd) // called when making constructor
{
	ForAny(vdc, variable_decl_t, vd.decls.front())
	{
		ForAny(vn, variable_name_t, vdc->info.names.front())
		{
			if (vdc->info.value)
			{
				Indent();
				fprintf(cfp, "%s = ", Name(vn->info.name()));
				Convert(GetExprP(vdc->info.value), cfp);
				fprintf(cfp, ";\n");
			}
		}
	}
}

//---------------------------------------------------------------------
// Transition body conversion

void Convert(assignment_t &a)
{
	Indent();
	data_def_t *dd = GetType(&a.lval);
	if (dd->tag == ARRAY_TYP)
	{
		fprintf(cfp, "memset(&");
		Convert(&a.lval, cfp);
		// we assume that rval is a var ref!
		assert(GetExprP(a.rval)->op == T_UNDEF &&
			GetUnOpP(GetExprP(a.rval)->l_op)->type == PE_VARREF);
		fprintf(cfp, ", &");
		Convert(GetExprP(a.rval), cfp);
		fprintf(cfp, ", sizeof(");
		Convert(&a.lval, cfp);
		fprintf(cfp, ")");
	}
	else
	{
		Convert(&a.lval, cfp);
		fprintf(cfp, " = ");
		Convert(GetExprP(a.rval), cfp);
	}
	fprintf(cfp, ";\n");
}

void Convert(output_t &o)
{
	static int scnt = 0;
	Indent(); fprintf(cfp, "{\n"); indent++;
	ForAny(oa, output_arg_t, o.signals.front())
	{
		scnt++;
		char *sid = Ident(oa->info.signal);
		Indent(); fprintf(cfp, "%s_t *__sig_%d = new %s_t;\n", sid, scnt, sid);
		int acnt = 0; // arg number
		ForAny(sa, heap_ptr_t, oa->info.args.front())
		{
			expression_t *e = GetExprP(sa->info);
			acnt++;
			if (e)
			{
				data_def_t *dd = GetType(e);
				Indent();
				if (dd && dd->tag == ARRAY_TYP)
				{
					fprintf(cfp, "memcpy(&__sig_%d->arg_%d, &",
						scnt, acnt);
					Convert(e, cfp);
					fprintf(cfp, ", sizeof(__sig_%d->arg_%d))",
						scnt, acnt);
				}
				else
				{
					fprintf(cfp, "__sig_%d->arg_%d = ", scnt, acnt);
					Convert(e, cfp);
				}
				fprintf(cfp, ";\n");
			}
		}
		Indent(); fprintf(cfp, "int pid_%d = ", scnt);
		if (o.hasDest)
		{
			fprintf(cfp, " = ");
			Convert(GetExprP(o.dest), cfp);
			fprintf(cfp, ";\n");
			Indent();
			fprintf(cfp, "if (pid==0) pid = -1; // 0 => no pid given\n");
		}
		else fprintf(cfp, "0;\n");
		signal_def_t *sd = (signal_def_t *)ScopeStack->dequalify(&oa->info.signal, Q_SIGNAL);
		int cnt = GetOutputDest(sd, o.via);
		assert(cnt>0);
		for (int i = 0; i < cnt; i++)
		{
			Indent(); fprintf(cfp, "%sif (FindDest(pid_%d, (int)%s))\n",
				i?"else ":"", scnt,
				GetProcessID(destinations[i][0]));
			indent++;
			Indent();
			fprintf(cfp, "Output((signal_t *)__sig_%d, Self(), pid_%d, %d);\n",
				scnt, scnt, Router->ChanIdx(destinations[i][1])/2);
			indent--;
		}
		Indent(); fprintf(cfp, "else assert(0); // no dest found\n");
	}
	indent--; Indent(); fprintf(cfp, "}\n");
}

int res_cnt = 0;

void Convert(invoke_node_t &c, int isCall)
{
	int parcnt = 1;
	Indent();
	if (isCall)
	{
		fprintf(cfp, "{\n"); indent++;
		char *pt = Ident(c.ident, Q_PROCEDURE);
		Indent(); fprintf(cfp, "%s *__p = new %s(", pt, pt);
		if (inProcedure)
			fprintf(cfp, "owner, this");
		else 
			fprintf(cfp, "this, NULL");
		fprintf(cfp, ");\n");
		procedure_def_t *pd = (procedure_def_t *)ScopeStack->dequalify(&c.ident, Q_PROCEDURE);
		lh_list_node_t<procedure_formal_param_t> *fpd = pd->params.front();
		lh_list_node_t<heap_ptr_t> *ap = c.args.front(); // Actual parameters
		while (fpd)
		{
			lh_list_node_t<procedure_param_name_t> *fpn = fpd->info.names.front();
			while (fpn)
			{
				expression_t *e = GetExprP(ap->info);
				if (e)
				{
					Indent();
					fprintf(cfp, "__p->SetParam%d(", parcnt);
					if (!fpd->info.isValue)
						fprintf(cfp, "&");
					Convert(e, cfp);
					fprintf(cfp, ");\n");
				}
				parcnt++;
				ap = ap->next();
				fpn = fpn->next();
			}
			fpd = fpd->next();
		}
		Indent(); fprintf(cfp, "return Call(__p, %d);\n", res_cnt);
		indent--; Indent(); fprintf(cfp, "}\n");
		indent--; Indent(); indent++;
		fprintf(cfp, "__resume_%d:\n", res_cnt++);
	}
	else
	{
		char *pt = Ident(c.ident, Q_PROCESS);
		fprintf(cfp, "{\n"); indent++;
		Indent(); fprintf(cfp, "%s *__p = new %s(Self());\n", pt, pt);
		lh_list_node_t<heap_ptr_t> *ap = c.args.front(); // Actual parameters
		while (ap)
		{
			expression_t *e = GetExprP(ap->info);
			if (e)
			{
				Indent();
				fprintf(cfp, "__p->SetParam%d(", parcnt);
				Convert(e, cfp);
					fprintf(cfp, ");\n");
			}
			parcnt++;
			ap = ap->next();
		}
		Indent(); fprintf(cfp, "Offspring(AddProcess(__p) ? __p->Self() : 0);\n");
		indent--; Indent(); fprintf(cfp, "}\n");
	}
}

void Convert(timer_set_t &ts)
{
	static int tcnt = 0;
	tcnt++;
	Indent(); fprintf(cfp, "{\n"); indent++;
	Indent(); fprintf(cfp, "signal_t *__timer_%d = (signal_t *)new %s_t;\n",
			tcnt, Ident(ts.timer, Q_TIMER));
	int acnt = 0; // arg number
	ForAny(ta, heap_ptr_t, ts.args.front())
	{
		expression_t *e = GetExprP(ta->info);
		acnt++;
		if (e)
		{
			Indent(); fprintf(cfp, "__timer_%d->arg_%d = ", tcnt, acnt);
			Convert(e, cfp);
			fprintf(cfp, ";\n");
		}
	}
	Indent(); fprintf(cfp, "ResetTimer(%d);\n", testnum);
	ConvertTimerTest(testnum++, ts.timer, ts.args);
	Indent(); fprintf(cfp, "SetTimer(__timer_%d, ", tcnt);
	Convert(GetExprP(ts.time), cfp);
	fprintf(cfp, ");\n");
	indent--; Indent(); fprintf(cfp, "}\n");
}

void Convert(timer_reset_t &tr)
{
	Indent(); fprintf(cfp, "ResetTimer(%d);\n", testnum);
	ConvertTimerTest(testnum++, tr.timer, tr.args);
}

void Convert(read_node_t &rn)
{
	Indent();
	fprintf(cfp, "scanf(\"%%d\", &");
	Convert(&rn.varref, cfp);
	fprintf(cfp, ");\n");
}

void Convert(write_node_t &wn)
{
	ForAny(e, heap_ptr_t, wn.exprs.front())
	{
		Indent();
		data_def_t *dd = GetType(GetExprP(e->info));
		if (dd == realType || dd == timeType || dd == durType)
			fprintf(cfp, "printf(\"%%f\\n\", (float)(");
		else
			fprintf(cfp, "printf(\"%%ld\\n\", (long)(");
		Convert(GetExprP(e->info), cfp);
		fprintf(cfp, "));\n");
	}
}

void Convert(transition_t &t);

void Convert(decision_node_t &wn)
{
	char *nxt = "";
	ForAny(a, subdecision_node_t, wn.answers.front())
	{
		if (a->info.answer.front())
		{
			Indent(); fprintf(cfp, "%sif (", nxt);
			nxt = "else ";
			indent++;
			char *sep = "";
			ForAny(rc, range_condition_t, a->info.answer.front())
			{
				if (sep[0])
				{
					fprintf(cfp, "\n");
					Indent();
					fprintf(cfp, sep);
				}
				if  (rc->info.op == RO_IN)
				{
					fprintf(cfp, "( (");
					Convert(GetExprP(wn.question), cfp);
					fprintf(cfp, ") >= ");
					Convert(GetExprP(rc->info.lower), cfp);
					fprintf(cfp, ") && ( (");
					Convert(GetExprP(wn.question), cfp);
					fprintf(cfp, ") <= ");
					Convert(GetExprP(rc->info.upper), cfp);
					fprintf(cfp, ")");
				}
				else
				{
					// we do a special case for bool lits
					expression_t *e = GetExprP(rc->info.upper);
					data_def_t *dd = GetType(e);
					if (dd == boolType && e->op==T_UNDEF)
					{
						unop_t *u = GetUnOpP(e->l_op);
						assert(u->type == PE_BLITERAL);
						if ((u->prim == 0) ^
							(rc->info.op==RO_NEQ))
								fprintf(cfp, "!");
						fprintf(cfp, "(");
						Convert(GetExprP(wn.question), cfp);
						fprintf(cfp, ")");
					}
					else
					{
						fprintf(cfp, "( (");
						Convert(GetExprP(wn.question), cfp);
						fprintf(cfp, ")");
						switch (rc->info.op)
						{
						case RO_NONE:
						case RO_EQU:	fprintf(cfp, "=="); 	break;
						case RO_NEQ:	fprintf(cfp, "!=");	break;
						case RO_LE:	fprintf(cfp, "<");	break;
						case RO_LEQ:	fprintf(cfp, "<=");	break;
						case RO_GT:	fprintf(cfp, ">");	break;
						case RO_GTQ:	fprintf(cfp, ">=");	break;
						default:			break;
						}
						Convert(GetExprP(rc->info.upper), cfp);
						fprintf(cfp, ")");
					}
				}
				sep = "|| ";
			}
			indent--;
			fprintf(cfp, ")\n");
		}
		else
		{
			Indent(); fprintf(cfp, "else\n");
		}
		Indent(); fprintf(cfp, "{\n");
		indent++;
		Convert(a->info.transition);
		indent--;
		Indent(); fprintf(cfp, "}\n");
	}
}

void Convert(gnode_t &n)
{
	if (n.label.name()[0] != '\0')
	{
		indent--;
		Indent(); fprintf(cfp, "%s: ;\n",  n.label.name());
		indent++;
	}
	void *node = GetVoidP(n.node);
	switch(n.type)
	{
		case N_TASK:
		{
			ApplyToAll(Convert, a, assignment_t,
				( (lh_list_t<assignment_t> *)node )->front());
			break;
		}
		case N_OUTPUT:
			Convert((*((output_t *)node)));
			break;
		case N_PRI_OUTPUT:
			break;
		case N_CREATE:
			Convert((*((invoke_node_t *)node)), 0);
			break;
		case N_CALL:
			Convert((*((invoke_node_t *)node)), 1);
			break;
		case N_SET:
		{
			ApplyToAll(Convert, ts, timer_set_t,
				( (lh_list_t<timer_set_t> *)node )->front());
			break;
		}
		case N_RESET:
		{
			ApplyToAll(Convert, tr, timer_reset_t,
				( (lh_list_t<timer_reset_t> *)node )->front());
			break;
		}
		case N_EXPORT:
		{
			ForAll(id, ident_t, ( (lh_list_t<ident_t> *)node )->front())
			{
				int off, lvl;
				data_def_t *dd;
				typeclass_t t;
				(void)ScopeStack->GetInfo(id->info, dd, lvl, off, t);
				char *n = Ident(id->info);
				Indent();
				if (dd && dd->tag == ARRAY_TYP)
					fprintf(cfp, "memcpy(__%s, %s, sizeof(%s));\n",
						n, n, n);
				else fprintf(cfp, "__%s = %s;\n", n, n);
			}
			break;
		}
		case N_DECISION:
			Convert((*((decision_node_t *)node)));
			break;
		case N_READ:
			Convert((*((read_node_t *)node)));
			break;
		case N_WRITE:
			Convert((*((write_node_t *)node)));
			break;
	}
}

void Convert(transition_t &t)
{
	ApplyToAll(Convert, nd, gnode_t, t.nodes.front());
	if (t.label.name()[0] != '\0')
	{
		indent--;
		Indent(); fprintf(cfp, "%s: ;\n",  t.label.name());
		indent++;
	}
	switch (t.type)
	{
		case NEXTSTATE:
			if (t.next.name()[0] != '\0')
			{
				if (!inProcedure)
				{
					Indent();
					fprintf(cfp, "LogState(Self(), StateName(state), StateName(%s_ID));\n",
						t.next.name());
				}
				Indent();
				fprintf(cfp, "state = %s_ID;\n", t.next.name());
			}
			Indent(); fprintf(cfp, "return DONETRANS;\n");
			break;
		case STOP:
			Indent(); fprintf(cfp, "return DONESTOP;\n");
			break;
		case RETURN:
			Indent(); fprintf(cfp, "return DONERETURN;\n");
			break;
		case JOIN:
			Indent(); fprintf(cfp, "goto %s;\n", t.next.name());
			break;
	}
}

//---------------------------------------------------------------
// State conversion

void ConvertInputs(state_node_t &sn)
{
	ForAny(in, input_part_t, sn.inputs.front())
	{
		if (!in->info.isAsterisk)
		{
			ForAny(st, stimulus_t, in->info.stimuli.front())
			{
				if (!st->info.variables.front())
				{
					++inputnum;
					continue;
				}
				Indent();
				fprintf(cfp, "case %d:\n", inputnum++);
				indent++;
				// get the args
				int anum = 1;
				ForAll(id, ident_t, st->info.variables.front())
				{
					data_def_t *dd;
					int lvl, off;
					typeclass_t t;
					(void)ScopeStack->GetInfo(id->info, dd, lvl, off, t);
					Indent();
					if (dd && dd->tag == ARRAY_TYP)
					{
						fprintf(cfp, "memcpy(&%s, &((",
							Ident(id->info));
						fprintf(cfp, "%s_t *)s)->arg_%d, sizeof(",
							Ident(st->info.signal, Q_SIGNAL), anum);
						fprintf(cfp, "%s));\n",
							Ident(id->info));
					}
					else
					{
						fprintf(cfp, "%s = ", Ident(id->info));
						fprintf(cfp, "((%s_t *)s)->arg_%d;\n",
							Ident(st->info.signal, Q_SIGNAL),
							anum);
					}
					anum++;
				}
//				Indent();
//				fprintf(cfp, "delete (%s_t *)s;\n",
//					Ident(st->info.signal, Q_SIGNAL));
				Indent(); fprintf(cfp, "break;\n");
				indent--;
			}
		}
	}
}

//-------------------------------------------------------------------
// Compute size of case statements in generated methods
// by counting the number of inputs, provided csigs, timer resets,
// saves, and timer tests.

void GetCounts(expression_t *e);

void ZeroCounts()
{
	inputcnt = provcnt = savecnt = testcnt = 0;
	statecnt = 0; // is this needed?? as it is computed elsewhere...
}

void GetCounts(cond_expr_t *ce)
{
	GetCounts(GetExprP(ce->if_part));
	GetCounts(GetExprP(ce->then_part));
	expression_t *ep = GetExprP(ce->else_part);
	if (ep) GetCounts(ep);
}

void GetCounts(timer_active_expr_t *u)
{
	testcnt++; (void)u;
	// must still do args!! currently nested active expressions
	// will cause the code generator to fail.
}

void GetCounts(var_ref_t *v)
{
	ForAny(sel, selector_t, v->sel.front())
	{
		if (sel->info.type == SEL_ELT)
			GetCounts(GetExprP(sel->info.val));
	}
}

void GetCounts(unop_t *u)
{
	void *p = GetVoidP(u->prim);
	switch (u->type)
	{
	case PE_FIX:
	case PE_EXPR:
	case PE_FLOAT:
		expression_t *e = (expression_t *)p;
		GetCounts(e);
		break;
	case PE_CONDEXPR:
		cond_expr_t *ce = (cond_expr_t *)p;
		GetCounts(ce);
		break;
	case PE_TIMERACTIV:
		GetCounts((timer_active_expr_t *)p);
		break;
	case PE_VARREF:
		GetCounts((var_ref_t *)p);
		break;
	}
}

void GetCounts(expression_t *e)
{
	if (e->op == T_UNDEF) // primary?
		GetCounts(GetUnOpP(e->l_op));
	else
	{
		GetCounts(GetExprP(e->l_op));
		GetCounts(GetExprP(e->r_op));
	}
}

void GetCounts(assignment_t &a)
{
	GetCounts(&a.lval);
	GetCounts(GetExprP(a.rval));
}

void GetCounts(output_t &o)
{
	ForAny(oa, output_arg_t, o.signals.front())
	{
		ForAny(sa, heap_ptr_t, oa->info.args.front())
		{
			expression_t *e = GetExprP(sa->info);
			if (e) GetCounts(e);
		}
	}
}

void GetCounts(invoke_node_t &c)
{
	lh_list_node_t<heap_ptr_t> *ap = c.args.front(); // Actual parameters
	while (ap)
	{
	 	expression_t *e = GetExprP(ap->info);
	 	if (e) GetCounts(e);
		ap = ap->next();
	}
}

void GetCounts(timer_set_t &ts)
{
	testcnt++; // implicit reset
	ForAny(ta, heap_ptr_t, ts.args.front())
	{
		expression_t *e = GetExprP(ta->info);
		if (e) GetCounts(e);
	}
}

void GetCounts(timer_reset_t &tr)
{
	ForAny(ta, heap_ptr_t, tr.args.front())
	{
		expression_t *e = GetExprP(ta->info);
		if (e) GetCounts(e);
	}
	testcnt++;
}

void GetCounts(write_node_t &wn)
{
	ForAny(e, heap_ptr_t, wn.exprs.front())
	{
		GetCounts(GetExprP(e->info));
	}
}

void GetCounts(transition_t &t);

void GetCounts(decision_node_t &wn)
{
	ForAny(a, subdecision_node_t, wn.answers.front())
	{
		if (a->info.answer.front())
		{
			ForAny(rc, range_condition_t, a->info.answer.front())
			{
				GetCounts(GetExprP(wn.question));
				if  (rc->info.op == RO_IN)
					GetCounts(GetExprP(rc->info.lower));
				GetCounts(GetExprP(rc->info.upper));
			}
		}
		GetCounts(a->info.transition);
	}
}

void GetCounts(gnode_t &n)
{
	void *node = GetVoidP(n.node);
	switch(n.type)
	{
		case N_TASK:
		{
			ApplyToAll(GetCounts, a, assignment_t,
				( (lh_list_t<assignment_t> *)node )->front());
			break;
		}
		case N_OUTPUT:
			GetCounts((*((output_t *)node)));
			break;
		case N_PRI_OUTPUT:
			break;
		case N_CALL:
		case N_CREATE:
			GetCounts((*((invoke_node_t *)node)));
			break;
		case N_SET:
		{
			ApplyToAll(GetCounts, ts, timer_set_t,
				( (lh_list_t<timer_set_t> *)node )->front());
			break;
		}
		case N_RESET:
		{
			ApplyToAll(GetCounts, tr, timer_reset_t,
				( (lh_list_t<timer_reset_t> *)node )->front());
			break;
		}
		case N_DECISION:
			GetCounts((*((decision_node_t *)node)));
			break;
		case N_WRITE:
			GetCounts((*((write_node_t *)node)));
			break;
	}
}

void GetCounts(transition_t &t)
{
	ForAny(nd, gnode_t, t.nodes.front())
		GetCounts(nd->info);
}

void GetCounts(state_node_t &sn)
{
	ForAny(sv, save_part_t, sn.saves.front())
	{
		savecnt++;
	}
	ForAny(in, input_part_t, sn.inputs.front())
	{
		if (!in->info.isAsterisk)
		{
			ForAny(st, stimulus_t, in->info.stimuli.front())
			{
				++inputcnt;
			}
		}
		if (in->info.enabler)
			++provcnt;
		GetCounts(in->info.transition);
	}
	ForAny(cs, continuous_signal_t, sn.csigs.front())
	{
		if (!IsTrue(GetExprP(cs->info.condition)))
			++provcnt;
		GetCounts(cs->info.transition);
	}
}

void ConvertProvided(state_node_t &sn)
{
	ForAny(in, input_part_t, sn.inputs.front())
	{
		if (in->info.enabler)
		{
			if (IsTrue(GetExprP(in->info.enabler)))
				continue;
			Indent();
			fprintf(cfp, "case %d:\n", provnum++);
			indent++;
			Indent();
			fprintf(cfp, "return (");
			Convert(GetExprP(in->info.enabler), cfp);
			fprintf(cfp, ");\n");
			indent--;
		}
	}
	ForAny(cs, continuous_signal_t, sn.csigs.front())
	{
		if (IsTrue(GetExprP(cs->info.condition)))
			continue;
		Indent();
		fprintf(cfp, "case %d:\n", provnum++);
		indent++;
		Indent();
		fprintf(cfp, "return (");
		Convert(GetExprP(cs->info.condition), cfp);
		fprintf(cfp, ");\n");
		indent--;
	}
}

void ConvertActions(state_node_t &sn)
{
	ForAny(in, input_part_t, sn.inputs.front())
	{
		Indent();
		fprintf(cfp, "case %d:\n", actnum++);
		indent++;
		Convert(in->info.transition);
		indent--;
	}
	ForAny(cs, continuous_signal_t, sn.csigs.front())
	{
		Indent();
		fprintf(cfp, "case %d:\n", actnum++);
		indent++;
		Convert(cs->info.transition);
		indent--;
	}
}

void ConvertSaves(int &cnt, signallist_t &s)
{
	ForAny(sl, ident_t, s.signallists.front())
	{
		signallist_t *slp = (signallist_t *)ScopeStack->dequalify(&sl->info, Q_SIGLIST);
		ConvertSaves(cnt, *slp);
	}
	ForAny(sg, ident_t, s.signals.front())
	{
		if (cnt)
		{
			fprintf(cfp, "\n");
			Indent(); fprintf(cfp, "   && ");
		}
		fprintf(cfp, "(s->id != %s_ID)", Ident(sg->info, Q_SIGNAL));
		cnt++;
	}
}

void ConvertSaves(state_node_t &sn)
{
	ForAny(sv, save_part_t, sn.saves.front())
	{
		Indent(); fprintf(cfp, "case %d:\n", savenum);
		indent++;
		if (!sv->info.isAsterisk)
		{
			Indent(); fprintf(cfp, "if (");
			int cnt = 0;
			ConvertSaves(cnt, sv->info.savesigs);
			fprintf(cfp, ") return NULL;\n");
		}
		Indent(); fprintf(cfp, "break;\n");
		indent--;
		savenum++;
	}
}

void AddTableEntry(int pri, char *state, char *input, int prov,
	int deq, int act, int sav)
{
	assert(tcount<256);
	stateTable[tcount].pri	 = pri;
	stateTable[tcount].state = new char[strlen(state)+1];
	strcpy(stateTable[tcount].state, state);
	stateTable[tcount].input = new char[strlen(input)+1];
	strcpy(stateTable[tcount].input, input);
	stateTable[tcount].prov	 = prov;
	stateTable[tcount].deq	 = deq;
	stateTable[tcount].act	 = act;
	stateTable[tcount].sav	 = sav;
	tcount++;
}

void AddToStateTable(char *state, state_node_t &sn)
{
	int save = -1;
	if (sn.saves.front()) save = savenum++;
	ForAny(in, input_part_t, sn.inputs.front())
	{
		int pn;
		if (in->info.enabler)
			pn = provnum++;
		else pn = -1;
		if (!in->info.isAsterisk)
		{
			ForAny(st, stimulus_t, in->info.stimuli.front())
			{
				char buf[80];
				sprintf(buf, "(int)%s_ID", Ident(st->info.signal, Q_SIGNAL));
				AddTableEntry(-1, state, buf, pn, inputnum++, actnum, save);
			}
		}
		else AddTableEntry(-1, state, "-1", pn, -1, actnum, save);
		actnum++;
	}
	ForAny(cs, continuous_signal_t, sn.csigs.front())
		if (!IsTrue(GetExprP(cs->info.condition)))
			AddTableEntry(-1, state, "-1", provnum++, -1, actnum++, save);
		else
			AddTableEntry(-1, state, "-1", -1, -1, actnum++, save);
}

void AddToStateTable(state_node_t &sn)
{
	int st;
	if (sn.isAsterisk)
		AddToStateTable("-1", sn);
	else ForAny(snm, name_t, sn.states.front())
	{
		char buf[80];
		sprintf(buf, "(int)%s_ID", snm->info.name());
		AddToStateTable(buf, sn);
	}
}

void SortStateTable() // must improve this - currently bubblesort!!
{
	for (int i = 0; i < (tcount-1); i++)
	{
		for (int j = i+1; j < tcount; j++)
		{
			if (stateTable[i].pri>stateTable[j].pri)
			{
				state_tbl_entry_t tmp = stateTable[j];
				stateTable[j] = stateTable[i];
				stateTable[i] = tmp;
			}
		}
	}
}

//------------------------------------------------------------------
// Conversion routines common to processes/procedures

void Convert(lh_list_t<data_def_t> &l, char *nm)
{
	if (l.front())
	{
		Strip(hfp);
		fprintf(hfp, "// Data types for %s %s\n",
			inProcedure?"procedure":"process", nm);
		ForAll(t, data_def_t, l.front())
			Convert(t->info);
	}
}

void Convert(procedure_def_t &prc); // forward decl

void Convert(lh_list_t<procedure_def_t> &l)
{
	ForAny(p, procedure_def_t, l.front())
		Convert(p->info);
}

void ConvertFriend(procedure_def_t &prc)
{
	ScopeStack->EnterScope(Q_PROCEDURE, &prc);
	fprintf(hfp, "\tfriend class %s;\n", Name());
	lh_list_t<procedure_def_t> *pl =
		(lh_list_t<procedure_def_t> *)GetVoidP(prc.procedures);
	if (pl)
		ForAny(p, procedure_def_t, pl->front())
			ConvertFriend(p->info);
	ScopeStack->ExitScope();
}

void Convert2Class(char *nm, lh_list_t<state_node_t> &sl,
	lh_list_t<variable_def_t> &vl, transition_t &start,
	lh_list_t<procedure_def_t> *pl,
	lh_list_t<process_formal_param_t> *prcpar,
	lh_list_t<procedure_formal_param_t> *prdpar)
{
	char *name = new char [strlen(nm)+1];
	char *typ = inProcedure ? "procedure" : "process";
	strcpy(name, nm);
	Strip(hfp); fprintf(hfp, "// Class for %s %s\n\n", typ, nm);
	Strip(cfp); fprintf(cfp, "// Methods for %s class %s\n\n", typ, nm);
	fprintf(hfp, "class %s : public SDL_%s_t\n{\n", name, typ);
	indent++;
	// get counts and make state enumeration 
	ZeroCounts();
	if (sl.front())
	{
		char *sep = "";
		ForAll(si, state_node_t, sl.front())
		{
			GetCounts(si->info);
			if (!si->info.isAsterisk)
			{
				ForAny(snm, name_t, si->info.states.front())
				{
					if (statecnt==0)
						fprintf(hfp, "\tenum state_t\n\t{\n\t\t");
					fprintf(hfp, "%s%s_ID", sep, snm->info.name());
					sep = ",\n\t\t";
					statecnt++;
				}
			}
		}
		if (statecnt)
			fprintf(hfp, "\n\t};\n");
	}
	// Non-revealed or exported variables become private data members
	hasViewed = 0;
	hasExported = 0;
	if (vl.front())
		ApplyToAll(Convert, v, variable_def_t, vl.front());
	// Parameters also become variables. Reference parameters
	// are pointers

	if (prcpar)
	{
		ForAny(par, process_formal_param_t, prcpar->front())
		{
			int ch = ' ';
			fprintf(hfp, "\t%s_typ\t", Ident(par->info.sort, Q_TYPE));
			ForAll(pnm, process_param_name_t, par->info.names.front())
			{
				fprintf(hfp, "%c %s", ch,
					Name(pnm->info.name()));
				ch = ',';
			}
			fprintf(hfp, ";\n");
		}
	}
	if (prdpar)
	{
		ForAny(par, procedure_formal_param_t, prdpar->front())
		{
			int ch = ' ';
			fprintf(hfp, "\t%s_typ\t", Ident(par->info.sort, Q_TYPE));
			ForAll(pnm, procedure_param_name_t, par->info.names.front())
			{
				fprintf(hfp, "%c %s%s", ch,
					(par->info.isValue)?"":"*",
					Name(pnm->info.name()));
				ch = ',';
			}
			fprintf(hfp, ";\n");
		}
	}

	fprintf(hfp, "public:\n");

	if (inProcedure)
		fprintf(hfp, "\t%s(SDL_process_t *owner, SDL_procedure_t *caller = NULL);\n",
			name);
	else
		fprintf(hfp, "\t%s(int ppid_in = 0);\n", name);
	fprintf(hfp, "\t~%s();\n", name);
	if (hasViewed)
	{
		fprintf(hfp, "\tvoid *Reveal(int idx);\n");
		fprintf(cfp, "void *%s::Reveal(int idx)\n{\n", name);
		ApplyToAll(ConvertReveal, v, variable_def_t, vl.front());
		fprintf(cfp, "}\n\n");
	}
	if (hasExported)
	{
		fprintf(hfp, "\tvoid *Export(int idx);\n");
		fprintf(cfp, "void *%s::Export(int idx)\n{\n", name);
		ApplyToAll(ConvertExport, v, variable_def_t, vl.front());
		fprintf(cfp, "}\n\n");
	}
	if (savecnt)
	{
		fprintf(hfp, "\tsignal_t\t*GetHead(int saveidx, int wanted);\n");
		fprintf(cfp, "signal_t *%s::GetHead(int saveidx, int wanted)\n", name);
		fprintf(cfp, "{\n\tsignal_t *s = %sport;\n",
			inProcedure?"owner->":"");
		fprintf(cfp, "\twhile(s)\n\t{\n");
		fprintf(cfp, "\t\tif (s->id == wanted) return s;\n");
		fprintf(cfp, "\t\tswitch(saveidx)\n\t\t{\n");
		indent = 3;
		ForAll(si, state_node_t, sl.front())
			ConvertSaves(si->info);
		fprintf(cfp, "\t\t}\n\t\ts = s->Next();\n\t}\n");
		fprintf(cfp, "\treturn NULL;\n}\n\n");
	}
	if (statecnt)
		fprintf(hfp, "\tchar*\t\tStateName(int s);\n");
	if (provcnt)
		fprintf(hfp, "\tint\t\tProvided(int p);\n");
	if (inputcnt)
		fprintf(hfp, "\tvoid\t\tInput(int d, int sigid);\n");
	if (testcnt)
		fprintf(hfp, "\tint\t\tCompareTimer(int idx, signal_t *s);\n");
	fprintf(hfp, "\texec_result_t\tAction(int a);\n");
	if (inProcedure)
	{
		fprintf(hfp, "\t%s *Owner();\n", Owner);
		if (Caller[0])
			fprintf(hfp, "\t%s *Caller();\n", Caller);
	}

	// Methods to initialise parameters

	int parcnt = 1;
	if (prcpar)
	{
		ForAny(par, process_formal_param_t, prcpar->front())
		{
			ForAll(pnm, process_param_name_t, par->info.names.front())
			{
				fprintf(hfp, "\tvoid SetParam%d(%s_typ ",
					parcnt++, Ident(par->info.sort, Q_TYPE));
				char *n = Name(pnm->info.name());
				fprintf(hfp, "%s_in)\n", n);
				fprintf(hfp, "\t{\n\t\t%s = %s_in;\n\t}\n", n, n);
			}
		}
	}
	if (prdpar)
	{
		ForAny(par, procedure_formal_param_t, prdpar->front())
		{
			data_def_t *dd = (data_def_t *)ScopeStack->dequalify(&par->info.sort, Q_TYPE);
			ForAll(pnm, procedure_param_name_t, par->info.names.front())
			{
				fprintf(hfp, "\tvoid SetParam%d(%s_typ ",
					parcnt++, Ident(par->info.sort, Q_TYPE));
				char *n = Name(pnm->info.name());
				if (!par->info.isValue)
					fprintf(hfp, "*");
				fprintf(hfp, "%s_in)\n\t{\n\t\t", n);
				if (par->info.isValue && dd->tag == ARRAY_TYP)
					fprintf(hfp, "memcpy(%s, %s_in, sizeof(%s));\n",
						n, n, n);
				else
					fprintf(hfp, "%s = %s_in;\n", n, n);
				fprintf(hfp, "\t}\n");
			}
		}
	}

	// declare all subprocedures as friends
	if (pl)
		ForAny(pp, procedure_def_t, pl->front())
			ConvertFriend(pp->info);

	fprintf(hfp, "};\n\n");
	indent--;

	inputnum = provnum = 0;
	actnum = 1;

	// State name lookup member function

	if (statecnt)
	{
		fprintf(cfp, "char *%s::StateName(int s)\n{\n", name);
		fprintf(cfp, "\tswitch(s)\n\t{\n");
		ForAny(si, state_node_t, sl.front())
		{
			if (!si->info.isAsterisk)
			{
				ForAny(snm, name_t, si->info.states.front())
				{
					fprintf(cfp, "\t\tcase %s_ID:\n\t\t\treturn \"%s\";\n",
						snm->info.name(),
						snm->info.name());
				}
			}
		}
		fprintf(cfp, "\t\tdefault:\n\t\t\treturn \"Unknown\";\n\t}\n}\n\n");
	}

	// Owner and Caller member functions

	if (inProcedure)
	{
		fprintf(cfp, "%s *%s::Owner()\n{\n\treturn (%s *)owner;\n}\n\n",
			Owner, name, Owner);
		if (Caller[0])
			fprintf(cfp, "%s *%s::Caller()\n{\n\treturn (%s *)caller;\n}\n\n",
				Caller, name, Caller);
	}

	// Signal input member function

	indent = 2;
	if (inputcnt)
	{
		fprintf(cfp, "void %s::Input(int d, int sigid)\n{\n", name);
		fprintf(cfp, "\tsignal_t *s = %sDequeue(sigid);\n",
			inProcedure?"Owner()->":"");
		if (sl.front())
		{
			fprintf(cfp, "\tswitch(d)\n");
			fprintf(cfp, "\t{\n");
			ForAll(st1, state_node_t, sl.front())
				ConvertInputs(st1->info);
			Indent(); fprintf(cfp, "default:\n");
			indent++; Indent(); fprintf(cfp, "break;\n");
			indent--;
			fprintf(cfp, "\t}\n");
		}
		fprintf(cfp, "\tLogInput(Self(), s);\n\tdelete s;\n");
		fprintf(cfp, "}\n\n");
	}

	// `Provided' checking member function

	if (provcnt)
	{
		fprintf(cfp, "int %s::Provided(int p)\n{\n", name);
		if (sl.front())
		{
			fprintf(cfp, "\tswitch(p)\n");
			fprintf(cfp, "\t{\n");
			ForAll(s, state_node_t, sl.front())
				ConvertProvided(s->info);
			Indent(); fprintf(cfp, "default:\n");
			indent++; Indent(); fprintf(cfp, "break;\n");
			indent--;
			fprintf(cfp, "\t}\n");
		}
		fprintf(cfp, "\tassert(0);\n");
		fprintf(cfp, "\treturn 0;\n");
		fprintf(cfp, "}\n\n");
	}

	if (testcnt)
	{
		fprintf(testfp, "\nint %s::CompareTimer(int idx, signal_t *s)\n", name);
		fprintf(testfp, "{\n\tswitch(idx)\n\t{\n");
		indent = 2;
	}
	// Transition body handling member function
	fprintf(cfp, "exec_result_t %s::Action(int a)\n{\n", name);
	fprintf(cfp, "\texec_result_t rtn = DONETRANS;\n");
	fprintf(cfp, "\tswitch(a)\n");
	fprintf(cfp, "\t{\n");

	testnum = 0;
	res_cnt = 0;
	fprintf(cfp, "\t\tcase 0: // start transition\n");
	indent = 3;
	if (!inProcedure)
	{
		Indent(); fprintf(cfp, "LogCreate(Self());\n");
	}
	Convert(start);
	indent = 2;
	ForAny(st3, state_node_t, sl.front())
	{
		ConvertActions(st3->info);
	}
	if (res_cnt>0)
	{
		fprintf(cfp, "\t\tcase -1: // resume after procedure call\n");
		if (res_cnt==1)
			fprintf(cfp, "\t\t\tgoto __resume_0;\n");
		else
		{
			fprintf(cfp, "\t\t\tswitch (__resume_idx)\n");
			fprintf(cfp, "\t\t\t{\n");
			for (int i = 0; i < res_cnt; i++)
				fprintf(cfp, "\t\t\t\tcase %d: goto __resume_%d;\n", i, i);
			fprintf(cfp, "\t\t\t}\n");
		}
	}
	fprintf(cfp, "\t}\n");
	fprintf(cfp, "\tassert(0);\n");
	fprintf(cfp, "\treturn rtn;\n");
	fprintf(cfp, "}\n\n");
	if (testcnt)
		fprintf(testfp, "\t}\n\treturn 0;\n}\n");

	// build state table 

	inputnum = provnum = savenum = 0;
	actnum = 1;
	tcount = 0;
	ForAny(st4, state_node_t, sl.front())
		AddToStateTable(st4->info);
	SortStateTable();

	// Constructor

	indent = 1;
	if (inProcedure)
	{
		fprintf(cfp, "%s::%s(SDL_process_t *owner_in, SDL_procedure_t *caller_in)\n",
			name, name);
		fprintf(cfp, "\t: SDL_procedure_t(%d, owner_in, caller_in)\n{\n",
			tcount);
	}
	else
	{
		fprintf(cfp, "%s::%s(int ppid_in)\n", name, name);
		fprintf(cfp, "\t: SDL_process_t(%d, (int)%s_ID, ppid_in)\n{\n", tcount, name);
	}
	if (vl.front())
	{
		ApplyToAll(Convert2, v, variable_def_t, vl.front());
	}
	for (int t = 0; t < tcount; t++)
	{
		fprintf(cfp, "\tMakeEntry(%d, %s, %s, %d, %d, %d, %d);\n",
			stateTable[t].pri, stateTable[t].state,
			stateTable[t].input, stateTable[t].prov,
			stateTable[t].deq, stateTable[t].act,
			stateTable[t].sav);
		delete [] stateTable[t].state;
		delete [] stateTable[t].input;
	}
	fprintf(cfp, "}\n\n");

	// Destructor

	fprintf(cfp, "%s::~%s()\n{\n", name, name);
	fprintf(cfp, "\tprintf(\"Time %%-5f Process %s (%%d) terminated (state %%s, last event time %%f)\\n\",\n",
		name);
	fprintf(cfp, "\t\tNow(), Self(), StateName(state), lasteventtime);\n");
	fprintf(cfp, "}\n\n");
	indent = 0;
	delete [] name;
}

//------------------------------------------------------------------
// Procedure conversion

void Convert(procedure_def_t &prc)
{
	inProcedure++;
	ScopeStack->EnterScope(Q_PROCEDURE, &prc);
	char MyCaller[128];
	strcpy(MyCaller, Caller);
	strcpy(Caller, Name());
	Convert(prc.types, Name());
	lh_list_t<procedure_def_t> *pl =
		(lh_list_t<procedure_def_t> *)GetVoidP(prc.procedures);
	if (pl) Convert(*pl);
	strcpy(Caller, MyCaller);
	Convert2Class(Name(), prc.states, prc.variables, prc.start, pl,
		NULL, &prc.params);
	ScopeStack->ExitScope();
	inProcedure--;
}

void ConvertForward(procedure_def_t &prc)
{
	ScopeStack->EnterScope(Q_PROCEDURE, &prc);
	lh_list_t<procedure_def_t> *pl =
		(lh_list_t<procedure_def_t> *)GetVoidP(prc.procedures);
	if (pl)
		ForAny(p, procedure_def_t, pl->front())
			ConvertForward(p->info);
	fprintf(hfp, "class %s;\n", Name());
	ScopeStack->ExitScope();
}

//------------------------------------------------------------------
// Process conversion

void Convert(process_def_t &prc)
{
	ScopeStack->EnterScope(Q_PROCESS, &prc);

	// Data type definitions local to the process
	strcpy(Owner, Name());
	Caller[0] = 0;
	Convert(prc.types, Name());

	// Signal class definitions local to the process

	if (prc.signals.front())
	{
		Strip(hfp); fprintf(hfp, "// Signals for process %s\n\n", Name());
		ForAll(sig, signal_def_t, prc.signals.front())
			Convert(sig->info);
	}

	// As above but for timers

	if (prc.timers.front())
	{
		Strip(hfp); fprintf(hfp, "// Timers for process %s\n\n", Name());
		ForAll(tm, timer_def_t, prc.timers.front())
			Convert(tm->info);
	}
	Convert(prc.procedures);
	Convert2Class(Name(), prc.states, prc.variables, prc.start,
		&prc.procedures, &prc.params, NULL);
	ScopeStack->ExitScope();
}

void ConvertForward(process_def_t &prc)
{
	ScopeStack->EnterScope(Q_PROCESS, &prc);
	ForAny(p, procedure_def_t, prc.procedures.front())
		ConvertForward(p->info);
	fprintf(hfp, "class %s;\n", Name());
	ScopeStack->ExitScope();
}

//------------------------------------------------------------------
// Block conversion

void Convert(block_def_t &blk)
{
	ScopeStack->EnterScope(Q_BLOCK, &blk);

	// Data type definitions local to the block

	if (blk.types.front())
	{
		Strip(hfp); fprintf(hfp, "// Data types for block %s\n\n", Name());
		ForAll(t, data_def_t, blk.types.front())
			Convert(t->info);
	}

	// Signal class definitions local to the block

	if (blk.signals.front())
	{
		Strip(hfp); fprintf(hfp, "// Signals for block %s\n\n", Name());
		ForAll(sig, signal_def_t, blk.signals.front())
			Convert(sig->info);
	}

	// Convert contained processes

//	Strip(cfp); fprintf(cfp, "// Processes for block %s\n\n", Name());
//	Strip(hfp); fprintf(hfp, "// Processes for block %s\n\n", Name());
	ForAny(p, process_def_t, blk.processes.front())
		Convert(p->info);

	ScopeStack->ExitScope();
}

void ConvertForward(block_def_t &blk)
{
	ScopeStack->EnterScope(Q_BLOCK, &blk);
	ForAny(p, process_def_t, blk.processes.front())
		ConvertForward(p->info);
	ScopeStack->ExitScope();
}

//--------------------------------------------------------------------
// System conversion

void PrintTitle(FILE *fp)
{
	time_t tm = time(NULL);
	fprintf(fp, "//---------------------------------------------------------------\n");
	fprintf(fp, "// Generated by SDL*Designer on %s",
		asctime(localtime(&tm)));
	fprintf(fp, "// from AST form of source files:\n");
	int lim = fileTable->getIndex();
	for (int f = 1; f <= lim; f++)
		fprintf(fp, "//\t%s\n", fileTable->getName(f));
	fprintf(fp, "// SDL*Designer written by Graham Wheeler\n");
	fprintf(fp, "// (c) 1994 by Graham Wheeler and the DNA Laboratory\n");
	fprintf(fp, "//---------------------------------------------------------------\n");
}

void Convert(system_def_t *sys)
{
	ScopeStack->EnterScope(Q_SYSTEM, sys);
	PrintTitle(cfp);
	PrintTitle(hfp);
	fprintf(cfp, "\n#include \"sdlrte.h\"\n");
	fprintf(cfp, "#include \"sdlgen.h\"\n");

	// Block type ID enumeration - probably not used!!

	char *sep = "";
	{
		Strip(hfp);
		fprintf(hfp, "// Block type IDs\n\n");
		fprintf(hfp, "typedef enum\n{");
		ForAny(b, block_def_t, sys->blocks.front())
		{
			ScopeStack->EnterScope(Q_BLOCK, &b->info);
			fprintf(hfp, "%s\n\t%s_ID", sep, Name());
			ScopeStack->ExitScope();
			sep = ",";
		}
		fprintf(hfp, "\n} block_type_t;\n");
	}

	// Process type ID enumeration (.h) and name table (.cpp)

	{
		Strip(hfp); fprintf(hfp, "// Process type IDs\n\n");
		fprintf(hfp, "typedef enum\n{");
		Strip(cfp); fprintf(cfp, "// Process name table\n\n");
		fprintf(cfp, "char *process_names[] =\n{");
		sep = "";
		ForAny(b, block_def_t, sys->blocks.front())
		{
			ScopeStack->EnterScope(Q_BLOCK, &b->info);
			ForAny(p, process_def_t, b->info.processes.front())
			{
				ScopeStack->EnterScope(Q_PROCESS, &p->info);
				char *n = Name();
				fprintf(cfp, "%s\n\t\"%s\"", sep, n);
				fprintf(hfp, "%s\n\t%s_ID", sep, n);
				sep = ",";
				ScopeStack->ExitScope();
			}
			ScopeStack->ExitScope();
		}
		fprintf(cfp, "\n};\n");
		fprintf(hfp, "\n} process_type_t;\n");
	}

	// Signal type ID enumeration (.h) and name table (.cpp)

	Strip(hfp);
	fprintf(hfp, "// Signal type IDs and names\n\n");
	fprintf(hfp, "typedef enum\n{");
	Strip(cfp);
	fprintf(cfp, "// Signal name table\n\n");
	fprintf(cfp, "char *signal_names[] =\n{");
	sep = "";
	ForAny(ss, signal_def_t, sys->signals.front())
	{
		char *n = Name(ss->info.name());
		fprintf(cfp, "%s\n\t\"%s\"", sep, n);
		fprintf(hfp, "%s\n\t%s_ID", sep, n);
		sep = ",";
	}
	ForAny(bb, block_def_t, sys->blocks.front())
	{
		ScopeStack->EnterScope(Q_BLOCK, &bb->info);
		ForAny(s, signal_def_t, bb->info.signals.front())
		{
			char *n = Name(s->info.name());
			fprintf(cfp, "%s\n\t\"%s\"", sep, n);
			fprintf(hfp, "%s\n\t%s_ID", sep, n);
			sep = ",";
		}
		ForAny(p, process_def_t, bb->info.processes.front())
		{
			ScopeStack->EnterScope(Q_PROCESS, &p->info);
			ForAny(s, signal_def_t, p->info.signals.front())
			{
				char *n = Name(s->info.name());
				fprintf(cfp, "%s\n\t\"%s\"", sep, n);
				fprintf(hfp, "%s\n\t%s_ID", sep, n);
				sep = ",";
			}
			ForAny(t, timer_def_t, p->info.timers.front())
			{
				char *n = Name(t->info.name());
				fprintf(cfp, "%s\n\t\"%s\"", sep, n);
				fprintf(hfp, "%s\n\t%s_ID", sep, n);
				sep = ",";
			}
			ScopeStack->ExitScope();
		}
		ScopeStack->ExitScope();
	}
	if (sep[0] == 0)
	{
		fprintf(cfp, "\n\t\"Unknown\"");
		fprintf(hfp, "\n\tUnknown_ID");
	}
	fprintf(cfp, "\n};\n");
	fprintf(hfp, "\n} signal_type_t;\n");

	// System-wide data type and signal definitions

	if (sys->types.front())
	{
		Strip(hfp);
		fprintf(hfp, "// Top-level data types\n\n");
		ForAll(t, data_def_t, sys->types.front())
			Convert(t->info);
	}
	if (sys->signals.front())
	{
		Strip(hfp); fprintf(hfp, "// Top-level signals\n\n");
		ForAll(sig, signal_def_t, sys->signals.front())
			Convert(sig->info);
	}

	// Forward decclarations of process/procedure classes

	{
		Strip(hfp);
		ForAny(b, block_def_t, sys->blocks.front())
			ConvertForward(b->info);
	}

	// Convert the contained blocks

	{
		ForAny(b, block_def_t, sys->blocks.front())
			Convert(b->info);
	}

	// Make the CreateInitial() function to create the 
	// initial processes

	Strip(cfp);
	fprintf(cfp, "void CreateInitial()\n{\n");
	int first = 1, bcnt = 0;
	ForAny(b5, block_def_t, sys->blocks.front())
	{
		ScopeStack->EnterScope(Q_BLOCK, &b5->info);
		ForAny(p, process_def_t, b5->info.processes.front())
		{
			ScopeStack->EnterScope(Q_PROCESS, &p->info);
			fprintf(cfp, "\tMaxInstances[(int)%s_ID] = %d;\n",
				Name(), p->info.maxval);
			int lim = p->info.initval;
			char *spc;
			if (lim>1)
			{
				if (first)
				{
					fprintf(cfp, "\tint p;\n");
					first = 0;
				}
				spc = "\t\t";
				fprintf(cfp, "\tfor (p=0; p<%d; p++)\n", lim);
			}
			else spc = "\t";
			if (lim) fprintf(cfp, "%sAddProcess(new %s);\n", spc, Name());
			bcnt++;
			ScopeStack->ExitScope();
		}
		ScopeStack->ExitScope();
	}
	fprintf(cfp, "\tfor (int i = 0; i < %d; i++) Instances[i] = 0;\n",
		bcnt);
	fprintf(cfp, "}\n\n");

	// Make the instance count tables

	Strip(cfp);
	fprintf(cfp, "\nint Instances[%d];\n", bcnt);
	fprintf(cfp, "int MaxInstances[%d];\n", bcnt);

	// Channel delay/reliability lookup

	Strip(cfp);
	if (sys->channels.front())
	{
		fprintf(cfp, "// Channel table\n\nchannel_info_t channelTbl[] =\n{");
		sep = "";
		ForAll(c, channel_def_t, sys->channels.front())
		{
			fprintf(cfp, "%s\n\t{ 100, 1. }", sep);
			sep = ",";
		}
		fprintf(cfp, "\n};\n\n");
		fprintf(cfp, "// Modify this as necessary\n\n");
		fprintf(cfp, "void GetChannelInfo(int ch, signal_t *s, float &dlay, int &rel)\n");
		fprintf(cfp, "{\n\t(void)s;\n");
		fprintf(cfp, "\tdlay = channelTbl[ch].deelay;\n");
		fprintf(cfp, "\trel = channelTbl[ch].reliability;\n}\n");
	}
	else
	{
		fprintf(cfp, "// Empty channel table\n\nchannel_info_t channelTbl[1];\n\n");
		fprintf(cfp, "void GetChannelInfo(int ch, signal_t *s, float &dlay, int &rel)\n");
		fprintf(cfp, "{\n\t(void)ch; (void)s;\n");
		fprintf(cfp, "\tdlay = 1.;\n");
		fprintf(cfp, "\trel = 100;\n}\n");
	}
	ScopeStack->ExitScope();
}

static void useage()
{
	fprintf(stderr, "Useage: sdl2cpp [-H]\nwhere:");
	fprintf(stderr, "\t-H enables heap statistics\n");
	fprintf(stderr, "\t-s uses shorter mangled identifiers\n");
	exit(-1);
}

void Merge(char *target, char *src)
{
	FILE *ofp = fopen(target, "a");
	FILE *ifp = fopen(src, "r");
	assert(ofp && ifp);
	for (;;)
	{
		char buf[512];
		fgets(buf, 512, ifp);
		if (feof(ifp)) break;
		fputs(buf, ofp);
	}
	fclose(ofp);
	fclose(ifp);
}

int main(int argc, char *argv[])
{
	fprintf(stderr, "DNA SDL AST to C++ Convertor v1.0\n");
	fprintf(stderr, "written by Graham Wheeler\n");
	fprintf(stderr, "(c) 1994 Graham Wheeler and the DNA Laboratory\n");
	RestoreAST("ast.sym");
	for (int i=1;i<argc;i++)
	{
		if (argv[i][0]=='-')
		{
			switch(argv[i][1])
			{
			case 'H':
				if (isdigit(argv[i][2]))
					localHeap->debug=argv[i][2]-'0';
				else
					localHeap->debug++;
				break;
			case 's':
				longIDs = 0;
				break;
			default:
				useage();
			}
		}
		else useage();
	}
	cfp = fopen("sdlgen.cpp", "w");
	hfp = fopen("sdlgen.h", "w");
	testfp = fopen("sdl__tst.tmp", "w");
	if (cfp==NULL || hfp==NULL || testfp==NULL)
		fprintf(stderr, "Cannot open output file(s)!\n");
	else
		Convert(sys);
	Strip(cfp);
	if (cfp) fclose(cfp);
	if (hfp) fclose(hfp);
	if (testfp) fclose(testfp);
	Merge("sdlgen.cpp", "sdl__tst.tmp");
	unlink("sdl__tst.tmp");
	DeleteAST();
	return 0;
}

