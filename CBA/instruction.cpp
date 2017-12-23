#include "instruction.h"
#include "assembler.h"
#include "error.h"

#define EnforceType(a, b, act) if(a.type != b) {Error_TypeMismatch(b, a.value); act;}

#define EnforceRegister(a, b, act) if(a.value != b) {Error_NeedRegister(b ,a.value); act;}
#define EnforceRegisterV(a, act) if(a.value > VF) {Error_NeedRegisterV(a.value); act;}

#define EnforceLiteralSize(a, b, act) if(a.bitcount > b) {Error_SizeMismatch(b, a.count); act;}

#define X(a, x, y) {#a , {op_##a, x, y}},
std::map<std::string, instruction> instructions = { BASIC_SYNTAX };
#undef X

Opcode(cls) {
	/* Clear the display buffer */
	Word_Output(0x00E0);
}

Opcode(ret) {
	/* Set the program counter to the address at the top of the stack, then subtract 1 from the stack pointer. */
	Word_Output(0x00EE);
}

Opcode(jp) {
	EnforceType(args[0], TYPE_LITERAL, return);
	if (args.size() == 2) {
		EnforceType(args[1], TYPE_REGISTER, return);
		EnforceRegister(args[1], V0, return);
		/* Set the program counter to <address> plus the value of V0 */
		Word_Output(0xB000 | args[0].value);
	}
	else {
		/* Set the program counter to <address> */
		Word_Output(0x1000 | args[0].value);
	}
}

Opcode(call) {
	/* Put the current program counter address on the top of the stack and increment the stack pointer.
	   The program counter is then set the subroutine at <address> */
	EnforceType(args[0], TYPE_LITERAL, return);
	Word_Output(0x2000 | args[0].value);
}

Opcode(se) {
	EnforceType(args[0], TYPE_REGISTER, return);
	EnforceRegisterV(args[0], return);
	byte x = args[0].value;
	if (args[1].type == TYPE_LITERAL) {
		/* Skip next instruction if Vx == <byte literal> */
		Word_Output(0x30 | x, args[1].value);
	}
	else if (args[1].type == TYPE_REGISTER) {
		EnforceRegisterV(args[1], return);
		byte y = args[1].value;
		Word_Output(0x50 | x, y << 4);
	}
	else Error_Log("Expected V register or byte literal as second argument.");
}

Opcode(sne) {
	EnforceType(args[0], TYPE_REGISTER, return);
	EnforceRegisterV(args[0], return);
	byte x = args[0].value;

	if (args[1].type == TYPE_LITERAL) {
		/* Skip next instruction if Vx != <byte literal> */
		Word_Output(0x40 | x, args[1].value);
	}
	else if (args[1].type == TYPE_REGISTER) {
		/* Skip next instruction if Vx != Vy */
		EnforceRegisterV(args[1], return);
		byte y = args[1].value;
		Word_Output(0x90 | x, y << 4);
	}
	else Error_Log("Expected V register or byte literal as second argument.");
}

Opcode(ld) {
	EnforceType(args[0], TYPE_REGISTER, return);
	if (args[0].value <= VF) {
		uint x = args[0].value;
		if (args[1].type == TYPE_REGISTER) {
			if (args[1].value <= VF) {
				/* Load the value of Vy into Vx */
				byte y = args[1].value;
				Word_Output(0x80 | x, y << 4);
			}
			else if (args[1].value == DT) {
				/* Load the value of DT into Vx */
				Word_Output(0xF0 | x, 0x07);
			}
			else if (args[1].value == I) {
				/* Load into registers V0 to Vx values starting from memory address I*/
				Word_Output(0xF0 | x, 0x65);
			}
			else Error_Log("Register ST is write-only.");
		}
		else if (args[1].type == TYPE_LITERAL) {
			/* Load the value <byte literal> into Vx */
			Word_Output(0x60 | x, args[1].value);
		}
		else Error_Log("Expected V register or byte literal as second argument.");
	}
	else if (args[0].value == I) {
		if (args[1].type == TYPE_LITERAL) {
			/* Load <address> into I */
			Word_Output(0xA000 | args[1].value);
		}
		else if (args[1].type == TYPE_REGISTER) {
			/* Write to memory address I with values from registers V0 to Vx */
			EnforceRegisterV(args[1], return);
			Word_Output(0xF0 | args[1].value, 0x55);
		}
		else Error_Log("Expected address or V register as second argument.");
	}
	else if (args[0].value == DT) {
		/* Load value of Vx into DT */
		EnforceType(args[1], TYPE_REGISTER, return);
		EnforceRegisterV(args[1], return);
		Word_Output(0xF0 | args[1].value, 0x15);
	}
	else if (args[0].value == ST) {
		EnforceType(args[1], TYPE_REGISTER, return);
		EnforceRegisterV(args[1], return);
		Word_Output(0xF0 | args[1].value, 0x18);
	}
}

Opcode(or) {
	/* Performs bitwise OR on Vx and Vy, stores the result in Vx */
	EnforceType(args[0], TYPE_REGISTER, return);
	EnforceRegisterV(args[0], return);
	EnforceType(args[1], TYPE_REGISTER, return);
	EnforceRegisterV(args[1], return);
	Word_Output(0x80 | args[0].value, (args[1].value << 4) | 0x01);
}

Opcode(and) {
	/* Performs bitwise AND on Vx and Vy, stores the result in Vx */
	EnforceType(args[0], TYPE_REGISTER, return);
	EnforceRegisterV(args[0], return);
	EnforceType(args[1], TYPE_REGISTER, return);
	EnforceRegisterV(args[1], return);
	Word_Output(0x80 | args[0].value, (args[1].value << 4) | 0x02);
}

Opcode(xor) {
	/* Performs bitwise XOR on Vx and Vy, stores the result in Vx */
	EnforceType(args[0], TYPE_REGISTER, return);
	EnforceRegisterV(args[0], return);
	EnforceType(args[1], TYPE_REGISTER, return);
	EnforceRegisterV(args[1], return);
	Word_Output(0x80 | args[0].value, (args[1].value << 4) | 0x03);
}

Opcode(add) {
	EnforceType(args[0], TYPE_REGISTER, return);
	if(args[0].value <= VF) {
		if (args[1].type == TYPE_LITERAL) {
			/* Set Vx to the value of Vx + <byte literal> */
			Word_Output(0x70 | args[0].value, args[1].value);
		}
		else if (args[1].type == TYPE_REGISTER) {
			/* Set Vx to the value of Vx + Vy. Set VF to 1 if there is a carry, 0 if there is not */
			EnforceRegisterV(args[1], return);
			Word_Output(0x80 | args[0].value, (args[1].value << 4) | 0x04);
		}
		else Error_Log("Expected V register or byte literal as second argument.");
	}
	else if(args[0].value == I) {
		/* Set I to memory address I + Vx */
		EnforceRegisterV(args[1], return);
		Word_Output(0xF | args[1].value, 0x1E);
	}
	else Error_Log("Expected V register or I register as first argument.");
}

Opcode(sub) {
	/* Set Vx to the value of Vx - Vy. Set VF to 0 if there is a borrow, 1 if there is not */
	EnforceType(args[0], TYPE_REGISTER, return);
	EnforceRegisterV(args[0], return);
	EnforceType(args[1], TYPE_REGISTER, return);
	EnforceRegisterV(args[1], return);
	Word_Output(0x80 | args[0].value, (args[1].value << 4) | 0x05);
}

Opcode(shr) {
	EnforceType(args[0], TYPE_REGISTER, return);
	EnforceRegisterV(args[0], return);
	if (args.size() == 2) {
		EnforceType(args[1], TYPE_REGISTER, return);
		EnforceRegisterV(args[1], return);
		/* Store Vy bitshifted right into Vx. Set VF to lsb of Vy */
		Word_Output(0x80 | args[0].value, (args[1].value << 4) | 0x06);
	}
	else {
		/* Bitshift Vx to the right. Set VF to lsb of Vx before shift */
		Word_Output(0x80 | args[0].value, (args[0].value << 4) | 0x06);
	}
}
Opcode(shl) {
	EnforceType(args[0], TYPE_REGISTER, return);
	EnforceRegisterV(args[0], return);
	if (args.size() == 2) {
		EnforceType(args[1], TYPE_REGISTER, return);
		EnforceRegisterV(args[1], return);
		/* Store Vy bitshifted left into Vx. Set VF to lsb of Vy */
		Word_Output(0x80 | args[0].value, (args[1].value << 4) | 0x0E);
	}
	else {
		/* Bitshift Vx to the left. Set VF to msb of Vx before shift */
		Word_Output(0x80 | args[0].value, (args[0].value << 4) | 0x0E);
	}
}

Opcode(subn) {
	/* Set Vx to the value of Vy - Vx. Set VF to 0 if there is a borrow, 1 if there is not */
	EnforceType(args[0], TYPE_REGISTER, return);
	EnforceRegisterV(args[0], return);
	EnforceType(args[1], TYPE_REGISTER, return);
	EnforceRegisterV(args[1], return);
	Word_Output(0x80 | args[0].value, (args[1].value << 4) | 0x07);
}

/*
 Some implementations of RND use the byte literal as a max value rather than peform bitwise AND with it.
 Regardless, this cannot really be controlled by opcodes.
*/
Opcode(rand) {
	EnforceType(args[0], TYPE_REGISTER, return);
	EnforceRegisterV(args[0], return);
	if (args.size() == 2) {
		/* Set Vx to a random value with the range of 0 to <byte literal> */
		EnforceType(args[1], TYPE_LITERAL, return);
		Word_Output(0xC0 | args[0].value, args[1].value);
	}
	else {
		/* Set Vx to a random value with the range of 0 to 255 */
		Word_Output(0xC0 | args[0].value, 0xFF);
	}
}

Opcode(draw) {
	/* 
	   Display n-byte sprite starting at the memory address in I. Draw the sprite at coordinates (Vx, Vy).
	   If the sprite is drawn over any existing pixels, set VF to 1, otherwise 0.
	*/
	EnforceType(args[0], TYPE_REGISTER, return);
	EnforceRegisterV(args[0], return);
	EnforceType(args[1], TYPE_REGISTER, return);
	EnforceRegisterV(args[1], return);
	EnforceLiteralSize(args[2], LITERAL_4, return);
	Word_Output(0xD0 | args[0].value, (args[1].value) << 4 | args[2].value);
}

Opcode(skp) {
	/* Skip next instruction if the key in Vx is pressed */
	EnforceType(args[0], TYPE_REGISTER, return);
	EnforceRegisterV(args[0], return);
	Word_Output(0xE0 | args[0].value, 0x9E);
}

Opcode(sknp) {
	/* Skip next instruction if the key in Vx is NOT pressed */
	EnforceType(args[0], TYPE_REGISTER, return);
	EnforceRegisterV(args[0], return);
	Word_Output(0xE0 | args[0].value, 0xA1);
}

Opcode(wkp) {
	/* Halt instruction execution until a key is pressed, then store the key value in Vx */
	EnforceType(args[0], TYPE_REGISTER, return);
	EnforceRegisterV(args[0], return);
	Word_Output(0xF0 | args[0].value, 0x0A);
}

Opcode(db) {
	EnforceType(args[0], TYPE_LITERAL, return);
	Byte_Output(args[0].value);
}