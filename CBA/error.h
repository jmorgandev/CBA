#ifndef PTS_ERROR
#define PTS_ERROR
typedef unsigned int uint;

extern uint error_count;

void Error_Log(const char* s);

void Error_TypeMismatch(uint a, uint b);
void Error_NeedRegister(uint a, uint b);
void Error_NeedRegisterV(uint a);

#endif