#define SDLError(err) SDLFatalError(__FILE__, __LINE__, err);
void SDLFatalError(const char *, int, const char *);
