struct slist {
	struct slist *next;
	void *data;
};

typedef void (*ForeachFunc)(void *, void *);
typedef int (*CompareFunc)(const void *, const void *);

int slist_length(struct slist *l);
struct slist *slist_append(struct slist *l, void *d);
struct slist *slist_remove(struct slist *l, void *d);
struct slist *slist_nth(struct slist *l, int n);
struct slist *slist_insert_sorted(struct slist *l, void *d, CompareFunc c);
struct slist *slist_find_custom(struct slist *l, void *d, CompareFunc c);
struct slist *slist_next(struct slist *l);
void slist_foreach(struct slist *l, ForeachFunc f, void *user);
void slist_free(struct slist *l);
void slist_usage(void);
