typedef struct {
	int (*InitScene)(void);
	void (*QuitScene)(void);
	void (*Render)(void);
	} Scene;

typedef struct {
	int (*InitMenu)(void);
	void (*QuitMenu)(void);
	void (*Render)(void);
	int back;
	} Menu;

extern int quit;
extern Menu *activeMenu;
extern Scene *activeScene;
