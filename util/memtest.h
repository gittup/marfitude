#ifdef DEBUG_MEM
	#define malloc(x) MyMalloc(x, __LINE__, __FILE__)
	#define realloc(p, x) MyRealloc(p, x, __LINE__, __FILE__)
	#define free(p) MyFree(p, __LINE__, __FILE__)
	#define calloc(x, y) MyCalloc(x, y, __LINE__, __FILE__)
#endif

void *MyMalloc(size_t x, int line, char *file);
void MyFree(void *p, int line, char *file);
void *MyRealloc(void *p, int x, int line, char *file);
void *MyCalloc(size_t nm, size_t x, int line, char *file);
void CheckMemUsage(); // checks all blocks to see if they're active
int QueryMemUsage();  // returns current mem usage in bytes
