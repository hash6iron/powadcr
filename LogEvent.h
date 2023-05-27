#ifndef LOGEVENT_H
#define LOGEVENT_H

#include <string>
#include "ObserverEvents.h"

// logging levels described by RFC 5424.
// @see http://seldaek.github.io/monolog/doc/01-usage.html#log-levels
enum LogLevel {
  // Detailed debug information.
  DEBUG = 100,
  // Interesting events. Examples: User logs in, SQL logs.
  INFO = 200,
  //  Normal but significant events.
  NOTICE = 250,
  //  Exceptional occurrences that are not errors. Examples: Use of deprecated APIs, poor use of an API, undesirable things that are not necessarily wrong.
  WARNING = 300,
  // Runtime errors that do not require immediate action but should typically be logged and monitored.
  ERROR = 400,
  // Critical conditions. Example: Application component unavailable, unexpected exception.
  CRITICAL = 500,
  //  Action must be taken immediately. Example: Entire website down, database unavailable, etc. This should trigger the SMS alerts and wake you up.
  ALERT = 550,
  // Emergency: system is unusable.
  EMERGENCY  = 600
};

class LogEvent : public ObservableEvent {
  public:
    LogLevel level;
    std::string msg;

    LogEvent(std::string msg);
    LogEvent(std::string msg, LogLevel state);
};

#endif // LOGEVENT_H