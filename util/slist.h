typedef struct slist {
	struct slist *next;
	void *data;
	} slist;

typedef void (*ForeachFunc)(void *, void *);
typedef int (*CompareFunc)(const void *, const void *);

int slist_length(slist *l);
slist *slist_append(slist *l, void *d);
slist *slist_remove(slist *l, void *d);
slist *slist_nth(slist *l, int n);
slist *slist_insert_sorted(slist *l, void *d, CompareFunc c);
slist *slist_find_custom(slist *l, void *d, CompareFunc c);
slist *slist_next(slist *l);
void slist_foreach(slist *l, ForeachFunc f, void *user);
void slist_free(slist *l);
