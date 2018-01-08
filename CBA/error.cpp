#include "error.h"
#include "assembler.h"
#include <sstream>

uint format_width;
std::string error_file;

static bool hold_error = false;
static std::string held_error;

std::list<std::string> error_list;

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

void HoldNextError() {
	hold_error = true;
	held_error.clear();
}
void CommitHeldError() {
	hold_error = false;
	error_list.push_back(held_error);
}

std::string GetErrorLocation() {
	char buffer[256] = { 0 };
	if (error_file != file_trace.back()) {
		error_file = file_trace.back();
		sprintf_s(buffer, "\n(%s)\n", error_file.c_str());
	}
	uint len = strlen(buffer);
	sprintf_s(buffer + len, 256 - len, "   Line %*i: ", format_width, line_number);
	return std::string(buffer);
}

void PushError(const char* fmt, ...) {
	va_list args;
	char buffer[512];
	va_start(args, fmt);
	uint len = vsprintf_s(buffer, fmt, args);
	va_end(args);
	std::string complete_error = GetErrorLocation() + std::string(buffer) + "\n";

	if (hold_error && held_error.empty()) {
		held_error = complete_error;
	}
	else error_list.push_back(complete_error);
}

void PrintAllErrors() {
	for (const auto& err : error_list) {
		printf(err.c_str());
	}
	printf("\nTotal Errors: %i\n", error_list.size());
}