// DOS Driver for the various passes

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#if __MSDOS__
#include <process.h>
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

static int runCommand(char **args, int argcnt)
{
	args[argcnt] = NULL;
#if __MSDOS__
	return spawnvp(P_WAIT,args[0],args);
#else
	// simple spawn
	int status, pid = fork();
	if (pid == 0) // child?
		execvp(args[0], args);
	else if (pid > 0) // parent?
	{
		for(;;)
		{
			if (wait(&status)==pid && !WIFSTOPPED(status))
				return WIFEXITED(status) ?
					WEXITSTATUS(status) :
					-1;
		}
	}
	return -1;
#endif
}

static void useage()
{
	fprintf(stderr, "Useage: sdlcc [<options>] [<synonyms>] <files>\n\n");
	fprintf(stderr, "Options must begin with one of -P (preprocessor),\n");
	fprintf(stderr, "-1 (parser) or -2 (checker). They are used in the\n");
	fprintf(stderr, "invocation of the respective pass. For example, to\n");
	fprintf(stderr, "pass -H2 to the preprocessor, use -PH2.\n");
	fprintf(stderr, "External synonym values can be defined as `<name>=<value>'\n");
	exit(-1);
}

int main(int argc, char *argv[])
{
	char *args[256];
	char *ppout = tempnam(NULL, "sdl");
	fprintf(stderr, "SDL*Design Compiler v1.0\n");
	fprintf(stderr, "written by Graham Wheeler\n");
	fprintf(stderr, "(c) 1994 Graham Wheeler and the University of Cape Town\n");
	args[0] = "sdlpp";
	args[1] = "-o";
	args[2] = ppout;
	int i, j, gotfile = 0;
	for (i = 1, j = 3; i<argc; i++)
	{
		if (argv[i][0]=='-')
		{
			if (argv[i][1]=='P')
			{
				args[j] = new char[strlen(argv[i])];
				assert(args[j]);
				args[j][0] = '-';
				strcpy(&args[j][1], &argv[i][2]);
				j++;
			}
		}
		else if (strchr(argv[i],'=')==NULL)
		{
			args[j] = new char[strlen(argv[i])+1];
			assert(args[j]);
			strcpy(args[j], argv[i]);
			j++;
			gotfile = 1;
		}
	}
	if (!gotfile) useage();
	if (runCommand(args, j)==0)
	{
		while (j-- > 3) delete args[j];
		args[0] = "sdlc1";
		args[1] = "-S";
		for (i = 1, j = 2; i<argc; i++)
		{
			if (argv[i][0]=='-' && argv[i][1]=='1')
			{
				args[j] = new char[strlen(argv[i])];
				assert(args[j]);
				args[j][0] = '-';
				strcpy(&args[j][1], &argv[i][2]);
				j++;
			}
			else if (strchr(argv[i],'='))
			{
				args[j] = new char[strlen(argv[i])+1];
				assert(args[j]);
				strcpy(args[j], argv[i]);
				j++;
			}
		}
		args[j++] = ppout;
		if (runCommand(args, j)==0)
		{
			while (--j > 1) delete args[j];
			args[0] = "sdlc2";
			for (i = j = 1; i<argc; i++)
			{
				if (argv[i][0]=='-' && argv[i][1]=='2')
				{
					args[j] = new char[strlen(argv[i])];
					assert(args[j]);
					args[j][0] = '-';
					strcpy(&args[j][1], &argv[i][2]);
					j++;
				}
			}
			if (runCommand(args, j) == 0)
			{
				while (j-- > 1) delete args[j];
				unlink(ppout);
				exit(0);
			}
			else while (j-- > 1) delete args[j];
		}
		else while (j-- > 2) delete args[j];
		unlink(ppout);
	}
	else while (j-- > 3) delete args[j];
	exit(-1);
	return 0; // avoid warning
}


