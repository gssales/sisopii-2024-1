#include "include/interface.h"

#include <iostream>
#include <chrono>
#include <thread>
#include <iomanip>
#include "include/network.h"
#include "include/utils.h"

using namespace std;

void clear_screen()
{
  cout << "\033[2J";;
}

void gotoxy(int x, int y)
{
  cout << "\033[" << y << ";" << x << "H";
}

void print_cell(string value, int width)
{
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
  print_cell("MAC ADDRESS", 20);
  print_cell("IP ADDRESS", 20);
  print_cell("STATUS", 10);
  cout << endl;
  print_hr();
  cout << endl;
}

void print_row(Station *station)
{
  std::string type = "  ";
  if (station->GetType() == MANAGER)
    type = " *";
  else if (station->GetType() == CANDIDATE)
    type = " ?";

  print_cell(type, 3);
  print_cell(station->GetHostname(), 30);
  print_cell(station->GetMacAddress(), 20);
  print_cell(station->GetIpAddress(), 20);
  print_cell(StationStatus_to_string(station->GetStatus()), 20);
  cout << endl;
}

void print_row(std::pair<station_serial, station_item> station, options_t *options)
{
  std::string type = "  ";
  if (station.first.type == MANAGER)
    type = " *";
  else if (station.first.type == CANDIDATE)
    type = " ?";

  auto host = station.first;
  auto host_info = station.second;
  
  print_cell(type, 3);
  print_cell(host.hostname, 30);
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
    if (station->GetType() == MANAGER)
    {
      if (station_table->has_update)
      {
        for (int i = 0; i < 10; i++)
          cout << "\033[" << (3+i) << ";1H \033[K" << endl;
        
        gotoxy(1, 3);
        for (auto &host_pair : station_table->clone())
          print_row(host_pair.second, options);

        station_table->has_update = false;
      }
    }
    else if (station->has_update)
    {
      for (int i = 0; i < 10; i++)
        cout << "\033[" << (3+i) << ";1H \033[K" << endl;

      gotoxy(1, 3);
      if (station->GetManager() != NULL)
        print_row(station->GetManager());
      print_row(station);
      
      station->has_update = false;
    }

    if (logger->has_changes)
    {
      gotoxy(1, 17);
      cout << "\033[J";
      cout << logger->get_lines(10) << endl;
    }

    goto_input();
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

	std::string command_values[5];
  while (station->GetStatus() != EXITING)
  {
		std::string command;
		getline(cin, command);
		
		std::stringstream ss(command);
		std::string word;
		
		int pointer = 0;
		while (ss >> word) {
			command_values[pointer] = word;
			pointer++;
		}
		
		if (station->GetType() == MANAGER) 
		{
			// if (command_values[0].compare("wakeup") == 0) 
			// {
			// 	string macAddress = "";
			// 	table->mutex_write.lock();
			// 	for (auto &tupla : table->table)
			// 	{
			// 		if (tupla.second.GetHostname().compare(command_values[1]))
			// 		{
			// 			macAddress = tupla.second.GetMacAddress();
			// 			break;
			// 		}
			// 	}
			// 	table->mutex_write.unlock();

			// 	if (macAddress.size() > 0)
			// 	{
			// 		stringstream cmd;
			// 		cmd << "wakeonlan " << macAddress;
			// 		system(cmd.str().c_str());
			// 	}
			// }
		}
		
		if (command_values[0].compare(CMD_EXIT) == 0)
		{
			station->SetStatus(EXITING);
      auto leaving_message = network::create_packet(network::LEAVING, station->serialize());
      network::datagram(INADDR_BROADCAST, leaving_message, params->logger, params->options);
      params->ui_lock.unlock();
		}
    
		if (command_values[0].compare(CMD_REFRESH) == 0)
      params->ui_lock.unlock();

    goto_input();
  }

  return 0;
}
