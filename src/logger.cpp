#include "include/logger.h"

#include <vector>

void Logger::info(std::string message) 
{
  if (!this->debug)
    return;
  std::cerr << "INFO " << message << std::endl;
  this->has_changes = true;
  this->ui_lock->unlock();
}

void Logger::error(std::string message, char* error) 
{
  std::cerr << "ERROR " << message << error << std::endl;
  this->has_changes = true;
  this->ui_lock->unlock();
}

std::string Logger::get_lines(int max_lines) 
{
  auto clone = std::stringstream(this->buf->str());
  auto lines = std::vector<std::string>();
  std::string line;
  while (getline(clone, line))
    lines.push_back(line);

  if (int(lines.size()) > max_lines)
    lines.erase(lines.begin(), lines.begin() + (lines.size() - max_lines));

  std::stringstream ss;
  for (auto line : lines)
    ss << line << std::endl;

  return ss.str();
}