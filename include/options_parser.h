#ifndef OPTIONS_PARSER_H
#define OPTIONS_PARSER_H

#include <string>
#include <map>

#define OPT_MANAGER "manager"
#define OPT_DEBUG "-d"
#define OPT_PORT_DGRAM "-p-dgram"
#define OPT_PORT_STREAM "-p-stream"
#define OPT_TIMEOUT "-timeout"
#define OPT_SLEEP "-sleep"
#define OPT_REFRESH "-refresh"
#define OPT_RETRY "-retry"

typedef std::map<std::string, int> options_t;

void parseOptions(int argc, const char* argv[], options_t *options);
int get_option(options_t *options, std::string key, int default_value);

#endif