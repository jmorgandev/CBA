#include "assembler.h"
#include "instruction.h"
#include <fstream>
#include <algorithm>
#include "types.h"

#define CHIP8_MEMSIZE 4096
#define CHIP8_MEMSTART 512
#define MAX_ROMSIZE (CHIP8_MEMSIZE - CHIP8_MEMSTART)
#define INSTRUCTION_SIZE 2
#define LITERAL_SIZE 1

#define ROM_EXTENSION ".c8"

char rom_output[MAX_ROMSIZE];
uint rom_index = 0;
uint line_num  = 1;
uint error_count = 0;

std::fstream source_file;
std::fstream binary_file;
std::string  bin_file_name;

std::map<std::string, short> labels;

#define WRITE_SUCCESS 0x01
#define WRITE_NOSPACE 0x02

std::vector<std::string> Tokenize_Line(std::string line) {
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
	return instruction_map.find(token) != instruction_map.end();
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

uint Write_Bytes(byte* output) {
	if (rom_index + 2 > MAX_ROMSIZE) {
		return WRITE_NOSPACE;
	}
	rom_output[rom_index] = output[0];
	rom_output[rom_index + 1] = output[1];
	rom_index += INSTRUCTION_SIZE;
	return WRITE_SUCCESS;
}
uint Write_Byte(byte output) {
	if (rom_index + 1 > MAX_ROMSIZE) {
		return WRITE_NOSPACE;
	}
	rom_output[rom_index] = output;
	rom_index += LITERAL_SIZE;
	return WRITE_SUCCESS;
}

bool Label_Exists(std::string label) {
	return (labels.find(label) != labels.end());
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

void Print_Error_Location(uint ln, uint addr) {
	printf("Error at line %i (0x%X): ", ln, addr + CHIP8_MEMSTART);
	error_count += 1;
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

void ASM_ProcessLabels() {
	std::string linefeed;
	rom_index = 0;
	while (std::getline(source_file, linefeed)) {
		if (linefeed.empty()) {
			line_num++;
			continue;
		}
		std::transform(linefeed.begin(), linefeed.end(), linefeed.begin(), ::tolower);
		size_t label_end = std::string::npos;
		for (int i = 0; i < linefeed.size(); i++) {
			if (linefeed[i] == ':') {
				if (label_end != std::string::npos) {
					Print_Error_Location(line_num, rom_index);
					printf("Invalid label declaration \"%s\"\n", linefeed.substr(0, i).c_str());
					return;
				}
				label_end = i;
			}
		}
		if (label_end != std::string::npos) labels[linefeed.substr(0, label_end)] = rom_index + CHIP8_MEMSTART;
		else {
			auto tokens = Tokenize_Line(linefeed);
			if (!tokens.empty()) {
				if (Valid_Instruction(tokens[0])) rom_index += 2;
				else rom_index += 1;
			}
		}
		line_num++;
	}
	rom_index = 0;
	line_num = 1;
	source_file.clear();
	source_file.seekg(0, std::fstream::beg);
}

bool ASM_Build() {
	std::string linefeed;
	byte output[2];
	bool single_byte = false;
	bool do_output = false;
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
						Print_Error_Location(line_num, rom_index);
						printf("\"%s\" takes exactly %i argument%c\n", first.c_str(), inst.min_args, (inst.min_args > 1) ? 's' : '\0');
					}
					else {
						Print_Error_Location(line_num, rom_index);
						printf("\"%s\" takes between %i and %i argument%c\n", first.c_str(), inst.min_args, inst.max_args, (inst.max_args > 1) ? 's' : '\0');
					}
				}
				else {
					std::vector<std::string> args(tokens.begin() + 1, tokens.end());
					inst.callback(args, output);
					do_output = true;
				}
			}
			else if (Valid_BinaryLiteral(first) || Valid_HexLiteral(first)) {
				if (tokens.size() == 1) {
					op_rawbyte(first, output);
					single_byte = true;
					do_output = true;
				}
				else {
					Print_Error_Location(line_num, rom_index);
					printf("Invalid literal declaration \"%s\"\n", tokens[1].c_str());
				}
			}
			else {
				Print_Error_Location(line_num, rom_index);
				printf("Invalid instruction/literal \"%s\"\n", first.c_str());
			}
		}
		if (do_output) {
			if (single_byte) {
				if (Write_Byte(output[0]) == WRITE_NOSPACE) {
					Print_Error_Location(line_num, rom_index);
					printf("Exceeds %i byte memory size\n", MAX_ROMSIZE);
					return false;
				}
				single_byte = false;
			}
			else {
				if (Write_Bytes(output) == WRITE_NOSPACE) {
					Print_Error_Location(line_num, rom_index);
					printf("Exceeds %i byte memory size\n", MAX_ROMSIZE);
					return false;
				}
			}
			do_output = false;
		}
		line_num++;
	}
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