#ifdef LOG
#define Log(s) do {LogFile(__FILE__, __LINE__); LogMsg s;} while(0)
#define ELog(s) do {ELogFile(__FILE__, __LINE__); ELogMsg s; LogMsg s;} while(0)
#else
#define Log(s)
#define ELog(s)
#endif

void LogFile(const char *file, int line);
void LogMsg(const char *s, ...);
void ELogFile(const char *file, int line);
void ELogMsg(const char *s, ...);
void QuitLog(void);
int InitLog(void);
