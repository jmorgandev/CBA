#ifndef CBA_TYPES_H

#define CBA_TYPES_H
#include <vector>
#include <string>
#include <map>

typedef unsigned char byte;
typedef unsigned int uint;
typedef void(*op_ptr)(std::vector<std::string>, byte*);
#endif