struct scene {
	int (*InitScene)(void);
	void (*QuitScene)(void);
	void (*Render)(void);
};

struct menu {
	int (*InitMenu)(void);
	void (*QuitMenu)(void);
	void (*Render)(void);
	int back;
};

extern int quit;
extern struct menu *activeMenu;
extern struct scene *activeScene;
