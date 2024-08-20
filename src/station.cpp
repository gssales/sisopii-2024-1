#include "include/station.h"
#include <unistd.h>
#include <ifaddrs.h>
#include <linux/if.h>
#include <sys/ioctl.h>
#include <cstring>
#include <iostream>
#include "include/utils.h"

std::string StationType_to_string(StationType type)
{
  switch (type)
  {
  case MANAGER:
    return "MANAGER";
  case HOST:
    return "HOST";
  case CANDIDATE:
    return "CANDIDATE";
  default:
    return "UNKNOWN";
  }
}

std::string StationStatus_to_string(StationStatus status)
{
  switch (status)
  {
  case AWAKEN:
    return "AWAKEN";
  case ELECTING:
    return "ELECTING";
  case WAITING_MANAGER:
    return "WAITING_MANAGER";
  case ASLEEP:
    return "SLEEPING";
  case EXITING:
    return "EXITING";
  default:
    return "UNKNOWN";
  }
}

void Station::init()
{
  char hostname[HOST_NAME_MAX];
  gethostname(hostname, HOST_NAME_MAX);
  this->hostname = hostname;

  this->pid = getpid();

  this->findIPAddress();
  this->findInterfaceName();
  this->findMacAddress();
}

void Station::findIPAddress()
{
  struct ifaddrs *addrs;
  getifaddrs(&addrs);
  for (struct ifaddrs *addr = addrs; addr != nullptr; addr = addr->ifa_next) 
  {
    if (addr->ifa_addr && addr->ifa_addr->sa_family == AF_INET) 
    {
      if (strncmp("en", addr->ifa_name, 2) == 0 || strncmp("eth", addr->ifa_name, 3) == 0)
      {
				struct sockaddr_in *sock_addr = (struct sockaddr_in *)addr->ifa_addr;
				this->s_addr = sock_addr->sin_addr.s_addr;

        char ipAddress[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sock_addr->sin_addr, ipAddress, INET_ADDRSTRLEN);
        this->ipAddress = ipAddress;
        break;
      }
    }
  }
  freeifaddrs(addrs);
}

void Station::findInterfaceName()
{
  struct ifaddrs *addrs;
  getifaddrs(&addrs);
  for (struct ifaddrs *addr = addrs; addr != nullptr; addr = addr->ifa_next) 
  {
    // if (addr->ifa_addr && addr->ifa_addr->sa_family == AF_LINK) 
    // {
		// 	unsigned char *ptr = (unsigned char *)LLADDR((struct sockaddr_dl *)(addr)->ifa_addr);
		// 	printf("%s: %02x:%02x:%02x:%02x:%02x:%02x\n",
		// 											(addr)->ifa_name,
		// 											*ptr, *(ptr+1), *(ptr+2), *(ptr+3), *(ptr+4), *(ptr+5));
		// }
    if (addr->ifa_addr && addr->ifa_addr->sa_family == AF_PACKET) 
    {
      if (strncmp("en", addr->ifa_name, 2) == 0 || strncmp("eth", addr->ifa_name, 3) == 0)
      {
        this->interface = addr->ifa_name;
        break;
      }
    }
  }
  freeifaddrs(addrs);
} 

void Station::findMacAddress() 
{
	struct ifreq ifr;
	int fd = socket(AF_INET, SOCK_DGRAM, 0);

	ifr.ifr_addr.sa_family = AF_INET;
	strncpy((char *)ifr.ifr_name, this->interface.c_str(), IFNAMSIZ-1);

	ioctl(fd, SIOCGIFHWADDR, &ifr);

	close(fd);
	
	unsigned char* mac = (unsigned char *)ifr.ifr_addr.sa_data;

  char macAddress[MAC_ADDRESS_MAX];
	sprintf(macAddress, (const char *)"%02x:%02x:%02x:%02x:%02x:%02x",
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  this->macAddress = macAddress;
}

void Station::print()
{
  std::cout << "Pid: " << this->pid << std::endl;
  std::cout << "Hostname: " << this->hostname << std::endl;
  std::cout << "MAC Address: " << this->macAddress << std::endl;
  std::cout << "Interface: " << this->interface << std::endl;
  std::cout << "IP Address: " << this->ipAddress << std::endl << std::endl;
}

std::string Station::to_string()
{
  std::string str = "Pid: " + std::to_string(this->pid) + ";";
  str += "Type: " + StationType_to_string(this->type) + ";";
  str += "Hostname: " + this->hostname + ";";
  str += "MAC Address: " + this->macAddress + ";";
  str += "Interface: " + this->interface + ";";
  str += "IP Address: " + this->ipAddress + ";";
  return str;
}

void Station::GeneratePid()
{
  unsigned long timestamp = now();
  this->pid = timestamp % 1000;
}

struct station_serial Station::serialize()
{
  struct station_serial serialized;

  serialized.pid = this->pid;
  serialized.clock = this->clock;
  serialized.type = this->type;
  serialized.status = this->status;
  strncpy(serialized.hostname, this->hostname.c_str(), HOST_NAME_MAX);
  strncpy(serialized.ipAddress, this->ipAddress.c_str(), INET_ADDRSTRLEN);
  strncpy(serialized.macAddress, this->macAddress.c_str(), MAC_ADDRESS_MAX);

  return serialized;
}

void Station::deserialize(Station *station, struct station_serial serialized)
{
  station->pid = serialized.pid;
  station->clock = serialized.clock;
  station->type = serialized.type;
  station->status = serialized.status;
  station->hostname = std::string(serialized.hostname);
  station->ipAddress = std::string(serialized.ipAddress);
  station->macAddress = std::string(serialized.macAddress);
}

auto Station::atomic_get(auto &&callback)
{
  mutex_station.lock();
  auto response = callback(this);
  mutex_station.unlock();
  return response;
}

void Station::atomic_set(std::function<void(Station *)> callback)
{
  mutex_station.lock();
  callback(this);
  this->has_update = true;
  mutex_station.unlock();
}

void Station::SetType(StationType type) {
  atomic_set([&](Station *self) {self->type = type;});
}

void Station::SetManager(Station *manager) {
  atomic_set([&](Station *self) {self->manager = manager;});
}

void Station::SetStatus(StationStatus status) {
  atomic_set([&](Station *self) {self->status = status;});
}


std::map<std::string, std::pair<station_serial, station_item>> StationTable::clone()
{
  std::map<std::string, std::pair<station_serial, station_item>> clone;
  this->mutex.lock();
  clone = this->table;
  this->mutex.unlock();
  return clone;
}

std::list<std::pair<station_serial, station_item>> StationTable::list(unsigned short pid)
{
  std::list<std::pair<station_serial, station_item>> list;
  this->mutex.lock();
  for (auto &host_pair : this->table)
  {
    if (host_pair.second.first.pid > pid)
      list.push_back(host_pair.second);
  }
  list.sort([](auto &a, auto &b) { return a.first.pid > b.first.pid; });
  this->mutex.unlock();
  return list;
}

void StationTable::serialize(station_serial *arr)
{
  int i = 0;
  for (auto &host_pair : this->table)
  {
    memcpy(arr+i, &host_pair.second.first, sizeof(station_serial));
    i++;
  }
}

void StationTable::deserialize(StationTable *table, station_serial serialized[5], unsigned short table_count, unsigned int clock)
{
  table->mutex.lock();
  table->table.clear();
  for (int i = 0; i < table_count; i++)
  {
    table->table.insert_or_assign(serialized[i].hostname, std::pair(serialized[i], station_item()));
  }
  table->clock = clock;
  table->has_update = true;
  table->mutex.unlock();
}

bool StationTable::has(std::string key)
{
  return this->table.find(key) != this->table.end();
}

void StationTable::insert(std::string key, station_serial item)
{
  this->mutex.lock();
  station_item i;
  i.last_update = now();
  i.retry_counter = 0;
  this->table.insert_or_assign(key, std::pair(item, i));
  this->clock++;
  this->has_update = true;
  this->mutex.unlock();
}

void StationTable::remove(std::string key)
{
  this->mutex.lock();
  this->table.erase(key);
  this->clock++;
  this->has_update = true;
  this->mutex.unlock();
}

void StationTable::update(std::string key, StationStatus new_status, StationType new_type)
{
  if (this->has(key))
  {
    this->mutex.lock();
    if (this->table[key].first.status != new_status || this->table[key].first.type != new_type) {
      this->table[key].first.status = new_status;
      this->table[key].first.type = new_type;
      this->table[key].second.last_update = now();
      this->table[key].second.retry_counter = 0;
      this->clock++;
      this->has_update = true;
    }
    this->mutex.unlock();
  }
}

uint8_t StationTable::update_retry(std::string key, bool reset)
{
  if (this->has(key))
  {
    this->mutex.lock();
    auto counter = this->table[key].second.retry_counter;
    counter = reset ? 0 : counter + 1;
    if (this->table[key].second.retry_counter != counter)
      this->has_update = true;
    this->table[key].second.last_update = now();
    this->table[key].second.retry_counter = counter;
    this->mutex.unlock();
    return counter;
  }
  return 0;
}