#include "error.h"
#include "instruction.h"
#include "assembler.h"

uint error_count = 0;

static void ErrorBegin() {
	PrintLineNumber();
	error_count++;
}

void Error_Log(const char* s) {
	ErrorBegin();
	printf("Error: %s\n", s);
}

void Error_TypeMismatch(uint type_expected, uint type_found) {
	ErrorBegin();
	printf("Type Error: Expected %s, found %s.\n", type_names[type_expected], type_names[type_found]);
}

void Error_NeedRegister(uint reg_expected, uint reg_found) {
	ErrorBegin();
	printf("Register Error: Expected %s, found %s.\n", reg_names[reg_expected], reg_names[reg_found]);
}

void Error_NeedRegisterV(uint reg_found) {
	ErrorBegin();
	printf("Register Error: Expected V[0-F], found %s.\n", reg_names[reg_found]);
}

void Error_SizeMismatch(uint size_expected, uint size_found) {
	ErrorBegin();
	printf("Size Error: Expected %d-bit literal, found %d-bit literal.\n", size_expected, size_found);
}

void Error_InvalidLabelName(const char* name) {
	ErrorBegin();
	printf("Label Error: Invalid label name \"%s\".\n", name);
}

void Error_DirectiveArgs(const char* dir, uint args_expected, uint args_found) {
	ErrorBegin();
	printf("Directive Error: %s expected %d arguments, found %d.\n", dir, args_expected, args_found);
}

void Error_AliasDefined(const char* name) {
	ErrorBegin();
	printf("Directive Error: alias %s is already defined.\n", name);
}