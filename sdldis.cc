#pragma hdrfile "CODE.SYM"
#include "sdlcode.h"
#pragma hdrstop

char fname[MAX_FILES][MAX_FNAME_LEN];

int MyFilter(s_code_word_t *args)
{
	if (args[0]==OP_NEWLINE)
	{
		cout << endl << "File " << fname[args[1]] <<
			"  Line " << args[2] << endl;
		return 1;
	}
	return 0;
}

int main()
{
	cerr << "DNA S-Code Disassembler v1.0" << endl;
	cerr << "written by Graham Wheeler" << endl;
	cerr << "(c) 1994 Graham Wheeler and the DNA Laboratory" << endl;
#ifdef __MSDOS__
	ifstream is("sdl.cod", ios::binary);
#else
	ifstream is("sdl.cod");
#endif
	for (int i = 0; i<MAX_FILES; i++)
		is.read(fname[i], MAX_FNAME_LEN);
	Code = new codespace_t;
	assert(Code);
	if (Code->Read(NULL, &is)==0)
		Code->Disassemble(MyFilter);
	delete Code;
	return 0;
}
