#ifndef PARADOX_CONTROL_PANEL_H
#define PARADOX_CONTROL_PANEL_H

#include "Arduino.h"
#include <ESP8266HTTPClient.h>
#include <LinkedList.h>

enum ProcessStatus {
  loggedOut,
  sessionIdRetrieved,
  authenticated,
  loggedIn
};

enum Action {
  noAction,
  getStatus,
  armArea,
  keepAlive
};

enum ArmType {
  force,
  regular,
  stay,
  instant
};

struct AreaNameIndex {
  String name;
  int    index;
};

struct QueueItem {
  String areaName;
  Action action;
};


class ParadoxControlPanel {
public:

  ParadoxControlPanel(String moduleHostname = "",
                      String modulePassword = "",
                      String userPin        = "");

  bool          process();
  void          queueActionAdd(QueueItem& item);
  String        getLatestAreasInfo();
  static String getAreaStatusFriendlyName(uint8_t status);

private:

  bool          getTerminology();
  bool          getStatus();
  bool          logout(bool silentLogOut = false);
  bool          armArea(QueueItem item,
                        ArmType   armArea = ArmType::force);
  bool          keepAlive();

  bool          httpLoginGetSessionId();
  bool          httpLoginAuthenticate();
  bool          httpLoginWaitForModuleInit(int timeout   = 10000,
                                           int poolDelay = 1500);
  String        httpRequestGet(String location,
                               int    delayBeforeRequest = 500);
  QueueItem     queueActionGet();
  int           getAreaIndexByName(String areaName);
  static String getZoneStatusFriendlyName(uint8_t status);

  HTTPClient http;
  String lastUrl;
  String sessionId                        = "";
  String moduleHostname                   = "";
  String modulePassword                   = "";
  String userPin                          = "";
  String zonesNameStr                     = "";
  String areasStatusStr                   = "";
  String zonesStatusStr                   = "";
  String areasInfoStr                     = "";
  bool areasInfoStrIsTaken                = false;
  ProcessStatus processStatus             = ProcessStatus::loggedOut;
  LinkedList<QueueItem>queueItems         = LinkedList<QueueItem>();
  LinkedList<AreaNameIndex>areasNameIndex = LinkedList<AreaNameIndex>();
};

#endif // ifndef PARADOX_CONTROL_PANEL_H
