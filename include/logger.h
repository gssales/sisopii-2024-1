#ifndef _LOGGER_H
#define _LOGGER_H

#include <string>
#include <iostream>
#include <sstream>
#include <mutex>
#include "include/utils.h"

class Logger {
public:
  bool has_changes;
  bool debug;

  Logger(std::mutex *ui_lock) {
    this->has_changes = false;
    this->debug = false;
    this->buf = new std::stringstream();
    this->redirect = new cerr_redirect(buf->rdbuf());
    this->ui_lock = ui_lock;
  }

  ~Logger() {
    delete this->redirect;
    delete this->buf;
  }	

  void info(std::string message);
  void error(std::string message, char* error);

  std::string print() {
    return this->buf->str();
  }

  std::string get_lines(int max_lines);

private:
  std::stringstream *buf;
  cerr_redirect *redirect;
  std::mutex *ui_lock;
};

#endif