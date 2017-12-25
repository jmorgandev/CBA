#include "error.h"
#include "assembler.h"
#include "instruction.h"

uint error_count = 0;

const char* type_names[] = {
	"literal", "register",
};
const char* reg_names[] = {
	"v0", "v1", "v2", "v3",
	"v4", "v5", "v6", "v7",
	"v8", "v9", "va", "vb",
	"vc", "vd", "ve", "vf",
	"i", "dt", "st"
};

static inline void BeginError() {
	PrintLineNumber();
	error_count++;
}
static inline void FinishError() {
	printf("\n\n");
}

void Error_Log(const char* s) {
	BeginError();
	printf("%s", s);
	FinishError();
}

void Error_TypeMismatch(uint type_expected, uint type_found) {
	BeginError();
	printf("Expected %s, found %s.", type_names[type_expected], type_names[type_found]);
	FinishError();
}

void Error_NeedRegister(uint reg_expected, uint reg_found) {
	BeginError();
	printf("Expected %s, found %s.", reg_names[reg_expected], reg_names[reg_found]);
	FinishError();
}

void Error_NeedRegisterV(uint reg_found) {
	BeginError();
	printf("Expected V[0-F], found %s.", reg_names[reg_found]);
	FinishError();
}

void Error_SizeMismatch(uint size_expected, uint size_found) {
	BeginError();
	printf("Expected %d-bit literal, found %d-bit literal.", size_expected, size_found);
	FinishError();
}

void Error_InvalidLabelName(const char* name) {
	BeginError();
	printf("Invalid label name \"%s\".", name);
	FinishError();
}

void Error_DirectiveArgs(const char* dir, uint args_expected, uint args_found) {
	BeginError();
	printf("%s expected %d arguments, found %d.", dir, args_expected, args_found);
	FinishError();
}

void Error_AliasDefined(const char* name) {
	BeginError();
	printf("alias \"%s\" is already defined.", name);
	FinishError();
}

void Error_UnknownIdentifier(const char* name) {
	BeginError();
	printf("Unknown identifier \"%s\".", name);
	FinishError();
}

void Error_InvalidToken(const char* tkn) {
	BeginError();
	printf("Invalid instruction/literal \"%s\".", tkn);
	FinishError();
}

void Error_InvalidDirective(const char* dir) {
	BeginError();
	printf("Unrecognised directive \"%s\".", dir);
	FinishError();
}

void Error_InstructionArgs(const char* name, uint min, uint max, uint args) {
	BeginError();
	if (min == max) printf("%s expected %d arguments, found %d.", name, min, args);
	else printf("%s expected %d-%d arguments, found %d.", name, min, max, args);
	FinishError();
}

void Error_NoSourceFile(const char* file) {
	BeginError();
	printf("Could not find source file \"%s\".", file);
	FinishError();
}

void Error_FileOutputFail(const char* file) {
	BeginError();
	printf("Could not create binary \"%s\"(Is this file already open?).", file);
	FinishError();
}

void Error_SizeLimitReached() {

}