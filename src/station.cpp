#include "include/station.h"
#include <unistd.h>
#include <ifaddrs.h>
#include <linux/if.h>
#include <sys/ioctl.h>
#include <cstring>
#include <iostream>
#include "include/utils.h"

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
    return "ASLEEP";
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

void Station::SetStationTable(StationTable *station_table) {
  atomic_set([&](Station *self) {self->station_table = station_table;});
}


std::list<std::pair<station_serial, station_item>> StationTable::getValues()
{
  std::list<std::pair<station_serial, station_item>> values;
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
  station_item i;
  i.last_update = now();
  i.retry_counter = 0;
  this->table.insert_or_assign(key, std::pair(item, i));
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
    if (this->table[key].first.status != new_status || this->table[key].first.type != new_type) {
      this->table[key].first.status = new_status;
      this->table[key].first.type = new_type;
      this->table[key].second.last_update = now();
      this->table[key].second.retry_counter = 0;
      this->clock++;
      this->has_update = true;
    }
    this->lock.unlock();
  }
}

void StationTable::update_retry(std::string key, u_int8_t retry_counter)
{
  if (this->has(key))
  {
    std::cout << "has key " << int(retry_counter) << std::endl;
    this->lock.lock();
    this->table[key].second.last_update = now();
    this->table[key].second.retry_counter = retry_counter;
    this->lock.unlock();
  }
}