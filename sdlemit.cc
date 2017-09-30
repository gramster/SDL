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
#pragma hdrfile "CODE.SYM"
#include "sdlcode.h"

int nextLabel = 1;

void scope_stack_t::Emit()
{
	for (int i = 0; i < level; i++)
		Code->Emit(OP_SETSCOPE, type[i], offset[i]);
}

void EmitDest(s_code_word_t ch, s_code_word_t pr)
{
	Code->Emit(OP_CONSTANT, ch); // channel
	Code->Emit(OP_CONSTANT, pr); // process
}

static int EmitOutputDest(signal_def_t *sig, lh_list_t<ident_t> &via)
{
	return findOutputDest(0, 0, sig, via, True);
}

//-------------------------------------------------------
// Location of exported/viewed variables

int process_def_t::findVar(
	lh_list_node_t<qualifier_elt_t> *q, name_t &n,
	data_def_t* &typ,
	Bool_t isExported, Bool_t isRevealed,
	int &idx, process_def_t* &p, Bool_t matchQ)
{
	lh_list_node_t<variable_def_t> *v = variables.front();
	// Qualifier must terminate here else we fail
	if (q)
	{
		if (q->info.qt == Q_PROCESS)
		{
			if (q->info.nm.index() == nm.index())
			{
				q = q->next();
				goto matched;
			}
		}
	}
	if (matchQ) return 0;
matched:
	if (q) return 0;
	int cnt = 0;
	data_def_t *rtntyp = NULL;
	for ( ; v ; v = v->next())
	{
		if ((isExported && !v->info.isExported)
			|| (isRevealed && !v->info.isRevealed))
				continue;
		lh_list_node_t<variable_decl_t> *d = v->info.decls.front();
		for ( ; d ; d = d->next())
		{
			lh_list_node_t<variable_name_t> *vn = d->info.names.front();
			for ( ; vn ; vn = vn->next())
			{
				if (vn->info.index() == n.index())
				{
					data_def_t *dd = (data_def_t *)ScopeStack->dequalify(&d->info.sort, Q_TYPE);
					if (dd && (typ==NULL || typ == dd))
					{
						idx = vn->info.offset;
						p = this;
						rtntyp = dd;
						cnt++;
					}
				}
			}
		}
	}
	if (typ==NULL) typ = rtntyp;
	return cnt;
}

int block_def_t::findVar(
	lh_list_node_t<qualifier_elt_t> *q, name_t &n,
	data_def_t* &typ,
	Bool_t isExported, Bool_t isRevealed,
	int &idx, process_def_t* &p, Bool_t matchQ
	)
{
	lh_list_node_t<process_def_t> *pr = processes.front();
	if (q)
	{
		if (q->info.qt == Q_BLOCK)
		{
			if (q->info.nm.index() == nm.index())
			{
				q = q->next();
				matchQ = True;
				goto matched;
			}
			else return 0;
		}
	}
	if (matchQ) return 0; // failed to continue dequalify
matched:
	int cnt = 0;
	while (pr)
	{
		cnt += pr->info.findVar(q, n, typ, isExported, isRevealed, idx, p, matchQ);
		pr = pr->next();
	}
	return cnt;
}

int system_def_t::findVar(
	lh_list_node_t<qualifier_elt_t> *q, name_t &n, 
	data_def_t* &typ,
	Bool_t isExported, Bool_t isRevealed,
	int &idx, process_def_t* &p
	)
{
	Bool_t matchQ = False;
	lh_list_node_t<block_def_t> *b = blocks.front();
	if (q)
	{
		if (q->info.qt == Q_SYSTEM)
		{
			if (q->info.nm.index() == nm.index())
			{
				q = q->next();
				matchQ = True;
			}
			else return 0;
		}
	}
	int cnt = 0;
	while (b)
	{
		cnt += b->info.findVar(q, n, typ, isExported, isRevealed, idx, p, matchQ);
		b = b->next();
	}
	return cnt;
}

int findVar(ident_t &id,
	data_def_t* &typ,
	Bool_t isExported, Bool_t isRevealed,
	int &idx, process_def_t* &p)
{
	return sys->findVar(id.qual.front(),id.nm, typ,
		isExported, isRevealed, idx, p);
}

int findVar(name_t &nm, data_def_t* &typ,
	Bool_t isExported, Bool_t isRevealed,
	int &idx, process_def_t* &p)
{
	return sys->findVar(NULL, nm, typ, isExported, isRevealed, idx, p);
}

//---------------------------------------------------------------------
template<class T>
void EmitL(lh_list_t<T> &l)
{
	lh_list_node_t<T> *p = l.front();
	while (p)
	{
		p->info.Emit();
		p = p->next();
	}
}

void EmitL(lh_list_t<variable_decl_t>&);
void EmitL(lh_list_t<variable_def_t>&);
void EmitL(lh_list_t<gnode_t>&);
void EmitL(lh_list_t<procedure_def_t>&);
void EmitL(lh_list_t<process_def_t>&);
void EmitL(lh_list_t<block_def_t>&);

void variable_decl_t::Emit()
{
	lh_list_node_t<variable_name_t> *nl = names.front();
	int typsize = sort.Size();
	if (typsize<1) return;
//	data_def_t *dd = (data_def_t *)ScopeStack->dequalify(&sort, Q_TYPE);
//	data_def_t *defdd = GetDataDefP(vtype);
	while (nl)
	{
		Code->Emit(OP_LOCALVAR, nl->info.offset);
		if (value)
		{
			// assuming one word; checked by SDLcheck
			assert(typsize==1);
			Code->Emit(OP_CONSTANT, ival);
//			if ((dd == realType || dd == durType || dd == timeType)
//				&& (defdd == intType || defdd == naturalType))
//				Code->Emit(OP_FLOAT);
		}
		else for (int i = 0; i < typsize; i++)
			Code->Emit(OP_CONSTANT, UNDEFINED);
		Code->Emit(OP_ASSIGN, typsize);
		nl = nl->next();
	}
}

void variable_def_t::Emit()
{
	EmitL(decls);
}

void int_literal_t::Emit()
{
	Code->Emit(OP_CONSTANT, value);
}

void real_literal_t::Emit()
{
	Code->Emit(OP_FCONST, value);
}

void enum_ref_t::Emit()
{
	Code->Emit(OP_CONSTANT, value);
}

void synonym_ref_t::Emit()
{
	data_def_t *dd = GetDataDefP(defn);
	synonym_def_t *sd = GetSynonymDefP(dd->contents);
	if (sd->type == REAL_TYP)
		Code->Emit(OP_FCONST, sd->value);
	else
		Code->Emit(OP_CONSTANT, sd->value);
}

int cond_expr_t::Emit()
{
	int rtn;
	expression_t *ip = GetExprP(if_part);
	expression_t *tp = GetExprP(then_part);
	expression_t *ep = GetExprP(else_part);
	s_code_word_t l = newlabel();
	Code->Emit(OP_NEWLINE, ip->file, ip->place);
	ip->Emit();
	Code->Emit(OP_DO, l);
	Code->Emit(OP_NEWLINE, tp->file, tp->place);
	rtn = tp->Emit();
	if (ep)
	{
		s_code_word_t e = newlabel();
		Code->Emit(OP_GOTO, e);
		Code->Emit(OP_DEFADDR, l);
		Code->Emit(OP_NEWLINE, ep->file, ep->place);
		ep->Emit();
		Code->Emit(OP_DEFADDR, e);
	}
	else Code->Emit(OP_DEFADDR, l);
	return rtn;
}

static int EmitSigArgs(lh_list_t<ident_t> &fpars, lh_list_t<heap_ptr_t> &apars)
{
	int len = 0;
	lh_list_node_t<ident_t> *fpt = fpars.front();
	lh_list_node_t<heap_ptr_t> *apt = apars.front();
	while (apt)
	{
		expression_t *e = GetExprP(apt->info);
		if (e) 
			len += e->Emit();
		else
		{
			int l = fpt->info.Size(); // Get size of type
			len += l;
			while (l--)
				Code->Emit(OP_CONSTANT, UNDEFINED);
		}
		fpt = fpt->next();
		apt = apt->next();
	}
	return len;
}

void timer_active_expr_t::Emit()
{
	timer_def_t *td = (timer_def_t *)ScopeStack->dequalify(&timer, Q_TIMER);
	int len = EmitSigArgs(td->paramtypes, params);
	Code->Emit(OP_TESTTIMER, td->idx, len);
}

int unop_t::Emit(Bool_t isRefPar) // return size of result
{
	int rtn;
	Bool_t IsI, IsR;
	void *p = GetVoidP(prim);
	switch (type)
	{
	case PE_EMPTY:
		rtn = 0;
		break;
	case PE_SLITERAL:
		assert(0);
	case PE_RLITERAL:
		((real_literal_t *)p)->Emit();
		rtn = 1;
		break;
	case PE_NLITERAL:
	case PE_ILITERAL:
		((int_literal_t *)p)->Emit();
		rtn = 1;
		break;
	case PE_SYNONYM:
		((synonym_ref_t *)p)->Emit();
		rtn = 1;
		break;
	case PE_ENUMREF:
		((enum_ref_t *)p)->Emit();
		rtn = 1;
		break;
	case PE_BLITERAL:
		Code->Emit(OP_CONSTANT, (u_s_code_word_t )prim);
		rtn = 1;
		break;
	case PE_EXPR:
		rtn = ((expression_t *)p)->Emit();
		break;
	case PE_FIX:
		rtn = ((expression_t *)p)->Emit();
		Code->Emit(OP_FIX);
		break;
	case PE_FLOAT:
		rtn = ((expression_t *)p)->Emit();
		Code->Emit(OP_FLOAT);
		break;
	case PE_CONDEXPR:
		rtn = ((cond_expr_t *)p)->Emit();
		break;
	case PE_NOW:
		Code->Emit(OP_GLOBALTIME);
		rtn = 1;
		break;
	case PE_SELF:	
		Code->Emit(OP_PREDEFVAR, SELF);
		rtn = 1;
		break;
	case PE_PARENT:
		Code->Emit(OP_PREDEFVAR, PARENT);
		rtn = 1;
		break;
	case PE_OFFSPRING:
		Code->Emit(OP_PREDEFVAR, OFFSPRING);
		rtn = 1;
		break;
	case PE_SENDER:
		Code->Emit(OP_PREDEFVAR, SENDER);
		rtn = 1;
		break;
	case PE_TIMERACTIV:
		timer_active_expr_t *ta = (timer_active_expr_t *)p;
		ta->Emit();
		rtn = 1;
		break;
	case PE_IMPORT:
		IsI = True;
		IsR = False;
		goto import;
	case PE_VIEW:
		IsR = True;
		IsI = False;
		goto import;
	import:
	{
		process_def_t *p;
		int idx;
		view_expr_t *ie = (view_expr_t *)p;
		data_def_t *dd;
		int cnt = findVar(ie->var, dd, IsR, IsI, idx, p);
		assert(cnt==1 && dd && p);
		expression_t *e = GetExprP(ie->pid_expr);
		e->Emit();
		Code->Emit(OP_SDLIMPORT, GetOffset(p), idx, rtn = dd->Size());
	}
	case PE_VARREF:
		int sz, plc;
		((var_ref_t *)p)->Emit(sz, plc);
		if (sz)
			if (!isRefPar) Code->Emit(OP_VALUE, sz);
			else sz = 1; // ref param is address = 1 word
		rtn = sz;
		isRefPar = False;
		break;
	}
	assert(!isRefPar);
	if (op == T_MINUS)
		Code->Emit(OP_MINUS);
	else if (op == T_NOT)
		Code->Emit(OP_NOT);
	return rtn;
}

int expression_t::Emit(Bool_t isRefPar) // returns size of result on stack
{
	int len;
	if (op == T_UNDEF)
	{
		unop_t *u = GetUnOpP(l_op);
		len = u->Emit(isRefPar);
	}
	else
	{
		assert(!isRefPar);
		expression_t *l = GetExprP(l_op);
		expression_t *r = GetExprP(r_op);
		data_def_t *ldd = GetDataDefP(l->etype);
		data_def_t *rdd = GetDataDefP(r->etype);
		int isReal =
			(ldd == realType || ldd == timeType || ldd == durType ||
			 rdd == realType || rdd == timeType || rdd == durType);
		len = l->Emit();
//		if ((ldd == intType || ldd == naturalType) &&
//			(rdd == realType || rdd == durType || rdd == timeType))
//			Code->Emit(OP_FLOAT);
		int len2 = r->Emit();
//		if ((rdd == intType || rdd == naturalType) &&
//			(ldd == realType || ldd == durType || ldd == timeType))
//			Code->Emit(OP_FLOAT);
		assert(len == len2);
		switch (op)
		{
		case T_BIMP:
			assert(0);
			break;
		case T_OR:
			Code->Emit(OP_OR);
			break;
		case T_XOR:
			Code->Emit(OP_XOR);
			break;
		case T_AND:
			Code->Emit(OP_AND);
			break;
		case T_NOT:
			assert(0);
		case T_EQ:
			Code->Emit(isReal ? OP_FEQU : OP_EQUAL);
			break;
		case T_NE:
			Code->Emit(isReal ? OP_FNEQ : OP_NOTEQUAL);
			break;
		case T_GT:
			Code->Emit(isReal ? OP_FGTR : OP_GREATER);
			break;
		case T_GE:
			Code->Emit(isReal ? OP_FGEQ : OP_NOTLESS);
			break;
		case T_LESS:
			Code->Emit(isReal ? OP_FLES : OP_LESS);
			break;
		case T_LE:
			Code->Emit(isReal ? OP_FLEQ : OP_NOTGREATER);
			break;
		case T_IN:
			assert(0);
		case T_EQUALS:
			assert(0);
		case T_MINUS:
			Code->Emit(isReal ? OP_FSUB : OP_SUBTRACT);
			break;
		case T_CONCAT:
			assert(0);
			break;
		case T_PLUS:
			Code->Emit(isReal ? OP_FADD : OP_ADD);
			break;
		case T_ASTERISK:
			Code->Emit(isReal ? OP_FMUL : OP_MULTIPLY);
			break;
		case T_SLASH:
			Code->Emit(isReal ? OP_FDIV : OP_DIV);
			break;
		case T_MOD:
			Code->Emit(OP_MODULO);
			break;
		case T_REM:
			Code->Emit(OP_REM);
			break;
		}
	}
	return len;
}

Bool_t var_ref_t::Emit(int &sz, int &offset)
{
	data_def_t *dd;
	int off, lvl;
	typeclass_t t;
	ident_t *idp = GetIdentP(id);
	void *v = ScopeStack->GetInfo(*idp, dd, lvl, off, t);
	assert(dd);
	offset = off;
	if (dd->size<1)
	{
		sz = 0;
		return False;
	}
	// If a reference parameter we do a OP_VARPARAM, else OP_VARIABLE
	if (t==Q_PROCEDUREPAR && !((procedure_formal_param_t *)v)->isValue)
		Code->Emit(OP_VARPARAM, ScopeStack->RelLvl(lvl), off);
	else
		Code->Emit(OP_VARIABLE, ScopeStack->RelLvl(lvl), off);
	lh_list_node_t<selector_t> *sl = sel.front();
	while (sl)
	{
		switch (sl->info.type)
		{
		case SEL_ELT:
			assert(dd->tag == ARRAY_TYP);
			{
				array_def_t *ad = GetArrDefP(dd->contents);
				expression_t *e = GetExprP(sl->info.val);
				e->Emit();
				Code->Emit(OP_INDEX, 0, ad->dval-1, ad->sort.Size());
				dd = (data_def_t *)ScopeStack->dequalify(&ad->sort, Q_TYPE);
			}
			break;
		case SEL_FIELD:
			assert(dd->tag == STRUCT_TYP);
			{
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
							Code->Emit(OP_FIELD, fn->info.offset);
							dd = (data_def_t *)ScopeStack->dequalify(&f->info.sort, Q_TYPE);
							goto nextsel;
						}
						fn = fn->next();
					}
					f = f->next();
				}
			}
			assert(0); // unknown selector
			break;
		}
	nextsel:
		sl = sl->next();
	}
	sz = dd->size;
	return (t==Q_VARDEF && ((variable_def_t *)v)->isRevealed)
		? True : False; // must export if changed?
}

void assignment_t::Emit()
{
	int sz, offset;
	Code->Emit(OP_NEWLINE, file, place);
	// This must almost certainly be modified to handle activation
	// records properly!!
	Bool_t mustExport = lval.Emit(sz, offset);
	expression_t *e = GetExprP(rval);
	// Get the types by calling Check again
	data_def_t *lt, *rt;
	lval.Check(lt);
	e->Check(rt);
	int len = e->Emit();
	assert(sz == len);
//	if (lt==realType && (rt==intType || rt==naturalType))
//		Code->Emit(OP_FLOAT);
	if (mustExport) Code->Emit(OP_SDLEXPORT, offset, sz);
	if (lt == naturalType)
		Code->Emit(OP_NATASSIGN);
	else
		Code->Emit(OP_ASSIGN,sz);
}

void output_t::Emit()
{
	lh_list_node_t<output_arg_t> *ol = signals.front();
	while (ol)
	{
		Code->Emit(OP_NEWLINE, ol->info.file, ol->info.place);
		signal_def_t *sd = (signal_def_t *)ScopeStack->dequalify(&ol->info.signal, Q_SIGNAL);
		int len = EmitSigArgs(sd->sortrefs, ol->info.args);
		int destCnt = EmitOutputDest(sd, via);
		if (hasDest) GetExprP(dest)->Emit();
		else Code->Emit(OP_CONSTANT, UNDEFINED);
		Code->Emit(OP_SDLOUTPUT,sd->idx, destCnt, len);
		ol = ol->next();
	}
}

static int EmitProcessArgs(lh_list_t<process_formal_param_t> &fpars, lh_list_t<heap_ptr_t> &apars)
{
	int len = 0;
	lh_list_node_t<process_formal_param_t> *fpt = fpars.front();
	lh_list_node_t<heap_ptr_t> *apt = apars.front();
	while (fpt)
	{
		lh_list_node_t<process_param_name_t> *fpn = fpt->info.names.front();
		int l = fpt->info.sort.Size(); // Get size of type
		while (fpn)
		{
			expression_t *e = GetExprP(apt->info);
			apt = apt->next();
			if (e) 
				len += e->Emit();
			else
			{
				len += l;
				for (int i = 0; i < l; i++)
					Code->Emit(OP_CONSTANT, UNDEFINED);
			}
			fpn = fpn->next();
		}
		fpt = fpt->next();
	}
	return len;
}

static int EmitDummyProcessArgs(lh_list_t<process_formal_param_t> &fpars)
{
	int len = 0;
	lh_list_node_t<process_formal_param_t> *fpt = fpars.front();
	while (fpt)
	{
		lh_list_node_t<process_param_name_t> *fpn = fpt->info.names.front();
		int l = fpt->info.sort.Size(); // Get size of type
		while (fpn)
		{
			len += l;
			for (int i = 0; i < l; i++)
				Code->Emit(OP_CONSTANT, UNDEFINED);
			fpn = fpn->next();
		}
		fpt = fpt->next();
	}
	Code->Emit(OP_CONSTANT, 0); // PARENT param
	return len;
}

static int EmitProcedureArgs(lh_list_t<procedure_formal_param_t> &fpars, lh_list_t<heap_ptr_t> &apars)
{
	int len = 0;
	lh_list_node_t<procedure_formal_param_t> *fpt = fpars.front();
	lh_list_node_t<heap_ptr_t> *apt = apars.front();
	while (fpt)
	{
		lh_list_node_t<procedure_param_name_t> *fpn = fpt->info.names.front();
		int l = fpt->info.isValue ? fpt->info.sort.Size() : 1; // Get size of type
		while (fpn)
		{
			expression_t *e = GetExprP(apt->info);
			apt = apt->next();
			if (e) 
				len += e->Emit(fpt->info.isValue?False:True);
			else
			{
				len += l;
				for (int i = 0; i < l; i++)
					Code->Emit(OP_CONSTANT, UNDEFINED);
			}
			fpn = fpn->next();
		}
		fpt = fpt->next();
	}
	return len;
}

void invoke_node_t::Emit()
{
	Code->Emit(OP_NEWLINE, file, place);
	if (tag == T_CREATE)
	{
		process_def_t *pd = (process_def_t *)ScopeStack->dequalify(&ident, Q_PROCESS);
		int len = EmitProcessArgs(pd->params, args);
		Code->Emit(OP_SDLINIT, pd->startLbl, len,
			pd->level - ScopeStack->Depth() - 1, pd->maxval);
	}
	else
	{
		procedure_def_t *pd = (procedure_def_t *)ScopeStack->dequalify(&ident, Q_PROCEDURE);
		int len = EmitProcedureArgs(pd->params, args);
		Code->Emit(OP_SDLCALL,pd->startLbl,len,
			pd->level - ScopeStack->Depth() - 1);
	}
}

void timer_set_t::Emit()
{
	Code->Emit(OP_NEWLINE, file, place);
	timer_def_t *td = (timer_def_t *)ScopeStack->dequalify(&timer, Q_TIMER);
	int len = EmitSigArgs(td->paramtypes, args);
	expression_t *e = GetExprP(time);
	(void)e->Emit();
	Code->Emit(OP_SETTIMER, td->idx, len);
}

void timer_reset_t::Emit()
{
	Code->Emit(OP_NEWLINE, file, place);
	timer_def_t *td = (timer_def_t *)ScopeStack->dequalify(&timer, Q_TIMER);
	int len = EmitSigArgs(td->paramtypes, args);
	Code->Emit(OP_RESETTIMER, td->idx, len);
}

void read_node_t::Emit()
{
	int sz, offset;
	Code->Emit(OP_NEWLINE, file, place);
	// This must almost certainly be modified to handle activation
	// records properly??
	Bool_t mustExport = varref.Emit(sz, offset);
	assert(sz==1);
	Code->Emit(OP_READ);
	if (mustExport) Code->Emit(OP_SDLEXPORT, offset, sz);
	Code->Emit(OP_SIMPLEASSIGN);
}

void write_node_t::Emit()
{
	Code->Emit(OP_NEWLINE, file, place);
	lh_list_node_t<heap_ptr_t> *e = exprs.front();
	while (e)
	{
		data_def_t *edd;
		(void)GetExprP(e->info)->Check(edd); // get type
		GetExprP(e->info)->Emit();
		if (edd == realType || edd == timeType || edd == durType)
			Code->Emit(OP_WRITE, 1);
		else
			Code->Emit(OP_WRITE, 0);
		e = e->next();
	}
}

void gnode_t::Emit()
{
	if (label.name()[0])
		Code->Emit(OP_DEFADDR, ScopeStack->findLabel(label));
	Code->Emit(OP_NEWLINE, file, place);
	void *p = GetVoidP(node);
	switch (type)
	{
	case N_TASK:
		EmitL(*((lh_list_t<assignment_t> *)p));
		break;
	case N_OUTPUT:
		((output_t *)p)->Emit();
		break;
//	case N_PRI_OUTPUT:
//		((pri_output_t *)p)->Emit();
//		break;
	case N_CREATE:
		((invoke_node_t *)p)->Emit();
		break;
	case N_CALL:
		((invoke_node_t *)p)->Emit();
		break;
	case N_SET:
		EmitL(*((lh_list_t<timer_set_t> *)p));
		break;
	case N_RESET:
		EmitL(*((lh_list_t<timer_reset_t> *)p));
		break;
	case N_EXPORT:
	{
		lh_list_node_t<ident_t> *i = ((lh_list_t<ident_t> *)p)->front();
		data_def_t *dd;
		int off, lvl;
		while (i)
		{
			typeclass_t t;
			ScopeStack->GetInfo(i->info, dd, lvl, off, t);
			assert(dd && t==Q_VARDEF);
			int sz = dd->Size();
			Code->Emit(OP_VARIABLE, ScopeStack->RelLvl(lvl), off);
			Code->Emit(OP_VALUE, sz);
			Code->Emit(OP_SDLEXPORT, off, sz);
			Code->Emit(OP_POP, sz);
			i = i->next();
		}
		break;
	}
	case N_DECISION:
		((decision_node_t *)p)->Emit();
		break;
	case N_READ:
		((read_node_t *)p)->Emit();
		break;
	case N_WRITE:
		((write_node_t *)p)->Emit();
		break;
	}
}

void transition_t::Emit(int lbl)
{
	Code->Emit(OP_DEFADDR, lbl);
	Code->Emit(OP_NEWLINE, file, place);
	EmitL(nodes);
	if (label.name()[0])
	{
		lbl = ScopeStack->findLabel(label);
		assert(lbl);
		Code->Emit(OP_DEFADDR, lbl);
		Code->Emit(OP_NEWLINE, label.file, label.place);
	}
	switch (type)
	{
	case NEXTSTATE:
		if (next.name()[0])
			Code->Emit(OP_SDLNEXT, ScopeStack->findState(next));
		else
			Code->Emit(OP_SDLNEXT, -1);
		break;
	case STOP:
		Code->Emit(OP_SDLSTOP);
		break;
	case RETURN:
		Code->Emit(OP_SDLRETURN);
		break;
	case JOIN:
		lbl = ScopeStack->findLabel(next);
		assert(lbl);
		Code->Emit(OP_GOTO, lbl);
		break;
	}
}

void range_condition_t::Emit()
{
	Code->Emit(OP_SDLRANGE, op, lval, uval);
}

void decision_node_t::Emit()
{
	// Lots of the funny stuff here is just to keep the
	// stack consistent, even if subtransitions terminate
	Code->Emit(OP_NEWLINE, file, place);
	if (!answers.isEmpty())
	{
		(void)GetExprP(question)->Emit();
		lh_list_node_t<subdecision_node_t> *sdn = answers.front();
		int l = newlabel(), e = newlabel();
		Code->Emit(OP_CONSTANT, 0); // default false
		// We rely on the fact that one path must be chosen
		while (sdn)
		{
			Code->Emit(OP_NEWLINE, sdn->info.file, sdn->info.place);
			if (!sdn->info.answer.isEmpty())
			{
				EmitL(sdn->info.answer);
				Code->Emit(OP_TESTRANGE, l);
				sdn->info.transition.Emit(0);
				Code->Emit(OP_GOTO, e);
				Code->Emit(OP_DEFADDR, l);
				l = newlabel();
			}
			else // else part
			{
				Code->Emit(OP_POP, 2);
				sdn->info.transition.Emit(0);
				assert(sdn->next()==NULL);
			}
			sdn = sdn->next();
		}
		Code->Emit(OP_DEFADDR, e);
	}
}

// recursive routine to reverse order for popping off stack

static void EmitInputArgs(lh_list_node_t<ident_t> *args,
	lh_list_node_t<ident_t> *vars)
{
	if (args->next())
		EmitInputArgs(args->next(),vars?vars->next():NULL);
	int sz = args->info.Size();
	if (vars && vars->info.name()[0])
	{
		data_def_t *dd;
		int off, lvl;
		typeclass_t t;
		void *p = ScopeStack->GetInfo(vars->info, dd, lvl, off, t);
		if (t==Q_PROCEDUREPAR && ((procedure_formal_param_t *)p)->isValue==False)
			Code->Emit(OP_VARPARAM, ScopeStack->RelLvl(lvl), off);
		else 
			Code->Emit(OP_VARIABLE, ScopeStack->RelLvl(lvl), off);
		Code->Emit(OP_ARGASSIGN,sz);
		if (t==Q_VARDEF && ((variable_def_t *)p)->isExported)
			Code->Emit(OP_SDLEXPORT,off,sz);
	}
	else
		Code->Emit(OP_POP,sz);
}

int state_node_t::Emit(int &lbl)
{
	// Compute saves
	// loop thru inputs
	int cnt = 0, start, prov, when;
	lh_list_node_t<input_part_t> *il = inputs.front();
	while (il)
	{
		s_code_word_t ASTloc = GetOffset(&il->info.transition);
		start = newlabel();
		prov = newlabel();
		when = newlabel();
		if (il->info.isAsterisk)
		{
			cnt++;
			Code->Emit(OP_DEFADDR, lbl);
			lbl = newlabel();
#ifdef W16
			Code->Emit(OP_SDLTRANS,from[0],from[1],from[2],from[3],
				0, prov, when, lbl, start, ASTloc);
#else
			Code->Emit(OP_SDLTRANS,from[0],from[1],
				0, prov, when, lbl, start, ASTloc);
#endif
			Code->Emit(OP_DEFARG,when,-1);
		}
		else
		{
			lh_list_node_t<stimulus_t> *st = il->info.stimuli.front();
			while (st)
			{
				cnt++;
				Code->Emit(OP_DEFADDR, lbl);
				lbl = newlabel();
#ifdef W16
				Code->Emit(OP_SDLTRANS,from[0],from[1],from[2],from[3],
					0, prov, when, lbl, when, ASTloc);
#else
				Code->Emit(OP_SDLTRANS,from[0],from[1],
					0, prov, when, lbl, when, ASTloc);
#endif
				Code->Emit(OP_DEFADDR, when);
				Code->Emit(OP_NEWLINE, st->info.file, st->info.place);
				int sigId;
				signal_def_t *s =
					(signal_def_t *)ScopeStack->dequalify(&st->info.signal, Q_SIGNAL);
				lh_list_node_t<ident_t> *arg;
				if (s)
				{
					arg = s->sortrefs.front();
					sigId = s->idx;
				}
				else
				{
					timer_def_t *t =
						(timer_def_t *)ScopeStack->dequalify(&st->info.signal, Q_TIMER);
					assert(t);
					arg = t->paramtypes.front();
					sigId = t->idx;
				}
#ifdef W16
				Code->Emit(OP_SDLINPUT, sigId,
					save.bitmask[0], save.bitmask[1],
					save.bitmask[2], save.bitmask[3]);
#else
				Code->Emit(OP_SDLINPUT, sigId,
					save.bitmask[0], save.bitmask[1]);
#endif
				Code->Emit(OP_ENDCLAUSE);
				if (arg)
				{
					lh_list_node_t<ident_t> *vars = st->info.variables.front();
					EmitInputArgs(arg,vars);
				}
				Code->Emit(OP_GOTO, start);
				st = st->next();
			}
		}
		if (il->info.enabler)
		{
			Code->Emit(OP_DEFADDR, prov);
			expression_t *e = GetExprP(il->info.enabler);
			Code->Emit(OP_NEWLINE, e->file, e->place);
			e->Emit();
			Code->Emit(OP_ENDCLAUSE);
		}
		else Code->Emit(OP_DEFARG, prov, -1);
		il->info.transition.Emit(start);
//		Code->Emit(OP_ENDTRANS);
		il = il->next();
	}
	lh_list_node_t<continuous_signal_t> *cs = csigs.front();
	while (cs)
	{
		s_code_word_t ASTloc = GetOffset(&cs->info.transition);
		cnt++;
		Code->Emit(OP_DEFADDR, lbl);
		lbl = newlabel();
		start = newlabel();
		prov = newlabel();
		when = newlabel();
#ifdef W16
		Code->Emit(OP_SDLTRANS, from[0],from[1],from[2],from[3],
			cs->info.priority, prov, when, lbl, start, ASTloc);
#else
		Code->Emit(OP_SDLTRANS, from[0],from[1],
			cs->info.priority, prov, when, lbl, start, ASTloc);
#endif
		Code->Emit(OP_DEFARG,when,-1);
		if (cs->info.condition)
		{
			Code->Emit(OP_DEFADDR, prov);
			expression_t *e = GetExprP(cs->info.condition);
			Code->Emit(OP_NEWLINE, e->file, e->place);
			e->Emit();
			Code->Emit(OP_ENDCLAUSE);
		}
		else Code->Emit(OP_DEFARG, prov, -1);
		cs->info.transition.Emit(start);
//		Code->Emit(OP_ENDTRANS);
		cs = cs->next();
	}
	return cnt;
}

void procedure_def_t::Emit()
{
	lh_list_t<procedure_def_t> *pl = 
		(lh_list_t<procedure_def_t> *)GetVoidP(procedures);
	ScopeStack->EnterScope(Q_PROCEDURE,this);
	if (verbose)
		cout << endl << endl<< "Generating code for procedure " << name() << endl;
	Code->Emit(OP_DEFADDR, startLbl);
	int istart = newlabel();
	int tstart = newlabel();
	int tcount = newlabel();
	Code->Emit(OP_SDLMODULE,GetOffset(this),Q_PROCEDURE,varSize,parSize,
		tcount,istart,tstart, ScopeStack->Depth());
	ScopeStack->Emit();
	if (procedures) EmitL(*pl);
	Code->Emit(OP_DEFADDR, istart);
	int bstart = newlabel();
#ifdef W16
	Code->Emit(OP_SDLTRANS, -1, -1, -1, -1, 0, -1, -1, -1, bstart, GetOffset(&start));
#else
	Code->Emit(OP_SDLTRANS, -1, -1, 0, -1, -1, -1, bstart, GetOffset(&start));
#endif
	Code->Emit(OP_DEFADDR, bstart);
	EmitL(variables);
	start.Emit(0);
//	Code->Emit(OP_ENDTRANS);
	lh_list_node_t<state_node_t> *sn = states.front();
	int cnt = 0;
	while (sn)
	{
		cnt += sn->info.Emit(tstart);
		sn = sn->next();
	}
	Code->Emit(OP_DEFARG, tstart, -1);
	Code->Emit(OP_DEFARG, tcount, cnt);
	delete [] labelTbl;
	if (verbose)
		cout << endl << endl<< "End of code for procedure " << name() << endl;
	ScopeStack->ExitScope();
}

void process_def_t::Emit()
{
	ScopeStack->EnterScope(Q_PROCESS,this);
	if (verbose)
		cout << endl << endl<< "Generating code for process " << name() << endl;
	// Emit scope instructions
	Code->Emit(OP_DEFADDR, startLbl);
	int istart = newlabel();
	int tstart = newlabel();
	int tcount = newlabel();
	Code->Emit(OP_SDLMODULE,GetOffset(this),Q_PROCESS,varSize,parSize,
		tcount,istart,tstart, ScopeStack->Depth());
	ScopeStack->Emit();
	EmitL(procedures);
	Code->Emit(OP_DEFADDR, istart);
	int bstart = newlabel();
#ifdef W16
	Code->Emit(OP_SDLTRANS, -1, -1, -1, -1, 0, -1, -1, -1, bstart, GetOffset(&start));
#else
	Code->Emit(OP_SDLTRANS, -1, -1, 0, -1, -1, -1, bstart, GetOffset(&start));
#endif
	Code->Emit(OP_DEFADDR, bstart);
	EmitL(variables);
	start.Emit(0);
//	Code->Emit(OP_ENDTRANS);
	lh_list_node_t<state_node_t> *sn = states.front();
	int cnt = 0;
	while (sn)
	{
		cnt += sn->info.Emit(tstart);
		sn = sn->next();
	}
	Code->Emit(OP_DEFARG, tstart, -1);
	Code->Emit(OP_DEFARG, tcount, cnt);
	delete [] labelTbl;
	if (verbose)
		cout << endl << endl<< "End of code for process " << name() << endl;
	ScopeStack->ExitScope();
}

void subblock_def_t::Emit()
{
	ScopeStack->EnterScope(Q_SUBSTRUCTURE,this);
	if (verbose)
		cout << endl << endl<< "Generating code for subblock " << name() << endl;
	if (verbose)
		cout << endl << endl<< "End of code for subblock " << name() << endl;
	ScopeStack->ExitScope();
}

void block_def_t::Emit()
{
	ScopeStack->EnterScope(Q_BLOCK,this);
	if (verbose)
		cout << endl << endl<< "Generating code for block " << name() << endl;
	int tstart = newlabel();
	Code->Emit(OP_DEFADDR, startLbl);
	Code->Emit(OP_SDLMODULE,GetOffset(this),Q_BLOCK,0,0,-1,tstart,-1,
		ScopeStack->Depth());
	ScopeStack->Emit();
	Code->Emit(OP_DEFADDR, tstart);
	tstart = newlabel();
#ifdef W16
	Code->Emit(OP_SDLTRANS, -1, -1, -1, -1, 0, -1, -1, -1, tstart, 0);
#else
	Code->Emit(OP_SDLTRANS, -1, -1, 0, -1, -1, -1, tstart, 0);
#endif
	Code->Emit(OP_DEFADDR,tstart);
	lh_list_node_t<process_def_t> *pn = processes.front();
	while (pn)
	{
		int cnt = pn->info.initval;
		while (cnt--)
		{
			Code->Emit(OP_CONSTANT, UNDEFINED);
			int len = EmitDummyProcessArgs(pn->info.params);
			Code->Emit(OP_SDLINIT, pn->info.startLbl, len,
					pn->info.maxval);
		}
		pn = pn->next();
	}
	Code->Emit(OP_SDLSTOP);
	EmitL(processes);
	if (verbose)
		cout << endl << endl<< "End of code for block " << name() << endl;
	ScopeStack->ExitScope();
}

void system_def_t::Emit(int doLink)
{
	Code = new codespace_t;
	assert(Code);
	if (verbose) Code->verbose = 1;
	ScopeStack->EnterScope(Q_SYSTEM,this);
	if (verbose)
		cout << endl << endl<< "Generating code for system " << name() << endl;
	int tstart = newlabel();
	Code->Emit(OP_SDLMODULE,GetOffset(this),Q_SYSTEM,0,0,-1,tstart,-1,1);
	(void)ScopeStack->Emit();
	Code->Emit(OP_DEFADDR, tstart);
	tstart = newlabel();
#ifdef W16
	Code->Emit(OP_SDLTRANS, -1, -1, -1, -1, 0, -1, -1, -1, tstart, 0);
#else
	Code->Emit(OP_SDLTRANS, -1, -1, 0, -1, -1, -1, tstart, 0);
#endif
	Code->Emit(OP_DEFADDR, tstart);
	// emit channel info for channel table
	lh_list_node_t<channel_def_t> *cn = channels.front();
	while (cn)
	{
		Code->Emit(OP_CHANNEL, GetOffset(&cn->info));
		cn = cn->next();
	}
	lh_list_node_t<block_def_t> *bn = blocks.front();
	while (bn)
	{
		Code->Emit(OP_CONSTANT, UNDEFINED); // PARENT param
		Code->Emit(OP_SDLINIT, bn->info.startLbl, 0, 1);
		bn = bn->next();
	}
	Code->Emit(OP_SDLSTOP);
	EmitL(blocks);
	ScopeStack->ExitScope();
	if (errorCount[ERROR]==0 && errorCount[FATAL]==0)
	{
#ifdef __MSDOS__
		ofstream os("sdl.cod", ios::binary);
#else
		ofstream os("sdl.cod");
#endif
		fileTable->Write(&os);
		if (doLink) Code->Link();
		Code->Write(NULL, &os);
	}
	if (verbose)
		cout << endl << endl<< "End of code for system " << name() << endl;
	delete Code;
}


