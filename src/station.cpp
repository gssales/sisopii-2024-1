#include "include/station.h"
#include <unistd.h>
#include <ifaddrs.h>
#include <linux/if.h>
#include <sys/ioctl.h>

#include <cstring>
#include <iostream>

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
        char ipAddress[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &((struct sockaddr_in *)addr->ifa_addr)->sin_addr, ipAddress, INET_ADDRSTRLEN);
        this->ipAddress = ipAddress;
        break;
      }
    }
  }
}

void Station::findInterfaceName()
{
  struct ifaddrs *addrs;
  getifaddrs(&addrs);
  for (struct ifaddrs *addr = addrs; addr != nullptr; addr = addr->ifa_next) 
  {
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
  serialized.table_clock = this->table_clock;
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
  station->table_clock = serialized.table_clock;
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