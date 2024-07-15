#ifndef SERVICE_H
#define SERVICE_H

#include <mutex>

#include "station.h"
#include "options_parser.h"
#include "logger.h"

struct service_params_t
{
  Station *station;
  StationTable *station_table;
  options_t *options;
  Logger *logger;
  std::mutex ui_lock;
};

#endif