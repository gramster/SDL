#ifndef _SDLRUN_H
#define _SDLRUN_H

#include <stdio.h> // for sprintf
#define PRINT
#include "sdlast.h"
#include "sdlcode.h"
#include "smachine.h"

// Error types

typedef enum
{
	ERR_CUSTOM, ERR_ZERODIV, ERR_PROCTBLFULL, ERR_NODEST, ERR_MULTIDEST,
	ERR_DEADLOCK, ERR_MAXEXPORT, ERR_IMPORTSZ, ERR_NOEXPORT, ERR_UNDEFVAL,
	ERR_UNNATURAL
}
error_t;

extern void SDLfatal(const error_t err, const char *arg = NULL);
extern void SDLerror(const error_t err, const char *arg = NULL);
extern void SDLwarning(const error_t err, const char *arg = NULL);

// Error classes

#define WARN	0
#define ERROR	1
#define FATAL	2

extern char fname[MAX_FILES][MAX_FNAME_LEN];

#endif

