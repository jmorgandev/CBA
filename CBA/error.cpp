#include "error.h"
#include "assembler.h"

uint error_count = 0;

const char* type_names[] = {
	"literal", "register",
};
const char* reg_names[] = {
	"v0", "v1", "v2", "v3",
	"v4", "v5", "v6", "v7",
	"v8", "v9", "va", "vb",
	"vc", "vd", "ve", "vf",
	"I", "dt", "st"
};

static void ErrorBegin() {
	PrintLineNumber();
	error_count++;
}

void Error_Log(const char* s) {
	ErrorBegin();
	printf("%s\n", s);
}

void Error_TypeMismatch(uint type_expected, uint type_found) {
	ErrorBegin();
	printf("(Type Mismatch): Expected %s, found %s.\n", type_names[type_expected], type_names[type_found]);
}

void Error_NeedRegister(uint reg_expected, uint reg_found) {
	ErrorBegin();
	printf("(Register Mismatch): Expected %s, found %s.\n", reg_names[reg_expected], reg_names[reg_found]);
}

void Error_NeedRegisterV(uint reg_found) {
	ErrorBegin();
	printf("(Register Mismatch): Expected V[0-F], found %s.\n", reg_names[reg_found]);
}

void Error_SizeMismatch(uint size_expected, uint size_found) {
	ErrorBegin();
	printf("(Size Mismatch): Expected %d-bit literal, found %d-bit literal.\n", size_expected, size_found);
}

void Error_InvalidLabelName(const char* name) {
	ErrorBegin();
	printf("(Syntax Error): Invalid label name \"%s\".\n", name);
}

void Error_DirectiveArgs(const char* dir, uint args_expected, uint args_found) {
	ErrorBegin();
	printf("(Syntax Error): %s expected %d arguments, found %d.\n", dir, args_expected, args_found);
}

void Error_AliasDefined(const char* name) {
	ErrorBegin();
	printf("(Alias Error): alias \"%s\" is already defined.\n", name);
}

void Error_UnknownIdentifier(const char* name) {
	ErrorBegin();
	printf("(Syntax Error): Unknown identifier \"%s\".\n", name);
}

void Error_InvalidToken(const char* tkn) {
	ErrorBegin();
	printf("(Syntax Error): Invalid token \"%s\".\n", tkn);
}