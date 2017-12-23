#include "assembler.h"
#include "instruction.h"
#include <fstream>
#include <algorithm>
#include <unordered_map>
#include "stdafx.h"
#include "error.h"

#define CHIP8_MEMSIZE 4096
#define CHIP8_MEMSTART 512
#define MAX_ROMSIZE (CHIP8_MEMSIZE - CHIP8_MEMSTART)
#define INSTRUCTION_SIZE 2
#define LITERAL_SIZE 1

#define ROM_EXTENSION ".c8"

#define EnforceValidLabel(a, act) if(!ValidLabelName(a)) {Error_InvalidLabelName(a); act;}
#define EnforceDirectiveArgs(dir, a, b, act) if(a != b) {Error_DirectiveArgs(dir, b, a); act;}
#define EnforceUndefinedAlias(ali, act) if(AliasExists(ali)) { Error_AliasDefined(ali##.c_str()); act;}

static char rom_output[MAX_ROMSIZE];
static uint rom_index = 0;
static uint line_number = 1;

static std::fstream source_file;
static std::fstream binary_file;
static std::string  bin_file_name;

std::map<std::string, short> labels;
std::unordered_map<std::string, std::string> aliases;

bool IsComment(std::string str) {
	return (str[0] == ';');
}

bool IsAlphaNumeric(char c) {
	return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool IsNumeric(char c) {
	return c >= '0' && c <= '9';
}
bool IsNumeric(std::string str) {
	for (const char& c : str) {
		if (!IsNumeric(c)) return false;
	}
	return true;
}

bool IsRegister(std::string str) {
	for (uint i = 0; i < REGISTER_COUNT; i++)
		if (str == reg_names[i]) return true;
	return false;
}
uint RegisterValue(std::string str) {
	for (uint i = 0; i < REGISTER_COUNT; i++) {
		if (str == reg_names[i]) return i;
	}
	return 0;
}

bool LabelExists(std::string label) {
	return (labels.find(label) != labels.end());
}

bool AliasExists(std::string name) {
	return (aliases.find(name) != aliases.end());
}

bool ValidBinaryLiteral(std::string str) {
	if (str.size() != 4 && str.size() != 8) return false;
	for (const char& c : str)
		if (c != '0' && c != '1') return false;
	return true;
}
uint GetBinaryValue(std::string str) {
	uint result = 0;
	for (const char& c : str)
		result = (result << 1) + (c == '1') ? 1 : 0;
	return result;
}

bool ValidHexLiteral(std::string str) {
	if ((str.size() > 5 && str.size < 2) || str[0] != '$') return false;
	for (const char& c : str) {
		if (c == '$') continue;
		if ((c < '0' || c > '9') && (c < 'a' || c > 'f')) return false;
	}
	return true;
}
uint GetHexValue(std::string str) {
	uint result = 0;
	for (const char& c : str) {
		if (c == '$') continue;
		result = (result << 4) + (c > '9') ? c - ('a' + 0xA) : c - '0';
	}
	return result;
}

static bool ValidLiteral(std::string str) {
	return (ValidHexLiteral(str) || ValidBinaryLiteral(str));
}

static bool AllowedLabelCharacter(const char& c) {
	return (c == '_') || IsAlphaNumeric(c);
}

static bool ValidLabelName(std::string str) {
	if (uint i = str.find(':') != std::string::npos) {
		if (i != str.size() - 1) return false;
		if (std::find_if_not(str.begin(), str.end(), AllowedLabelCharacter) != str.end()) return false;
	}
	else return false;
	return true;
}

static bool ValidInstruction(std::string str) {
	return instructions.find(str) != instructions.end();
}

std::string TrimSpaces(std::string str) {
	if (str[0] == ' ') str = str.substr(str.find_first_not_of(' '));
	for (uint i = 0; i < str.size();) {
		if (str[i] == ' ' && (i == str.size() - 1 || str[i + i] == ' ' )) str.erase(i);
		else i++;
	}
	return str;
}

static bool MakeToken(std::string str, token* result) {
	if (IsRegister(str)) *result = { TYPE_REGISTER, RegisterValue(str), NULL };
	else if (LabelExists(str)) *result = { TYPE_LITERAL, labels[str], LITERAL_12 };
	else if (ValidBinaryLiteral(str)) *result = { TYPE_LITERAL, GetBinaryValue(str), str.size() };
	else if (ValidHexLiteral(str)) *result = { TYPE_LITERAL, GetHexValue(str), str.size() * 4 };
	else if (AliasExists(str)) return MakeToken(aliases[str], result);
	else return false;
	return true;
}

static std::vector<std::string> StringSplit(std::string str, std::string seperators) {
	std::vector<std::string> result;
	while (!str.empty()) {
		uint i = str.find_first_of(seperators);
		if (i != std::string::npos) {
			result.push_back(str.substr(0, i));
			str = str.substr(i + 1);
		}
		else {
			result.push_back(str);
			str = "";
		}
	}
	return result;
}

static std::vector<std::string> Tokenize_Line(std::string line) {
	std::vector<std::string> result;
	std::string token;
	uint prev = 0;
	uint i = 0;
	
	//@TODO: Make more robust whitespace handling
	for (; i < line.size(); ++i) {
		if (line[i] == ' ') {
			token = line.substr(prev, i - prev);
			if (!token.empty())	result.push_back(token);
			prev = i + 1;
		}
		else if (line[i] == '#') break;
	}

	token = line.substr(prev, i - prev);
	if (!token.empty()) result.push_back(line.substr(prev));
	return result;
}

bool Valid_Address(short addr) {
	return (addr < CHIP8_MEMSIZE);
}

bool Valid_Label(std::string token) {
	size_t label_end = std::string::npos;
	for (int i = 0; i < token.size(); ++i) {
		if (token[i] == ':') {
			if (label_end != std::string::npos) return false;
			label_end = i;
		}
		else if (token[i] == '#') break;
	}
	return label_end != std::string::npos;
}

bool Valid_Instruction(std::string token) {
	return instructions.find(token) != instructions.end();
}

bool Valid_Hex(char c) {
	return (c >= '0' && c <= '9' || c >= 'a' && c <= 'f');
}
bool Valid_Hex(std::string token) {
	if (token.size() != 1 && token.size() != 2) return false;
	for (const char& c : token) {
		if (!Valid_Hex(c)) return false;
	}
	return true;
}
bool Valid_HexLiteral(std::string token) {
	return (Valid_Hex(token) && token.size() == 2);
}

bool Valid_Binary(std::string token) {
	if (token.size() != 4 && token.size() != 8) return false;
	for (const char& c : token) {
		if (c != '0' && c != '1') return false;
	}
	return true;
}
bool Valid_BinaryLiteral(std::string token) {
	return (Valid_Binary(token) && token.size() == 8);
}

byte Token_Byte(std::string token) {
	byte result = 0;
	if (Valid_BinaryLiteral(token)) {
		for (int i = 0; i < 8; ++i) {
			result = result << 1;
			result = result | (token[i] - 48);
		}
	}
	else if (Valid_HexLiteral(token)) result = (Char_Hex(token[0]) << 4) | Char_Hex(token[1]);
	return result;
}
byte Token_Nibble(std::string token) {
	byte result = 0;
	if (Valid_Binary(token)) {
		for (int i = 0; i < 4; i++) {
			result = result << 1;
			result = result | (token[i] - 48);
		}
	}
	else if (Valid_Hex(token)) result = Char_Hex(token[0]);
	return result;
}
byte Char_Hex(char c) {
	return c - ((c <= '9') ? 48 : 87);
}
bool Valid_Register(std::string token) {
	if (token.size() == 0 || token.size() > 2) return false;
	if (token[0] == 'v') return (Valid_Hex(token[1]));
	else return (token == "dt" || token == "st" || token == "i");
}
bool Valid_Register_VX(std::string token) {
	if (token.size() == 0 || token.size() > 2) return false;
	return (token[0] == 'v' && Valid_Hex(token[1]));
}

void PrintLineNumber() {
	printf("[Line %i (0x%X)] ", line_number, rom_index + CHIP8_MEMSTART);
}


bool ASM_Begin(std::string path) {
	std::memset(rom_output, NULL, MAX_ROMSIZE);
	
	source_file.open(path, std::fstream::in);
	if (!source_file.is_open()) {
		printf("Could not open source file: \"%s\"\n", path.c_str());
		return false;
	}
	size_t ext = path.find_last_of('.');
	bin_file_name = path.substr(0, ext) + ROM_EXTENSION;
	return true;
}

void ASM_Process() {
	rom_index = 0;
	std::string linefeed;
	while (std::getline(source_file, linefeed)) {
		if (!linefeed.empty()) {
			std::transform(linefeed.begin(), linefeed.end(), linefeed.begin(), ::tolower);
			linefeed = TrimSpaces(linefeed);
			std::vector<std::string> tokens = StringSplit(linefeed, " ,");
			
			if (tokens[0][0] == '.') {
				//Process directive
				if (tokens[0] == ".alias") {
					EnforceDirectiveArgs(".alias", 2, tokens.size() - 1, continue);
					EnforceUndefinedAlias(tokens[1], continue);
					aliases[tokens[1]] = tokens[2];
				}
			}
			else if (tokens[0].find(':') != std::string::npos) {
				//Process label
				EnforceValidLabel(tokens[0].c_str(), continue);
				labels[tokens[0]] = rom_index + CHIP8_MEMSTART;
			}
			else if (ValidInstruction(tokens[0])) rom_index += instructions[tokens[0]].out_size;
		}
		line_number++;
	}
}

void ASM_Build() {
	std::string linefeed;
	rom_index = 0;
	while (std::getline(source_file, linefeed)) {
		if (!linefeed.empty()) {

		}
	}
}

bool ASM_BuildEX() {
	std::string linefeed;
	while (std::getline(source_file, linefeed)) {
		if (linefeed.empty()) {
			line_num++;
			continue;
		}
		std::transform(linefeed.begin(), linefeed.end(), linefeed.begin(), ::tolower);
		auto tokens = Tokenize_Line(linefeed);
		if (!tokens.empty() && !Valid_Label(tokens[0])) {
			std::string first = tokens[0];
			if (Valid_Instruction(first)) {
				instruction inst = instruction_map[first];
				size_t arg_count = tokens.size() - 1;
				if (arg_count < inst.min_args || arg_count > inst.max_args) {
					if (inst.min_args == inst.max_args) {
						Print_Error_Location();
						printf("\"%s\" takes exactly %i argument%c\n", first.c_str(), inst.min_args, (inst.min_args > 1) ? 's' : '\0');
					}
					else {
						Print_Error_Location();
						printf("\"%s\" takes between %i and %i argument%c\n", first.c_str(), inst.min_args, inst.max_args, (inst.max_args > 1) ? 's' : '\0');
					}
				}
				else {
					std::vector<std::string> args(tokens.begin() + 1, tokens.end());
					inst.callback(args);
				}
			}
			else {
				Print_Error_Location();
				printf("Invalid instruction \"%s\"\n", first.c_str());
			}
		}
		line_num++;
	}

	if (error_count > 0) return false;

	binary_file.open(bin_file_name, std::fstream::in | std::fstream::binary | std::fstream::trunc);
	if (!binary_file.is_open()) {
		binary_file.open(bin_file_name, std::fstream::out | std::fstream::binary);
		if (!binary_file.is_open()) {
			printf("Could not create/open binary file: \"%s\"\n", bin_file_name.c_str());
			return false;
		}
	}
	binary_file.write(rom_output, rom_index);
	printf("Wrote %i bytes to %s (%i bytes remaining)\n", rom_index, bin_file_name.c_str(), MAX_ROMSIZE - rom_index);
	return true;
}

void Byte_Output(byte in) {
	if (rom_index + 1 > MAX_ROMSIZE) {
		Print_Error_Location();
		printf("ROM space limit reached.\n");
	}
	else rom_output[rom_index++] = in;
}
void Word_Output(word in) {
	Word_Output(in >> 8, in & 0x00FF);
}
void Word_Output(byte upper, byte lower) {
	if (rom_index + 2 > MAX_ROMSIZE) {
		Print_Error_Location();
		printf("ROM space limit reached.\n");
	}
	else {
		rom_output[rom_index++] = upper;
		rom_output[rom_index++] = lower;
	}
}