/*
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

/** @file
 * A pretty crappy token parsing implementation
 */

/** A token structure for parsing configuration files */
struct token {
	char *token; /**< The text of the token */
	int value;   /**< The value of the token if it's a VALUE type */
	int type;    /**< Either HEADER or VALUE  */
};

int get_token(FILE *f, char stopper, struct token *t);

/** Defines a token as a header type. It has no value, only text */
#define HEADER 0

/** Defines a token as a value type. It has both text and an integer value */
#define VALUE 1
