/*
 * sdlc2.h - compiler 2nd pass defs & prototypes
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
 */

#ifndef _SDLC2_H
#define _SDLC2_H


// Error types

typedef enum
{
	ERR_MISSBLOCK, ERR_MISSDEFS, ERR_MISSSUBLOCK, ERR_DUPPARAM, ERR_DUPVAR,
	ERR_NOREMDEF, ERR_INITNEG, ERR_MOREMAX, ERR_MAXPOS, ERR_BADPROC,
	ERR_BADBLOCK, ERR_BOTHENV, ERR_PATHDUP, ERR_PATHDIF, ERR_NOXPORT,
	ERR_NOREVEAL, ERR_INTIMER, ERR_BADSIGNAL, ERR_NOSIGLIST, ERR_NOTYPE,
	ERR_MAXSIGS, ERR_VARREF, ERR_VARTYPE, ERR_PARAMTYPE, ERR_BADELTSEL,
	ERR_BADFLDSEL, ERR_UNKNOWNSEL, ERR_BOOLEXPR, ERR_CONDEXPR, ERR_INTEXPR,
	ERR_INTCHEXPR, ERR_BASICTYPE, ERR_EXPRTYPES, ERR_ASSIGNTYPE, ERR_INDEX,
	ERR_TIMERID, ERR_MISSPARAM, ERR_APARAMTYPE, ERR_EXTRAPARAM, ERR_SIGNALID,
	ERR_PROCESSID, ERR_PROCEDUREID, ERR_NOPRIOUT, ERR_UNSUPPORT,
	ERR_VARINITSZ, ERR_QUESTIONTYP, ERR_MAXRANGES, ERR_RANGENE,
	ERR_RANGECROSS, ERR_RANGEHOLE, ERR_UNREACH, ERR_BADRETURN,
	ERR_BADSTOP, ERR_BADNEXT, ERR_NOTERM, ERR_GROUNDEXPR, ERR_INITTYPE,
	ERR_ANSWTYPE, ERR_NONONASTRSK, ERR_DUPLABEL, ERR_VIAROUTE,
	ERR_AMBIGUOUS, ERR_NOPATH, ERR_CHTBLFULL, ERR_ROUTETBLFULL, ERR_C2RBLK,
	ERR_C2RROUTE, ERR_JOINLBL, ERR_NEXTSTATE, ERR_NOVSIG, ERR_VIEW,
	ERR_IMPORT, ERR_VIADUP, ERR_CONNCH, ERR_BADROUTE, ERR_UNCONNCH,
	ERR_SIGMISMATCH, ERR_PIDEXPR,
	ERR_DUPIMPORT, ERR_BADTYPE, ERR_NOIMPORTDEF,
	ERR_DUPVIEW, ERR_DUPVIEWDEF, ERR_NOVIEWDEF, ERR_REFPARAM,
	ERR_NOPRIORITY, ERR_DUPPRI, ERR_PRIRANGE, ERR_NOSIGNAL, ERR_DUPINPUT,
	ERR_DUPENUMELT, ERR_DUPFIELD, ERR_INTRLEXPR, ERR_INTCHRLEXPR,
	ERR_NATEXPR, ERR_EXPTIME, ERR_UNNATURAL, ERR_READTYPE, ERR_WRITETYPE,
	ERR_DESTTYPE, ERR_FIX, ERR_FLOAT
}
error_t;

extern void SDLfatal(const fil, const ln, const error_t err, const char *arg = NULL);
extern void SDLerror(const fil, const ln, const error_t err, const char *arg = NULL);
extern void SDLwarning(const fil, const ln, const error_t err, const char *arg = NULL);

// Error classes

#define WARN	0
#define ERROR	1
#define FATAL	2

// Predefined `variables'

#define PARENT	0
#define SELF		1
#define OFFSPRING	2
#define SENDER	3

#define VAR_START	3	// offset of first user-defined var
#define PARAM_START	0	// offset of last predefined param+1

// Global variables

extern ofstream errStr;
extern ifstream inStr;
extern int errorCount[];

extern int findOutputDest(int file, int place, signal_def_t *sig,
			  lh_list_t<ident_t> &via, Bool_t mustEmit);
extern void EmitDest(s_code_word_t, s_code_word_t);
extern int findVar(ident_t &id, data_def_t* &typ, Bool_t isExported,
	Bool_t isRevealed, int &idx, process_def_t* &p);
extern int findVar(name_t &nm, data_def_t* &typ, Bool_t isExported,
	Bool_t isRevealed, int &idx, process_def_t* &p);

extern int verbose;

extern int nextLabel;
#define newlabel()	(nextLabel++)

#endif

