#ifndef CBA_OPCODE_H
#define CBA_OPCODE_H
#pragma once
#include "stdafx.h"

/*
http://devernay.free.fr/hacks/chip8/C8TECH10.HTM
http://mattmik.com/files/chip8/mastering/chip8.html
https://en.wikipedia.org/wiki/CHIP-8
*/

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

struct token {
	uint value;
	uint type;
	uint bitcount;
};
typedef void(*op_ptr)(std::vector<token>);
typedef void(*dir_ptr)(std::vector<std::string>);

struct opcode {
	op_ptr callback;
	uint   min;
	uint   max;
};

#define Opcode(a) void op_##a(std::vector<token> args)

#define CORE_OPCODES \
	X(cls,  0   )\
	X(ret,  0   )\
	X(jp,   1, 2)\
	X(call, 1   )\
	X(se,   2   )\
	X(sne,  2   )\
	X(ld,   2   )\
	X(or,   2   )\
	X(and,  2   )\
	X(xor,  2   )\
	X(add,  2   )\
	X(sub,  2   )\
	X(shr,  1, 2)\
	X(subn, 2   )\
	X(shl,  1, 2)\
	X(rand, 1, 2)\
	X(draw, 3   )\
	X(skp,  1   )\
	X(sknp, 1   )\
	X(wkp,  1   )\
	X(fnt,  1   )\
	X(bcd,	1	)\
	X(dw,   1, 2)\
	X(db,   1   )\
	X(dbs,  1, 99)

#define X(a, x, y) Opcode(a);
CORE_OPCODES
#undef X

extern std::map<std::string, opcode> opcode_list;

#endif