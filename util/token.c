/*
   Copyright (C) 2004 Mike Shal

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "token.h"
#include "memtest.h"

 /* skips empty & commented lines, NULL on EOF/error */
int GetToken(FILE *f, char stopper, struct token *t)
{
	/* endofline gets increased at each newline
	 * and cleared at each non-newline
	 * so if endofline > 1 its a blank line, so it gets
	 * skipped
	 */
	static int endofline = 1;
	char input[500];
	char c;
	int x = 0;
	t->type = VALUE;
	while((c = fgetc(f)) != EOF)
	{
		if(c == '\n')
		{
			endofline++;
			break;
		}
		endofline = 0;
		if(c == stopper) break;
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
			/* type of VALUE is already set, no else needed */
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
			fprintf(stderr, "GetToken Error: line too long!\n");
			return 0;
		}
	}
	if(x == 0)
	{
		if(c == EOF) return 0;
		if(endofline > 1)
		{
			return GetToken(f, stopper, t); /* skip empty lines */
		}
	}
	input[x] = 0;
	t->token = (char*)malloc(x+1);
	strcpy(t->token, input);
	t->value = atoi(t->token);
	return 1;
}
