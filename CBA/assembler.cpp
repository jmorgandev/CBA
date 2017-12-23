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
static uint byte_overflow = 0;

static std::fstream source_file;
static std::fstream binary_file;
static std::string  bin_file_name;

std::map<std::string, uint> labels;
std::unordered_map<std::string, std::string> aliases;

/*****************************************/
/*										 */
/*			AUX FUNCTIONS				 */
/*                                       */
/*****************************************/
static inline bool IsComment(std::string str) {
	return (str[0] == ';');
}

static inline bool IsAlphaNumeric(char c) {
	return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static inline bool IsNumeric(char c) {
	return c >= '0' && c <= '9';
}

static bool IsNumeric(std::string str) {
	for (const char& c : str) {
		if (!IsNumeric(c)) return false;
	}
	return true;
}

static bool IsRegister(std::string str) {
	for (uint i = 0; i < REGISTER_COUNT; i++)
		if (str == reg_names[i]) return true;
	return false;
}

static inline bool LabelExists(std::string label) {
	return (labels.find(label) != labels.end());
}

static inline bool AliasExists(std::string name) {
	return (aliases.find(name) != aliases.end());
}

static bool ValidBinaryLiteral(std::string str) {
	if (str.size() != 4 && str.size() != 8) return false;
	for (const char& c : str)
		if (c != '0' && c != '1') return false;
	return true;
}

static bool ValidHexLiteral(std::string str) {
	if ((str.size() > 5 && str.size() < 2) || str[0] != '$') return false;
	for (const char& c : str) {
		if (c == '$') continue;
		if ((c < '0' || c > '9') && (c < 'a' || c > 'f')) return false;
	}
	return true;
}

static inline bool ValidLiteral(std::string str) {
	return (ValidHexLiteral(str) || ValidBinaryLiteral(str));
}

static inline bool AllowedLabelCharacter(const char& c) {
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

static inline bool ValidInstruction(std::string str) {
	return instructions.find(str) != instructions.end();
}


static uint RegisterValue(std::string str) {
	for (uint i = 0; i < REGISTER_COUNT; i++) {
		if (str == reg_names[i]) return i;
	}
	return 0;
}

static uint GetBinaryValue(std::string str) {
	uint result = 0;
	for (const char& c : str)
		result = (result << 1) + (c == '1') ? 1 : 0;
	return result;
}

static uint GetHexValue(std::string str) {
	uint result = 0;
	for (const char& c : str) {
		if (c == '$') continue;
		result = (result << 4) + (c > '9') ? c - ('a' + 0xA) : c - '0';
	}
	return result;
}

static std::string TrimSpaces(std::string str) {
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

void PrintLineNumber() {
	printf("[Line %i (0x%X)] ", line_number, rom_index + CHIP8_MEMSTART);
}


/*****************************************/
/*										 */
/*			MAIN ASM FUNCTIONS			 */
/*                                       */
/*****************************************/
void Byte_Output(byte in) {
	if (rom_index + 1 <= MAX_ROMSIZE) {
		rom_output[rom_index++] = in;
	}
	else byte_overflow += 1;
}
void Word_Output(word in) {
	Word_Output(in >> 8, in & 0x00FF);
}
void Word_Output(byte upper, byte lower) {
	if (rom_index + 2 <= MAX_ROMSIZE) {
		rom_output[rom_index++] = upper;
		rom_output[rom_index++] = lower;
	}
	else byte_overflow += 2;
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
			else Error_UnknownIdentifier(tokens[0].c_str());
		}
		line_number++;
	}
}

bool ASM_Build() {
	std::string linefeed;
	rom_index = 0;
	line_number = 1;
	while (std::getline(source_file, linefeed)) {
		if (!linefeed.empty()) {
			std::transform(linefeed.begin(), linefeed.end(), linefeed.begin(), ::tolower);
			linefeed = TrimSpaces(linefeed);
			std::vector<std::string> tokens = StringSplit(linefeed, " ,");

			if (ValidInstruction(tokens[0])) {
				std::vector<token> instruction_args;
				for (auto it = tokens.begin() + 1; it != tokens.end(); it++) {
					token new_token = { 0 };
					if (MakeToken(*it, &new_token)) instruction_args.push_back(new_token);
					else Error_InvalidToken(it->c_str());
				}
			}
		}
	}

	if (error_count != 0) return false;
	if (byte_overflow != 0) {
		printf("ROM size limit reached: %d/%d bytes (0x%X/0x%X).\n",
			MAX_ROMSIZE + byte_overflow, MAX_ROMSIZE, MAX_ROMSIZE + byte_overflow, MAX_ROMSIZE);
		error_count++;
		return false;
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