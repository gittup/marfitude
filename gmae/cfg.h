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

/** @file
 * Has some functions to query and set configuration options
 */

void CfgSetS(const char *key, char *value);
void CfgSetIp(const char *header, const char *option, int value);
void CfgSetI(const char *key, int value);
/*void CfgSetF(const char *key, float value);*/
char *CfgSCpy(const char *header, const char *option);
char *CfgSp(const char *header, const char *option);
char *CfgS(const char *key);
int CfgIp(const char *header, const char *option);
int CfgI(const char *key);
/*float CfgF(const char *key);*/
int CfgEq(const char *key, const char *string);
int InitConfig(void);
void QuitConfig(void);
