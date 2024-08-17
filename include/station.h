#ifndef _STATION_H
#define _STATION_H

#include <map>
#include <list>
#include <utility>
#include <string>
#include <mutex>
#include <functional>
#include <limits.h>
#include <arpa/inet.h>

#define MAC_ADDRESS_MAX 18

enum StationType : uint8_t 
{
  MANAGER,
  HOST,
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
std::string StationStatus_to_string(StationStatus status);

class StationTable;

class Station
{
private:
  unsigned int pid = 0;
  unsigned int clock = 0;
  bool debug = false;

  StationType type = HOST;
  StationStatus status = AWAKEN;

  Station *manager;
  std::string interface;

  std::string macAddress;
  std::string ipAddress;
  in_addr_t s_addr;
  std::string hostname;

  std::mutex mutex_station;

  void findIPAddress();
  void findInterfaceName();
  void findMacAddress();

public:
  bool has_update;
  
  Station() {
    this->has_update = false;
  }

  void init();
  void print();
  
  struct station_serial serialize();
  static void deserialize(Station* station, struct station_serial serialized);

  /**
   * USAR GET E SET atomico PARA A ESTAÇÃO ATUAL DO SISTEMA
  */
  auto atomic_get(auto &&callback);
  void atomic_set(std::function<void(Station *)> callback);

  Station* GetManager() const { return this->manager; }
  void SetManager(Station *manager);

  StationType GetType() const { return this->type; }
  void SetType(StationType type);

  in_addr_t GetInAddr() const { return this->s_addr; }

  std::string GetHostname() const { return this->hostname; }

  std::string GetMacAddress() const { return this->macAddress; }

  std::string GetIpAddress() const { return this->ipAddress; }

  unsigned int GetClock() const { return this->clock; }

  StationStatus GetStatus() const { return this->status; }
  void SetStatus(StationStatus status);
};

/**
 * Struct para enviar no pacote
*/
struct station_serial
{
  unsigned int pid;
  unsigned int clock;
  char hostname[HOST_NAME_MAX];
  char ipAddress[INET_ADDRSTRLEN];
  char macAddress[MAC_ADDRESS_MAX];
  StationStatus status;
  StationType type;
};

/**
 * Struct para armazenar
*/
struct station_item
{
  u_int64_t last_update;
  u_int8_t retry_counter;
};

class StationTable
{
  public:
    unsigned long clock;
    std::mutex mutex;
    bool has_update;
    std::map<std::string, std::pair<station_serial, station_item>> table;

    StationTable()
    {   
      this->clock = 0;
      this->has_update = false;
    }

    void serialize(station_serial *arr);
    static void deserialize(StationTable* table, struct station_serial serialized[5]);

    std::map<std::string, std::pair<station_serial, station_item>> clone();
    bool has(std::string key);
    
    void insert(std::string key, station_serial item);
    void remove(std::string key);
    void update(std::string key, StationStatus new_status, StationType new_type);
    void update_retry(std::string key, u_int8_t retry_counter);
};

#endif