#include <stdio.h>
#include "assembler.h"

#define VERSION "1.1"

//@TODO: More helpful comments, before I forget any of this...

// Returns 0 if assembly was successful, 1 if there was an error
int main(int argc, char** args) {
	printf("Chip-8 Basic Assembler\nVersion %s\n\n", VERSION);

	if (argc != 2) {
		printf("Use source file as first argument to assemble.\n");
		printf("e.g: \"cba (game.txt/game.cba)\"\n");
		return 1;
	}

	if (ASM_Begin(args[1])) {
		ASM_ProcessLabels();
		if (!ASM_Build()) {
			printf("\nTotal errors: %i\n", error_count);
			return 1;
		}
	}
	else {
		printf("Init sequence failure.\n");
		return 1;
	}

	return 0;
}