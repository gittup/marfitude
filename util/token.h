typedef struct {
	char *token;
	int value;
	int type;
	} Token;

int GetToken(FILE *f, char stopper, Token *t);

#define HEADER 0
#define VALUE 1
