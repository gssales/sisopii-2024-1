#include "include/interface.h"

#include <iostream>
#include <chrono>
#include <thread>
#include <iomanip>

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
  print_cell(StationStatus_to_string(station->GetStatus()), 15);
  cout << endl;
}

void print_row(std::pair<station_serial, station_item> station)
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
  print_cell(StationStatus_to_string(host.status), 10);
  cout << endl;
}

void *interface::screen(Station *station)
{
  int loop = 0;
  clear_screen();
  gotoxy(1, 1);
  header();
  while (true) 
  {
    gotoxy(1, 3);
    // cout << "\033[10M";
    if (station->GetType() == MANAGER)
    {
      for (auto &host_pair : station->GetStationTable()->getValues())
        print_row(host_pair);
    }
    else
    {
      if (station->GetManager() != NULL)
        print_row(station->GetManager());
      print_row(station);
    }
    gotoxy(1, 14);
    print_hr();


	  std::this_thread::sleep_for(std::chrono::seconds(1));
    loop++;
  }
}

