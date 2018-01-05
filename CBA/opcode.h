#ifndef CBA_OPCODE_H
#define CBA_OPCODE_H
#pragma once
#include "stdafx.h"

#define TYPE_LITERAL  0x00
#define TYPE_REGISTER 0x01

#define LITERAL_4  4
#define LITERAL_8  8
#define LITERAL_12 12
#define LITERAL_16 16

enum RegisterValues {
	V0, V1, V2, V3,
	V4, V5, V6, V7,
	V8, V9, VA, VB,
	VC, VD, VE, VF,
	I, DT, ST,
	REGISTER_COUNT
};

struct opcode {
	op_ptr callback;
	uint min_arg = 0;
	uint max_arg = 0;
};

struct token {
	uint value;
	uint type;
	uint bitcount;
};
typedef void(*op_ptr)(std::vector<token>);
typedef void(*dir_ptr)(std::vector<std::string>);

#define Opcode(a) void op_##a(std::vector<token> args)

#endif