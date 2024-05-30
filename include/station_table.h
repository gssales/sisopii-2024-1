#ifndef _STATION_TABLE_H
#define _STATION_TABLE_H

#include <map>
#include <list>
#include <string>
#include <mutex>

#include "include/station.h"

class StationTable
{
  public:
    unsigned long clock;
    std::mutex lock;
    bool has_update;
    std::map<std::string, station_serial> table;

    StationTable()
    {   
      this->clock = 0;
      this->has_update = false;
    }

    std::list<station_serial> getValues();
    bool has(std::string key);
    
    void insert(std::string key, station_serial item);
    void remove(std::string key);
    void update(std::string key, StationStatus new_status, StationType new_type);
};

#endif
