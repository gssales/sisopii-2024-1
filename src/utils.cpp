#include "include/utils.h"
#include <chrono>

uint64_t now()
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::system_clock::now().time_since_epoch()).count();
}

bool map_has_key(std::map<std::string, int> map, std::string key)
{
  return map.find(key) != map.end();
}