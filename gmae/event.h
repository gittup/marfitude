typedef struct {
	int type; /* -1 for keybd, 0-n for joysticks */
	int button;	/* keysym.sym for keybd, button # for joystick button
			 * 1 if axis>0, -1 if axis<0 */
	int axis; /* -1 if no axis, >=0 if this was an axis */
	} JoyKey;

typedef void (*EventHandler)(void);
typedef void (*KeyHandler)(JoyKey *);

void ClearEvents(void);
void EventLoop(void);
void EventMode(int mode);
void FireEvent(int event);
int ConfigureJoyKey(void); /* initialies joykeys from config file */
char *JoyKeyName(int button); /* button defined B_ below (caller must free string) */
int SetButton(int b, JoyKey *jk);
int RegisterKeyEvent(KeyHandler handler);
void DeregisterKeyEvent(void);
int RegisterEvent(int event, EventHandler handler, int stopHere);
void DeregisterEvent(int event, EventHandler handler);

/* KEY mode allows raw key input - eg. for configuring */
/* MENU mode is the same as GAME but also allows some default keys */
/*  (eg. escape key is always the "menu" key, enter functions as a "button" key */
/* GAME mode is based solely on the config file */
#define KEY 0
#define MENU 1
#define GAME 2

#define EVENTTYPE_MULTI 0 /* allows already registered events to fire */
#define EVENTTYPE_STOP 1 /* stops all future events from firing, until deregistered */

/* first B_LAST events correspond to the buttons below */
typedef enum {
	EVENT_UP, EVENT_DOWN, EVENT_LEFT, EVENT_RIGHT,
	EVENT_BUTTON1, EVENT_BUTTON2, EVENT_BUTTON3, EVENT_BUTTON4,
	EVENT_MENU,
	EVENT_ENTER, EVENT_SHOWMENU, EVENT_HIDEMENU,
	EVENT_PAGEUP, EVENT_PAGEDOWN,
	EVENT_LAST} EventType;

typedef enum {
	B_UP = 0, B_DOWN, B_LEFT, B_RIGHT,
	B_BUTTON1, B_BUTTON2, B_BUTTON3, B_BUTTON4,
	B_MENU,
	B_LAST} ButtonType;

#define JK_KEYBOARD -1
#define JK_JOYBUTTON -1
