#include <stdio.h>

#define HASHES 40

int marks;

void ProgressMeter(char *text)
{
	int i;
        printf("%s: [", text);
        for(i=0;i<HASHES;i++) printf(" ");
        printf("]");
        for(i=0;i<HASHES+1;i++) printf("%c", 8);
	marks = 0;
}

void UpdateProgress(int part, int whole)
{
	while(part * HASHES >= (marks+1) * whole)
	{
		printf("#");
		marks++;
	}
	fflush(stdout);
}

void EndProgressMeter()
{
	printf("]\n");
}
