/*
   Marfitude
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

#include "cfg.h"
#include "log.h"

#include "memtest.h"
#include "fatalerror.h"
#include "token.h"
#include "strfunc.h"

struct option {
	char *key;
	char *value;
};

struct header {
	char *header;
	struct option *ops;
	int numOps;
};

/*static char *Cat(const char *header, const char *value);*/
static const char *OptionPart(const char *s);
static char *HeaderPart(const char *s);
/*static int HeaderEq(const char *a, const char *b);*/
static void AddOp(struct header *h, const char *key, const char *value);
static void CfgAdd(const char *header, const char *key, const char *value);

struct header *cfg = NULL;
int numHeaders = 0;
int cfgInited = 0;

/*char *Cat(const char *header, const char *value)
{
	char *ret;
	ret = (char*)malloc(strlen(header) + strlen(value) + 2);
	strcpy(ret, header);
	strcat(ret, ".");
	strcat(ret, value);
	return ret;
}*/

/* s is "a.b", returns "a" - must be freed */
char *HeaderPart(const char *s)
{
	int x = 0;
	char *h;
	while(s[x] != '.')
	{
		if(!s[x]) return NULL;
		x++;
	}
	h = (char*)malloc(sizeof(char) * (x+1));
	strncpy(h, s, x);
	h[x] = 0;
	return h;
}

/* s is "a.b", returns "b", no freeing */
const char *OptionPart(const char *s)
{
	int x = 0;
	while(s[x])
	{
		if(s[x] == '.') return s+x+1;
		x++;
	}
	return NULL;
}

/*int HeaderEq(const char *a, const char *b)
{
	int x = 0;
	if(a == NULL || b == NULL) return 0;
	while(a[x] && b[x])
	{
		if(a[x] != b[x]) return 0;
		if(a[x] == b[x] && a[x] == '.') return 1;
		x++;
	}
	return 0;
}*/

void AddOp(struct header *h, const char *key, const char *value)
{
	h->ops = (struct option*)realloc(h->ops, sizeof(struct option) * (h->numOps+1));
	h->ops[h->numOps].key = (char*)malloc(sizeof(char) * (strlen(key)+1));
	h->ops[h->numOps].value = (char*)malloc(sizeof(char) * (strlen(value)+1));
	strcpy(h->ops[h->numOps].key, key);
	strcpy(h->ops[h->numOps].value, value);
	h->numOps++;
}

void CfgAdd(const char *header, const char *key, const char *value)
{
	int x, y;
	int foundHeader = 0, foundOp = 0;
	for(x=0;x<numHeaders;x++)
	{
		if(strcmp(cfg[x].header, header) == 0)
		{
			foundHeader = 1;
			for(y=0;y<cfg[x].numOps;y++)
			{
				if(strcmp(cfg[x].ops[y].key, key) == 0)
				{
					foundOp = 1;
					free(cfg[x].ops[y].value);
					cfg[x].ops[y].value = (char*)malloc(sizeof(char) * (strlen(value)+1));
					strcpy(cfg[x].ops[y].value, value);
				}
			}
			if(!foundOp)
				AddOp(cfg+x, key, value);
		}
	}
	if(!foundHeader)
	{
		cfg = (struct header*)realloc(cfg, sizeof(struct header) * (numHeaders+1));
		cfg[numHeaders].numOps = 0;
		cfg[numHeaders].ops = 0;
		cfg[numHeaders].header = (char*)malloc(sizeof(char) * (strlen(header)+1));
		strcpy(cfg[numHeaders].header, header);
		AddOp(cfg+numHeaders, key, value);
		numHeaders++;
	}
}

void CfgSetS(const char *key, char *value)
{
	char *header;
	header = HeaderPart(key);
	CfgAdd(header, OptionPart(key), value);
	free(header);
}

void CfgSetIp(const char *header, const char *option, int value)
{
	char *s;
	s = (char*)malloc(IntLen(value) + 1);
	sprintf(s, "%i", value);
	CfgAdd(header, option, s);
	free(s);
}

void CfgSetI(const char *key, int value)
{
	char *header;
	header = HeaderPart(key);
	CfgSetIp(header, OptionPart(key), value);
	free(header);
}

/*void CfgSetF(const char *key, float value)
{
	char *header;
	char *s;
	s = MallocString("%f", value);
	header = HeaderPart(key);
	CfgAdd(header, OptionPart(key), s);
	free(header);
	free(s);
}*/

/* Copy the CfgS of header, option into a new string
 * return value must be freed
 */
char *CfgSCpy(const char *header, const char *option)
{
	char *s;
	char *t;
	t = CfgSp(header, option);
	s = (char*)malloc(strlen(t) + 1);
	strcpy(s, t);
	return s;
}

char *CfgSp(const char *header, const char *option)
{
	int x, y;
	for(x=0;x<numHeaders;x++) {
		if(strcmp(cfg[x].header, header) == 0) {
			for(y=0;y<cfg[x].numOps;y++) {
				if(strcmp(cfg[x].ops[y].key, option) == 0) {
					return cfg[x].ops[y].value;
				}
			}
		}
	}
	return NULL;
}

char *CfgS(const char *key)
{
	char *header;
	char *s;
	header = HeaderPart(key);
	s = CfgSp(header, OptionPart(key));
	free(header);
	return s;
}

int CfgIp(const char *header, const char *option)
{
	char *s;
	s = CfgSp(header, option);
	if(s == NULL) return 0;
	return atoi(s);
}

int CfgI(const char *key)
{
	char *s;
	s = CfgS(key);
	if(s == NULL) return 0;
	return atoi(s);
}

/*float CfgF(const char *key)
{
	char *s;
	s = CfgS(key);
	if(s == NULL) return 0.0;
	return atof(s);
}*/

int CfgEq(const char *key, const char *string)
{
	char *s;
	s = CfgS(key);
	if(s == NULL) return 0;
	if(strcmp(s, string) == 0) return 1;
	return 0;
}

int InitConfig(void)
{
	char *header;
	FILE *cfgfile;
	struct token t, eq;

	cfgfile = fopen("init.cfg", "r");
	if(cfgfile == NULL)
	{
		Error("opening config file");
		return 0;
	}

	header = (char*)malloc(5);
	strcpy(header, "null");

	while(GetToken(cfgfile, '=', &t))
	{
		switch(t.type)
		{
			case HEADER:
				free(header);
				header = t.token;
				break;
			case VALUE:
				if(!GetToken(cfgfile, 0, &eq))
				{
					Log(("Invalid format for config file. Paramaters must be <name>=<value>\n"));
				}
				CfgAdd(header, t.token, eq.token);
				free(eq.token);
				free(t.token);
				break;
			default:
				break;
		}
	}
	free(header);
	cfgInited = 1;
	printf("Configuration loaded.\n");
	return 1;
}

void QuitConfig(void)
{
	int x, y;
	FILE *f;
	if(!cfgInited) return;
	f = fopen("init.cfg", "w");
	if(f == NULL)
	{
		Error("opening 'init.cfg'");
	}
	else
	{
		Log(("Saving configuration...\n"));
		for(x=0;x<numHeaders;x++)
		{
			fprintf(f, "[%s]\n", cfg[x].header);
			free(cfg[x].header);
			for(y=0;y<cfg[x].numOps;y++)
			{
				fprintf(f, "%s=%s\n", cfg[x].ops[y].key, cfg[x].ops[y].value);
				free(cfg[x].ops[y].key);
				free(cfg[x].ops[y].value);
			}
			free(cfg[x].ops);
		}
		free(cfg);
		cfg = NULL;
		fclose(f);
		Log(("Done\n"));
	}
	printf("Config shutdown\n");
}
