#include <stdio.h>
#include "assembler.h"
#include "error.h"

//@TODO: More helpful comments, before I forget any of this...

//@PLANNED FEATURES
// .include
// .sprite (??)
// multi-instruction opcodes (e.g. jnz might expand into 3 basic opcodes)
// .lock <reg> (So multi-instruction opcodes won't touch a particular register)
// .unlock <reg> (Allow multi-instruction opcodes to modify a register)


// If a multi-instruction opcode is declared, but no unlocked register is found,
// then the assembler might produce even more opcodes to store the value of a
// locked register in memory, perform the instruction, and then produce more
// opcodes to load the previous value back into the locked register.

// There might be an option in the future to prevent the assembler from doing this,
// so instead the assembler will probably just throw an error.


// Returns 0 if assembly was successful, 1 if there was an error
// (For making build tools...?)
int main(int argc, char** args) {
	printf("Chip-8 Basic Assembler (CBA) Version %s\n\n", CBA_VERSION);

	argc = 2;
	args[1] = "newTest.cba";

	if (argc != 2) {
		printf("Use source file as first argument to assemble.\n");
		printf("e.g: \"cba (game.txt/game.cba)\"\n");
		return 0;
	}
	
	ASM_Begin(args[1]);
	if (!error_list.empty()) {
		printf("\nTotal Errors: %i\n", error_list.size());
		getchar();
		return 1;
	}

	getchar();
	return 0;
}