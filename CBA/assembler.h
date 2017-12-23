#ifndef CBA_ASSEMBLER_H
#define CBA_ASSEMBLER_H

#include "stdafx.h"

void PrintLineNumber();
//bool LabelExists(std::string label);
//
//bool Valid_Address(short addr);
//bool Valid_Register(std::string token);
//bool Valid_Register_VX(std::string token);
//bool Valid_BinaryLiteral(std::string token);
//bool Valid_HexLiteral(std::string token);
//bool Valid_Binary(std::string token);
//bool Valid_Hex(std::string token);
//
//byte Token_Byte(std::string token);
//byte Token_Nibble(std::string token);
//byte Char_Hex(char c);

bool ASM_Begin(std::string path);
void ASM_Process();
bool ASM_Build();

void Byte_Output(byte in);
void Word_Output(word in);
void Word_Output(byte upper, byte lower);
#endif