#include <stdio.h>

void rot13(char *s)
{
	int x = 0;
	while(s[x])
	{
		if(s[x] == ' ')
		{
			x++;
			continue;
		}
		if((s[x] >= 'A' && s[x] <= 'M') || (s[x] >= 'a' && s[x] <= 'm')) s[x] += 13;
		else if((s[x] >= 'N' && s[x] <= 'Z') || (s[x] >= 'n' && s[x] <= 'z')) s[x] -= 13;
		x++;
	}
	printf("%s ", s);
}

int main(int argc, char **argv)
{
	int x = 0;
	if(argc < 2)
	{
		printf("Usage: rot13 [text]");
	}
	for(x=1;x<argc;x++)
		rot13(argv[x]);
	printf("\n");
}
