#define Error(err) FatalError(__FILE__, __LINE__, err);
void FatalError(const char *, int, const char *);
