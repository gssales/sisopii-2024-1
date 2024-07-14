#ifndef _UTILS_H
#define _UTILS_H

#include <cstdint>
#include <map>
#include <string>

uint64_t now();

bool map_has_key(std::map<std::string, int> map, std::string key);

#endif
