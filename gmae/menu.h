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

int SwitchMenu(int menu);
void ShadedBox(int, int, int, int);

/* NULLMENU used to shutdown, NOMENU used when there is no active menu */
/* (NOMENU still has the escape key event registered */
#define NULLMENU 0
#define NOMENU 1
#define MAINMENU 2
#define FIGHTMENU 3
#define CONFIGMENU 4
#define QUITMENU 5

extern int menuActive;
