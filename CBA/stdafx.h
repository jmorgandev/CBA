#ifndef CBA_STDAFX
#define CBA_STDAFX

#include <vector>
#include <string>
#include <map>

typedef unsigned char byte;
typedef unsigned int uint;
typedef unsigned short word;

#define CBA_VERSION "1.1"

#define CHIP8_MEMSIZE 4096
#define CHIP8_MEMSTART 512
#define MAX_ROMSIZE (CHIP8_MEMSIZE - CHIP8_MEMSTART)
#define INSTRUCTION_SIZE 2
#define LITERAL_SIZE 1

#define ROM_EXTENSION ".c8"

#define COMMENT_SYM '#'

#endif