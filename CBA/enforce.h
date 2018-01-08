#ifndef CBA_ENFORCE_H
#define CBA_ENFORCE_H
#pragma once
#include "stdafx.h"
#include "error.h"
struct token;
struct opcode;
bool EnforceType(token tkn, uint type);
bool EnforceRegister(token tkn, uint reg);
bool EnforceRegisterV(token tkn);
bool EnforceBitcount(token tkn, uint bits);
bool EnforceBitcountEx(token tkn, uint bits);
#endif