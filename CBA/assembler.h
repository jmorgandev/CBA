#ifndef CBA_ASSEMBLER_H
#define CBA_ASSEMBLER_H

#include "types.h"

extern std::map<std::string, short> labels;
extern uint rom_index;
extern uint line_num;
extern uint error_count;

void Print_Error_Location(uint ln, uint addr);

bool Label_Exists(std::string label);
bool Valid_Address(short addr);
bool Valid_Register(std::string token);
bool Valid_Register_VX(std::string token);
bool Valid_BinaryLiteral(std::string token);
bool Valid_HexLiteral(std::string token);
bool Valid_Binary(std::string token);
bool Valid_Hex(std::string token);

byte Token_Byte(std::string token);
byte Token_Nibble(std::string token);
byte Char_Hex(char c);

bool ASM_Begin(std::string path);
void ASM_ProcessLabels();
bool ASM_Build();
#endif