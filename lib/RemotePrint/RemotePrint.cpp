#include "RemotePrint.h"
#include "RemoteDebug.h"

RemotePrint::RemotePrint(String  hostName,
                         uint8_t startingDebugLevel,
                         bool    serialEnabled,
                         bool    telnetEnabled) {
  if (telnetEnabled) {
    Debug.begin(hostName, startingDebugLevel);
    Debug.setSerialEnabled(serialEnabled);
    Debug.setResetCmdEnabled(true); // Enable the reset command
  }
}

// Define the static members
RemotePrint *RemotePrint::s_instance = NULL;
uint8_t RemotePrint::logLevel        = 0;
bool    RemotePrint::serialEnabled   = false;
bool    RemotePrint::telnetEnabled   = false;

RemotePrint * RemotePrint::instance() {
  if (!s_instance) {
    #ifndef LOG_OUTPUT
    # define LOG_OUTPUT "SERIAL_TELNET"
    #endif // ifndef LOG_OUTPUT

    logLevel      = RemoteDebug::INFO;
    serialEnabled = (LOG_OUTPUT == "SERIAL" || LOG_OUTPUT == "SERIAL_TELNET");
    telnetEnabled = (LOG_OUTPUT == "TELNET" || LOG_OUTPUT == "SERIAL_TELNET");

    if (serialEnabled) {
      #ifndef SERIAL_BAND
      # define SERIAL_BAND 115200
      #endif // ifndef SERIAL_BAND
      Serial.begin(SERIAL_BAND);
      Serial.setTimeout(2000);
    }

    #ifdef LOG_LEVEL

    if (LOG_LEVEL == "ANY") logLevel = RemoteDebug::ANY;
    else if (LOG_LEVEL == "VERBOSE") logLevel = RemoteDebug::VERBOSE;
    else if (LOG_LEVEL == "DEBUG") logLevel = RemoteDebug::DEBUG;
    else if (LOG_LEVEL == "INFO") logLevel = RemoteDebug::INFO;
    else if (LOG_LEVEL == "WARNING") logLevel = RemoteDebug::WARNING;
    else if (LOG_LEVEL == "ERROR") logLevel = RemoteDebug::ERROR;
    else if (LOG_LEVEL == "PROFILER") logLevel = RemoteDebug::PROFILER;
    #endif // ifdef LOG_LEVEL
    s_instance = new RemotePrint("telnet_hostname",
                                 logLevel,
                                 serialEnabled,
                                 telnetEnabled);
  }
  return s_instance;
}

void RemotePrint::handle() {
  if (RemotePrint::telnetEnabled) {
    Debug.handle();
  }
}

void RemotePrint::print(String args, uint8_t _logLevel) {
  if (!isActive(_logLevel)) return;

  if (telnetEnabled) {
    Debug.print(args);
  } else if (serialEnabled) {
    Serial.print(args);
  }
}

void RemotePrint::println(String args, uint8_t _logLevel) {
  if (!isActive(_logLevel)) return;

  if (telnetEnabled) {
    Debug.println(args);
  } else if (serialEnabled) {
    Serial.println(args);
  }
}

bool RemotePrint::isActive(uint8_t _logLevel) {
  if (telnetEnabled) {
    return Debug.isActive(_logLevel);
  } else if (serialEnabled) {
    return _logLevel >= logLevel;
  }
  return false;
}
