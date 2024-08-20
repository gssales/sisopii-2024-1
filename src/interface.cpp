#include "include/interface.h"

#include <iostream>
#include <chrono>
#include <thread>
#include <iomanip>
#include <vector>
#include <signal.h>
#include "include/network.h"
#include "include/discovery.h"
#include "include/utils.h"

#define WHITE_BG "\033[47m"
#define BLACK_FG "\033[30m"
#define YELLOW_FG "\033[33m"
#define RESET "\033[0m"

using namespace std;

void clear_screen()
{
  cout << "\033[2J";;
}

void gotoxy(int x, int y)
{
  cout << "\033[" << y << ";" << x << "H";
}

void print_cell(unsigned short value, int width)
{
  cout << right << setw(width-1) << setfill(' ') << value << " ";
}

void print_cell(string value, int width, bool align_right = false)
{
  if (align_right)
    cout << right << setw(width-1) << setfill(' ') << value << " ";
  else
    cout << left << setw(width) << setfill(' ') << value;
}

void print_hr()
{
  cout << left << setw(110) << setfill('-') << "-";
  cout << endl;
}

void header()
{  
  print_cell(" ", 3);
  print_cell("HOSTNAME", 30);
  print_cell("PID", 8, true);
  print_cell("MAC ADDRESS", 20);
  print_cell("IP ADDRESS", 20);
  print_cell("STATUS", 10);
  cout << endl;
  print_hr();
  cout << endl;
}

void print_row(std::string type, std::pair<station_serial, station_item> station, bool current, options_t *options)
{
  auto host = station.first;
  auto host_info = station.second;
  
  if (current)
    cout << YELLOW_FG;
  print_cell(type, 3);
  if (current)
    print_cell("["+string(host.hostname)+"]", 30);
  else
    print_cell(host.hostname, 30);
  print_cell(host.pid, 8);
  print_cell(host.macAddress, 20);
  print_cell(host.ipAddress, 20);
  if (host_info.retry_counter > 0)
  {
    int max_retries = get_option(options, OPT_RETRY, 2);
    auto buf = std::stringstream();
    buf << StationStatus_to_string(host.status) << " (" << int(host_info.retry_counter) << "/" << max_retries << ")";
    print_cell(buf.str(), 20);
  }
  else
    print_cell(StationStatus_to_string(host.status), 20);
  if (current)
    cout << RESET;
  cout << endl;
}

void print_row(std::pair<station_serial, station_item> station, bool current, options_t *options)
{
  std::string type = "  ";
  if (station.first.type == MANAGER)
    type = " *";
  else if (station.first.type == CANDIDATE)
  {
    if (station.first.status == ELECTING)
      type = " !";
    else
      type = " ?";
  }

  print_row(type, station, current, options);
}

void print_row(Station *station, bool current)
{ 
  std::string type = "  ";
  if (station->GetType() == MANAGER)
    type = " *";
  else if (station->GetType() == CANDIDATE)
  {
    if (station->GetStatus() == ELECTING)
      type = " !";
    else
      type = " ?";
  }

  if (current)
    cout << YELLOW_FG;
  print_cell(type, 3);
  if (current)
    print_cell("["+station->GetHostname()+"]", 30);
  else
    print_cell(station->GetHostname(), 30);
  print_cell(station->GetPid(), 8);
  print_cell(station->GetMacAddress(), 20);
  print_cell(station->GetIpAddress(), 20);
  print_cell(StationStatus_to_string(station->GetStatus()), 20);
  if (current)
    cout << RESET;
  cout << endl;
}

void goto_input()
{
  gotoxy(1, 15);
  cout << "\033[K";
  cout << "> ";
}

void *interface::interface(service_params_t *params)
{
  auto station = params->station;
  auto station_table = params->station_table;
  auto options = params->options;
  auto logger = params->logger;

  clear_screen();
  gotoxy(1, 1);
  header();
  gotoxy(1, 14);
  print_hr();

  while (station->GetStatus() != EXITING) 
  {
    params->ui_lock.lock();
    if (station->GetType() == MANAGER || (station->GetType() == HOST && station->GetManager() == NULL))
    {
      if (station_table->has_update)
      {
        auto list = station_table->list(0);
        station_table->mutex.lock();
        for (int i = 0; i < 10; i++)
          cout << "\033[" << (3+i) << ";1H \033[K" << endl;
        
        gotoxy(1, 3);
        for (auto &host_pair : list)
          print_row(host_pair, station->GetMacAddress().compare(host_pair.first.macAddress) == 0, options);
        goto_input();

        station_table->has_update = false;
        station_table->mutex.unlock();
      }
    }
    else if (station->GetType() == HOST)
    {
      if (station_table->has_update)
      {
        auto manager = station->GetManager();
        auto list = station_table->list(0);
        station_table->mutex.lock();
        for (int i = 0; i < 10; i++)
          cout << "\033[" << (3+i) << ";1H \033[K" << endl;
        
        gotoxy(1, 3);
        for (auto &host_pair : list)
        {
          auto type = "  ";
          if (manager != NULL && manager->GetMacAddress().compare(host_pair.first.macAddress) == 0)
            type = " *";
          print_row(type, host_pair, station->GetMacAddress().compare(host_pair.first.macAddress) == 0, options);
        }
        goto_input();

        station_table->has_update = false;
        station_table->mutex.unlock();
      }
    }
    else 
    {
      for (int i = 0; i < 10; i++)
        cout << "\033[" << (3+i) << ";1H \033[K" << endl;
      gotoxy(1, 3);
      print_row(station, true);
      goto_input();
    }

    if (logger->has_changes)
    {
      gotoxy(1, 17);
      cout << "\033[J";
      cout << logger->get_lines(10) << endl;
      goto_input();
      logger->has_changes = false;
    }
    cout << flush;

    params->ui_lock.unlock();
    params->ui_lock.lock();
  }
  gotoxy(1, 27);
  cout << flush;

  return 0;
}

void *interface::command(service_params_t *params)
{
  auto station = params->station;
  auto station_table = params->station_table;

  while (station->GetStatus() != EXITING)
  {
    std::vector<std::string> command_values;
    std::string command;
    getline(cin, command);
    goto_input();
    if (std::cin.fail() || std::cin.eof()) {
      std::cin.clear();
      discovery::leave(params);
      continue;
    }
    
    std::stringstream ss(command);
    std::string word;
    
    while (ss >> word)
      command_values.push_back(word);

    if (command_values.size() == 0)
      continue;
    
    if (station->GetType() == MANAGER && command_values.size() > 0) 
    {
      if (command_values.front().compare(CMD_WAKEUP) == 0) 
      {
        std::string macAddress = "";
        
        station_table->mutex.lock();
        if (station_table->has(command_values[1]))
          macAddress = station_table->table[command_values[1]].first.macAddress;
        station_table->mutex.unlock();

        if (macAddress.size() > 0)
        {
          std::stringstream cmd;
          cmd << "wakeonlan " << macAddress;
          system(cmd.str().c_str());
        }
      }
    }
    
    if (command_values.front().compare(CMD_EXIT) == 0)
      discovery::leave(params);
    
    if (command_values.front().compare(CMD_REFRESH) == 0)
    {
      params->station_table->has_update = true;
      params->logger->has_changes = true;
      params->ui_lock.unlock();
    }

    goto_input();
    cout << flush;
  }

  return 0;
}
