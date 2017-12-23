#ifndef CBA_INSTRUCTION_H
#define CBA_INSTRUCTION_H

/*
http://devernay.free.fr/hacks/chip8/C8TECH10.HTM
http://mattmik.com/files/chip8/mastering/chip8.html
https://en.wikipedia.org/wiki/CHIP-8
*/

#include "stdafx.h"

#define TYPE_LITERAL  0x00
#define TYPE_REGISTER 0x01

#define LITERAL_4  4
#define LITERAL_8  8
#define LITERAL_12 12
#define LITERAL_16 16

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
enum RegisterValues {
	V0, V1, V2, V3,
	V4, V5, V6, V7,
	V8, V9, VA, VB,
	VC, VD, VE, VF,
	I, DT, ST,
	REGISTER_COUNT
};

struct token {
	word value;
	uint type;
	byte bitcount;
};

typedef void(*op_ptr)(std::vector<token>);

struct instruction {
	op_ptr callback;
	uint min_arg = 0;
	uint max_arg = 0;
	uint out_size = 2;
};

#define Opcode(a) void op_##a(std::vector<token> args)

#define ARG_MAX 3

#define BASIC_SYNTAX \
	X(cls,  0, 0)\
	X(ret,  0, 0)\
	X(jp,   1, 2)\
	X(call, 1, 1)\
	X(se,   2, 2)\
	X(sne,  2, 2)\
	X(ld,   2, 2)\
	X(or ,  2, 2)\
	X(and,  2, 2)\
	X(xor,  2, 2)\
	X(add,  2, 2)\
	X(sub,  2, 2)\
	X(shr,  1, 2)\
	X(subn, 2, 2)\
	X(shl,  1, 2)\
	X(rand, 1, 2)\
	X(draw, 3, 3)\
	X(skp,  1, 1)\
	X(sknp, 1, 1)\
	X(wkp,  1, 1)\
	X(db,   1, 1, 1)

#define X(a, x, y) Opcode(a);
BASIC_SYNTAX
#undef X

extern std::map<std::string, instruction> instructions;

#endif