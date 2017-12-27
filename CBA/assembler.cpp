#include "assembler.h"
#include "instruction.h"
#include <fstream>
#include <algorithm>
#include <unordered_map>
#include "stdafx.h"
#include "error.h"

#define EnforceValidLabel(a, act)\
	if(!ValidLabelDefinition(a)) {Error_InvalidLabelName(a); act;}

#define EnforceDirectiveArgs(dir, a, b, act)\
	if(a != b) {Error_DirectiveArgs(dir, b, a); act;}

#define EnforceUndefinedAlias(ali, act)\
	if(AliasExists(ali)) { Error_AliasDefined(ali##.c_str()); act;}

#define EnforceArgCount(inst, count, act)\
	if(count < instructions[inst].min_arg || count > instructions[inst].max_arg) {\
		Error_InstructionArgs(inst.c_str(), instructions[inst].min_arg, instructions[inst].max_arg, count); act;}

static char rom_output[MAX_ROMSIZE];
static uint rom_index = 0;
static uint line_number = 1;
static uint byte_overflow = 0;
static uint source_lines = 0;

static std::fstream source_file;
static std::fstream binary_file;
static std::string  bin_file_name;

std::map<std::string, uint> labels;
std::unordered_map<std::string, std::string> aliases;
std::unordered_map<uint, std::string> pending_lines;

static uint formatting_width = 0;

/*****************************************/
/*										 */
/*			AUX FUNCTIONS				 */
/*                                       */
/*****************************************/
static inline bool IsComment(std::string str) {
	return (str[0] == '#');
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
	return (c == '_') || (c == ':') || IsAlphaNumeric(c);
}

static inline bool ValidLabelName(std::string str) {
	return (std::find_if_not(str.begin(), str.end(), AllowedLabelCharacter) == str.end());
}
static bool ValidLabelDefinition(std::string str) {
	uint i = str.find(':');
	if (i == std::string::npos) return false;
	if (std::count(str.begin(), str.end(), ':') > 1) return false;
	if (i != str.size() - 1) return false;
	return ValidLabelName(str);
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
		result = (result << 1) + ((c == '1') ? 1 : 0);
	return result;
}

static uint GetHexValue(std::string str) {
	uint result = 0;
	for (const char& c : str) {
		if (c == '$') continue;
		result = (result << 4) + ((c > '9') ? (c - 'a') + 0xA : c - '0');
	}
	return result;
}

static std::string TrimSpaces(std::string str) {
	if (str[0] == ' ') str = str.substr(str.find_first_not_of(' '));
	for (uint i = 0; i < str.size();) {
		if (str[i] == ' ' && (i == str.size() - 1 || str[i + 1] == ' ')) str.erase(i);
		else i++;
	}
	return str;
}

static bool MakeToken(std::string str, token* result) {
	if (IsRegister(str)) *result = { RegisterValue(str), TYPE_REGISTER, NULL };
	else if (LabelExists(str)) *result = { labels[str], TYPE_LITERAL, LITERAL_12 };
	else if (ValidBinaryLiteral(str)) *result = { GetBinaryValue(str), TYPE_LITERAL, str.size() };
	else if (ValidHexLiteral(str)) *result = { GetHexValue(str), TYPE_LITERAL, (str.size()-1) * 4 };
	else if (AliasExists(str)) return MakeToken(aliases[str], result);
	else return false;
	return true;
}

std::vector<token> MakeTokens(std::vector<std::string> strings, std::string line) {
	std::vector<token> result;
	for (auto it = strings.begin() + 1; it != strings.end(); it++) {
		token new_token = { 0 };
		if (IsComment(*it)) break;
		if (MakeToken(*it, &new_token))
			result.push_back(new_token);
		else if (ValidLabelName(*it)) {
			//Potential unencountered label, resolve in second pass
			new_token = { 0x000, TYPE_LITERAL, LITERAL_12 };
			result.push_back(new_token);
			pending_lines[rom_index] = line;
		}
		else {
			Error_InvalidToken(it->c_str());
			continue;
		}
	}
	return result;
}

static std::vector<std::string> StringSplit(std::string str, std::string seperators) {
	std::vector<std::string> result;
	str = str.substr(str.find_first_not_of(seperators));
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
	printf("Line %*i: ", formatting_width, line_number, rom_index + CHIP8_MEMSTART);
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

void ASM_Begin(std::string path) {
	std::memset(rom_output, NULL, MAX_ROMSIZE);

	source_file.open(path, std::fstream::in);
	if (!source_file.is_open()) {
		Error_NoSourceFile(path.c_str());
		return;
	}

	std::string temp;
	while (std::getline(source_file, temp))
		source_lines++;
	source_file.clear();
	source_file.seekg(0, std::fstream::beg);

	formatting_width = (source_lines > 0) ? (uint)log10((double)source_lines) + 1 : 1;

	size_t ext = path.find_last_of('.');
	bin_file_name = path.substr(0, ext) + ROM_EXTENSION;

	printf("Assembling %s...\n\n", path.substr(0, ext).c_str());

	ASM_FirstPass();
}

void ASM_FirstPass() {
	line_number = 1;
	rom_index = 0;
	std::string linefeed;
	while (std::getline(source_file, linefeed)) {
		if (!linefeed.empty()) {
			std::transform(linefeed.begin(), linefeed.end(), linefeed.begin(), ::tolower);
			std::vector<std::string> tokens = StringSplit(linefeed, " ,\t");

			if (tokens[0][0] == '.') {
				//Process directive
				if (tokens[0] == ".alias") {
					EnforceDirectiveArgs(".alias", 2, tokens.size() - 1, continue);
					EnforceUndefinedAlias(tokens[1], continue);
					aliases[tokens[1]] = tokens[2];
				}
				else Error_InvalidDirective(tokens[0].c_str());
			}
			else if (tokens[0].find(':') != std::string::npos) {
				//Process label
				EnforceValidLabel(tokens[0].c_str(), continue);
				std::string name = tokens[0].substr(0, tokens[0].size() - 1);
				labels[name] = rom_index + CHIP8_MEMSTART;
			}
			//else if (ValidInstruction(tokens[0])) rom_index += instructions[tokens[0]].out_size;
			else if (ValidInstruction(tokens[0])) {
				std::vector<token> valid_args = MakeTokens(tokens, linefeed);
				EnforceArgCount(tokens[0], valid_args.size(), continue);
				instructions[tokens[0]].callback(valid_args);
			}
			else if(!IsComment(tokens[0])) Error_UnknownIdentifier(tokens[0].c_str());
		}
		line_number++;
	}

	if (error_count == 0) ASM_SecondPass();
}

void ASM_SecondPass() {
	uint temp = rom_index;

	source_file.clear();
	source_file.seekg(0, std::fstream::beg);
	line_number = 1;
	
	for (auto it = pending_lines.begin(); it != pending_lines.end(); it++) {
		rom_index = it->first;
		std::vector<std::string> tokens = StringSplit(it->second, " ,\t");
		std::vector<token> valid_args = MakeTokens(tokens, it->second);
		EnforceArgCount(tokens[0], valid_args.size(), continue);
		instructions[tokens[0]].callback(valid_args);
	}
	rom_index = temp;

	if(error_count == 0) ASM_WriteToFile();
}

void ASM_WriteToFile() {
	if (byte_overflow != 0) {
		uint overflow = MAX_ROMSIZE + byte_overflow;
		printf("ROM size limit reached: %d/%d bytes (0x%X/0x%X).\n",
			overflow, MAX_ROMSIZE, overflow, MAX_ROMSIZE);
		error_count++;
		return;
	}

	binary_file.open(bin_file_name, std::fstream::out | std::fstream::binary | std::fstream::trunc);
	if (!binary_file.is_open()) {
		printf("Could not create/open binary file: \"%s\"\n", bin_file_name.c_str());
		return;
	}
	binary_file.write(rom_output, rom_index);
	binary_file.flush();
	printf("Wrote %i bytes to %s.\n", rom_index, bin_file_name.c_str());
	printf("%d bytes remaining (0x%X/0xFFF).\n", MAX_ROMSIZE - rom_index, rom_index + CHIP8_MEMSTART);
}