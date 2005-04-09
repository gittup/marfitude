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
 * Handles all input from SDL.
 *
 * This file handles input from SDL (eg: keyboard, mouse, joystick) and
 * attempts to make it all somewhat generic.
 */

/** Lists the types of buttons that can be configured. This probably shouldn't
 * be here, but in the actual game code instead.
 */
enum buttonType {
	B_UP = 0, B_DOWN, B_LEFT, B_RIGHT,
	B_BUTTON1, B_BUTTON2, B_BUTTON3, B_BUTTON4,
	B_SELECT, B_SHIFT, B_MENU,
	B_LAST
};

/** Data for the "button" event.
 */
struct button_e {
	enum buttonType button; /**< The button that caused the event */
	int shift;              /**< 1 if shift is held down, 0 if not */
};

/** A joystick / keyboard wrapper struct.
 * Also functions as the data for the "key" event.
 */
struct joykey {
	int type;    /**< -1 for keybd, -2 for mouse, 0-n for joysticks */
	int button;  /**< keysym.sym for keybd, mouse button for mouse,
		      * button # for joystick button
		      * 1 if axis>0, -1 if axis<0
		      */
	int axis;    /**< -1 if no axis, >=0 if this was an axis */
};

enum event_mode {
	KEY,  /**< KEY mode allows raw key input - eg. for configuring  */
	MENU, /**< MENU mode is the same as GAME but also allows some default
	       * keys (eg. escape key is always the "menu" key, enter functions
	       * as a "button" key
	       */
	GAME  /**< GAME mode is based solely on the config file */
};

void clear_input(void);
void input_loop(void);
void input_mode(enum event_mode mode);
int configure_joykeys(void);
char *joykey_name(int button);
int set_button(int b, const struct joykey *jk);
