#ifndef PTS_ERROR
#define PTS_ERROR
typedef unsigned int uint;

extern uint error_count;

void Error_Log(const char* s);

void Error_TypeMismatch(uint type_expected, uint type_found);
void Error_NeedRegister(uint reg_expected, uint reg_found);
void Error_NeedRegisterV(uint reg_found);
void Error_SizeMismatch(uint size_expected, uint size_found);

void Error_InvalidLabelName(const char* name);

void Error_DirectiveArgs(const char* dir, uint args_expected, uint args_found);

void Error_AliasDefined(const char* name);

#endif