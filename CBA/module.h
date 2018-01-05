#ifndef CBA_MODULE_H
#define CBA_MODULE_H
#pragma once
#include <vector>
#include "stdafx.h"
#include "opcode.h"

typedef bool(*module_proc)(std::string);

struct module {
	std::string name;
	module_proc proc;
	std::map<std::string, opcode> opcodes;
	std::map<std::string, dir_ptr> directives;
};

bool DeclareModule(module mod);
bool LoadModule(std::string name);
bool ModuleLoaded(std::string name);

bool PassLineToModules(std::string line);
bool PassLineToModule(module mod, std::string line);
#endif