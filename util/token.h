struct token {
	char *token;
	int value;
	int type;
};

int GetToken(FILE *f, char stopper, struct token *t);

#define HEADER 0
#define VALUE 1
