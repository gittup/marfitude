void CfgSetS(char *key, char *value);
void CfgSetI(char *key, int value);
void CfgSetF(char *key, float value);
char *CfgS(char *key);
int CfgI(char *key);
float CfgF(char *key);
int CfgEq(char *key, char *string);
int InitConfig(void);
void QuitConfig(void);
