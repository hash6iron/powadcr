#include <string>
#include "LogEvent.h"

// Logger Handler
void loggerHandler(const LogEvent& event) {
  std::string level;

  if (event.level < MIN_LOG_LEVEL) {
    return;
  }

  switch (event.level) {
    case DEBUG: level = "DEBUG"; break;
    case INFO: level = "INFO"; break;
    case NOTICE: level = "NOTICE"; break;
    case WARNING: level = "WARNING"; break;
    case ERROR: level = "ERROR"; break;
    case CRITICAL: level = "CRITICAL"; break;
    case ALERT: level = "ALERT"; break;
    case EMERGENCY: level = "EMERGENCY"; break;
  }
  Serial.print("[");
  Serial.print(level.c_str());
  Serial.print("] ");
  Serial.println(event.msg.c_str());
}

// Attach logger handler.
void initLogger() {
  LogEvent event("");
  evManager.attachEventHandler(event, [](const ObservableEvent& event) {
    const LogEvent& logEvent = static_cast<const LogEvent&>(event);
    loggerHandler(logEvent);
  });
}

// Dispach debug log.
void debugLog(std::string msg) {
  LogEvent event(msg, DEBUG);
  evManager.notifyEvent(event);
}

// Dispach info log.
void infoLog(std::string msg) {
  LogEvent event(msg, INFO);
  evManager.notifyEvent(event);
}

// Dispach notice log.
void noticeLog(std::string msg) {
  LogEvent event(msg, NOTICE);
  evManager.notifyEvent(event);
}

// Dispach warning log.
void warningLog(std::string msg) {
  LogEvent event(msg, WARNING);
  evManager.notifyEvent(event);
}

// Dispach error log.
void errorLog(std::string msg) {
  LogEvent event(msg, ERROR);
  evManager.notifyEvent(event);
}

// Dispach CRITICAL log.
void criticalLog(std::string msg) {
  LogEvent event(msg, CRITICAL);
  evManager.notifyEvent(event);
}

// Dispach ALERT log.
void alertLog(std::string msg) {
  LogEvent event(msg, ALERT);
  evManager.notifyEvent(event);
}

// Dispach EMERGENCY log.
void emergencyLog(std::string msg) {
  LogEvent event(msg, EMERGENCY);
  evManager.notifyEvent(event);
}