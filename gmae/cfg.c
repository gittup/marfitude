/*
   Marfitude
   Copyright (C) 2005 Mike Shal

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

#include "util/memtest.h"
#include "util/fatalerror.h"
#include "util/token.h"
#include "util/strfunc.h"

/** @file
 * Provides access to read/write the configuration file
 */

/** Maps a configuration key to a value */
struct option {
	char *key;   /**< The key to the option */
	char *value; /**< The value of the option */
};

/** Contains a header and a list of options */
struct header {
	char *header;       /**< The name of the [header] in the .cfg file */
	struct option *ops; /**< The list of options in this section */
	int numOps;         /**< The number of the struct options */
};

static const char *OptionPart(const char *s);
static char *HeaderPart(const char *s);
static void AddOp(struct header *h, const char *key, const char *value);
static void CfgAdd(const char *header, const char *key, const char *value);
static int LoadConfig(const char *filename);
static int SaveConfig(const char *filename);

static struct header *cfg = NULL;
static int numHeaders = 0;
static int cfgInited = 0;
static char *cfgFileName = NULL;

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
	h = malloc(sizeof(char) * (x+1));
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

void AddOp(struct header *h, const char *key, const char *value)
{
	h->ops = (struct option*)realloc(h->ops, sizeof(struct option) * (h->numOps+1));
	h->ops[h->numOps].key = malloc(sizeof(char) * (strlen(key)+1));
	h->ops[h->numOps].value = malloc(sizeof(char) * (strlen(value)+1));
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
					cfg[x].ops[y].value = malloc(sizeof(char) * (strlen(value)+1));
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
		cfg[numHeaders].header = malloc(sizeof(char) * (strlen(header)+1));
		strcpy(cfg[numHeaders].header, header);
		AddOp(cfg+numHeaders, key, value);
		numHeaders++;
	}
}

/** Sets the configuration @a key to the new string @a value */
void CfgSetS(const char *key, char *value)
{
	char *header;
	header = HeaderPart(key);
	CfgAdd(header, OptionPart(key), value);
	free(header);
}

/** Sets the configuration header.option to the new integer @a value */
void CfgSetIp(const char *header, const char *option, int value)
{
	char *s;
	s = malloc(IntLen(value) + 1);
	sprintf(s, "%i", value);
	CfgAdd(header, option, s);
	free(s);
}

/** Sets the configuration @a key to the new integer @a value */
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

/** Copy the CfgS of header.option into a new string.
 * @return The value of the configuration option, which must be freed.
 */
char *CfgSCpy(const char *header, const char *option)
{
	char *s;
	char *t;
	t = CfgSp(header, option);
	s = malloc(strlen(t) + 1);
	strcpy(s, t);
	return s;
}

/** Returns a pointer to the configuration value for the header.option key */
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

/** Returns a pointer to the configuration value for the @a key */
char *CfgS(const char *key)
{
	char *header;
	char *s;
	header = HeaderPart(key);
	s = CfgSp(header, OptionPart(key));
	free(header);
	return s;
}

/** Returns the integer value of the header.option key */
int CfgIp(const char *header, const char *option)
{
	char *s;
	s = CfgSp(header, option);
	if(s == NULL) return 0;
	return atoi(s);
}

/** Returns the integer value for the @a key */
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

/** Returns 1 if the value of the configuration @a key is equal to @a string */
int CfgEq(const char *key, const char *string)
{
	char *s;
	s = CfgS(key);
	if(s == NULL) return 0;
	if(strcmp(s, string) == 0) return 1;
	return 0;
}

/** Loads a the configuration from @a filename
 * @param filename The configuration file to load
 * @return 0 on success, error otherwise
 */
int LoadConfig(const char *filename)
{
	char *header;
	FILE *cfgfile;
	struct token t, eq;

	if(filename == NULL) return 1;

	cfgfile = fopen(filename, "r");
	if(cfgfile == NULL)
	{
		ELog(("Couldn't open config file '%s'\n", filename));
		Error("opening config file");
		return 2;
	}

	header = malloc(5);
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
	return 0;
}

int SaveConfig(const char *filename)
{
	int x, y;
	FILE *f;

	if(filename == NULL) return 0;

	f = fopen(filename, "w");
	if(f == NULL)
	{
		ELog(("Error opening config file for write: %s\n", filename));
		Error("saving config file");
		return 0;
	}

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
	return 1;
}

/** Initializes the configuration from a file */
int InitConfig(void)
{
	char *homedir;

	homedir = getenv("HOME");
	if(homedir != NULL) {
		/* this is freed in QuitConfig() */
		cfgFileName = malloc(sizeof(char) * (strlen(homedir) + 16));
		strcpy(cfgFileName, homedir);
		strcat(cfgFileName, "/.marfitude.cfg");
	}

	if(LoadConfig(cfgFileName)) {
		if(LoadConfig("init.cfg")) {
			ELog(("Error: Couldn't load '%s' or init.cfg config files.", cfgFileName));
			return 1;
		}
		else
		{
			free(cfgFileName);
			cfgFileName = malloc(strlen("init.cfg") + 1);
			strcpy(cfgFileName, "init.cfg");
		}
	}

	printf("%s loaded.\n", cfgFileName);

	cfgInited = 1;
	return 0;
}

/** Saves the new configuration (if it was changed by the program) and frees
 * all memory used to hold the data.
 */
void QuitConfig(void)
{
	if(!cfgInited) return;

	if(cfgFileName == NULL) {
		ELog(("Warning: Configuration not saved since there is no HOME environment variable set.\n"));
	} else {
		SaveConfig(cfgFileName);
		free(cfgFileName);
	}
	printf("Config shutdown\n");
}
