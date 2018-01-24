#include "assembler.h"
#include "opcode.h"
#include <fstream>
#include <algorithm>
#include <unordered_map>
#include <tuple>
#include <numeric>
#include "stdafx.h"
#include "error.h"

uint line_number = 1;
std::string base_dir;
std::string output_name;
std::vector<std::string> file_trace;

static char rom_output[MAX_ROMSIZE];
static uint rom_index = 0;
static uint byte_overflow = 0;

std::map<std::string, uint> labels;
std::unordered_map<std::string, std::string> aliases;
//std::unordered_map<uint, std::string> pending_lines;
std::list<std::tuple<uint, uint, std::vector<std::string>>> pending_statements;

static uint formatting_width = 0;
static uint total_lines;

/*****************************************/
/*										 */
/*			AUX FUNCTIONS				 */
/*                                       */
/*****************************************/
static inline void CalculateFormatWidth() {
	format_width = (total_lines > 0) ? (uint)log10((double)total_lines) + 1 : 1;
}

static inline bool IsComment(std::string str) {
	return (str[0] == COMMENT_SYM);
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
	if (str[0] != '$' && str.substr(0, 2) != "0x") return false;
	str = (str[0] == '$') ? str.substr(1) : str.substr(2);
	if (str.empty() || str.size() > 4) return false;
	for (const char& c : str) {
		if ((c < '0' || c > '9') && (c < 'a' || c > 'f')) return false;
	}
	return true;
}

static bool ValidDecLiteral(std::string str) {
	for (const char& c : str) {
		if (c < '0' || c > '9') return false;
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
	if (i == str.npos) return false;
	if (std::count(str.begin(), str.end(), ':') > 1) return false;
	if (i != str.size() - 1) return false;
	return ValidLabelName(str);
}

static inline bool ValidInstruction(std::string str) {
	return opcode_list.find(str) != opcode_list.end();
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
	str = (str[0] == '$') ? str.substr(1) : str.substr(2);
	for (const char& c : str) {
		result = (result << 4) + ((c > '9') ? (c - 'a') + 0xA : c - '0');
	}
	return result;
}

static uint GetDecValue(std::string str) {
	return std::atoi(str.c_str());
}

static uint GetBitCount(uint value) {
	uint result = 0;
	while (value) {
		value = value >> 4;
		result += 4;
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
	else if (ValidHexLiteral(str)) *result = { GetHexValue(str), TYPE_LITERAL, GetBitCount(GetHexValue(str)) };
	else if (ValidDecLiteral(str)) *result = { GetDecValue(str), TYPE_LITERAL, GetBitCount(GetDecValue(str)) };
	else if (AliasExists(str)) return MakeToken(aliases[str], result);
	else return false;
	return true;
}

bool MakeTokens(std::vector<std::string> strings, std::vector<token>& result) {
	for (auto it = strings.begin() + 1; it != strings.end(); it++) {
		token new_token = { 0 };
		if (IsComment(*it)) break;
		if (MakeToken(*it, &new_token))
			result.push_back(new_token);
		else if (ValidLabelDefinition(*it)) {
			//Potential unencountered label, resolve in second pass
			new_token = { 0x000, TYPE_LITERAL, LITERAL_12 };
			result.push_back(new_token);
			auto t = std::make_tuple(line_number, rom_index, strings);
			pending_statements.push_back(t);
		}
		else {
			PushError("Invalid token \"%s\"", it->c_str());
			return false;
		}
	}
	return true;
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
	printf("Line %*i: ", formatting_width, line_number);
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

	std::ifstream source_file(path);
	if (!source_file.is_open()) {
		printf("File \"%s\" could not be opened/found.", path.c_str());
		return;
	}

	if (path.find_first_of("\\/") != path.npos) {
		base_dir = path.substr(0, path.find_last_not_of("\\/"));
	}
	else base_dir = "";
	output_name = path.substr(0, path.find_last_of('.')) + ROM_EXTENSION;
	
	printf("Assembling \"%s\"...\n", output_name.c_str());

	rom_index = 0;

	file_trace.push_back(path);
	ASM_FirstPass(source_file);
	if (!error_list.empty()) return;

	ASM_SecondPass();
	if (!error_list.empty()) return;

	ASM_WriteToFile();
}

void ASM_FirstPass(std::ifstream& file) {
	if (!file.is_open()) {
		PushError("File \"%s\" could not be opened/found.", file_trace.back().c_str());
		file_trace.pop_back();
		return;
	}
	{
		std::string temp;
		while (std::getline(file, temp))
			total_lines++;
		CalculateFormatWidth();
		file.clear();
		file.seekg(0, std::ifstream::beg);
	}
	
	line_number = 0;
	std::string linefeed;
	while (std::getline(file, linefeed)) {
		line_number++;
		if (!linefeed.empty()) {
			std::transform(linefeed.begin(), linefeed.end(), linefeed.begin(), ::tolower);
			std::vector<std::string> tstrings = StringSplit(linefeed, " ,\t");

			if (tstrings[0][0] == '.') {
				//Process directive
				if (tstrings[0] == ".alias") {
					if (tstrings.size() - 1 != 2)
						PushError("Alias expected %i args, found %i", tstrings[0].c_str(), 2, tstrings.size() - 1);
					else if (AliasExists(tstrings[1]))
						PushError("Alias %s is already defined.", tstrings[1].c_str());
					else aliases[tstrings[1]] = tstrings[2];
				}
				else if (tstrings[0] == ".include") {
					std::ifstream included_file(base_dir + tstrings[1]);
					file_trace.push_back(base_dir + tstrings[1]);
					uint temp = line_number;
					ASM_FirstPass(included_file);
					line_number = temp;
					included_file.close();
				}
				else PushError("Unrecognised directive \"%s\".", tstrings[0].c_str());
			}
			else if (tstrings[0].find(':') != std::string::npos) {
				//Process label
				if (!ValidLabelDefinition(tstrings[0])) {
					PushError("Invalid label name \"%s\"", tstrings[0].c_str());
					continue;
				}
				std::string name = tstrings[0].substr(0, tstrings[0].size() - 1);
				labels[name] = rom_index + CHIP8_MEMSTART;
			}
			else if (ValidInstruction(tstrings[0])) {
				std::vector<token> tokens;
				if (MakeTokens(tstrings, tokens)) {
					opcode& op = opcode_list[tstrings[0]];
					if (op.min > op.max) {
						if (tokens.size() != op.min) {
							PushError("%s expected %i args, found %i.",
								tstrings[0].c_str(), op.min, tokens.size());
							continue;
						}
					}
					else if (tokens.size() < op.min || tokens.size() > op.max) {
						PushError("%s expected %i-%i args, found %i.",
							tstrings[0].c_str(), op.min, op.max, tokens.size());
						continue;
					}
					op.callback(tokens);
				}
			}
			else if (!IsComment(tstrings[0]))
				PushError("Unknown identifier \"%s\"", tstrings[0].c_str());
		}
	}
	file_trace.pop_back();
}

void ASM_SecondPass() {
	uint temp = rom_index;
	
	for (auto it = pending_statements.begin(); it != pending_statements.end(); it++) {
		line_number = std::get<0>(*it);
		rom_index = std::get<1>(*it);
		std::vector<std::string> tstrings = std::get<2>(*it);
		std::vector<token> tokens;
		if (MakeTokens(tstrings, tokens)) {
			opcode& op = opcode_list[tstrings[0]];
			if (op.min > op.max) {
				if (tokens.size() != op.min) {
					PushError("%s expected %i args, found %i.",
						tstrings[0].c_str(), op.min, tokens.size());
					continue;
				}
			}
			else if (tokens.size() < op.min || tokens.size() > op.max) {
				PushError("%s expected %i-%i args, found %i.",
					tstrings[0].c_str(), op.min, op.max, tokens.size());
				continue;
			}
			op.callback(tokens);
		}
	}
	rom_index = temp;
}

void ASM_WriteToFile() {
	if (byte_overflow != 0) {
		uint overflow = MAX_ROMSIZE + byte_overflow;
		PushError("ROM size limit reached: %i/%i bytes (0x%X/0x%X)",
				  overflow, MAX_ROMSIZE, CHIP8_MEMSIZE + byte_overflow - 1, CHIP8_MEMSIZE - 1);
		return;
	}
	std::ofstream bin_file(base_dir + output_name, std::ofstream::binary | std::ofstream::trunc);
	if (!bin_file.is_open()) {
		printf("Could not create/open binary file: \"%s\"\n", (base_dir + output_name).c_str());
		return;
	}
	bin_file.write(rom_output, rom_index);
	bin_file.flush();
	printf("Wrote %i bytes to %s.\n", rom_index, (base_dir + output_name).c_str());
	printf("%i bytes remaining (0x%X/0x%X).\n",
			MAX_ROMSIZE - rom_index, rom_index + CHIP8_MEMSTART, CHIP8_MEMSIZE - 1);
}