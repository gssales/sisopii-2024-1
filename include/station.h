#ifndef _STATION_H
#define _STATION_H

#include <string>
#include <mutex>
#include <functional>
#include <limits.h>
#include <arpa/inet.h>

#define MAC_ADDRESS_MAX 18

enum StationType : uint8_t 
{
  MANAGER,
  PARTICIPANT,
  CANDIDATE
};

enum StationStatus : uint8_t 
{
  AWAKEN,
  ELECTING,
  WAITING_MANAGER,
  ASLEEP,
  EXITING
};

class Station
{
private:
  unsigned int pid = 0;
  unsigned int table_clock = 0;
  bool debug = false;

  StationType type = PARTICIPANT;
  StationStatus status = AWAKEN;

  Station *manager;
  std::string interface;

  std::string macAddress;
  std::string ipAddress;
  std::string hostname;

  std::mutex mutex_station;

  void findIPAddress();
  void findInterfaceName();
  void findMacAddress();

public:
  
  Station() {}

  void init();
  void print();
  
  struct station_serial serialize();
  static void deserialize(Station* station, struct station_serial serialized);
  
  /**
   * USAR GET E SET atomico PARA A ESTAÇÃO ATUAL DO SISTEMA
  */
  auto atomic_get(auto &&callback);
  void atomic_set(std::function<void(Station *)> callback);

  StationType GetType() const { return this->type; }
  void SetType(StationType type);
};

/**
 * Struct para enviar no pacote
*/
struct station_serial
{
  unsigned int pid;
  unsigned int table_clock;
  char hostname[HOST_NAME_MAX];
  char ipAddress[INET_ADDRSTRLEN];
  char macAddress[MAC_ADDRESS_MAX];
  StationStatus status;
  StationType type;
};

#endif