#ifndef CBA_ERROR_H
#define CBA_ERROR_H
#pragma once
#include "stdafx.h"
#include <stdarg.h>

extern uint format_width;
extern std::list<std::string> error_list;

extern const char* type_names[];
extern const char* reg_names[];

void PushError(const char* fmt, ...);
void PrintAllErrors();

#endif