#include "include/station_table.h"

std::list<station_serial> StationTable::getValues()
{
  std::list<station_serial> values;
  this->lock.lock();
  for (auto &item : this->table)
    values.push_back(item.second);
  this->lock.unlock();
  return values;
}

bool StationTable::has(std::string key)
{
  return this->table.find(key) != this->table.end();
}

void StationTable::insert(std::string key, station_serial item)
{
  this->lock.lock();
  this->table.insert_or_assign(key, item);
  this->clock++;
  this->has_update = true;
  this->lock.unlock();
}

void StationTable::remove(std::string key)
{
  this->lock.lock();
  this->table.erase(key);
  this->clock++;
  this->has_update = true;
  this->lock.unlock();
}

void StationTable::update(std::string key, StationStatus new_status, StationType new_type)
{
  if (this->has(key))
  {
    this->lock.lock();
    if (this->table[key].status != new_status || this->table[key].type != new_type) {
      this->table[key].status = new_status;
      this->table[key].type = new_type;
      this->clock++;
      this->has_update = true;
    }
    this->lock.unlock();
  }
}
