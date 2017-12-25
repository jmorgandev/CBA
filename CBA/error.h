#ifndef PTS_ERROR
#define PTS_ERROR
typedef unsigned int uint;

struct instruction;

extern uint error_count;

extern const char* type_names[];
extern const char* reg_names[];

void Error_Log(const char* s);

void Error_TypeMismatch(uint type_expected, uint type_found);

void Error_NeedRegister(uint reg_expected, uint reg_found);

void Error_NeedRegisterV(uint reg_found);

void Error_SizeMismatch(uint size_expected, uint size_found);

void Error_InvalidLabelName(const char* name);

void Error_DirectiveArgs(const char* dir, uint args_expected, uint args_found);

void Error_AliasDefined(const char* name);

void Error_UnknownIdentifier(const char* name);

void Error_InvalidToken(const char* tkn);

void Error_InvalidDirective(const char* dir);

void Error_InstructionArgs(const char* name, uint min, uint max, uint args);

void Error_NoSourceFile(const char* file);

void Error_FileOutputFail(const char* file);

void Error_SizeLimitReached();

#endif