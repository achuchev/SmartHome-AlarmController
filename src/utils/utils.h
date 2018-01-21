#ifndef ALARM_CONTROLLER_UTILS_H
#define ALARM_CONTROLLER_UTILS_H

#include <Arduino.h>
#include <RemotePrint.h>
#include <LinkedList.h>

class Utils {
public:

  static String getValue(String data,
                         int    index,
                         char   separator = ',');
  static int    getValueInt(String data,
                            int    index,
                            char   separator = ',');
  static size_t countElementsInString(String data,
                                      char   separator = ',');
  static String getSubString(String data,
                             String prefix,
                             String suffix,
                             bool   toUpper = false);
  static LinkedList<String>splitStringToList(String data,
                                             char   separator = ',');
};

#endif // ifndef ALARM_CONTROLLER_UTILS_H
