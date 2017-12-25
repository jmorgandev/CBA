#ifndef CBA_ASSEMBLER_H
#define CBA_ASSEMBLER_H

#include "stdafx.h"

void PrintLineNumber();

void ASM_Begin(std::string path);
void ASM_FirstPass();
void ASM_SecondPass();
void ASM_WriteToFile();

void Byte_Output(byte in);
void Word_Output(word in);
void Word_Output(byte upper, byte lower);
#endif