void CfgSetS(const char *key, char *value);
void CfgSetIp(const char *header, const char *option, int value);
void CfgSetI(const char *key, int value);
/*void CfgSetF(const char *key, float value);*/
char *CfgSCpy(const char *header, const char *option);
char *CfgSp(const char *header, const char *option);
char *CfgS(const char *key);
int CfgIp(const char *header, const char *option);
int CfgI(const char *key);
/*float CfgF(const char *key);*/
int CfgEq(const char *key, const char *string);
int InitConfig(void);
void QuitConfig(void);
