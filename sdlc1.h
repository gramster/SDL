/*
 * sdlcc.h - prototypes for global functions and data
 *
 * Written by Graham Wheeler , February 1994
 * Data Network Architecture Laboratory
 * Department of Computer Science
 * University of Cape Town
 * 
 * (c) 1994 Graham Wheeler and the Data Network Architectures Laboratory
 * All Rights Reserved
 *
 * Last modified: 23-2-94
 */

#ifndef _SDLCC_H
#define _SDLCC_H

//------------------------------------------------

#define MAX_EXT_SYN		32	// max external synonyms

typedef struct
{
	char *name;
	char *value;
} ext_synonym_t;

extern ext_synonym_t ext_synonym_tbl[MAX_EXT_SYN];
extern int num_ext_synonym;

//------------------------------------------------

typedef struct { /* Reserved words lookup table */
	char *rw_name;
#ifdef DEBUG
	char			*rwtoken;
#else
	SDL_token_t	rw_token;
#endif
} rw_entry_t;

extern rw_entry_t rwtable[];

//------------------------------------------------
// Error types

typedef enum
{
	ERR_NAMES, ERR_SYNTAX, ERR_NOREMOTE, ERR_NOFILE, ERR_STRING, ERR_COMMENT,
	ERR_BADCHAR, ERR_INTEGER, ERR_BADNAME, ERR_EXPECT, ERR_QUALIFY,
	ERR_IDENTS, ERR_INFORMAL, ERR_NOMACROS, ERR_NEWTYPE, ERR_SYNONYM,
	ERR_SYNTYPE, ERR_GENERATOR, ERR_SELECT, ERR_NOREFINE, ERR_DUPREMDEF,
	ERR_DIFINIT, ERR_DIFMAX, ERR_BADENDSTATE, ERR_REAL,
	ERR_SELTYPE, ERR_SELEXPR, ERR_SELEOF, ERR_NOSELECT
}
error_t;

extern void SDLfatal(const error_t err, const char *arg = NULL);
extern void SDLerror(const error_t err, const char *arg = NULL);
extern void SDLwarning(const error_t err, const char *arg = NULL);

//------------------------------------------------
// Global variables

extern ofstream errStr;
extern ifstream inStr;
extern char token[MAX_TOKEN_LENGTH];
extern int lineNum;
extern SDL_token_t symbol;
extern char **fileList;

//------------------------------------------------
// Prototypes from sdlcc.cpp

extern void STRLWR(register char *s);

//------------------------------------------------
// Prototypes from sdllex.cpp

extern void nextchar(void);
extern void nextsymbol(void);

//------------------------------------------------
// Prototypes from sdlparse.cpp

extern void SDLparse(void);

#endif

