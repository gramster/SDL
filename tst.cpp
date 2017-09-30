#include <stdio.h>
#include <assert.h>
#include "sdlrte.h"

int nchannels = 0;
int nroutes = 0;

//--------------------------------------------------------------------
// Block type IDs

typedef enum
{
	test_testblk_ID
} block_type_t;

//--------------------------------------------------------------------
// Process type IDs

typedef enum
{
	testblk_testprocess_ID
} process_type_t;

//--------------------------------------------------------------------
// Data types

const	float pi = 3.1415927;
const int zok = 99;
const int ext = EXTERNAL;

//-------------------------------------------------------------------
// Testprocess

class testblk_testprocess_t : public SDL_process_t
{
public:
	testblk_testprocess_t();
	~testblk_testprocess_t();
	int		Provided(int p);
	void		Input(int d);
	exec_result_t	Action(int a);
};

int testblk_testprocess_t::Provided(int p)
{
	assert(0);
	return 0;
}

void testblk_testprocess_t::Input(int d)
{
	signal_t *s = Dequeue();
	switch(d)
	{
		default:
			assert(0);
	}
}

exec_result_t testblk_testprocess_t::Action(int a)
{
	exec_result_t rtn = DONETRANS;
	switch(a)
	{
		case 0: // start transition
			printf("%f\n", pi);
			printf("%d\n", zok);
			printf("%d\n", ext);
			printf("%d\n", zok*ext);
			printf("%f\n", pi*2.);
			printf("%f\n", pi+pi);
			printf("%f\n", pi/pi);
			return DONESTOP;
		default:
			break;
	}
	assert(0);
	return rtn;
}

testblk_testprocess_t::testblk_testprocess_t()
	: SDL_process_t((int)testblk_testprocess_ID)
{
	printf("Created testprocess (%d) at time %f\n", Self(), Now());
}

void CreateInitial()
{
	AddProcess(new testblk_testprocess_t);
}

