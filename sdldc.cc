/*
 * SDL Decompiler
 *
 * Written by Graham Wheeler, February 1994
 * Data Network Architecture Laboratory
 * Department of Computer Science
 * University of Cape Town
 * 
 * (c) 1994 Graham Wheeler and the Data Network Architectures Laboratory
 * All Rights Reserved
 *
 * Last modified: 13-6-94
 *
 */

#pragma hdrfile "PRINT.SYM"
#define PRINT
#include "sdlast.h"

// stub needed by sdlast

int getLineNumber()
{
	return 0;
}

int main(void)
{
	cerr << "DNA AST Decompiler v1.0" << endl;
	cerr << "written by Graham Wheeler" << endl;
	cerr << "(c) 1994 Graham Wheeler and the DNA Laboratory" << endl;
	RestoreAST("ast.out");
	cout << *sys ;
	DeleteAST();
	return 0;
}

