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

/** @file
 * Allows switching between menus.
 */

/** Contains data to display a menu */
struct menu {
	int (*InitMenu)(void);  /**< Pointer to the init function */
	void (*QuitMenu)(void); /**< pointer to the quit function */
	void (*Render)(void);   /**< Pointer to the runtime function */
	int back;               /**< Refers to the 'previous' menu */
};

const struct menu *ActiveMenu(void);
int SwitchMenu(int menu);

/** Data for the "menu" event */
struct menu_e {
	int active; /**< 1 if the menu has been activated, 0 if it has been
		     * deactivated
		     */
};

/** Used to shutdown */
#define NULLMENU 0
/** Used when there is no active menu. Unlike NULLMENU, this still has the
 * escape key event registered.
 */
#define NOMENU 1
/** The main menu that can access other menus */
#define MAINMENU 2
/** The fight selection menu */
#define FIGHTMENU 3
/** The configuration menu */
#define CONFIGMENU 4
/** The quit menu */
#define QUITMENU 5

extern int menuActive;
