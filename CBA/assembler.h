#ifndef CBA_ASSEMBLER_H
#define CBA_ASSEMBLER_H
#include "stdafx.h"
#include <fstream>

extern uint line_number;
extern std::vector<std::string> file_trace;

void ASM_Begin(std::string path);
void ASM_FirstPass(std::ifstream& file);
void ASM_SecondPass();
void ASM_WriteToFile();

void Byte_Output(byte in);
void Word_Output(word in);
void Word_Output(byte upper, byte lower);

void ASM_Compile(const char* path);
#endif