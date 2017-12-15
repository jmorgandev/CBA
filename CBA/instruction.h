#ifndef CBA_INSTRUCTION_H
#define CBA_INSTRUCTION_H

/*
http://devernay.free.fr/hacks/chip8/C8TECH10.HTM
http://mattmik.com/files/chip8/mastering/chip8.html
https://en.wikipedia.org/wiki/CHIP-8
*/

#include "stdafx.h"

typedef void(*op_ptr)(std::vector<std::string>);

struct instruction {
	op_ptr callback;
	int min_args = 0;
	int max_args = 0;
};

#define Opcode(a) void op_##a(std::vector<std::string> args)

#define CBA_MNEMONICS \
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
	X(shr,  1, 1)\
	X(subn, 2, 2)\
	X(shl,  1, 1)\
	X(rand,  2, 2)\
	X(draw,  3, 3)\
	X(skp,  1, 1)\
	X(sknp, 1, 1)\
	X(wkp,  1, 1)

#define X(a, x, y) Opcode(a);
CBA_MNEMONICS
#undef X
void op_rawbyte(std::string token);

extern std::map<std::string, instruction> instruction_map;

#endif