#include <string>
#include "LogEvent.h"

LogEvent::LogEvent(std::string msg) : LogEvent(msg, INFO) {};

LogEvent::LogEvent(std::string msg, LogLevel state) {
  this->id = "LogEvent";
  this->level = state;
  this->msg = msg;
}
