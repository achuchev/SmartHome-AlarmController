#ifndef REMOTE_PRINT_H
#define REMOTE_PRINT_H

#include "Arduino.h"
#include "RemoteDebug.h"

class RemotePrint {
public:

  RemotePrint(String  hostName,
              uint8_t startingDebugLevel = RemoteDebug::DEBUG,
              bool    serialEnabled      = false,
              bool    telnetEnabled      = true);
  void                print(String  args,
                            uint8_t debugLevel = RemoteDebug::INFO);
  void                println(String  args,
                              uint8_t debugLevel = RemoteDebug::INFO);
  void                handle();
  static RemotePrint* instance();

private:

  static RemotePrint *s_instance;
  static uint8_t logLevel;
  static bool serialEnabled;
  static bool telnetEnabled;
  RemoteDebug Debug;
  bool isActive(uint8_t _logLevel);
};

#define PRINT(...) { RemotePrint::instance()->print(String(__VA_ARGS__)); }
#define PRINTLN(...) { RemotePrint::instance()->println(String(__VA_ARGS__)); }
#define PRINT_D(...) { RemotePrint::instance()->print(String(__VA_ARGS__), RemoteDebug::DEBUG); }
#define PRINTLN_D(...) { RemotePrint::instance()->println(String(__VA_ARGS__), RemoteDebug::DEBUG); }
#define PRINT_E(...) { RemotePrint::instance()->print(String(__VA_ARGS__), RemoteDebug::ERROR); }
#define PRINTLN_E(...) { RemotePrint::instance()->println(String(__VA_ARGS__), RemoteDebug::ERROR); }
#define PRINT_W(...) { RemotePrint::instance()->print(String(__VA_ARGS__), RemoteDebug::WARNING); }
#define PRINTLN_W(...) { RemotePrint::instance()->println(String(__VA_ARGS__), RemoteDebug::WARNING); }
#define REMOTE_PRINT_HANDLER { RemotePrint::instance()->handle(); }

#endif // ifndef REMOTE_PRINT_H
