#ifndef CBA_INSTRUCTION_H
#define CBA_INSTRUCTION_H

/*
http://devernay.free.fr/hacks/chip8/C8TECH10.HTM
http://mattmik.com/files/chip8/mastering/chip8.html
https://en.wikipedia.org/wiki/CHIP-8
*/

#include "stdafx.h"

#define TYPE_LITERAL  0x00
#define TYPE_NIBBLE   0x01
#define TYPE_REGISTER 0x02
#define TYPE_ADDRESS  0x03

enum RegisterValues {
	V0, V1, V2, V3,
	V4, V5, V6, V7,
	V8, V9, VA, VB,
	VC, VD, VE, VF,
	I, DT, ST
};

struct token {
	word value;
	uint type;
	char text[3];
};

typedef void(*op_ptr)(std::vector<token>);

struct instruction {
	op_ptr callback;
	int min_args = 0;
	int max_args = 0;
};

#define Opcode(a) void op_##a(std::vector<token> args)

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
	X(db,   1, 1)

#define X(a, x, y) Opcode(a);
BASIC_SYNTAX
#undef X

extern std::map<std::string, instruction> instruction_map;

#endif