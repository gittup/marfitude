typedef struct {
	int (*InitScene)();
	void (*QuitScene)();
	void (*Render)();
	} Scene;

typedef struct {
	int (*InitMenu)();
	void (*QuitMenu)();
	void (*Render)();
	int back;
	} Menu;

extern int quit;
extern Menu *activeMenu;
extern Scene *activeScene;
