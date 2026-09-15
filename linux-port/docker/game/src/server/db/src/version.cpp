#include <stdio.h>
#include <stdlib.h>

void WriteVersion()
{
#ifndef __WIN32__
	FILE* fp(fopen("VERSION.txt", "w"));

	if (NULL != fp)
	{
		fprintf(fp, "__DB_VERSION__: %s\n", __DB_VERSION__);
		fprintf(fp, "%s@%s:%s\n", "dmVldmVl", __HOSTNAME__, __PWD__);
		fclose(fp);
	}
	else
	{
		fprintf(stderr, "cannot open VERSION.txt\n");
		exit(0);
	}
#endif
}
//martysama0134's 4e4e75d8b719b9240e033009cf4d7b0f

// Files shared by GameCore.top
