#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "../util/memtest.h"
#include "../util/fatalerror.h"
#include "../util/token.h"
#include "../util/strfunc.h"

typedef struct {
	char *key;
	char *value;
	} Option;

typedef struct {
	char *header;
	Option *ops;
	int numOps;
	} Header;

Header *cfg = NULL;
int numHeaders = 0;
int cfgInited = 0;

char *Cat(char *header, char *value)
{
	char *ret;
	ret = (char*)malloc(strlen(header) + strlen(value) + 2);
	strcpy(ret, header);
	strcat(ret, ".");
	strcat(ret, value);
	return ret;
}

// s is "a.b", returns "a" - must be freed
char *HeaderPart(char *s)
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

// s is "a.b", returns "b", no freeing
char *OptionPart(char *s)
{
	int x = 0;
	while(s[x])
	{
		if(s[x] == '.') return s+x+1;
		x++;
	}
	return NULL;
}

int HeaderEq(char *a, char *b)
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
}

void AddOp(Header *h, char *key, char *value)
{
	h->ops = (Option*)realloc(h->ops, sizeof(Option) * (h->numOps+1));
	h->ops[h->numOps].key = (char*)malloc(sizeof(char) * (strlen(key)+1));
	h->ops[h->numOps].value = (char*)malloc(sizeof(char) * (strlen(value)+1));
	strcpy(h->ops[h->numOps].key, key);
	strcpy(h->ops[h->numOps].value, value);
	h->numOps++;
}

void CfgAdd(char *header, char *key, char *value)
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
		cfg = (Header*)realloc(cfg, sizeof(Header) * (numHeaders+1));
		cfg[numHeaders].numOps = 0;
		cfg[numHeaders].ops = 0;
		cfg[numHeaders].header = (char*)malloc(sizeof(char) * (strlen(header)+1));
		strcpy(cfg[numHeaders].header, header);
		AddOp(cfg+numHeaders, key, value);
		numHeaders++;
	}
}

void CfgSetS(char *key, char *value)
{
	char *header;
	header = HeaderPart(key);
	CfgAdd(header, OptionPart(key), value);
	free(header);
}

void CfgSetI(char *key, int value)
{
	char *header;
	char *s;
	s = MallocString("%i", value);
	header = HeaderPart(key);
	CfgAdd(header, OptionPart(key), s);
	free(header);
	free(s);
}

void CfgSetF(char *key, float value)
{
	char *header;
	char *s;
	s = MallocString("%f", value);
	header = HeaderPart(key);
	CfgAdd(header, OptionPart(key), s);
	free(header);
	free(s);
}

char *CfgS(char *key)
{
	char *header;
	int x, y;
	header = HeaderPart(key);
	for(x=0;x<numHeaders;x++)
	{
		if(strcmp(cfg[x].header, header) == 0)
		{
			for(y=0;y<cfg[x].numOps;y++)
			{
				if(strcmp(cfg[x].ops[y].key, OptionPart(key)) == 0)
				{
					free(header);
					return cfg[x].ops[y].value;
				}
			}
		}
	}
	free(header);
	return NULL;
}

int CfgI(char *key)
{
	char *s;
	s = CfgS(key);
	if(s == NULL) return 0;
	return atoi(s);
}

float CfgF(char *key)
{
	char *s;
	s = CfgS(key);
	if(s == NULL) return 0.0;
	return atof(s);
}

int CfgEq(char *key, char *string)
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
	FILE *cfg;
	Token t, eq;

	cfg = fopen("init.cfg", "r");
	if(cfg == NULL)
	{
		Error("opening config file");
		return 0;
	}

	printf("InitConfig\n");
	header = malloc(5);
	strcpy(header, "null");

	while(GetToken(cfg, '=', &t))
	{
		switch(t.type)
		{
			case HEADER:
				free(header);
				header = t.token;
				break;
			case VALUE:
				if(!GetToken(cfg, 0, &eq))
				{
					Log("Invalid format for config file. Paramaters must be <name>=<value>\n");
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
		Log("Saving configuration...\n");
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
		Log("Done\n");
	}
	printf("Config shutdown\n");
}
