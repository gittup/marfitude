void CfgSetS(const char *key, char *value);
void CfgSetI(const char *key, int value);
/*void CfgSetF(const char *key, float value);*/
char *CfgS(const char *key);
int CfgI(const char *key);
/*float CfgF(const char *key);*/
int CfgEq(const char *key, const char *string);
int InitConfig(void);
void QuitConfig(void);
