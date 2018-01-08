#include "enforce.h"
#include "opcode.h"

bool EnforceType(token tkn, uint type) {
	if (tkn.type == type) return true;
	PushError("Expected type %s, found %s.", type_names[type], type_names[tkn.type]);
	return false;
}
bool EnforceRegister(token tkn, uint reg) {
	if (tkn.value == reg) return true;
	PushError("Expected register %s, found %s.", reg_names[reg], reg_names[tkn.value]);
	return false;
}
bool EnforceRegisterV(token tkn) {
	if (tkn.value <= VF) return true;
	PushError("Expected register V[0-F], found %s.", reg_names[tkn.value]);
	return false;
}
bool EnforceBitcount(token tkn, uint bits) {
	if (tkn.bitcount <= bits) return true;
	PushError("Expected %i-bit literal, found %i-bit literal.", bits, tkn.bitcount);
	return false;
}
bool EnforceBitcountEx(token tkn, uint bits) {
	if (tkn.bitcount == bits) return true;
	PushError("Expected %i-bit literal, found %i-bit literal.", bits, tkn.bitcount);
	return false;
}