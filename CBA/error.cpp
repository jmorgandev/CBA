#include "error.h"
#include "assembler.h"

const char* type_names[] = {
	"byte literal",
	"nibble literal"
	"register",
	"address"
};

const char* reg_names[] = {
	"v0", "v1", "v2", "v3",
	"v4", "v5", "v6", "v7",
	"v8", "v9", "va", "vb",
	"vc", "vd", "ve", "vf",
	"I", "dt", "st"
};

uint error_count = 0;

void Error_Log(const char* s) {
	PrintLineNumber();
	printf("Error: %s\n", s);
	error_count++;
}

void Error_TypeMismatch(uint a, uint b) {
	PrintLineNumber();
	printf("Type Error: Expected %s, found %s.\n", type_names[a], type_names[b]);
	error_count++;
}

void Error_NeedRegister(uint a, uint b) {
	PrintLineNumber();
	printf("Register Error: Expected %s, found %s.\n", reg_names[a], reg_names[b]);
	error_count++;
}

void Error_NeedRegisterV(uint a) {
	PrintLineNumber();
	printf("Register Error: Expected V[0-F], found %s", reg_names[a]);
	error_count++;
}