/*
 * SDL Abstract Syntax Tree definitions checker
 *
 * Written by Graham Wheeler
 * Data Network Architecture Laboratory
 * Department of Computer Science
 * University of Cape Town
 * 
 * (c) 1994 Graham Wheeler and the Data Network Architectures Laboratory
 * All Rights Reserved
 *
 * Last modified: 20-7-94
 *
 */

#pragma hdrfile "PASS2.SYM"
#include "sdlast.h"
#include "sdlc2.h"

//-------------------------------------------------------

static Bool_t IsENV(ident_t &i)
{
	if (strcmp(i.nm.name(), "ENV")==0 && i.qual.isEmpty())
		return True;
	else return False;
}

//-------------------------------------------------------
// Determine the possible destinations for an output.
// We assume no block substructures, thus the via list
// contains signalroutes.

int findOutputDest(int file, int place, signal_def_t *sig,
			  lh_list_t<ident_t> &via, Bool_t mustEmit)
{
	int cnt = GetOutputDest(sig, via);
	if (cnt == -1) SDLerror(file, place, ERR_VIAROUTE);
	else if (mustEmit)
	{
		for (int i = 0; i < cnt; i++)
			EmitDest(destinations[i][1], destinations[i][0]);
	}
	return cnt;
}

// Function templates for linked list iterators

template<class T>
void CheckL(lh_list_t<T> &l)
{
	lh_list_node_t<T> *p = l.front();
	while (p)
	{
		p->info.Check();
		p = p->next();
	}
}

template<class T>
void CheckRemoteL(lh_list_t<T> &l)
{
	lh_list_node_t<T> *p = l.front();
	while (p)
	{
		p->info.CheckRemote();
		p = p->next();
	}
}

template<class T>
void MarkL(lh_list_t<T> &l)
{
	lh_list_node_t<T> *p = l.front();
	while (p)
	{
		p->info.Mark();
		p = p->next();
	}
}

//-----------------------------------------------------------------
// The Mark and MarkL methods mark defining names in the 
// namestore, allowing us to ensure uniqueness of names
// where required.

void MarkL(lh_list_t<procedure_param_name_t> &l);
void MarkL(lh_list_t<process_param_name_t> &l);
void MarkL(lh_list_t<variable_name_t> &l);
void MarkL(lh_list_t<variable_decl_t> &l);
void MarkL(lh_list_t<enum_elt_t> &l);

void enum_elt_t::Mark()
{
	if (nameStore->IsNameMarked(index()))
		SDLerror(file, place, ERR_DUPENUMELT, name());
	else
		nameStore->MarkName(index());
}

void MarkEnumElts(lh_list_t<data_def_t> &l)
{
	lh_list_node_t<data_def_t> *dd = l.front();
	while (dd)
	{
		if (dd->info.tag == ENUM_TYP)
		{
			enum_def_t *ed = GetEnumDefP(dd->info.contents);
			MarkL(ed->elts);
		}
		dd = dd->next();
	}
}

void field_name_t::Mark()
{
	if (nameStore->IsNameMarked(index()))
		SDLerror(file, place, ERR_DUPFIELD, name());
	else
		nameStore->MarkName(index());
}

void fieldgrp_t::Mark()
{
	MarkL(names);
}

void procedure_param_name_t::Mark()
{
	if (nameStore->IsNameMarked(index()))
		SDLerror(file, place, ERR_DUPPARAM, name());
	else
		nameStore->MarkName(index());
}

void procedure_formal_param_t::Mark()
{
	MarkL(names);
}

void process_param_name_t::Mark()
{
	if (nameStore->IsNameMarked(index()))
		SDLerror(file, place, ERR_DUPPARAM, name());
	else
		nameStore->MarkName(index());
}

void process_formal_param_t::Mark()
{
	MarkL(names);
}

void variable_name_t::Mark()
{
	if (nameStore->IsNameMarked(index()))
		SDLerror(file, place, ERR_DUPVAR, name());
	else
		nameStore->MarkName(index());
}

void variable_decl_t::Mark()
{
	MarkL(names);
}

void variable_def_t::Mark()
{
	MarkL(decls);
}

static void MarkNameList(lh_list_t<name_t> &l, int file, int line, error_t err)
{
	lh_list_node_t<name_t> *n = l.front();
	while (n)
	{
		if (nameStore->IsNameMarked(n->info.index()))
			SDLerror(file, line, err, n->info.name());
		else n->info.Mark();
		n = n->next();
	}
}

void import_def_t::Mark()
{
	MarkNameList(names, file, place, ERR_DUPIMPORT);
}

//------------------------------------------------------------
// Ensure all remote references are resolved

void procedure_def_t::CheckRemote()
{
	if (!IsReference(RF_DEFINED))
		SDLerror(file, place, ERR_NOREMDEF, name());
}

void process_def_t::CheckRemote()
{
	if (!IsReference(RF_DEFINED))
		SDLerror(file, place, ERR_NOREMDEF, name());
}

void subblock_def_t::CheckRemote()
{
	if (!IsReference(RF_DEFINED))
		SDLerror(file, place, ERR_NOREMDEF, name());
}

void block_def_t::CheckRemote()
{
	if (!IsReference(RF_DEFINED))
		SDLerror(file, place, ERR_NOREMDEF, name());
}

//------------------------------------------------------------
// The Check and CheckL methods are responsible for checking
// the semantics of the corresponding AST nodes.

//void CheckL(lh_list_t<variable_decl_t> &l);
//void CheckL(lh_list_t<variable_def_t> &l);
//void CheckL(lh_list_t<fieldgrp_t> &l);
//void CheckL(lh_list_t<data_def_t> &l);
//void CheckL(lh_list_t<signal_def_t> &l);
//void CheckL(lh_list_t<siglist_def_t> &l);
//void CheckL(lh_list_t<channel_def_t> &l);
//void CheckL(lh_list_t<procedure_formal_param_t> &l);
//void CheckL(lh_list_t<procedure_def_t> &l);
//void CheckL(lh_list_t<process_formal_param_t> &l);
//void CheckL(lh_list_t<process_def_t> &l);
//void CheckL(lh_list_t<block_def_t> &l);

void fieldgrp_t::Check()
{
	data_def_t *dd = (data_def_t *)ScopeStack->dequalify(&sort, Q_TYPE);
	if (dd==NULL) SDLerror(file, place, ERR_NOTYPE, Id2Str(sort));
}

void array_def_t::Check()
{
	// checked index type in sdladdr
}

void synonym_def_t::Check()
{
	if (has_sort &&
		!GetDataDefP(type)->CanCast((data_def_t *)ScopeStack->dequalify(&sort, Q_TYPE)))
		SDLerror(file, place, ERR_INITTYPE, Id2Str(sort));
}

void struct_def_t::Check()
{
	// Check that all field names in structure types are unique
	nameStore->ClearNameMarks();
	MarkL(fieldgrps);
	CheckL(fieldgrps);
}

void data_def_t::Check()	
{
	if (this == intType || this == boolType || this == charType ||
		this == pidType || this == durType || this == timeType ||
		this == realType || this == naturalType)
			return;
	ScopeStack->EnterScope(Q_TYPE,this);

	// Check subnodes

	if (tag==SYNONYM_TYP)
	{
		synonym_def_t *sd = GetSynonymDefP(contents);
		sd->Check();
	}
	if (tag == STRUCT_TYP)
	{
		struct_def_t *sd = GetStrucDefP(contents);
		(void)sd->Check();
	}
	else
	{
		array_def_t *ad = GetArrDefP(contents);
		ad->Check();
	}
	ScopeStack->ExitScope();
}

void siglist_def_t::Check()	
{
}

void timer_def_t::Check()	
{
	lh_list_node_t<ident_t> *fpt = paramtypes.front();
	while (fpt)
	{
		data_def_t *fdd = (data_def_t *)ScopeStack->dequalify(&fpt->info, Q_TYPE);
		if (fdd==NULL) SDLerror(file, place, ERR_NOTYPE, Id2Str(fpt->info));
		fpt = fpt->next();
	}
}

void signal_def_t::Check()	
{
	ScopeStack->EnterScope(Q_SIGNAL,this);

	// Check that all defining names in scopeunit are unique??

	nameStore->ClearNameMarks();

	// Check that parameter sorts are indeed types

	lh_list_node_t<ident_t> *fpt = sortrefs.front();
	while (fpt)
	{
		data_def_t *fdd = (data_def_t *)ScopeStack->dequalify(&fpt->info, Q_TYPE);
		if (fdd==NULL) SDLerror(file, place, ERR_NOTYPE, Id2Str(fpt->info));
		fpt = fpt->next();
	}

	ScopeStack->ExitScope();
}

void c2r_def_t::Check()	
{
	// these are all done in Address.
	// the signal routes must be defined in the enclosing block,
	// and have ENV as one endpoint. The union of the signals
	// in the signal routes must equal the set of signals conveyed
	// on the channel for each direction. The set of channels
	// having the block as an endpoint must be equal to the set of
	// channels mentioned in connects in the block
}

static void checkPath(path_t paths[], typeclass_t typ, int endpt[])
{
	int i;
	void *orig[2], *dest[2];
	Bool_t has2 = (paths[1].Originator().name()[0]=='\0' ? False : True);
	for (i=0;i<2;i++)
	{
		if (IsENV(paths[i].originator))
		{
			orig[i] = NULL;
			endpt[i] = 0;
		}
		else 
		{
			orig[i] = ScopeStack->dequalify(&paths[i].originator, typ);
			if (orig[i] == NULL)
			{
				SDLerror(paths[i].originator.file,
					paths[i].originator.place, 
					(typ==Q_PROCESS ? ERR_BADPROC : ERR_BADBLOCK),
					"originator");
				orig[i] = &typ; // to distinguish from ENV
			}
			else endpt[i] = (typ==Q_PROCESS) ?
				((process_def_t *)orig[i])->num :
				((block_def_t *)orig[i])->num ;
		}
		if (IsENV(paths[i].destination))
		{
			dest[i] = NULL;
			endpt[1-i] = 0;
		}
		else 
		{
			dest[i] = ScopeStack->dequalify(&paths[i].destination, typ);
			if (dest[i] == NULL)
			{
				SDLerror(paths[i].destination.file,
					paths[i].destination.place,
					(typ==Q_PROCESS ? ERR_BADPROC : ERR_BADBLOCK),
					"destination");
				dest[i] = &typ; // to distinguish from ENV
			}
			else endpt[1-i] = (typ==Q_PROCESS) ?
				((process_def_t *)dest[i])->num :
				((block_def_t *)dest[i])->num ;
		}
		if (orig[i]==NULL && dest[i]==NULL)
			SDLerror(paths[i].file, paths[i].place, ERR_BOTHENV);
		else if (orig[i] == dest[i])
			SDLerror(paths[i].file, paths[i].place, ERR_PATHDUP);
		if (!has2) break;
	}
	if (has2)
	{
		if (orig[0] != dest[1] || orig[1] != dest[0])
			SDLerror(paths[0].file, paths[0].place, ERR_PATHDIF);
	}
}

void route_def_t::Check()	
{
	checkPath(paths, Q_PROCESS, endpt);
}

void connect_def_t::Check()	
{
}

void variable_decl_t::Check()
{
	// Evaluate the default value, if any
	data_def_t *defdd = NULL;
	if (value)
	{
		expression_t *e = GetExprP(value);
		if ((ival = e->EvalGround(defdd)) == UNDEFINED)
			SDLerror(file, place, ERR_GROUNDEXPR);
		vtype = GetOffset(defdd);
	}
	// Check that the type ID is a sort type, and give each name
	// node a reference offset to the type definition
	data_def_t *dd = (data_def_t *)ScopeStack->dequalify(&sort, Q_TYPE);
	if (dd==NULL) SDLerror(file, place, ERR_VARTYPE, Id2Str(sort));
	else 
	{
		if (defdd && !dd->CanCast(defdd))
			SDLerror(file, place, ERR_INITTYPE, Id2Str(sort));
		else if (dd==naturalType && ival < 0 && ival != UNDEFINED)
			SDLerror(file, place, ERR_UNNATURAL, (char *)ival);
		int sz = sort.Size();
		heap_ptr_t off = GetOffset(dd);
		if (ival != UNDEFINED && sz != 1)
			SDLerror(file, place, ERR_VARINITSZ);
		lh_list_node_t<variable_name_t> *vn = names.front();
		while (vn)
		{
			vn->info.type = off;
//cout << "Associating variable " << vn->info.name() << " with type " << dd->name() << endl;
			vn = vn->next();
		}
	}
}

void variable_def_t::Check()
{
	CheckL(decls);
}

void view_def_t::Check()
{
	data_def_t *typ = (data_def_t *)ScopeStack->dequalify(&sort, Q_TYPE);
	if (typ==NULL)
		SDLerror(file, place, ERR_BADTYPE, Id2Str(sort));
	lh_list_node_t<ident_t> *v = variables.front();
	for ( ; v ; v = v->next())
	{
		int idx;
		process_def_t *pr;
		int cnt = findVar(v->info, typ, False, True, idx, pr);
		if (cnt==0)
			SDLerror(file, place, ERR_VIEW, Id2Str(v->info));
		else if (cnt>1)
			SDLerror(file, place, ERR_DUPVIEW, Id2Str(v->info));
	}
}

void int_literal_t::Check(data_def_t* &dp)
{
	dp = (value >= 0) ? naturalType : intType;
}

void cond_expr_t::Check(data_def_t* &dp, Bool_t isRefParam)
{
	expression_t *ip = GetExprP(if_part);
	expression_t *tp = GetExprP(then_part);
	expression_t *ep = GetExprP(else_part);
	data_def_t *dt;
	ip->Check(dt);
	if (dt != boolType)
		SDLerror(file, place, ERR_BOOLEXPR);
	tp->Check(dp, isRefParam);
	if (ep) ep->Check(dt, isRefParam);
	if (dp != dt) SDLerror(file, place, ERR_CONDEXPR);
}

void var_ref_t::Check(data_def_t* &dp)
{
	data_def_t *dd;
	int off, lvl;
	typeclass_t t;
	ident_t *idp = GetIdentP(id);
	ScopeStack->GetInfo(*idp, dd, lvl, off, t);
	if (dd==NULL)
		SDLerror(file, place, ERR_VARREF, Id2Str(*idp));
	else 
	{
		lh_list_node_t<selector_t> *sl = sel.front();
		while (sl)
		{
			switch (sl->info.type)
			{
			case SEL_ELT:
				if (dd->tag != ARRAY_TYP)
					SDLerror(file, place, ERR_BADELTSEL, Id2Str(*idp));
				else
 				{
					array_def_t *ad = GetArrDefP(dd->contents);
					expression_t *e = GetExprP(sl->info.val);
					data_def_t *edd;
					e->Check(edd);
					if (edd != intType && edd != naturalType)
						SDLerror(file, place, ERR_INDEX);
					dd = (data_def_t *)ScopeStack->dequalify(&ad->sort, Q_TYPE);
				}
				break;
			case SEL_FIELD:
				if (dd->tag != STRUCT_TYP)
					SDLerror(file, place, ERR_BADFLDSEL, Id2Str(*idp));
				else
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
								dd = (data_def_t *)ScopeStack->dequalify(&f->info.sort, Q_TYPE);
								goto nextsel;
							}
							fn = fn->next();
						}
						f = f->next();
					}
				}
				SDLerror(file, place, ERR_UNKNOWNSEL, Id2Str(*idp));
				break;
			}
		nextsel:
			sl = sl->next();
		}
	}
	dp = dd;
}

static void CheckArgs(int file, int place, lh_list_t<ident_t> &fpars, lh_list_t<heap_ptr_t> &apars)
{
	int pos = 1;
	lh_list_node_t<ident_t> *fpt = fpars.front();
	lh_list_node_t<heap_ptr_t> *apt = apars.front();
	while (fpt)
	{
		if (apt==NULL)
		{
			SDLerror(file, place, ERR_MISSPARAM);
			break;
		}
		expression_t *e = GetExprP(apt->info);
		if (e)
		{
			data_def_t *add, *fdd;
			fdd = (data_def_t *)ScopeStack->dequalify(&fpt->info, Q_TYPE);
			e->Check(add);
			if (add && add != fdd && !add->CanCast(fdd))
				SDLerror(file, place, ERR_APARAMTYPE, (const char *)pos);
		}
		pos++;
		fpt = fpt->next();
		apt = apt->next();
	}
	if (apt) SDLerror(file, place, ERR_EXTRAPARAM);
}

void timer_active_expr_t::Check()
{
	// Make sure that `timer' is a timer ID, and check the
	// parameter types
	timer_def_t *td = (timer_def_t *)ScopeStack->dequalify(&timer, Q_TIMER);
	if (td==NULL) 
		SDLerror(file, place, ERR_TIMERID, Id2Str(timer));
	else
		CheckArgs(file, place, td->paramtypes, params);
}

void unop_t::Check(data_def_t* &dp, Bool_t isRefParam)
{
	Bool_t IsI, IsR;
	void *p = GetVoidP(prim);
	expression_t *e;
	view_expr_t *ie;
	process_def_t *pr;
	int idx, cnt;
	data_def_t *dd;
	switch (type)
	{
	case PE_EMPTY:
		dp = NULL;
		break;
	case PE_SLITERAL:
		assert(0);
	case PE_ENUMREF:
		dp = GetDataDefP(((enum_ref_t *)p)->type);
		break;
	case PE_SYNONYM:
	{
		dd = GetDataDefP(((synonym_ref_t *)p)->defn);
		synonym_def_t *sd = GetSynonymDefP(dd->contents);
		dp = GetDataDefP(sd->type);
		break;
	}
	case PE_NLITERAL:
	case PE_ILITERAL:
		((int_literal_t *)p)->Check(dp);
		break;
	case PE_RLITERAL:
		dp = realType;
		break;
	case PE_BLITERAL:
		dp = boolType;
		break;
	case PE_EXPR:
		((expression_t *)p)->Check(dp, isRefParam);
		isRefParam = False;
		break;
	case PE_FIX:
		((expression_t *)p)->Check(dp, isRefParam);
		isRefParam = False;
		if (dp != realType)
			SDLerror(file, place, ERR_FIX);
		dp = intType;
		break;
	case PE_FLOAT:
		((expression_t *)p)->Check(dp, isRefParam);
		isRefParam = False;
		if (dp != intType && dp!=naturalType)
			SDLerror(file, place, ERR_FLOAT);
		dp = realType;
		break;
	case PE_CONDEXPR:
		((cond_expr_t *)p)->Check(dp, isRefParam);
		break;
	case PE_NOW:
		dp = timeType;
		break;
	case PE_SELF:	
	case PE_PARENT:
	case PE_OFFSPRING:
	case PE_SENDER:
		dp = pidType;
		break;
	case PE_TIMERACTIV:
		timer_active_expr_t *ta = (timer_active_expr_t *)p;
		ta->Check();
		dp = boolType;
		break;
	case PE_IMPORT:
		// Make sure this is unambiguous, and that there
		// is a corresponding implicit var in the import
		// definition with same name and sort.
		ie = (view_expr_t *)p;
		cnt = findVar(ie->var, dd, True, False, idx, pr);
		if (dd == NULL)
			SDLerror(ie->file, ie->place, ERR_IMPORT, Id2Str(ie->var));
		else if (cnt>1)
			SDLerror(ie->file, ie->place, ERR_DUPIMPORT, Id2Str(ie->var));
		else
		{
			// Check that var is defined in import list with same type
			process_def_t *mypr = (process_def_t *)ScopeStack->GetContaining(Q_PROCESS);
			assert(mypr);
			lh_list_node_t<import_def_t> *im = mypr->imports.front();
			for (; im ; im=im->next())
			{
				data_def_t *typ = (data_def_t *)ScopeStack->dequalify(&im->info.sort, Q_TYPE);
				if (typ==NULL)
					SDLerror(im->info.file, im->info.place, ERR_BADTYPE, Id2Str(im->info.sort));
				else if (typ != dd)
					continue; // not same type
				// Go thru the names
				lh_list_node_t<name_t> *n = im->info.names.front();
				int id = ie->var.nm.index();
				for ( ; n ; n = n->next())
				{
					if (n->info.index() == id)
						break;
				}
				if (n) break;
			}
			if (im==NULL)
				SDLerror(ie->file, ie->place, ERR_NOIMPORTDEF, Id2Str(ie->var));
		}
		// Check that ie->pid_expr is of PId type
		e = GetExprP(ie->pid_expr);
		if (e)
		{
			data_def_t *edd;
			e->Check(edd);
			if (edd != pidType)
				SDLerror(file, place, ERR_PIDEXPR, "IMPORT");
		}
		break;
	case PE_VIEW:
		// We've done most of the checking in the view_def.
		// We just make sure the id corresponds to exactly one in the
		// view def list.
		ie = (view_expr_t *)p;
		int okCnt=0;
		cnt = findVar(ie->var, dd, False, True, idx, pr);
		if (dd == NULL)
			SDLerror(ie->file, ie->place, ERR_VIEW, Id2Str(ie->var));
		else if (cnt>1)
			SDLerror(ie->file, ie->place, ERR_DUPVIEW, Id2Str(ie->var));
		else
		{
			process_def_t *mypr = (process_def_t *)ScopeStack->GetContaining(Q_PROCESS);
			assert(mypr);
			lh_list_node_t<view_def_t> *v = mypr->views.front();
			for (; v ; v=v->next())
			{
				data_def_t *typ = (data_def_t *)ScopeStack->dequalify(&v->info.sort, Q_TYPE);
				if (typ==NULL) continue;
				lh_list_node_t<ident_t> *i = v->info.variables.front();
				for ( ; i ; i = i->next())
				{
					process_def_t *pr2;
					int idx2;
					int cnt2 = findVar(i->info, typ, False, True, idx2, pr2);
					if (cnt2==1)
					{
						if (pr2==pr && idx2==idx)
							okCnt++;
					}
				}
			}
		}
		if (okCnt==0)
			SDLerror(ie->file, ie->place, ERR_NOVIEWDEF, Id2Str(ie->var));
		else if (okCnt>1)
			SDLerror(ie->file, ie->place, ERR_DUPVIEWDEF, Id2Str(ie->var));
		// Check that ie->pid_expr is of PId type
		e = GetExprP(ie->pid_expr);
		if (e)
		{
			data_def_t *edd;
			e->Check(edd);
			if (edd != pidType)
				SDLerror(file, place, ERR_PIDEXPR, "VIEW");
		}
		break;
	case PE_VARREF:
		((var_ref_t *)p)->Check(dp);
		if (op != T_MINUS && op != T_NOT)
			isRefParam = False;
		break;
	}
	if (op == T_MINUS)
	{
		if (dp != intType && dp != realType && dp != naturalType)
			SDLerror(file, place, ERR_INTRLEXPR);
	}
	else if (op == T_NOT)
	{
		if (dp != boolType) SDLerror(file, place, ERR_BOOLEXPR);
	}
	if (isRefParam)
		SDLerror(file, place, ERR_REFPARAM);
}

void expression_t::Check(data_def_t* &dp, Bool_t isRefParam)
{
	if (op == T_UNDEF)
	{
		unop_t *u = GetUnOpP(l_op);
		u->Check(dp, isRefParam);
	}
	else if (isRefParam)
		SDLerror(file, place, ERR_REFPARAM);
	else
	{
		expression_t *l = GetExprP(l_op);
		expression_t *r = GetExprP(r_op);
		data_def_t *ldd, *rdd;
		char *opS;
		l->Check(ldd);
		r->Check(rdd);
		if (ldd == naturalType && (rdd == intType || rdd == naturalType))
			ldd = intType;
		if (rdd == naturalType && ldd == intType)
			rdd = intType;
		dp = ldd;
		if (ldd && rdd) switch (op)
		{
		case T_BIMP: // bool
			opS = "=>";
			SDLerror(file, place,ERR_UNSUPPORT, opS);
			goto checkbool;
		case T_OR: // bool
			opS = "OR";
			goto checkbool;
		case T_XOR: // bool
			opS = "XOR";
			goto checkbool;
		case T_AND: // bool
			opS = "AND";
			goto checkbool;
		case T_NOT: // bool
			assert(0);
		case T_EQ: // bool, char, int, pid =
			opS = "=";
			goto checkbasic;
		case T_NE: //  bool, char, int, pid /=
			opS = "/=";
			goto checkbasic;
		case T_GT: // char, int
			opS = ">";
			goto checkchintreal;
		case T_GE: // char, int
			opS = ">=";
			goto checkchintreal;
		case T_LESS: // char, int
			opS = "<";
			goto checkchintreal;
		case T_LE: // char, int
			opS = "<";
			goto checkchintreal;
		case T_IN: // generator
			SDLerror(file, place,ERR_UNSUPPORT, "IN");
			break;
		case T_EQUALS: // used in SDL ADTs
			SDLerror(file, place,ERR_UNSUPPORT, "==");
			break;
		case T_MINUS: // int
			opS = "-";
			if (ldd == timeType && (rdd == timeType ||
				rdd == durType || rdd == realType))
			{
				ldd = rdd = realType;
				dp = durType;
			}
			goto checkintreal;
		case T_CONCAT:
			SDLerror(file, place,ERR_UNSUPPORT, "||");
			break;
		case T_PLUS: // int
			opS = "+";
			if ((ldd == timeType && (rdd == durType || rdd == realType))
			 || (rdd == timeType && (ldd == durType || ldd == realType)))
			{
				ldd = rdd = realType;
				dp = durType;
			}
			goto checkintreal;
		case T_ASTERISK: // int
			opS = "*";
			goto checkintreal;
		case T_SLASH: // int
			opS = "/";
			goto checkintreal;
		case T_MOD:
			opS = "MOD";
			goto checkint;
		case T_REM:
			opS = "REM";
			goto checkint;
		checkbool:
			dp = boolType;
			if (ldd != rdd)
				SDLerror(file, place, ERR_EXPRTYPES, opS);
			else if (ldd != boolType)
				SDLerror(file, place, ERR_BOOLEXPR);
			break;
		checkint:
			dp = intType;
			if (ldd != rdd)
				SDLerror(file, place, ERR_EXPRTYPES, opS);
			else if (ldd != intType)
				SDLerror(file, place, ERR_INTEXPR);
			break;
		checkintreal:
			if (ldd == realType && (rdd == intType || rdd == naturalType))
				dp = rdd = realType;
			else if (rdd == realType && (ldd == intType || ldd == naturalType))
				dp = ldd = realType;
			if (ldd != rdd)
				SDLerror(file, place, ERR_EXPRTYPES, opS);
			else if (ldd != intType && ldd != realType)
				SDLerror(file, place, ERR_INTRLEXPR);
			break;
		checkchintreal:
			if (ldd == durType || ldd == timeType) ldd = realType;
			if (rdd == durType || rdd == timeType) rdd = realType;
			dp = boolType;
			if (ldd != rdd)
				SDLerror(file, place, ERR_EXPRTYPES, opS);
			else if (ldd != intType && ldd != charType &&
				ldd != realType)
				SDLerror(file, place, ERR_INTCHRLEXPR);
			break;
		checkbasic:
			if (ldd == durType || ldd == timeType) ldd = realType;
			if (rdd == durType || rdd == timeType) rdd = realType;
			dp = boolType;
			if (ldd != rdd)
				SDLerror(file, place, ERR_EXPRTYPES, opS);
			else if (ldd != intType && ldd != charType &&
				ldd != boolType && ldd != pidType &&
				ldd != realType && ldd->tag != ENUM_TYP)
					SDLerror(file, place, ERR_BASICTYPE);
			else break;
			ldd = NULL;
			break;
		}
	}
	etype = GetOffset(dp);
}

void assignment_t::Check()
{
	data_def_t *lt, *rt;
	int sz;
	lval.Check(lt);
	expression_t *e = GetExprP(rval);
	e->Check(rt);
	if (rt != lt && !lt->CanCast(rt))
		SDLerror(file, place, ERR_ASSIGNTYPE);
}

void output_arg_t::Check(Bool_t hasPId, lh_list_t<ident_t> &via)
{
	signal_def_t *sd = (signal_def_t *)ScopeStack->dequalify(&signal, Q_SIGNAL);
	if (sd==NULL) 
		SDLerror(file, place, ERR_SIGNALID, Id2Str(signal));
	else
	{
		int cnt = findOutputDest(file, place, sd, via, False);
		if (cnt==0) SDLerror(file, place, ERR_NOPATH);
		else if (cnt>1 && !hasPId) SDLerror(file, place, ERR_AMBIGUOUS);
		CheckArgs(file, place, sd->sortrefs, args);
	}
}

void output_t::Check()
{
	lh_list_node_t<output_arg_t> *o = signals.front();
	while (o)
	{
		o->info.Check(hasDest, via);
		o = o->next();
	}
	if (hasDest)
	{
		data_def_t *ddd;
		GetExprP(dest)->Check(ddd);
		if (ddd != pidType)
			SDLerror(file,place, ERR_DESTTYPE,
				ddd?ddd->nm.name():"unknown");
	}
	lh_list_node_t<ident_t> *sr = via.front();
	lh_list_node_t<ident_t> *sr2;
	for ( ; sr; sr = sr->next())
	{
		route_def_t *r = (route_def_t *)ScopeStack->dequalify(&sr->info, Q_SIGROUTE);
		if (r==NULL) continue;
		// Make sure it is unique
		sr2 = sr->next();
		while (sr2)
		{
			route_def_t *r2 = (route_def_t *)ScopeStack->dequalify(&sr2->info, Q_SIGROUTE);
			if (r2 && r2==r)
				SDLerror(file, place, ERR_VIADUP, Id2Str(sr->info));
			sr2 = sr2->next();
		}
	}
}

void pri_output_t::Check()
{
	SDLerror(file, place, ERR_NOPRIOUT);
}

static void CheckProcessArgs(int file, int place,
	lh_list_t<process_formal_param_t> &fpars,
	lh_list_t<heap_ptr_t> &apars)
{
	int pos = 1;
	lh_list_node_t<process_formal_param_t> *fpt = fpars.front();
	lh_list_node_t<heap_ptr_t> *apt = apars.front();
	while (fpt)
	{
		data_def_t *fdd = (data_def_t *)ScopeStack->dequalify(&fpt->info.sort, Q_TYPE);
		lh_list_node_t<process_param_name_t> *fnm = fpt->info.names.front();
		while (fnm)
		{
			if (apt==NULL)
			{
				SDLerror(file, place, ERR_MISSPARAM);
				return;
			}
			expression_t *e = GetExprP(apt->info);
			if (e)
			{
				data_def_t *add;
				e->Check(add);
				if (add && add != fdd)
					SDLerror(file, place, ERR_APARAMTYPE, (const char *)pos);
			}
			pos++;
			apt = apt->next();
			fnm = fnm->next();
		}
		fpt = fpt->next();
	}
	if (apt) SDLerror(file, place, ERR_EXTRAPARAM);
}

static void CheckProcedureArgs(int file, int place,
	lh_list_t<procedure_formal_param_t> &fpars,
	lh_list_t<heap_ptr_t> &apars)
{
	int pos = 1;
	lh_list_node_t<procedure_formal_param_t> *fpt = fpars.front();
	lh_list_node_t<heap_ptr_t> *apt = apars.front();
	while (fpt)
	{
		data_def_t *fdd = (data_def_t *)ScopeStack->dequalify(&fpt->info.sort, Q_TYPE);
		lh_list_node_t<procedure_param_name_t> *fnm = fpt->info.names.front();
		while (fnm)
		{
			if (apt==NULL)
			{
				SDLerror(file, place, ERR_MISSPARAM);
				return;
			}
			expression_t *e = GetExprP(apt->info);
			if (e)
			{
				data_def_t *add;
				e->Check(add, fpt->info.isValue ? False: True);
				if (add && add != fdd)
					SDLerror(file, place, ERR_APARAMTYPE, (const char *)pos);
			}
			pos++;
			apt = apt->next();
			fnm = fnm->next();
		}
		fpt = fpt->next();
	}
	if (apt) SDLerror(file, place, ERR_EXTRAPARAM);
}

void invoke_node_t::Check()
{
	lh_list_t<ident_t> *fp;
	if (tag==T_CREATE)
	{
		process_def_t *pd = (process_def_t *)ScopeStack->dequalify(&ident, Q_PROCESS);
		if (pd==NULL) 
			SDLerror(file, place, ERR_PROCESSID, Id2Str(ident));
		else 
			CheckProcessArgs(file, place, pd->params, args);
	}
	else
	{
		procedure_def_t *pd = (procedure_def_t *)ScopeStack->dequalify(&ident, Q_PROCEDURE);
		if (pd==NULL) 
			SDLerror(file, place, ERR_PROCEDUREID, Id2Str(ident));
		else
			CheckProcedureArgs(file, place, pd->params, args);
	}
	// Check params
}

void timer_set_t::Check()
{
	timer_def_t *td = (timer_def_t *)ScopeStack->dequalify(&timer, Q_TIMER);
	if (td==NULL) 
		SDLerror(file, place, ERR_TIMERID, Id2Str(timer));
	else
		CheckArgs(file, place, td->paramtypes, args);
	// Check the type of the time expression
	expression_t *e = GetExprP(time);
	data_def_t *ed;
	e->Check(ed, False);
	if (ed != realType && ed != durType && ed != timeType)
		SDLerror(file, place, ERR_EXPTIME);
}

void timer_reset_t::Check()
{
	timer_def_t *td = (timer_def_t *)ScopeStack->dequalify(&timer, Q_TIMER);
	if (td==NULL) 
		SDLerror(file, place, ERR_TIMERID, Id2Str(timer));
	else
		CheckArgs(file, place, td->paramtypes, args);
}

void read_node_t::Check()
{
	data_def_t *dd;
	varref.Check(dd);
	if (dd != intType)
		SDLerror(file, place, ERR_READTYPE);
}

void write_node_t::Check()
{
	lh_list_node_t<heap_ptr_t> *e = exprs.front();
	assert(e);
	while (e)
	{
		data_def_t *dd;
		GetExprP(e->info)->Check(dd);
		if (dd==NULL || dd->Size() != 1)
			SDLerror(file, place, ERR_WRITETYPE);
		e = e->next();
	}
}

int decision_node_t::Check(Bool_t isInitial, Bool_t isProcess)
{
	// Get the type of the question?
	data_def_t *et, *dd;
	expression_t *e = GetExprP(question);
	e->Check(et);
	int top, bottom;
	if (et->tag == ENUM_TYP)
	{
		enum_def_t *ed = GetEnumDefP(et->contents);
		top = ed->nelts;
		bottom = 1;
	}
	else if (et == naturalType) 
	{
		top = UNDEFINED-1;
		bottom = 0;
	}
	else if (et == intType) 
	{
		top = UNDEFINED-1;
		bottom = -UNDEFINED+1;
	}
	else if (et == boolType) 
	{
		top = 1;
		bottom = 0;
	}
	else if (et == pidType) 
	{
		top = UNDEFINED;
		bottom = 0;
	}
	else if (et == charType) 
	{
		top = 255;
		bottom = 0;
	}
	else SDLerror(file, place, ERR_QUESTIONTYP);
	// Check that the answers are mutually exclusive and
	// complete, and if all answers are terminating, the
	// decision is terminating
	lh_list_node_t<subdecision_node_t> *al = answers.front();
	Bool_t complete = False, hasOpNE = False;
	int lower[64], upper[64], plc[64], fil[64], i = 0, rtn = 1;
	range_op_t op[64];
	while (al)
	{
		if (al->info.answer.isEmpty()) // else part
			complete = True;
		else
		{
			lh_list_node_t<range_condition_t> *rc = al->info.answer.front();
			while (rc)
			{
				if (rc->info.op == RO_IN)
				{
					e = GetExprP(rc->info.lower);
					rc->info.lval = e->EvalGround(dd);
					if (dd != et)
						SDLerror(file, place, ERR_ANSWTYPE);
				}
				e = GetExprP(rc->info.upper);
				rc->info.uval = e->EvalGround(dd);
				if (dd != et)
					SDLerror(file, place, ERR_ANSWTYPE);
				plc[i] = place;
				fil[i] = file;
				switch (op[i] = rc->info.op)
				{
				case RO_IN:
					lower[i] = rc->info.lval;
					upper[i] = rc->info.uval;
					break;
				case RO_NEQ:
					hasOpNE = True;
				case RO_NONE:
				case RO_EQU:
					lower[i] = upper[i] = rc->info.uval;
					break;
				case RO_LE:
					lower[i] = -UNDEFINED+1;
					upper[i] = rc->info.uval-1;
					break;
				case RO_LEQ:
					lower[i] = -UNDEFINED+1;
					upper[i] = rc->info.uval;
					break;
				case RO_GT:
					lower[i] = rc->info.uval+1;
					upper[i] = UNDEFINED-1;
					break;
				case RO_GTQ:
					lower[i] = rc->info.uval;
					upper[i] = UNDEFINED-1;
					break;
				}
				if (++i == 64)
				{
					SDLerror(file, place, ERR_MAXRANGES);
					break;
				}
				rc = rc->next();
			}
		}
		if (!al->info.transition.Check(isInitial, False, isProcess))
			rtn = 0;
		al = al->next();
	}
	// Check the accumulated range conditions for conflicts
	// Because the answers must be mutually exclusive, we assume
	// that at most one RO_NE op can occur, and in this case only
	// one other condition is allowed which must be RO_NONE or
	// RO_EQ. Then we jump to the end. This means we don't have
	// to worry about RO_NE in the more complex computation below.

	if (hasOpNE)
	{
		if (i != 2)
			SDLerror(file, place, ERR_RANGENE);
		else
		{
			int p = (op[0]==RO_NEQ);
			if (op[p]!=RO_NONE && op[p] != RO_EQU)
				SDLerror(file, place, ERR_RANGENE);
			else if (upper[0] != upper[1])
				SDLerror(file, place, ERR_RANGENE);
		}
		goto OK;
	}

	int j, k;
	for (j=0;j<(i-1);j++)
	{
		for (k=j+1;k<i;k++)
 		{
			if ((lower[j] >= lower[k] && lower[j]<= upper[k]) ||
			    (lower[k] >= lower[j] && lower[k]<= upper[j]))
			     	SDLerror(fil[j], plc[j], ERR_RANGECROSS, (char *)plc[k]);
		}
	}
	// Check the accumulated range conditions for omissions
	// There must be a more efficient way than the dumb iterative
	// aproach here!
	if (!complete)
	{
		Bool_t Change = True;
		while (Change)
		{
			Change = False;
			for (j=0;j<(i-1);j++)
			{
				for (k=j+1;k<i;k++)
				{
					if (upper[k]>=(lower[j]-1) && lower[k] < lower[j])
					{
						lower[j] = lower[k];
						Change = True;
					}
					if (upper[k]>upper[j] && lower[k] <= (upper[j]+1))
					{
						upper[j] = upper[k];
						Change = True;
					}
				}
				if (lower[j]==bottom && upper[j]==top)
					goto OK;
			}
		}
		SDLerror(file, place,ERR_RANGEHOLE);
	}
OK:
	return rtn;
}

int gnode_t::Check(Bool_t isInitial, Bool_t isProcess)
{
	int rtn = 0;
	void *p = GetVoidP(node);
	switch (type)
	{
	case N_TASK:
		CheckL(*((lh_list_t<assignment_t> *)p));
		break;
	case N_OUTPUT:
		((output_t *)p)->Check();
		break;
//	case N_PRI_OUTPUT:
//		((pri_output_t *)p)->Check();
//		break;
	case N_CREATE:
		((invoke_node_t *)p)->Check();
		break;
	case N_CALL:
		((invoke_node_t *)p)->Check();
		break;
	case N_SET:
		CheckL(*((lh_list_t<timer_set_t> *)p));
		break;
	case N_RESET:
		CheckL(*((lh_list_t<timer_reset_t> *)p));
		break;
//	case N_EXPORT:
	case N_DECISION:
		// check that answers are mutually exclusive. If
		// the decision is terminating, report via return value
		// of Check
		rtn = ((decision_node_t *)p)->Check(isInitial, isProcess);
		break;
	case N_READ:
		((read_node_t *)p)->Check();
		break;
	case N_WRITE:
		((write_node_t *)p)->Check();
		break;
	}
	return rtn;
}

static int CheckNodeList(lh_list_t<gnode_t> &l, Bool_t isInitial, Bool_t isProcess)
{
	lh_list_node_t<gnode_t> *p = l.front();
	int rtn = 0;
	while (p)
	{
		if (rtn) 
		{
			SDLerror(p->info.file, p->info.place, ERR_UNREACH);
			break;
		}
		rtn = p->info.Check(isInitial, isProcess);
		p = p->next();
	}
	return rtn;
}

int transition_t::Check(Bool_t isInitial, Bool_t mustTerminate, Bool_t isProcess)
{
	int term = CheckNodeList(nodes, isInitial, isProcess);
	// if term, make sure there is no terminator; else
	// if mustTerminate, make sure there is one.
	// if (isInitial) then cannot have - nextstate
	if (term)
	{
		if (type!=EMPTY) SDLerror(file, place, ERR_UNREACH);
		return 1;
	}
	// Haven't terminated yet, so handle the terminator, if any
	if (type!=EMPTY)
	{
		if (isProcess && type == RETURN)
			SDLerror(file, place, ERR_BADRETURN);
		else if (!isProcess && type == STOP)
			SDLerror(file, place, ERR_BADSTOP);
		else if (type == NEXTSTATE)
		{
			if (next.name()[0] == '\0')
			{
				if (isInitial) SDLerror(file, place, ERR_BADNEXT);
			}
			else if (ScopeStack->findState(next)<0)
				SDLerror(file, place, ERR_NEXTSTATE, next.name());
		}
		else if (type==JOIN)
		{
			if (!ScopeStack->findLabel(next)<0)
				SDLerror(file, place, ERR_JOINLBL, next.name());
			return 0;
		}
		return 1;
	}
	// still no termination...
	if (mustTerminate)
		SDLerror(file, place, ERR_NOTERM);
	return 0;
}

void state_node_t::Check(Bool_t isProcess, Bool_t &hasNonAstrsk)
{
	if (!isAsterisk) hasNonAstrsk = True;
	nameStore->ClearNameMarks();
	MarkL(states);
	lh_list_node_t<input_part_t> *il = inputs.front();
	signalset_t isigset;
	while (il)
	{
		// check that input signals are distinct
		signalset_t stimset;
		if (il->info.isAsterisk) stimset.set();
		else stimset.clear();
		lh_list_node_t<stimulus_t> *stim = il->info.stimuli.front();
		for ( ; stim ; stim = stim->next())
		{
			int idx = -1;
			signal_def_t *sig = (signal_def_t *)ScopeStack->dequalify(&stim->info.signal, Q_SIGNAL);
			if (sig) idx = sig->idx;
			else
			{
				timer_def_t *t = (timer_def_t *)ScopeStack->dequalify(&stim->info.signal, Q_TIMER);
				if (t) idx = t->idx;
				else SDLerror(stim->info.file, stim->info.place, ERR_NOSIGNAL, Id2Str(stim->info.signal));
			}
			if (idx>=0)
			{
				if (il->info.isAsterisk)
					stimset.remove(idx);
				else
					stimset.add(idx);
			}
		}
		if (!isigset.disjoint(stimset))
			SDLerror(il->info.file, il->info.place, ERR_DUPINPUT);
		isigset.add(stimset);

		// Check that the enabling condition is boolean
		if (il->info.enabler)
		{
			data_def_t *pct;
			GetExprP(il->info.enabler)->Check(pct);
			if (pct != boolType)
				SDLerror(il->info.file, il->info.place, ERR_BOOLEXPR);
		}
		// Check the transition
		il->info.transition.Check(False, True, isProcess);
		il = il->next();
	}
	lh_list_node_t<continuous_signal_t> *cs = csigs.front();
	// check that priorities are present if
	// more than one csig, and that they are distinct,
	Bool_t hasPri = True;
	int psigCnt = 0;
	u_s_code_word_t pset[MAX_PRIORITIES/SWORD_SIZE];
	for (int i = 0; i < (MAX_PRIORITIES/SWORD_SIZE); i++)
		pset[i] = 0;
	for ( ; cs ; cs = cs->next())
	{
		s_code_word_t p = cs->info.priority;
		if (p != UNDEFINED)
		{
			if (p<0 || p>=MAX_PRIORITIES)
				SDLerror(cs->info.file, cs->info.place, ERR_PRIRANGE, (const char *)p);
			else if ((pset[p/SWORD_SIZE] & (1<<(p%SWORD_SIZE))) != 0)
				SDLerror(cs->info.file, cs->info.place, ERR_DUPPRI, (const char *)p);
			else pset[p/SWORD_SIZE] |= 1<<(p%SWORD_SIZE);
		}
		else hasPri = False;
		psigCnt++;
		// check that the expression is boolean.
		data_def_t *pct;
		GetExprP(cs->info.condition)->Check(pct);
		if (pct != boolType)
			SDLerror(cs->info.file, cs->info.place, ERR_BOOLEXPR);
		// check the transition
		cs->info.transition.Check(False, True, isProcess);
	}
	if (psigCnt>1 && !hasPri)
		SDLerror(file, place, ERR_NOPRIORITY);
}

void procedure_formal_param_t::Check()
{
	// Check that the type ID is a sort type, and give each name
	// node a reference offset to the type definition
	data_def_t *dd = (data_def_t *)ScopeStack->dequalify(&sort, Q_TYPE);
	if (dd==NULL) SDLerror(file, place, ERR_PARAMTYPE, Id2Str(sort));
	else 
	{
		heap_ptr_t off = GetOffset(dd);
		lh_list_node_t<procedure_param_name_t> *pn = names.front();
		while (pn)
		{
			pn->info.type = off;
			pn = pn->next();
		}
	}
}

void procedure_def_t::Check()
{
	lh_list_t<procedure_def_t> *pl = 
		(lh_list_t<procedure_def_t> *)GetVoidP(procedures);
	ScopeStack->EnterScope(Q_PROCEDURE,this);

	// Check that all remote references have been resolved

	if (procedures) CheckRemoteL(*pl);

	// Check that all defining names in scopeunit are unique

	nameStore->ClearNameMarks();
	MarkL(params);
	MarkL(types);
	MarkEnumElts(types);
	MarkL(variables);
	if (procedures) MarkL(*pl);

	// Check that no variables are exported or revealed

	lh_list_node_t<variable_def_t> *p = variables.front();
	while (p)
	{
		if (p->info.isExported)
			SDLerror(p->info.file, p->info.place, ERR_NOXPORT);
		if (p->info.isRevealed)
			SDLerror(p->info.file, p->info.place, ERR_NOREVEAL);
		p = p->next();
	}

	// Check subnodes

	CheckL(params);
	CheckL(variables);
	if (procedures) CheckL(*pl);

	start.Check(True, True, False);
	lh_list_node_t<state_node_t> *sn = states.front();
	Bool_t hasNonAstrsk = False;
	while (sn)
	{
		sn->info.Check(False, hasNonAstrsk);
		sn = sn->next();
	}
	if (!hasNonAstrsk) SDLwarning(file, place, ERR_NONONASTRSK, name());
	ScopeStack->ExitScope();
}

void process_formal_param_t::Check()
{
	// Check that the type ID is a sort type, and give each name
	// node a reference offset to the type definition
	data_def_t *dd = (data_def_t *)ScopeStack->dequalify(&sort, Q_TYPE);
	if (dd==NULL) SDLerror(file, place, ERR_PARAMTYPE, Id2Str(sort));
	else 
	{
		heap_ptr_t off = GetOffset(dd);
		lh_list_node_t<process_param_name_t> *pn = names.front();
		while (pn)
		{
			pn->info.type = off;
			pn = pn->next();
		}
	}
}

// Recursively search a signal list for timers and report them

static void ReportTimers(signallist_t &sl)
{
	lh_list_node_t<ident_t> *p = sl.signals.front();
	while (p)
	{
		if (ScopeStack->dequalify(&p->info, Q_TIMER))
			SDLerror(p->info.file, p->info.place, ERR_INTIMER);
		else if (ScopeStack->dequalify(&p->info, Q_SIGNAL)==NULL)
			SDLerror(p->info.file, p->info.place, ERR_BADSIGNAL, Id2Str(p->info));
		p = p->next();
	}
	p = sl.signallists.front();
	while (p)
	{
		void *rtn = ScopeStack->dequalify(&p->info, Q_SIGLIST);
		if (rtn==NULL)
			SDLerror(p->info.file, p->info.place, ERR_NOSIGLIST, Id2Str(p->info));
		else ReportTimers( ((siglist_def_t *)rtn)->s );
		p = p->next();
	}
}

void process_def_t::Check(Bool_t mustHaveVSig)
{
	ScopeStack->EnterScope(Q_PROCESS,this);

	// Check that all remote references have been resolved

	CheckRemoteL(procedures);

	// Check that all defining names in scopeunit are unique

	nameStore->ClearNameMarks();
	MarkL(params);
	MarkL(types);
	MarkEnumElts(types);
	MarkL(signals);
	MarkL(timers);
	MarkL(siglists);
	MarkL(imports);
	MarkL(variables);
	MarkL(procedures);

	// Check number of instances

	if (initval!=UNDEFINED && maxval!=UNDEFINED)
		if (initval<0)
			SDLerror(file, place, ERR_INITNEG);
		else if (initval>maxval)
			SDLerror(file, place, ERR_MOREMAX);
		else if (maxval<1)
			SDLerror(file, place, ERR_MAXPOS);

	// Check the valid input signal set. This cannot have timer IDs.
	// The complete valid input signal set is the union
	// of those specified here, the set of signals in all
	// signalroutes leading to the process, the implicit 
	// signals (I think that means locally defined signals)
	// and the timer signals.

	if (mustHaveVSig && validsig.signals.isEmpty() &&
	    validsig.signallists.isEmpty())
		SDLwarning(file, place, ERR_NOVSIG, name());
	else ReportTimers(validsig);

	// Check subnodes

	CheckL(types);
	CheckL(signals);
	CheckL(siglists);
	CheckL(params);
	CheckL(variables);
	CheckL(views);
	CheckL(procedures);

	start.Check(True, True, True);
	lh_list_node_t<state_node_t> *sn = states.front();
	Bool_t hasNonAstrsk = False;
	while (sn)
	{
		sn->info.Check(True, hasNonAstrsk);
		sn = sn->next();
	}
	if (!hasNonAstrsk) SDLwarning(file, place, ERR_NONONASTRSK, name());
	ScopeStack->ExitScope();
}

void channel_def_t::Check()
{
	checkPath(paths, Q_BLOCK, endpt);
}

void subblock_def_t::Check()
{
	ScopeStack->EnterScope(Q_SUBSTRUCTURE,this);

	// Check that all remote references have been resolved??

	// Check that all defining names in scopeunit are unique

	nameStore->ClearNameMarks();
	MarkL(types);
	MarkEnumElts(types);
	MarkL(signals);
	MarkL(siglists);
	MarkL(channels);

	// Check subnodes

	if (blocks==0)
		SDLerror(file, place, ERR_MISSSUBLOCK, name());

	ScopeStack->ExitScope();
}

void block_def_t::Check()
{
	ScopeStack->EnterScope(Q_BLOCK,this);

	// Check that all remote references have been resolved

	CheckRemoteL(processes);
	// substruct??

	// Check that all defining names in scopeunit are unique

	nameStore->ClearNameMarks();
	MarkL(types);
	MarkEnumElts(types);
	MarkL(signals);
	MarkL(siglists);
	MarkL(routes);
	MarkL(processes);

	// Number the processes for use by sdlgr...

	int pn = 1;
	for (lh_list_node_t<process_def_t> *p = processes.front(); p; p=p->next())
		p->info.num = pn++;

	// Check subnodes

	CheckL(types);
	CheckL(signals);
	CheckL(siglists);
	CheckL(routes);

	if (processes.isEmpty() || routes.isEmpty())
		SDLwarning(file, place, ERR_MISSDEFS, name());

	// If no routes then all processes must have valid input
	// signal sets.

 	Bool_t mustHaveVSig = (routes.isEmpty()) ? True : False;
	for (p = processes.front(); p; p = p->next())
		p->info.Check(mustHaveVSig);

	ScopeStack->ExitScope();
}

void system_def_t::Check()
{
	ScopeStack->EnterScope(Q_SYSTEM,this);

	// Check that all remote references have been resolved

	CheckRemoteL(blocks);

	// Check that all defining names in scopeunit are unique

	nameStore->ClearNameMarks();
	MarkL(types);
	MarkEnumElts(types);
	MarkL(signals);
	MarkL(siglists);
	MarkL(channels);
	MarkL(blocks);

	// Number the blocks for use by sdlgr...

	int bn = 1;
	for (lh_list_node_t<block_def_t> *b = blocks.front(); b; b=b->next())
		b->info.num = bn++;

	// Check subnodes

	CheckL(types);
	CheckL(signals);
	CheckL(siglists);
	CheckL(channels);
	if (blocks.isEmpty())
		SDLerror(file, place, ERR_MISSBLOCK, name());	
	else
		CheckL(blocks);
	ScopeStack->ExitScope();
	// Check the route/channel/process tables
	// The signals conveyed on routes connected to a channel
	// must be the same as those conveyed by the channel in
	// each direction.
	// Every channel with a block as an endpoint must be mentioned in
	// one or more connects in the block

	for (int i=0;i<Router->NumChans();i++)
	{
		Bool_t gotConn = False;
		signalset_t ss;
		for (int j=0; j<Router->NumRoutes(); j++)
		{
			if (Router->RouteBlock(j) == Router->ChanSource(i)
			   && Router->RouteChan(j) == i)
			{
				ss.add(Router->RouteSigsP(j)->sigset);
				gotConn = True;
			}
		}
		if (!gotConn)
			SDLerror(Router->ChanDefP(i)->file,
				Router->ChanDefP(i)->place,
				ERR_UNCONNCH, Router->ChanDefP(i)->name());
		else if (ss != Router->ChanSigsP(i)->sigset)
			SDLerror(Router->ChanDefP(i)->file,
				Router->ChanDefP(i)->place,
				ERR_SIGMISMATCH, Router->ChanDefP(i)->name());
	}
}

