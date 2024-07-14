#include "include/options_parser.h"

#include <iostream>
#include <cstring>

void parseOptions(int argc, const char* argv[], option_t *options) {
	for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], OPT_MANAGER) == 0)
      (*options)[OPT_MANAGER] = 1;
      
		if (argv[i][0] == '-') {
			if (i + 1 < argc && argv[i + 1][0] != '-' && strcmp(argv[i + 1], OPT_MANAGER) != 0) 
      {
        (*options)[argv[i]] = std::stoi(argv[i + 1]);
				i++;
			}
      else 
      {
        (*options)[argv[i]] = 1;
      }
		}
	}
}

int get_option(option_t *options, std::string key, int default_value) {
  if (options->find(key) != options->end()) {
    return options->at(key);
  }
  return default_value;
}
