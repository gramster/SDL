/*
 * sdlpp.h - prototypes for global functions and data
 *
 * Written by Graham Wheeler , February 1994
 * Data Network Architecture Laboratory
 * Department of Computer Science
 * University of Cape Town
 * 
 * (c) 1994 Graham Wheeler and the Data Network Architectures Laboratory
 * All Rights Reserved
 *
 * Last modified: 8-8-94
 */


#ifndef _SDLPP_H
#define _SDLPP_H

// Reserved words lookup table

typedef struct
{
	char *rw_name;
#ifdef DEBUG
	char			*rwtoken;
#else
	SDL_token_t	rw_token;
#endif
} rw_entry_t;

extern rw_entry_t rwtable[];

// Error types

typedef enum
{
	ERR_NAMES, ERR_SYNTAX, ERR_NOREMOTE, ERR_NOFILE, ERR_STRING, ERR_COMMENT,
	ERR_BADCHAR, ERR_INTEGER, ERR_REAL
}
error_t;

extern void SDLfatal(const error_t err, const char *arg = NULL);
extern void SDLerror(const error_t err, const char *arg = NULL);
extern void SDLwarning(const error_t err, const char *arg = NULL);

#endif

