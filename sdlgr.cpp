/*
 * SDL AST to SDL/GR convertor
 *
 * Written by Graham Wheeler, September 1994
 * Data Network Architecture Laboratory
 * Department of Computer Science
 * University of Cape Town
 * 
 * (c) 1994 Graham Wheeler and the Data Network Architectures Laboratory
 * All Rights Reserved
 *
 * Created 20-9-94
 * Last modified: 20-9-94
 *
 */

#pragma hdrfile "GR.SYM"
#include "sdlast.h"
#include <math.h>
#include <dos.h>
#include <graphics.h>
#include <conio.h>
#include "svga256.h"

#define max(a,b)		( ((a)>(b)) ? (a) : (b) )
#define min(a,b)		( ((a)>(b)) ? (b) : (a) )

// size of FSM nodes

#define NWIDTH		100
#define NHEIGHT		50

int gmode; // grafix mode
int nextx; // hackish thing used for decisions

// Variables used for graph layout on grid

#define MAX_BLOCKS	16

int badj[MAX_BLOCKS][MAX_BLOCKS]; // adjacency matrix
int nadj[MAX_BLOCKS], nb = 0; // number of adjacencies and nodes
int pos[MAX_BLOCKS][2];	// x,y coords of blocks
int layout[MAX_BLOCKS][MAX_BLOCKS]; // grid layout
int objid[MAX_BLOCKS];
int th;

// stub needed by sdlast

int getLineNumber()
{
	return 0;
}

// BGI functions are wrapped in macros for later porting

#define TextWidth(s)		textwidth(s)
#define TextHeight(s)		textheight(s)
#define Clear()			cleardevice(); setcolor(WHITE)
#define DrawBox(l,t,r,b)	rectangle(l,t,r,b)
#define DrawBlock(l,t,r,b)	rectangle(l,t,r,b)
#define DrawTask(l,t,r,b)	rectangle(l,t,r,b)
#define DrawLine(x1,y1,x2,y2)	line(x1,y1,x2,y2)
#define DrawText(x,y,s)		outtextxy(x,y,s)

void DrawStart(int l, int t, int r, int b)
{
	int rd = (b-t+1)/2; // arc radius
	line(l+rd, t, r-rd, t);
	line(l+rd, b, r-rd, b);
	arc(l+rd, t+rd, 90, 270, rd);
	arc(r-rd, t+rd, 270, 90, rd);
}

void DrawState(int l, int t, int r, int b)
{
	line(l, t, r, t);
	line(l, b, r, b);
	int dx = NWIDTH/2;
	int dy = NHEIGHT/2;
	int rd = (int)sqrt( (float)(dx*dx+dy*dy) ); // arc radius
	int theta = (int)(atan(((float)dy) / ((float)dx))*180./3.1415927);
	arc(l+dx, t+dy, 180-theta, 180+theta, rd);
	arc(r-dx, t+dy, 360-theta, theta, rd);
}

void DrawStop(int l, int t, int r, int b)
{
	line(l,t,r,b);
	line(l,b,r,t);
	// fix connector
	line(l + (r-l+1)/2 , t + (b-t+1)/2, l + (r-l+1)/2 , t);
}

void DrawJoin(int l, int t, int r, int b)
{
	line(l,t,r,t);
	line(l,b,r,b);
	line(l,t,l,b);
}

void DrawReturn(int l, int t, int r, int b)
{
	line(l,t,r,b);
	line(l,b,r,t);
	circle(l+(r-l+1)/2, t + (b-t+1)/2, (b-t+1)/2);
}

void DrawDecision(int l, int t, int r, int b)
{
	int dx = (r-l+1)/2;
	int dy = (b-t+1)/2;
	line(l, t+dy, l+dx, t);
	line(l, b-dy, l+dx, b);
	line(r, t+dy, r-dx, t);
	line(r, b-dy, r-dx, b);
}

void DrawProcess(int l, int t, int r, int b)
{
	line(l+15, t   , r-15, t   );	//  A    --------
	line(l   , t+15, l   , b-15);	//  B   |
	line(r   , t+15, r   , b-15);	//  C            |
	line(l+15, b   , r-15, b   );	//  D    --------
	line(l   , t+15, l+15, t   );	// connect B to A
	line(l   , b-15, l+15, b   );	// connect B to D
	line(r   , t+15, r-15, t   );	// connect C to A
	line(r   , b-15, r-15, b   );	// connect C to D
}

void DrawInput(int l, int t, int r, int b)
{
	int h = b-t+1;
	line(l, t, r, t);	//  A    --------
	line(l, t, l, b);	//  B   |
	line(l, b, r, b);	//  C    --------
	line(r, t, r-h/2, t+h/2);	// connect A
	line(r, b, r-h/2, b-h/2);	// connect C
}

void DrawOutput(int l, int t, int r, int b)
{
	int h = b-t+1;
	line(l, t, r-h/2, t);	//  A    --------
	line(l, t, l, b);	//  B   |
	line(l, b, r-h/2, b);	//  C    --------
	line(r-h/2, t, r, t+h/2);	// connect A
	line(r-h/2, b, r, b-h/2);	// connect C
}

void DrawCSignal(int l, int t, int r, int b)
{
	int dy = (b-t+1)/2;
	int dx = dy / 2;
	line(l, t+dy, l+dx, t);
	line(l, t+dy, l+dx, b);
	line(r, t+dy, r-dx, t);
	line(r, t+dy, r-dx, b);
}

void DrawCall(int l, int t, int r, int b)
{
	DrawBox(l,t,r,b);
	DrawBox(l+3,t,r-3,b);
}

void DrawCreate(int l, int t, int r, int b)
{
	DrawBox(l,t,r,b);
	DrawBox(l,t+3,r,b-3);
}

void DrawTimerSet(int l, int t, int r, int b)
{
	DrawTask(l,t,r,b);
}

void DrawTimerReset(int l, int t, int r, int b)
{
	DrawTask(l,t,r,b);
}

void DrawReadNode(int l, int t, int r, int b)
{
	DrawTask(l,t,r,b);
}

void DrawWriteNode(int l, int t, int r, int b)
{
	DrawTask(l,t,r,b);
}

void DrawDeclBox(int swidth, int decheight)
{
	DrawLine(10, th+5, swidth-50, th+5);
	DrawLine(swidth-50, th+5, swidth-10, th+40);
	DrawLine(swidth-50, th+5, swidth-50, th+40);
	DrawLine(swidth-50, th+40, swidth-10, th+40);
	DrawLine(swidth-10, th+40, swidth-10, decheight+2*th);
	DrawLine(10, th+5, 10, decheight+2*th);
	DrawLine(10, decheight+2*th, swidth-10, decheight+2*th);
}

void Blit(int &x, int &y, char *txt, int &width, int draw)
{
	if (draw) DrawText(x, y, txt);
	else
	{
		int w = TextWidth(txt);
		if (w>width) width = w;
	}
	y += TextHeight(txt)+5;
}

//------------------------------------------------------------------------

char *Print(expression_t *e, char *buf);

char *Print(int_literal_t *u, char *buf)
{
	char b[20];
	sprintf(b, "%ld", (long)u->value);
	strcat(buf, b);
	return buf;
}

char *Print(real_literal_t *u, char *buf)
{
	char b[32];
	sprintf(b, "%.2f", *((s_code_real_t *)&u->value));
	strcat(buf, b);
	return buf;
}

char *Print(cond_expr_t *ce, char *buf)
{
	expression_t *ip = GetExprP(ce->if_part);
	expression_t *tp = GetExprP(ce->then_part);
	expression_t *ep = GetExprP(ce->else_part);
	strcat(buf, "IF ");
	Print(ip, buf);
	strcat(buf, " THEN ");
	Print(tp, buf);
	if (ep)
	{
		strcat(buf, " ELSE ");
		Print(ep, buf);
	}
	return buf;
}

char *Print(timer_active_expr_t *u, char *buf)
{
	strcat(buf, "ACTIVE ");
	strcat(buf, Id2Str(u->timer));
	// must still do args
	return buf;
}

char *Print(view_expr_t *u, char *buf)
{
	strcat(buf, "VIEW ");
	strcat(buf, Id2Str(u->var));
	if (u->pid_expr)
	{
		strcat(buf, ", ");
		expression_t *e = GetExprP(u->pid_expr);
		Print(e, buf);
	}
	return buf;
}

char *Print(var_ref_t *v, char *buf)
{
	ident_t *id = GetIdentP(v->id);
	strcat(buf, Id2Str(*id));
	lh_list_node_t<selector_t> *sel = v->sel.front();
	while (sel)
	{
		switch(sel->info.type)
		{
		case SEL_FIELD:
			strcat(buf, "!");
			strcat(buf, GetNameP(sel->info.val)->name());
			break;
		case SEL_ELT:
			expression_t *e = GetExprP(sel->info.val);
			strcat(buf, "(");
			Print(e, buf);
			strcat(buf, ")");
			break;
		}
		sel = sel->next();
	}
	return buf;
}

char *Print(unop_t *u, char *buf)
{
	void *p = GetVoidP(u->prim);
	if (u->op == T_MINUS)
		strcat(buf, "-");
	else if (u->op == T_NOT)
		strcat(buf, "NOT ");
	switch (u->type)
	{
	case PE_EMPTY:
		break;
	case PE_SLITERAL:
		assert(0);
	case PE_NLITERAL:
	case PE_ILITERAL:
		int_literal_t *i = (int_literal_t *)p;
		Print(i, buf);
		break;
	case PE_RLITERAL:
		real_literal_t *r = (real_literal_t *)p;
		Print(r, buf);
		break;
	case PE_BLITERAL:
		if (u->prim)
			strcat(buf, "True");
		else
			strcat(buf, "False");
		break;
	case PE_EXPR:
		expression_t *e = (expression_t *)p;
		strcat(buf, "(");
		Print(e, buf);
		strcat(buf, ")");
		break;
	case PE_FIX:
		e = (expression_t *)p;
		strcat(buf, "FIX(");
		Print(e, buf);
		strcat(buf, ")");
		break;
	case PE_FLOAT:
		e = (expression_t *)p;
		strcat(buf, "FLOAT(");
		Print(e, buf);
		strcat(buf, ")");
		break;
	case PE_CONDEXPR:
		cond_expr_t *ce = (cond_expr_t *)p;
		Print(ce, buf);
		break;
	case PE_NOW:
		strcat(buf, "NOW");
		break;
	case PE_SELF:	
		strcat(buf, "SELF");
		break;
	case PE_PARENT:
		strcat(buf, "PARENT");
		break;
	case PE_OFFSPRING:
		strcat(buf, "OFFSPRING");
		break;
	case PE_SENDER:
		strcat(buf, "SENDER");
		break;
	case PE_TIMERACTIV:
		timer_active_expr_t *ta = (timer_active_expr_t *)p;
		Print(ta, buf);
		break;
	case PE_IMPORT:
		strcat(buf, "IMPORT (");
		goto view;
	case PE_VIEW:
		strcat(buf, "VIEW (");
	view:
		view_expr_t *ie = (view_expr_t *)p;
		Print(ie, buf);
		strcat(buf, " )");
		break;
	case PE_VARREF:
		var_ref_t *vr = (var_ref_t *)p;
		Print(vr, buf);
		break;
	case PE_ENUMREF:
		enum_ref_t *er = (enum_ref_t *)p;
		strcat(buf, nameStore->name(er->index)); 
		break;
	case PE_SYNONYM:
		synonym_ref_t *sr = (synonym_ref_t *)p;
		strcat(buf, nameStore->name(sr->index)); 
		break;
	}
	return buf;
}

char *Print(expression_t *e, char *buf)
{
	if (e->op == T_UNDEF) // primary?
	{
		unop_t *u = GetUnOpP(e->l_op);
		Print(u, buf);
	}
	else
	{
		expression_t *l = GetExprP(e->l_op);
		expression_t *r = GetExprP(e->r_op);
		Print(l, buf);
		switch(e->op)
		{
		case T_BIMP:
			strcat(buf, " => ");
			break;
		case T_OR:
			strcat(buf, " or ");
			break;
		case T_XOR:
			strcat(buf, " xor ");
			break;
		case T_AND:
			strcat(buf, " and ");
			break;
		case T_NOT:
			assert(0);
		case T_EQ:
			strcat(buf, " = ");
			break;
		case T_NE:
			strcat(buf, " /= ");
			break;
		case T_GT:
			strcat(buf, " > ");
			break;
		case T_GE:
			strcat(buf, " >= ");
			break;
		case T_LESS:
			strcat(buf, " < ");
			break;
		case T_LE:
			strcat(buf, " <= ");
			break;
		case T_IN:
			strcat(buf, "=>");
			break;
		case T_EQUALS:
			strcat(buf, " == ");
			break;
		case T_MINUS:
			strcat(buf, "-");
			break;
		case T_CONCAT:
			strcat(buf, "//");
				break;
		case T_PLUS:
			strcat(buf, "+");
			break;
		case T_ASTERISK:
			strcat(buf, "*");
			break;
		case T_SLASH:
			strcat(buf, "/");
			break;
		case T_MOD:
			strcat(buf, " mod ");
			break;
		case T_REM:
			strcat(buf, " rem ");
			break;
		}
		Print(r, buf);
	}
	return buf;
}

//------------------------------------------------------------------------

void Display(int x, int &y, data_def_t &d, int &width)
{
	char buf[160];
	if (&d == intType || &d == boolType || &d == charType ||
		&d == pidType || &d == durType || &d == timeType ||
		&d == realType || &d == naturalType)
			return;
	if (d.tag==SYNONYM_TYP)
	{
		synonym_def_t *sd = GetSynonymDefP(d.contents);
		sprintf(buf, "SYNONYM %s %s = ", d.name(), Id2Str(sd->sort));
		switch (sd->type)
		{
		case BOOLEAN_TYP:
			strcat(buf, (sd->value ? "True" : "False"));
			break;
		case NATURAL_TYP:
		case INTEGER_TYP:
			sprintf(buf+strlen(buf), "%ld", (long) sd->value);
			break;
		case REAL_TYP:
			sprintf(buf+strlen(buf), "%.3f", *((float *)&sd->value));
			break;
		}
		strcat(buf,";");
		Blit(x, y, buf, width, width);
	}
	else
	{
		int draw = (width != 0);
		sprintf(buf, "NEWTYPE %s", d.name());
		if (d.tag == STRUCT_TYP) strcat(buf, " STRUCT");
		Blit(x, y, buf, width, draw);
		switch (d.tag)
		{
		case ARRAY_TYP:
			array_def_t *a = GetArrDefP(d.contents);
			strcpy(buf, "    ARRAY (");
			Print(GetExprP(a->dimension), buf);
			strcat(buf, ") OF ");
			strcat(buf, Id2Str(a->sort));
			Blit(x, y, buf, width, draw);
			break;
		case STRUCT_TYP:
			for (lh_list_node_t<fieldgrp_t> *f =
				GetStrucDefP(d.contents)->fieldgrps.front();
				f; f = f->next())
			{
				strcpy(buf, "    ");
				for (lh_list_node_t<field_name_t> *n =
					f->info.names.front(); n; )
				{
					strcat(buf, n->info.name());
					n = n->next();
					if (n) strcat(buf, ",");
				}
				strcat(buf," ");
				strcat(buf, Id2Str(f->info.sort));
				strcat(buf,";");
				Blit(x, y, buf, width, draw);
			}
			break;
		case ENUM_TYP:
			strcpy(buf, "    LITERALS ");
			for (lh_list_node_t<enum_elt_t> *ee =
				GetEnumDefP(d.contents)->elts.front(); ee; )
			{
				strcat(buf, ee->info.name());
				ee = ee->next();
				if (ee) strcat(buf, ", ");
			}
			strcat(buf, ";");
			Blit(x, y, buf, width, draw);
			break;
		}
		sprintf(buf, "ENDNEWTYPE %s", d.name());
		Blit(x, y, buf, width, draw);
	}
}

void Display(int x, int &y, signal_def_t &sd, int &width)
{
	char buf[160];
	sprintf(buf, "SIGNAL %s", sd.name());
	if (!sd.sortrefs.isEmpty())
	{
		strcat(buf,"(");
		for (lh_list_node_t<ident_t> *sr = sd.sortrefs.front();
			sr;)
		{
			strcat(buf, Id2Str(sr->info));
			sr = sr->next();
			if (sr)	strcat(buf,", ");
		}
		strcat(buf,")");
	}
	strcat(buf,";");
	if (width) DrawText(x, y, buf);
	else width = TextWidth(buf);
	y += TextHeight("X")+5;
}

void Display(int x, int &y, siglist_def_t &sl, int &width)
{
	char buf[160];
	int gotsl = 0;
	sprintf(buf, "SIGNALLIST %s = ", sl.name());
	for (lh_list_node_t<ident_t> *sli = sl.s.signallists.front(); sli; )
	{
		gotsl = 1;
		strcat(buf,"(");
		strcat(buf,Id2Str(sli->info));
		strcat(buf,")");
		sli = sli->next();
		if (sli) strcat(buf,", ");
	}
	for (sli = sl.s.signals.front(); sli; )
	{
		if (gotsl)
		{
			gotsl = 0;
			strcat(buf,", ");
		}
		strcat(buf,Id2Str(sli->info));
		sli = sli->next();
		if (sli) strcat(buf,", ");
	}
	strcat(buf,";");
	if (width) DrawText(x, y, buf);
	else width = TextWidth(buf);
	y += TextHeight("X")+5;
}

void Display(int x, int &y, variable_def_t &vd, int &width)
{
	char buf[160];
	int w = 0;
	lh_list_node_t<variable_decl_t> *vdc = vd.decls.front();
	while (vdc)
	{
		sprintf(buf, "DCL%s%s ",
			(vd.isExported)?" EXPORTED":"",
			(vd.isRevealed)?" REVEALED":"");
		lh_list_node_t<variable_name_t> *vn = vdc->info.names.front();
		while (vn)
		{
			strcat(buf, vn->info.name());
			vn = vn->next();
			if (vn) strcat(buf, ", ");
		}
		strcat(buf, " ");
		strcat(buf, Id2Str(vdc->info.sort));
		if (vdc->info.value)
		{
			strcat(buf, "= ");
			Print(GetExprP(vdc->info.value), buf);
		}
		strcat(buf,";");
		if (width) DrawText(x, y, buf);
		else if (w < TextWidth(buf));
			w = TextWidth(buf);
		y += TextHeight("X")+5;
		vdc = vdc->next();
	}
	if (width==0) width = w;
}

template<class T>
void GetSize(lh_list_node_t<T> *n, int &height, int &width)
{
	while (n)
	{
		int w = 0;
		Display(0, height, n->info, w);
		if (w > width) width = w;
		n = n->next();
	}
}

template<class T>
void Display(int &x, int &y, lh_list_node_t<T> *n)
{
	int w = 1;
	while (n)
	{
		Display(x, y, n->info, w);
		n = n->next();
	}
}

int SolveAdjacencies(int nb, int placed, int used[], int badj[][MAX_BLOCKS],
	int layout[][MAX_BLOCKS], int pos[][2])
{
	int lim = nb/2 + 1;
	int pass = 1, done = 0;
	placed++;
again:
	for (int i = 1; i <= nb; i++)
	{
		int by, ty, bx, tx, j;
		if (used[i]) continue;
		// place pieces that are constrained by those already
		// placed first...
		if (pass == 2)
		{
			by = bx = j = 1;
			ty = tx = lim;
		}
		else
		{
			for (j = 1; j <= nb; j++)
				if (used[j] && badj[i][j])
					break;
			if (j > nb)
				continue;
			by = pos[j][1]-1;
			ty = by+2;
			if (by<1) by=1;
			if (ty>lim) ty=lim;
			bx = pos[j][0]-1;
			tx = bx+2;
			if (bx<1) bx=1;
			if (tx>lim) tx=lim;
		}
		for (int py = by; py <= ty; py++)
		{
			for (int px = bx; px <= tx; px++)
			{
				if (layout[px][py] && layout[px][py]!=objid[i])
					continue; // busy
				// can placing it here satisfy constraints?
				// first check if at edge for ENV
				if (badj[i][0] && (py<lim && px>0 && px<lim))
					continue;
				for (int a = j; a<=nb; a++)
				{
					if (used[a] && badj[i][a])
					{
						int dstx = abs(pos[a][0]-px);
						int dsty = abs(pos[a][1]-py);
						if ((dstx+dsty)>1) break;
//						if (dstx>1 || dsty>1) break;
					}
				}
				if (a>nb)
				{
					// Constraints OK, are there more to do?
					done = 1;
					layout[px][py] = objid[i];
					pos[i][0] = px;
					pos[i][1] = py;
					used[i] = 1;
					if (placed<nb)
					{
						if (SolveAdjacencies(nb, placed,
							used, badj, layout, pos))
							return 1;
					}
					else return 1; // done!
					layout[px][py] = 0;
					pos[i][0] = 0;
					pos[i][1] = 0;
					used[i] = 0;
				}
			}
		}
	}
	if (pass == 1 && !done)
	{
		pass = 2; // try again, placing non-constrained
		goto again;
	}
	return 0;
}

int huge TrueEnuf()
{
	return gmode;
}
 
static void InitGraphics()
{
 	int gdriver=DETECT, errorcode;
 	(void)installuserdriver("SVGA256",TrueEnuf);
 	errorcode = registerfarbgidriver(SVGA256_driver_far);
 	if (errorcode < 0)
 	{
 		fprintf(stderr, "Graphics error: %s\n",grapherrormsg(errorcode));
 		exit(-1);
 	}
 	initgraph(&gdriver, &gmode, "");
	if ((errorcode=graphresult()) != grOk)
	{
		printf("Graphics error: %s\n", grapherrormsg(errorcode));
		exit(1);
	}
}

void InitLayout()
{
	for (int i = 0; i < MAX_BLOCKS; i++)
	{
		nadj[i] = 0;
		objid[i] = i;
		pos[i][0] = pos[i][1] = 0;
		for (int j = 0; j < MAX_BLOCKS; j++)
		{
			badj[i][j] = layout[i][j] = 0;
		}
	}
	nb = 0;
}

void AddNode(int s, int d)
{
	assert(s>=0 && d>=0 && s<MAX_BLOCKS && d<MAX_BLOCKS);
	if (s!=d)
	{
		nb++;
		assert(nb<MAX_BLOCKS);
		badj[s][nb] = badj[nb][s] = 1;
		badj[nb][d] = badj[d][nb] = 1;
		nadj[s]++;
		nadj[d]++;
		nadj[nb]+=2;
		objid[nb] = (s<d)? (d*100+s) : (s*100+d);
	}
}

int FindLayout(char *what)
{
	// ensure no block has more than four adjacencies for now
	int used[MAX_BLOCKS];
	for (int i = 1; i <= nb; i++)
	{
		used[i] = 0;
		assert(nadj[i]<=4);
	}
	// Find a solution (we hope!)
	if (nb>1)
	{
		closegraph();
		// preplace the object with the greatest adjacencies
//		int maxa = 1;
//		for (i = 2; i <= nb; i++)
//			if (nadj[i]>nadj[maxa])
//				maxa = i;
//		int p = (nb+3)/4;
//		layout[p][p] = objid[maxa];
//		used[maxa] = 1;
//		pos[maxa][0] = p;
//		pos[maxa][1] = p;
		printf("Attempting to lay out %s - please wait...\n", what);
		sleep(1);
		int rtn = SolveAdjacencies(nb, 0, used, badj, layout, pos);
		if (rtn==0)
		{
			fprintf(stderr, "Failed to lay out %s\n", what);
			sleep(2);
		}
		InitGraphics();
		return rtn;
	}
	else pos[1][0] = pos[1][1] = 1;
	return 1;
}

int DrawSegment(int x, int y, int c, int cx, int cy, int dx, int dy,
	int displ, int &lblx, int &lbly)
{
	int lim = nb/2 + 1;
	if (y>1 && layout[x][y-1]==c) // connect down?
	{
		DrawLine(cx-displ,cy+dy,cx-displ,cy-displ);
		lblx = cx-displ+5;
		lbly = cy + dy/2 + displ;
		return 0;
	}
	else if (x>1 && layout[x-1][y]==c) // connect right?
	{
		DrawLine(cx+dx,cy-displ,cx-displ,cy-displ);
		lblx = cx+dx/3;
		lbly = cy-displ+(displ?-15:5);
	}
	else if (y<lim && layout[x][y+1]==c) // connect up?
	{
		DrawLine(cx-displ,cy-dy,cx-displ,cy-displ);
		lblx = cx-displ+5;
		lbly = cy - dy/2+displ;
		return 0;
	}
	else if (x<lim && layout[x+1][y]==c) // connect left?
	{
		DrawLine(cx-dx,cy-displ,cx-displ,cy-displ);
		lblx = cx-dx/3;
		lbly = cy-displ+(displ?-15:5);
	}
	return 1;
}

void DrawConnector(int srcx, int srcy, int dstx, int dsty, int c,
	int cx, int cy, int dx, int dy, int displ, char *label)
{
	int midx, midy;
	if (DrawSegment(srcx, srcy, c, cx, cy, dx, dy, displ, midx, midy))
	{
		DrawText(midx, midy, label);
		label = NULL;
	}
	DrawSegment(dstx, dsty, c, cx, cy, dx, dy, displ, midx, midy);
	if (label) DrawText(midx, midy, label);

}

#define CVTX(x)	(/*10 + */bwidth * (2*x-1))
#define CVTY(y)	(2*th+10 + bheight*(2*y-1))

void Connect(int s, int d, int i, int bwidth, int bheight,
	int firstx, int firsty, int starty, char *label)
{
	int sx = pos[s][0], sy = pos[s][1];
	int dx = pos[d][0], dy = pos[d][1];
	int cx = pos[i][0], cy = pos[i][1];
	int dup = 0;
	int lim = nb/2 + 1;
	// crude hack for ENV; must improve
	if (s==0)
	{
		if (cx==1)
		{
			sx = 0; sy = cy;
		}
		else if (cy==lim)
		{
			sx = cx; sy = cy+1;
		}
		else
		{
			sx = cx+1; sy = cy;
		}
	}
	if (d==0)
	{
		if (cx==1)
		{
			dx = 0; dy = cy; // left edge
		}
		else if (cy==lim)
		{
			dx = cx; dy = cy+1; // bottom edge
		}
		else
		{
			dx = cx+1; dy = cy; // right edge
		}
	}
	for (int k = i+1; k<=nb; k++)
		if (objid[k]==objid[i])
			dup++;
	// get bend point
	cx = CVTX((cx-firstx)) + bwidth/2;
	cy = starty + CVTY((cy-firsty)) + bheight/2;
	DrawConnector(sx, sy, dx, dy, objid[i], cx, cy,
		bwidth*3/2, bheight*3/2, dup*bheight/5,
		(char *)label);
}

void ShowContents(int x, int y, char *buf)
{
#define MAXLINES	4
	char *bp[MAXLINES], *b;
	int l = strlen(buf)/MAXLINES, nl = 0, p = 0;
	b = buf;
	while (b[p])
	{
		bp[nl] = b+p;
		b = bp[nl++];
		if (nl==MAXLINES) break;
		int o = l;
		int len = strlen(b);
		if (o>len) break;
		while (o>0 && b[o]!=' ') o--;
		if (o<=0)
		{
			o = 0;
			while (o < len && b[o]!=' ') o++;
			if (b[o]==0) b[o+1]=0; // hax for termination
		}
		b[o] = 0;
		p = o+1;
	}
	y += NHEIGHT/2 - (nl-1)*th/2;
	x += NWIDTH/2;
	settextjustify(CENTER_TEXT, CENTER_TEXT);
	for (int i = 0; i < nl; i++)
	{
		DrawText(x, y, bp[i]);
		y += th;
	}
	settextjustify(LEFT_TEXT, TOP_TEXT);
}

void ConnectNode(int x, int &y)
{
	DrawLine(x+NWIDTH/2, y, x+NWIDTH/2, y-NHEIGHT/2);
	y += NHEIGHT*3/2;
}

void Display(int x, int &y, assignment_t &a)
{
	DrawTask(x, y, x+NWIDTH, y+NHEIGHT);
	expression_t *e = GetExprP(a.rval);
	char buff[80], *bp[5];
	buff[0] = 0;
	Print(&a.lval, buff);
	strcat(buff, " := ");
	Print(e, buff);
	ShowContents(x,y,buff);
	ConnectNode(x,y);
}

void Display(int x, int &y, output_t &o)
{
	DrawOutput(x, y, x+NWIDTH, y+NHEIGHT);
	lh_list_node_t<output_arg_t> *oa = o.signals.front();
	int py = y+NHEIGHT/2-5;
	while (oa)
	{
		DrawText(x+10, py, Id2Str(oa->info.signal));
		py += th;
		oa = oa->next();
	}
	ConnectNode(x,y);
}

void Display(int x, int &y, invoke_node_t &c, int isCall)
{
	if (isCall)
		DrawCall(x, y, x+NWIDTH, y+NHEIGHT);
	else
		DrawCreate(x, y, x+NWIDTH, y+NHEIGHT);
	DrawText(x+10, y+NHEIGHT/2-5, Id2Str(c.ident));
	// must still do args
	ConnectNode(x,y);
}

void Display(int x, int &y, timer_set_t &ts)
{
	DrawTimerSet(x, y, x+NWIDTH, y+NHEIGHT);
	char buf[80];
	sprintf(buf, "SET %s", Id2Str(ts.timer));
	// must still do args
	ShowContents(x,y,buf);
	ConnectNode(x,y);
}

void Display(int x, int &y, timer_reset_t &tr)
{
	DrawTimerReset(x, y, x+NWIDTH, y+NHEIGHT);
	char buf[80];
	sprintf(buf, "SET %s", Id2Str(tr.timer));
	// must still do args
	ShowContents(x,y,buf);
	ConnectNode(x,y);
}

void Display(int x, int &y, read_node_t &rn)
{
	DrawReadNode(x, y, x+NWIDTH, y+NHEIGHT);
	char buf[80];
	strcpy(buf, "READ ");
	Print(&rn.varref, buf);
	ShowContents(x,y,buf);
	DrawText(x+10, y+NHEIGHT/2-5, buf);
	ConnectNode(x,y);
}

void Display(int x, int y, range_condition_t &rc)
{
	char buf[80];
	strcpy(buf, "(");
	char *opS = "";
	if  (rc.op == RO_IN)
		Print(GetExprP(rc.lower), buf);
	switch (rc.op)
	{
	case RO_EQU:	opS = "="; 	break;
	case RO_NEQ:	opS = "!=";	break;
	case RO_LE:	opS = "<";	break;
	case RO_LEQ:	opS = "<=";	break;
	case RO_GT:	opS = ">";	break;
	case RO_GTQ:	opS = ">=";	break;
	case RO_IN:	opS = " : ";	break;
	default:			break;
	}
	strcat(buf, opS);
	Print(GetExprP(rc.upper), buf);
	strcat(buf, ")");
	DrawText(x,y,buf);
}

void Display(int x, int &y, write_node_t &wn)
{
	DrawWriteNode(x, y, x+NWIDTH, y+NHEIGHT);
	char buf[80];
	strcpy(buf, "WRITE ");
	lh_list_node_t<heap_ptr_t> *e = wn.exprs.front();
	for (;;)
	{
		Print(GetExprP(e->info), buf);
		e = e->next();
		if (e) strcat(buf,",");
		else break;
	}
	strcat(buf,";");
	ShowContents(x,y,buf);
	ConnectNode(x,y);
}

int Display(int &x, int &y, transition_t &t);

int Display(int x, int &y, decision_node_t &wn)
{
	char buf[80];
	int rtn = 0;
	DrawDecision(x, y, x+NWIDTH, y+NHEIGHT);
	buf[0] = 0;
	Print(GetExprP(wn.question), buf);
	ShowContents(x,y,buf);
	ConnectNode(x,y);
	int topy = y, topx = x;
	ConnectNode(x,y);
	y-=NHEIGHT;
	lh_list_node_t<subdecision_node_t> *a = wn.answers.front();
	int alnx = x;
	int alny = y;
	int maxy = y;
	int cnx[16], cny[16], cnp = 0;
	while (a)
	{

		nextx = alnx + NWIDTH*3/2;
		alny = y;
		topx = alnx;
		// draw answer
		lh_list_node_t<range_condition_t> *rc = a->info.answer.front();
		int ay = topy+5;
		if (rc==NULL)
			DrawText(alnx+NWIDTH/2+5, ay, "else");
		else while (rc)
		{
			Display(alnx+NWIDTH/2+5, ay, rc->info);
			rc = rc->next();
			ay += th;
		}
		// draw transition
		if (Display(alnx, alny, a->info.transition))
		{
			assert(cnp<16);
			cnx[cnp] = alnx;
			cny[cnp] = alny;
			cnp++;
		}
		alnx = nextx;
		if (alny > maxy) maxy = alny;
		a = a->next();
	}
	line(x+NWIDTH/2, topy, topx+NWIDTH/2, topy);
	if (cnp)
	{
		rtn = 1;
		line(x+NWIDTH/2, maxy, cnx[cnp-1]+NWIDTH/2, maxy);
		while (cnp--)
			line(cnx[cnp]+NWIDTH/2, maxy,
			     cnx[cnp]+NWIDTH/2, cny[cnp]-NHEIGHT/2);
	}
	y = maxy;
	return rtn;
}

int Display(int &x, int &y, gnode_t &n)
{
	int rtn = 1;
	void *node = GetVoidP(n.node);
	switch(n.type)
	{
		case N_TASK:
		{
			lh_list_node_t<assignment_t> *a =
				( (lh_list_t<assignment_t> *)node )->front();
			while (a)
			{
				Display(x, y, a->info);
				a = a->next();
			}
			break;
		}
		case N_OUTPUT:
			Display(x, y, (*((output_t *)node)));
			break;
		case N_PRI_OUTPUT:
			break;
		case N_CREATE:
			Display(x, y, (*((invoke_node_t *)node)), 0);
			break;
		case N_CALL:
			Display(x, y, (*((invoke_node_t *)node)), 1);
			break;
		case N_SET:
		{
			lh_list_node_t<timer_set_t> *ts =
				( (lh_list_t<timer_set_t> *)node )->front();
			while (ts)
			{
				Display(x, y, ts->info);
				ts = ts->next();
			}
			break;
		}
		case N_RESET:
		{
			lh_list_node_t<timer_reset_t> *tr =
				( (lh_list_t<timer_reset_t> *)node )->front();
			while (tr)
			{
				Display(x, y, tr->info);
				tr = tr->next();
			}
			break;
		}
		case N_EXPORT:
//			os << "EXPORT " <<  (*((lh_list_t<ident_t> *)node)) << ';';
			break;
		case N_DECISION:
			rtn = Display(x, y, (*((decision_node_t *)node)));
			break;
		case N_READ:
			Display(x, y, (*((read_node_t *)node)));
			break;
		case N_WRITE:
			Display(x, y, (*((write_node_t *)node)));
			break;
	}
	return rtn;
}

int Display(int &x, int &y, transition_t &t)
{
	int rtn = 1;
	lh_list_node_t<gnode_t> *nd = t.nodes.front();
	while (nd)
	{
		if (Display(x, y, nd->info)==0)
			rtn = 0;
		nd = nd->next();
	}
	switch (t.type)
	{
		case NEXTSTATE:
			DrawState(x, y, x+NWIDTH, y+NHEIGHT);
			if (t.next.name()[0] == '\0')
				DrawText(x+20, y+NHEIGHT/2-5, "-");
			else
				DrawText(x+10, y+NHEIGHT/2-5, t.next.name());
			ConnectNode(x,y);
			rtn = 0;
			break;
		case STOP:
			DrawStop(x, y, x+NWIDTH, y+NHEIGHT);
			ConnectNode(x,y);
			rtn = 0;
			break;
		case RETURN:
			DrawReturn(x, y, x+NWIDTH, y+NHEIGHT);
			ConnectNode(x,y);
			rtn = 0;
			break;
		case JOIN:
			DrawJoin(x, y, x+NWIDTH, y+NHEIGHT);
			DrawText(x+10, y+NHEIGHT/2-5, t.next.name());
			ConnectNode(x,y);
			rtn = 0;
			break;
	}
	return rtn;
}

void Display(int &x, int &y, state_node_t &sn)
{
	lh_list_node_t<name_t> *n = sn.states.front();
	int slnx = x, maxy = y, maxx = x;
	int scnt = 0, ccnt = 0;
	int topx = slnx;
	if (sn.isAsterisk)
	{
		DrawState(slnx,y,slnx+NWIDTH, y+NHEIGHT);
		DrawText(slnx+10, y+NHEIGHT/2-5, "*");
		line(slnx+NWIDTH/2, y+NHEIGHT, slnx+NWIDTH/2, y+NHEIGHT*3/2);
		slnx += NWIDTH*3/2;
		scnt = 1;
	}
	else
	{
		while (n)
		{
			topx = slnx;
			DrawState(slnx,y,slnx+NWIDTH, y+NHEIGHT);
			DrawText(slnx+10, y+NHEIGHT/2-5, n->info.name());
			line(slnx+NWIDTH/2, y+NHEIGHT, slnx+NWIDTH/2, y+NHEIGHT*3/2);
			slnx += NWIDTH*3/2;
			scnt++;
			n = n->next();
		}
	}
	y += NHEIGHT*3/2;
	int statelny = y;
	maxx = slnx - NWIDTH/2;
	y += NHEIGHT/2;
	slnx = x;
	lh_list_node_t<input_part_t> *in = sn.inputs.front();
	while (in)
	{
		int ty = y;
		nextx = slnx + NWIDTH*3/2;
		if (slnx > topx) topx = slnx;
		DrawInput(slnx, ty, slnx+NWIDTH, ty+NHEIGHT);
		if (in->info.isAsterisk)
			DrawText(slnx+10, ty+NHEIGHT/2-5, "*");
		else
		{
			int scnt = 0;
			lh_list_node_t<stimulus_t> *st = in->info.stimuli.front();
			char buf[120];
			buf[0] = 0;
			while (st)
			{
				scnt++;
				strcat(buf, Id2Str(st->info.signal));
				if (st->info.variables.front())
				{
					strcat(buf, "(");
					lh_list_node_t<ident_t> *il = st->info.variables.front();
					while (il)
					{
						strcat(buf, Id2Str(il->info));
						il = il->next();
						if (il) strcat(buf, ", ");
					}
					strcat(buf, ")");
				}
				st = st->next();
			}
			ShowContents(slnx,ty,buf);
		}
		ConnectNode(slnx, ty);
		if (in->info.enabler)
		{
			char buf[80];
			buf[0] = 0;
			DrawCSignal(slnx, ty, slnx+NWIDTH, ty+NHEIGHT);
			Print(GetExprP(in->info.enabler), buf);
			ShowContents(x,y,buf);
			ConnectNode(slnx, ty);
		}
		Display(slnx, ty, in->info.transition);
		if (ty > maxy) maxy = ty;
		slnx = nextx;
		ccnt++;
		in = in->next();
	}
	lh_list_node_t<continuous_signal_t> *cs = sn.csigs.front();
	while (cs)
	{
		int ty = y;
		nextx = slnx + NWIDTH*3/2;
		if (slnx > topx) topx = slnx;
		DrawCSignal(slnx, ty, slnx+NWIDTH, ty+NHEIGHT);
		char buf[80];
		buf[0] = 0;
		Print(GetExprP(cs->info.condition), buf);
		if (cs->info.priority!=UNDEFINED)
			sprintf(buf+strlen(buf), "    %ld", (long)cs->info.priority);
		ShowContents(x,y,buf);
		ConnectNode(slnx, ty);
		Display(slnx, ty, cs->info.transition);
		if (ty > maxy) maxy = ty;
		slnx = nextx;
		ccnt++;
		cs = cs->next();
	}
	slnx -= NWIDTH/2;
	if (slnx>maxx) maxx = slnx;
	if (scnt>1 || ccnt>1)
		line(x+NWIDTH/2, statelny, topx+NWIDTH/2, statelny);
	y = maxy;
	x = maxx;
}

void Display(process_def_t *prc)
{
	char buf[80], tbuf[80];
	int page = 1, npg = 1; // first page has decs and start trans
	// compute number of pages
	for (lh_list_node_t<state_node_t> *s = prc->states.front();
		s; s = s->next())
			npg++;
	// do page 1
	sprintf(tbuf, "PROCESS %s", prc->name());
	int tsize = TextWidth(tbuf) + 120;
	// Compute the space required for declarations, by finding
	// longest line and number of lines
	int decheight = 0, decwidth = 0;
	GetSize(prc->types.front(), decheight, decwidth);
	GetSize(prc->signals.front(), decheight, decwidth);
	GetSize(prc->siglists.front(), decheight, decwidth);
	GetSize(prc->variables.front(), decheight, decwidth);
	decwidth += 100;
	if (decheight && decheight < 100)
		decheight = 100;
	int swidth = (decwidth > tsize) ? decwidth : tsize;

	Clear();

	// Draw the process title

	int sheight = decheight+5 * th;
	DrawText(10, 5, tbuf);

	// Draw the declaration box and declarations

	if (decheight)
	{
		DrawDeclBox(swidth, decheight);
		int x = 15, y = th+10;
		Display(x, y, prc->types.front());
		Display(x, y, prc->signals.front());
		Display(x, y, prc->siglists.front());
		Display(x, y, prc->variables.front());
	}
	// draw the start transition
	int y = sheight;
	int x = 100;
	DrawStart(x,y,x+NWIDTH, y+NHEIGHT);
	y+=NHEIGHT*3/2;
	Display(x, y, prc->start);

	// Draw the process box and page number

	DrawBox(1, 1, swidth, y+20);
	sprintf(buf, "%d(%d)", page, npg);
	DrawText(swidth - TextWidth(buf) - 10, 5, buf);

	// draw the remaining pages
	for (s = prc->states.front(); s; s = s->next())
	{
		getch();
		page++;
		Clear();
		DrawText(10, 5, tbuf);
		// draw the transition
		x = 100;
		y = 40;
		Display(x, y, s->info);
		// Draw the process box
		x = max(swidth, x)+10;
		DrawBox(1, 1, x, max(sheight, y)+20);
		sprintf(buf, "%d(%d)", page, npg);
		DrawText(x - TextWidth(buf) - 10, 5, buf);
	}
}

void Display(block_def_t *blk)
{
	char buf[80];
	sprintf(buf, "BLOCK %s", blk->name());
	int tsize = TextWidth(buf)+50;
	// Compute the space required for declarations, by finding
	// longest line and number of lines
	int decheight = 0, decwidth = 0;
	GetSize(blk->types.front(), decheight, decwidth);
	GetSize(blk->signals.front(), decheight, decwidth);
	GetSize(blk->siglists.front(), decheight, decwidth);
	decwidth += 100;
	if (decheight && decheight < 100)
		decheight = 100;
	int swidth = (decwidth > tsize) ? decwidth : tsize;

	// Compute the layout of processes and signalroutes

	// Clear...
	InitLayout();
	// ...count the processes...
	for (lh_list_node_t<process_def_t> *p = blk->processes.front();
		p; p = p->next())
			nb++;
	// ...fill in the matrix...
	for (lh_list_node_t<route_def_t> *r = blk->routes.front();
		r; r = r->next())
			AddNode(r->info.endpt[0], r->info.endpt[1]);
	// ...and find the layout
	if (!FindLayout("processes and signalroutes"))
		return;

	// Find the longest process name and use this to compute block size

	int bwidth = 0, bheight = 0;
	for (p = blk->processes.front(); p; p = p->next())
	{
		int w = TextWidth(p->info.name());
		if (w>bwidth)
		{
			bwidth = w;
			bheight = TextHeight(p->info.name());
		}
	}
	bheight *= 6;
	bwidth += 20;

	// Compute the max/min width and height
	int maxwidth = 0, maxheight = 0, minwidth=nb, minheight = nb;
	for (int i=1; i<=nb;i++)
	{
		if (pos[i][0]>maxwidth) maxwidth = pos[i][0];
		if (pos[i][1]>maxheight) maxheight = pos[i][1];
		if (pos[i][0]<minwidth) minwidth = pos[i][0];
		if (pos[i][1]<minheight) minheight = pos[i][1];
	}
	minwidth--;
	minheight--;
	// scale by block size
	maxwidth = bwidth + (maxwidth-minwidth) * bwidth*2;
	maxheight = bheight + (maxheight-minheight) * bheight*2;
	if (swidth < maxwidth) swidth = maxwidth;

	Clear();

	// Draw the block box and title

	int sheight = decheight+5 * th;
	DrawBox(1,1,swidth/*+20*/,sheight + maxheight + 4*th);
	DrawText(10,5, buf);

	// Draw the declaration box and declarations

	if (decheight)
	{
		DrawDeclBox(swidth, decheight);
		int x = 15, y = th+10;
		Display(x, y, blk->types.front());
		Display(x, y, blk->signals.front());
		Display(x, y, blk->siglists.front());
	}

	// Draw the processes and signalroutes

	for (p = blk->processes.front(), i = 1; p; p = p->next(), i++)
	{
		int px = pos[i][0]-minwidth, py = pos[i][1]-minheight;
		int x = CVTX(px), y = decheight+CVTY(py);
		DrawProcess(x, y, x+bwidth, y+bheight);
		DrawText(x+10, y+bheight/3, p->info.name());
	}
	for (r = blk->routes.front(); r; r = r->next(), i++)
		Connect(r->info.endpt[0], r->info.endpt[1], i,
			bwidth, bheight,
			minwidth, minheight,
			decheight, (char *)r->info.name());

	// While there are non-ESC keypresses, display the child processes
	for (p = blk->processes.front(); p; p = p->next())
	{
		if (getch() == 27) break;
		Display(&p->info);
	}
}

void Display(system_def_t *sys)
{
	char buf[80];
	sprintf(buf, "SYSTEM %s", sys->name());
	int tsize = TextWidth(buf)+50;
	// Compute the space required for declarations, by finding
	// longest line and number of lines
	int decheight = 0, decwidth = 0;
	GetSize(sys->types.front(), decheight, decwidth);
	GetSize(sys->signals.front(), decheight, decwidth);
	GetSize(sys->siglists.front(), decheight, decwidth);
	decwidth += 100;
	if (decheight && decheight < 100)
		decheight = 100;
	int swidth = (decwidth > tsize) ? decwidth : tsize;

	// Compute the layout of blocks and channels.

	// Clear...
	InitLayout();
	// ...count the blocks...
	for (lh_list_node_t<block_def_t> *b = sys->blocks.front();
		b; b = b->next())
			nb++;
	// ...fill in the matrix...
	for (lh_list_node_t<channel_def_t> *c = sys->channels.front();
		c; c = c->next())
			AddNode(c->info.endpt[0], c->info.endpt[1]);
	// ...and find the layout
	if (!FindLayout("blocks and channels"))
		return;

	// Find the longest block name and use this to compute block size

	int bwidth = 0, bheight = 0;
	for (b = sys->blocks.front(); b; b = b->next())
	{
		int w = TextWidth(b->info.name());
		if (w>bwidth)
		{
			bwidth = w;
			bheight = TextHeight(b->info.name());
		}
	}
	bheight *= 6;
	bwidth += 20;

	int maxwidth = 0, maxheight = 0, minwidth=nb, minheight = nb;
	for (int i=1; i<=nb;i++)
	{
		if (pos[i][0]>maxwidth) maxwidth = pos[i][0];
		if (pos[i][1]>maxheight) maxheight = pos[i][1];
		if (pos[i][0]<minwidth) minwidth = pos[i][0];
		if (pos[i][1]<minheight) minheight = pos[i][1];
	}
	minwidth--;
	minheight--;
	// scale by block size
	maxwidth = bwidth + (maxwidth-minwidth) * bwidth*2;
	maxheight = bheight + (maxheight-minheight) * bheight*2;
	if (swidth < maxwidth) swidth = maxwidth;

	Clear();

	// Draw the system box and title

	int sheight = decheight+5 * th;
	DrawBox(1,1,swidth/*+20*/,sheight + maxheight + 4*th);
	DrawText(10,5, buf);

	// Draw the declaration box and declarations

	if (decheight)
	{
		DrawDeclBox(swidth, decheight);
		int x = 15, y = th+10;
		Display(x, y, sys->types.front());
		Display(x, y, sys->signals.front());
		Display(x, y, sys->siglists.front());
	}

	// Draw the blocks and channels

	for (b = sys->blocks.front(), i = 1; b; b = b->next(), i++)
	{
		int px = pos[i][0]-minwidth, py = pos[i][1]-minheight;
		int x = CVTX(px), y = decheight+CVTY(py);
		DrawBlock(x, y, x+bwidth, y+bheight);
		DrawText(x+10, y+bheight/3, b->info.name());
	}
	for (c = sys->channels.front(); c; c = c->next(), i++)
		Connect(c->info.endpt[0], c->info.endpt[1], i,
			bwidth, bheight, minwidth, minheight,
			decheight, (char *)c->info.name());

	// While there are non-ESC keypresses, display the child blocks
	for (b = sys->blocks.front(); b; b = b->next())
	{
		if (getch() == 27) break;
		Display(&b->info);
	}
	getch();
	closegraph();
	for (i = 1; i <= nb; i++)
		printf("%d : %d,%d\n", i, pos[i][0], pos[i][1]);
}

int main(int argc, char *argv[])
{
	gmode = 3;
	if (argc>1)
		gmode = atoi(argv[1]);
	cerr << "DNA SDL AST to SDL/GR Convertor v1.0" << endl;
	cerr << "written by Graham Wheeler" << endl;
	cerr << "(c) 1994 Graham Wheeler and the DNA Laboratory" << endl;
	initGlobalObjects();
	RestoreAST("ast.sym");
	InitGraphics();
	th = TextHeight("X") + 5;
	Display(sys);
	DeleteAST();
	return 0;
}

