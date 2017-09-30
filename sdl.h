#ifndef _SDL_H
#define _SDL_H

// Things common to all source files

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

// The most important typedefs!

#ifdef MAXINT
#undef MAXINT
#endif
#ifdef W16
typedef short					s_code_word_t;
typedef unsigned short		u_s_code_word_t;
typedef short float			s_code_real_t;
#define UNDEFINED				(0x8000)
#define MAXINT				(0x7FFF)
#define DEFAULT_HEAP_SIZE		32000
#else
typedef long					s_code_word_t;
typedef unsigned long			u_s_code_word_t;
typedef float					s_code_real_t;
#define UNDEFINED				(0x80000000l)
#define MAXINT				(0x7FFFFFFFl)
#if __MSDOS__
#define DEFAULT_HEAP_SIZE		65000
#else
#define DEFAULT_HEAP_SIZE		200000
#endif
#endif

typedef union // for casting between int and real types
{
	s_code_word_t	iv;
	s_code_real_t	rv;
} s_code_cast_t;

#define SWORD_SIZE	(sizeof(s_code_word_t)*8)

typedef u_s_code_word_t		s_offset_t;

typedef enum
{
	False, True
} Bool_t;


// System limits.
// Many of these are not common to all files,
// but its useful to keep them together

#define MAX_FILES		10
#define MAX_FNAME_LEN	16
#define MAX_TOKEN_LENGTH	80
#define MAX_NEST		16		// max nesting of systems/blocks/processes..
#define MAX_STATES	64
#define MAX_SIGNALS	64					// max allowed signals
#define MAX_EXPORTS	16
#define MAX_FILTERS	32
#define MAX_INSTANCES	64
#define MAX_PROCESSES	30
#define MAX_CHANNELS	30
#define MAX_ROUTES	60
#define MAX_PRIORITIES	64
#define MAX_CODE_SIZE	15000
#define STACKSIZE	2048
#define MAX_DEST	20			// max allowed destinations per output

#define SIGSET_SIZE	(MAX_SIGNALS/SWORD_SIZE) // array of us_code_word_ts

#endif

