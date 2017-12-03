#include "instruction.h"
#include "assembler.h"

#define X(a, x, y) {#a , {op_##a, x, y}},
std::map<std::string, instruction> instruction_map = { CBA_MNEMONICS };
#undef X

Opcode(cls) {
	/* Clear the display buffer */
	output[0] = 0x00;
	output[1] = 0xE0;
}

Opcode(ret) {
	/* Set the program counter to the address at the top of the stack, then subtract 1 from the stack pointer. */
	output[0] = 0x00;
	output[1] = 0xEE;
}

Opcode(jp) {
	uint nibble = 0;
	std::string label;

	if (args.size() == 2) {
		if (args[0] != "v0") {
			Print_Error_Location(line_num, rom_index);
			printf("jp can only be used with register V0\n");
			return;
		}
		/* Set the program counter to <label> address plus the value of V0 */
		nibble = 0xB0;
		label = args[1];
	}
	else {
		/* Set the program counter to the address of <label> */
		nibble = 0x10;
		label = args[0];
	}
	if (!Label_Exists(label)) {
		Print_Error_Location(line_num, rom_index);
		printf("The label \"%s\" does not exist\n", label.c_str());
		return;
	}
	if (!Valid_Address(labels[label])) {
		Print_Error_Location(line_num, rom_index);
		printf("The address of label \"%s\" is invalid\n", label.c_str());
		return;
	}
	short addr = labels[label];
	output[0] = nibble | (addr >> 8);
	output[1] = addr & 0x00FF;
}

Opcode(call) {
	/* Put the current program counter address on the top of the stack and increment the stack pointer.
	   The program counter is then set the subroutine at <label> */
	std::string label = args[0];
	if (!Label_Exists(label)) {
		Print_Error_Location(line_num, rom_index);
		printf("The label \"%s\" does not exist\n", label.c_str());
		return;
	}
	if (!Valid_Address(labels[label])) {
		Print_Error_Location(line_num, rom_index);
		printf("The address of label \"%s\" is invalid\n", label.c_str());
		return;
	}
	short addr = labels[label];
	output[0] = 0x20 | (addr >> 8);
	output[1] = addr & 0x00FF;
}

Opcode(se) {
	if (!Valid_Register_VX(args[0])) {
		Print_Error_Location(line_num, rom_index);
		printf("Invalid V register \"%s\"\n", args[0].c_str());
		return;
	}
	byte x = Char_Hex(args[0][1]);
	if (Valid_HexLiteral(args[1]) || Valid_BinaryLiteral(args[1])) {
		/* Skip next instruction if Vx == <byte literal> */
		output[0] = 0x30 | x;
		output[1] = Token_Byte(args[1]);
	}
	else if (Valid_Register_VX(args[1])) {
		/* Skip next instruction if Vx == Vy */
		byte y = Char_Hex(args[1][1]);
		output[0] = 0x50 | x;
		output[1] = Char_Hex(y) << 4;
	}
	else {
		Print_Error_Location(line_num, rom_index);
		printf("Invalid V register/byte literal \"%s\"\n", args[1].c_str());
	}
}

Opcode(sne) {
	if (!Valid_Register_VX(args[0])) {
		Print_Error_Location(line_num, rom_index);
		printf("Invalid V register \"%s\"\n", args[0].c_str());
		return;
	}
	byte x = Char_Hex(args[0][1]);
	if (Valid_HexLiteral(args[1]) || Valid_BinaryLiteral(args[1])) {
		/* Skip next instruction if Vx != <byte literal> */
		output[0] = 0x40 | x;
		output[1] = Token_Byte(args[1]);
	}
	else if (Valid_Register_VX(args[1])) {
		/* Skip next instruction if Vx != Vy */
		byte y = Char_Hex(args[1][1]);
		output[0] = 0x90 | x;
		output[1] = Char_Hex(y) << 4;
	}
	else {
		Print_Error_Location(line_num, rom_index);
		printf("Invalid V register/byte literal \"%s\"\n", args[1].c_str());
	}
}

Opcode(ld) {
	if (!Valid_Register(args[0])) {
		Print_Error_Location(line_num, rom_index);
		printf("Invalid register \"%s\"\n", args[0].c_str());
		return;
	}
	if (Valid_Register_VX(args[0])) {
		uint x = Char_Hex(args[0][1]);
		if (Valid_Register(args[1])) {
			if (Valid_Register_VX(args[1])) {
				/* Load the value of Vy into Vx */
				byte y = Char_Hex(args[1][1]);
				output[0] = 0x80 | x;
				output[1] = Char_Hex(y) << 4;
			}
			else if (args[1] == "dt") {
				/* Load the value of DT into Vx */
				output[0] = 0xF0 | x;
				output[1] = 0x07;
			}
			else if (args[1] == "i") {
				/* Load into registers V0 to Vx values starting from memory address I*/
				output[0] = 0xF0 | x;
				output[1] = 0x65;
			}
			else if (args[1] == "st") {
				Print_Error_Location(line_num, rom_index);
				printf("Register \"st\" is write-only\n");
			}
		}
		else if (Valid_BinaryLiteral(args[1]) || Valid_HexLiteral(args[1])) {
			/* Load the value <byte literal> into Vx */
			output[0] = 0x60 | x;
			output[1] = Token_Byte(args[1]);
		}
		else {
			Print_Error_Location(line_num, rom_index);
			printf("Invalid V register / byte literal \"%s\"\n", args[1].c_str());
		}
	}
	else if (args[0] == "i") {
		if (Label_Exists(args[1]) && Valid_Address(labels[args[1]])) {
			/* Load <label address> into I */
			short addr = labels[args[1]];
			output[0] = 0xA0 | (addr >> 8);
			output[1] = addr & 0x00FF;
		}
		else if (Valid_Register_VX(args[1])) {
			/* Write to memory address I with values from registers V0 to Vx */
			output[0] = 0xF0 | Char_Hex(args[1][1]);
			output[1] = 0x55;
		}
		else {
			Print_Error_Location(line_num, rom_index);
			printf("Invalid V register/label \"%s\"\n", args[1].c_str());
		}
	}
	else if (args[0] == "dt") {
		if (Valid_Register_VX(args[1])) {
			/* Load value of Vx into DT */
			output[0] = 0xF0 | Char_Hex(args[1][1]);
			output[1] = 0x15;
		}
		else {
			Print_Error_Location(line_num, rom_index);
			printf("Can only load V register into DT\n");
		}
	}
	else if (args[0] == "st") {
		if (Valid_Register_VX(args[1])) {
			/* Load value of Vx into ST */
			output[0] = 0xF0 | Char_Hex(args[1][1]);
			output[1] = 0x18;
		}
		else {
			Print_Error_Location(line_num, rom_index);
			printf("Can only load V register into ST\n");
		}
	}
}

Opcode(or) {
	/* Performs bitwise OR on Vx and Vy, stores the result in Vx */
	if (!Valid_Register_VX(args[0])) {
		Print_Error_Location(line_num, rom_index);
		printf("Invalid V register \"%s\"\n", args[0].c_str());
		return;
	}
	if (!Valid_Register_VX(args[1])) {
		Print_Error_Location(line_num, rom_index);
		printf("Invalid V register \"%s\"\n", args[1].c_str());
		return;
	}
	output[0] = 0x80 | Char_Hex(args[0][1]);
	output[1] = (Char_Hex(args[1][1]) << 4) | 0x01;
}

Opcode(and) {
	/* Performs bitwise AND on Vx and Vy, stores the result in Vx */
	if (!Valid_Register_VX(args[0])) {
		Print_Error_Location(line_num, rom_index);
		printf("Invalid V register \"%s\"\n", args[0].c_str());
		return;
	}
	if (!Valid_Register_VX(args[1])) {
		Print_Error_Location(line_num, rom_index);
		printf("Invalid V register \"%s\"\n", args[1].c_str());
		return;
	}
	output[0] = 0x80 | Char_Hex(args[0][1]);
	output[1] = (Char_Hex(args[1][1]) << 4) | 0x02;
}

Opcode(xor) {
	/* Performs bitwise XOR on Vx and Vy, stores the result in Vx */
	if (!Valid_Register_VX(args[0])) {
		Print_Error_Location(line_num, rom_index);
		printf("Invalid V register \"%s\"\n", args[0].c_str());
		return;
	}
	if (!Valid_Register_VX(args[1])) {
		Print_Error_Location(line_num, rom_index);
		printf("Invalid V register \"%s\"\n", args[1].c_str());
		return;
	}
	output[0] = 0x80 | Char_Hex(args[0][1]);
	output[1] = (Char_Hex(args[1][1]) << 4) | 0x03;
}

Opcode(add) {
	if (!Valid_Register(args[0])) {
		Print_Error_Location(line_num, rom_index);
		printf("Invalid register \"%s\"\n", args[0].c_str());
		return;
	}
	if (Valid_Register_VX(args[0])) {
		if (Valid_BinaryLiteral(args[1]) || Valid_HexLiteral(args[1])) {
			/* Set Vx to the value of Vx + <byte literal> */
			output[0] = 0x70 | Char_Hex(args[0][1]);
			output[1] = Token_Byte(args[1]);
		}
		else if (Valid_Register_VX(args[1])) {
			/* Set Vx to the value of Vx + Vy. Set VF to 1 if there is a carry, 0 if there is not */
			output[0] = 0x80 | Char_Hex(args[0][1]);
			output[1] = (Char_Hex(args[1][1]) << 4) | 0x04;
		}
		else {
			Print_Error_Location(line_num, rom_index);
			printf("Invalid V register/byte literal \"%s\"\n", args[1].c_str());
		}
	}
	else if (args[0] == "i") {
		/* Set I to memory address I + Vx */
		if (Valid_Register_VX(args[1])) {
			output[0] = 0xF0 | Char_Hex(args[1][1]);
			output[1] = 0x1E;
		}
		else {
			Print_Error_Location(line_num, rom_index);
			printf("Invalid V register \"%s\"\n", args[1].c_str());
		}
	}
	else {
		Print_Error_Location(line_num, rom_index);
		printf("add must be used with V register or I register.\n");
	}
}

Opcode(sub) {
	/* Set Vx to the value of Vx - Vy. Set VF to 0 if there is a borrow, 1 if there is not */
	if (!Valid_Register_VX(args[0])) {
		Print_Error_Location(line_num, rom_index);
		printf("Invalid V register \"%s\"\n", args[0].c_str());
		return;
	}
	if (!Valid_Register_VX(args[1])) {
		Print_Error_Location(line_num, rom_index);
		printf("Invalid V register \"%s\"\n", args[1].c_str());
		return;
	}
	output[0] = 0x80 | Char_Hex(args[0][1]);
	output[1] = (Char_Hex(args[1][1]) << 4) | 0x05;
}

/* 
 Some implementations of SHR and SHL take a Vy argument that is the source value, and stores the shifted value into Vx. 
 For now Vx is the source and destination of the value shift.
 @TODO: Add Vy argument handling
*/
Opcode(shr) {
	/* If least-significant bit of Vx is 1, set VF to 1, otherwise 0. Then bitshift Vx once to the right */
	if (!Valid_Register_VX(args[0])) {
		Print_Error_Location(line_num, rom_index);
		printf("Invalid V register \"%s\"\n", args[0].c_str());
		return;
	}
	output[0] = 0x80 | Char_Hex(args[0][1]);
	output[1] = (Char_Hex(args[0][1]) << 4) | 0x06;
}
Opcode(shl) {
	/* If most-significant bit of Vx is 1, set VF to 1, otherwise 0. Then bitshift Vx once to the left */
	if (!Valid_Register_VX(args[0])) {
		Print_Error_Location(line_num, rom_index);
		printf("Invalid V register \"%s\"\n", args[0].c_str());
		return;
	}

	output[0] = 0x80 | Char_Hex(args[0][1]);
	output[1] = (Char_Hex(args[0][1]) << 4) | 0x0E;
}

Opcode(subn) {
	/* Set Vx to the value of Vy - Vx. Set VF to 0 if there is a borrow, 1 if there is not */
	if (!Valid_Register_VX(args[0])) {
		Print_Error_Location(line_num, rom_index);
		printf("Invalid V register \"%s\"\n", args[0].c_str());
		return;
	}
	if (!Valid_Register_VX(args[1])) {
		Print_Error_Location(line_num, rom_index);
		printf("Invalid V register \"%s\"\n", args[1].c_str());
		return;
	}
	output[0] = 0x80 | Char_Hex(args[0][1]);
	output[1] = (Char_Hex(args[1][1]) << 4) | 0x07;
}

/*
 Some implementations of RND use the byte literal as a max value rather than peform bitwise AND with it.
 Regardless, this cannot really be controlled by opcodes.
*/
Opcode(rnd) {
	/* Set Vx to a random value with the range of 0 to <byte literal> */
	if (!Valid_Register_VX(args[0])) {
		Print_Error_Location(line_num, rom_index);
		printf("Invalid V register \"%s\"\n", args[0].c_str());
		return;
	}
	if (!Valid_BinaryLiteral(args[1]) && !Valid_HexLiteral(args[1])) {
		Print_Error_Location(line_num, rom_index);
		printf("Invalid byte literal \"%s\"\n", args[1].c_str());
		return;
	}
	output[0] = 0xC0 | Char_Hex(args[0][1]);
	output[1] = Token_Byte(args[1]);
}

Opcode(drw) {
	/* 
	   Display n-byte sprite starting at the memory address in I. Draw the sprite at coordinates (Vx, Vy).
	   If the sprite is drawn over any existing pixels, set VF to 1, otherwise 0.
	*/
	if (!Valid_Register_VX(args[0])) {
		Print_Error_Location(line_num, rom_index);
		printf("Invalid V register \"%s\"\n", args[0].c_str());
		return;
	}
	if (!Valid_Register_VX(args[1])) {
		Print_Error_Location(line_num, rom_index);
		printf("Invalid V register \"%s\"\n", args[1].c_str());
		return;
	}
	if (!Valid_Binary(args[2]) && !Valid_Hex(args[2])) {
		Print_Error_Location(line_num, rom_index);
		printf("Invalid nibble literal \"%s\"\n", args[2].c_str());
		return;
	}
	output[0] = 0xD0 | Char_Hex(args[0][1]);
	output[1] = (Char_Hex(args[1][1]) << 4) | Token_Nibble(args[2]);
}

Opcode(skp) {
	/* Skip next instruction if the key in Vx is pressed */
	if (!Valid_Register_VX(args[0])) {
		Print_Error_Location(line_num, rom_index);
		printf("Invalid V register \"%s\"\n", args[0].c_str());
		return;
	}
	output[0] = 0xE0 | Char_Hex(args[0][1]);
	output[1] = 0x9E;
}

Opcode(sknp) {
	/* Skip next instruction if the key in Vx is NOT pressed */
	if (!Valid_Register_VX(args[0])) {
		Print_Error_Location(line_num, rom_index);
		printf("Invalid V register \"%s\"\n", args[0].c_str());
		return;
	}
	output[0] = 0xE0 | Char_Hex(args[0][1]);
	output[1] = 0xA1;
}

Opcode(wkp) {
	/* Halt instruction execution until a key is pressed, then store the key value in Vx */
	if (!Valid_Register_VX(args[0])) {
		Print_Error_Location(line_num, rom_index);
		printf("Invalid V register \"%s\"\n", args[0].c_str());
		return;
	}
	output[0] = 0xF0 | Char_Hex(args[0][1]);
	output[1] = 0x0A;
}

void op_rawbyte(std::string token, byte* output) {
	/* Just write a raw byte to the ROM (Usually for sprites) */
	output[0] = Token_Byte(token);
}