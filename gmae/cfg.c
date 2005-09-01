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

static void AddOp(struct header *h, const char *option, const char *value);
static int LoadConfig(const char *filename);
static int SaveConfig(const char *filename);

static struct header *cfg = NULL;
static int numHeaders = 0;
static int cfgInited = 0;
static char *cfgFileName = NULL;

void AddOp(struct header *h, const char *option, const char *value)
{
	h->ops = (struct option*)realloc(h->ops, sizeof(struct option) * (h->numOps+1));
	h->ops[h->numOps].key = malloc(sizeof(char) * (strlen(option)+1));
	h->ops[h->numOps].value = malloc(sizeof(char) * (strlen(value)+1));
	strcpy(h->ops[h->numOps].key, option);
	strcpy(h->ops[h->numOps].value, value);
	h->numOps++;
}

/** Set the @a header / @a option pair to be @a value.
 *
 * @param header The header part of the option
 * @param option The key part of the option
 * @param value The new string to set the option to
 */
void cfg_set(const char *header, const char *option, const char *value)
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
				if(strcmp(cfg[x].ops[y].key, option) == 0)
				{
					foundOp = 1;
					free(cfg[x].ops[y].value);
					cfg[x].ops[y].value = malloc(sizeof(char) * (strlen(value)+1));
					strcpy(cfg[x].ops[y].value, value);
				}
			}
			if(!foundOp)
				AddOp(cfg+x, option, value);
		}
	}
	if(!foundHeader)
	{
		cfg = (struct header*)realloc(cfg, sizeof(struct header) * (numHeaders+1));
		cfg[numHeaders].numOps = 0;
		cfg[numHeaders].ops = 0;
		cfg[numHeaders].header = malloc(sizeof(char) * (strlen(header)+1));
		strcpy(cfg[numHeaders].header, header);
		AddOp(cfg+numHeaders, option, value);
		numHeaders++;
	}
}

/** Sets the configuration header.option to the new integer @a value */
void cfg_set_int(const char *header, const char *option, int value)
{
	char *s;
	s = malloc(int_len(value) + 1);
	sprintf(s, "%i", value);
	cfg_set(header, option, s);
	free(s);
}

/** Copy the CfgS of header.option into a new string.
 *
 * @return The value of the configuration option, which must be freed.
 * @param header The header part
 * @param option The option part
 * @param unset What to return when header.option does not exist
 */
char *cfg_copy(const char *header, const char *option, const char *unset)
{
	char *s;
	const char *t;
	t = cfg_get(header, option, unset);
	s = malloc(strlen(t) + 1);
	strcpy(s, t);
	return s;
}

/** Returns a pointer to the configuration value for the header.option key
 *
 * @param header The header part
 * @param option The option part
 * @param unset What to return when header.option does not exist
 */
const char *cfg_get(const char *header, const char *option, const char *unset)
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
	return unset;
}

/** Returns the integer value of the header.option key
 *
 * @param header The header part
 * @param option The option part
 * @param unset What to return when header.option does not exist
 */
int cfg_get_int(const char *header, const char *option, int unset)
{
	const char *s;
	s = cfg_get(header, option, NULL);
	if(s == NULL)
		return unset;
	return atoi(s);
}

/** Returns 1 if the value of the configuration @a key is equal to @a string */
int cfg_eq(const char *header, const char *option, const char *string)
{
	const char *s;
	s = cfg_get(header, option, NULL);
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

	while(get_token(cfgfile, '=', &t))
	{
		switch(t.type)
		{
			case HEADER:
				free(header);
				header = t.token;
				break;
			case VALUE:
				if(!get_token(cfgfile, 0, &eq))
				{
					Log(("Invalid format for config file. Paramaters must be <name>=<value>\n"));
				}
				cfg_set(header, t.token, eq.token);
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
int init_config(void)
{
	char *homedir;

	homedir = getenv("HOME");
	if(homedir != NULL) {
		/* this is freed in quit_config() */
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
void quit_config(void)
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
