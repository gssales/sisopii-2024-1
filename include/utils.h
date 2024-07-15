#ifndef _UTILS_H
#define _UTILS_H

#include <cstdint>
#include <iostream>
#include <map>
#include <string>

uint64_t now();

bool map_has_key(std::map<std::string, int> map, std::string key);

struct cerr_redirect {
  cerr_redirect( std::streambuf * new_buffer ) 
    : old( std::cerr.rdbuf( new_buffer ) )
  { }

  ~cerr_redirect( ) {
    std::cerr.rdbuf( old );
  }

  private:
    std::streambuf * old;
};

#endif