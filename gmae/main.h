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

/** Contains functions to display a scene
 */
struct scene {
	int (*InitScene)(void);  /**< Pointer to the init function */
	void (*QuitScene)(void); /**< Pointer to the quit function */
	void (*Render)(void);    /**< Pointer to the runtime function */
};

/** Contains data to display a menu
 */
struct menu {
	int (*InitMenu)(void);  /**< Pointer to the init function */
	void (*QuitMenu)(void); /**< pointer to the quit function */
	void (*Render)(void);   /**< Pointer to the runtime function */
	int back;               /**< Refers to the 'previous' menu */
};

extern int quit;
extern struct menu *activeMenu;
extern struct scene *activeScene;
