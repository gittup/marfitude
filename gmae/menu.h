int SwitchMenu(int menu);

// NULLMENU used to shutdown, NOMENU used when there is no active menu
// (NOMENU still has the escape key event registered
#define NULLMENU 0
#define NOMENU 1
#define MAINMENU 2
#define FIGHTMENU 3
#define CONFIGMENU 4
#define QUITMENU 5

extern int menuActive;
