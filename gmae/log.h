#ifdef LOG
	#define Log(...) LogWrite(__FILE__, __LINE__, __VA_ARGS__)
	#define ELog(...) ELogWrite(__FILE__, __LINE__, __VA_ARGS__)
#else
	#define Log(...)
	#define ELog(...)
#endif

void LogWrite(char *file, int line, char *s, ...);
void ELogWrite(char *file, int line, char *s, ...);
void QuitLog();
int InitLog();
