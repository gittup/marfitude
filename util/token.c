#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "token.h"
#include "memtest.h"

 // skips empty & commented lines, NULL on EOF/error
int GetToken(FILE *f, char stopper, Token *t)
{
	char input[500];
	char c;
	int x = 0;
	t->type = VALUE;
	while((c = fgetc(f)) != EOF)
	{
		if(c == '\n' || c == stopper) break;
		if(x == 0)
		{
			if(c == '#')
			{
				while(c != '\n' && c != EOF) c = fgetc(f);
				break;
			}
			else if(c == '[')
			{
				t->type = HEADER;
				continue;
			}
			// type of VALUE is already set, no else needed
		}
		if(t->type == HEADER && c == ']')
		{
			while(c != '\n' && c != EOF) c = fgetc(f);
			break;
		}
		input[x] = c;
		x++;
		if(x == 500)
		{
			printf("GetToken Error: line too long!\n");
			return 0;
		}
	}
	if(x == 0)
	{
		if(c == EOF) return 0;
		return GetToken(f, stopper, t); // skip empty lines
	}
	input[x] = 0;
	t->token = (char*)malloc(x+1);
	strcpy(t->token, input);
	t->value = atoi(t->token);
	return 1;
}
