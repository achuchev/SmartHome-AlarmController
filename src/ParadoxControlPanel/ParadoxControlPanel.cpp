#include <Arduino.h>
#include <ParadoxControlPanel/ParadoxControlPanel.h>
#include <RemotePrint.h>
#include <ESP8266HTTPClient.h>
#include <CryptUtil.h>
#include <LinkedList.h>
#include <utils/utils.h>
#include <ArduinoJson.h>


ParadoxControlPanel::ParadoxControlPanel(String moduleHostname,
                                         String modulePassword,
                                         String userPin) {
  this->moduleHostname = moduleHostname;
  this->modulePassword = modulePassword;
  this->userPin        = userPin;
  this->sessionId      = "";

  this->http.setReuse(true);
  this->http.useHTTP10(false);
}

bool ParadoxControlPanel::process() {
  bool success   = false;
  QueueItem item = queueActionGet();

  if (item.action == Action::noAction) {
    // Nothing to process
    if (this->processStatus != ProcessStatus::loggedOut) {
      logout();
    }
    return true;
  }

  if (this->processStatus == ProcessStatus::loggedOut) {
    PRINTLN("Paradox: Logging in.");

    // First do silent logout, just to make sure we are not keeping the session
    // logout(true);

    success = httpLoginGetSessionId();

    if (!success) {
      PRINTLN_E("Could not get session ID.");
      this->processStatus = ProcessStatus::loggedOut;
      logout();
      return false;
    }
    this->processStatus = ProcessStatus::sessionIdRetrieved;
    return true;
  }

  if (this->processStatus == ProcessStatus::sessionIdRetrieved) {
    success = httpLoginAuthenticate();

    if (!success) {
      PRINTLN_E("Could not authenticate to the IP module.");
      this->processStatus = ProcessStatus::loggedOut;
      logout();
      return false;
    }
    this->processStatus = ProcessStatus::authenticated;
    return true;
  }

  if (this->processStatus == ProcessStatus::authenticated) {
    success = httpLoginWaitForModuleInit();

    if (!success) {
      PRINTLN_E("Error while waiting for the IP module initialization.");
      this->processStatus = ProcessStatus::loggedOut;
      logout();
      return false;
    }
    this->processStatus = ProcessStatus::loggedIn;
    return true;
  }

  if ((this->processStatus == ProcessStatus::loggedIn) &&
      (item.action == Action::getStatus)) {
    success = getTerminology();

    if (success) {
      success = getStatus();
    }

    if (!success) {
      PRINTLN_E("Error while getting control pannel status.");
      this->processStatus = ProcessStatus::loggedOut;
      logout();
      return false;
    }
    this->areasInfoStrIsTaken = false;

    this->queueItems.remove(0);
    return true;
  }

  if ((this->processStatus == ProcessStatus::loggedIn) &&
      (item.action == Action::armArea)) {
    success = getTerminology();

    if (success) {
      success = armArea(item);
    }

    if (!success) {
      PRINTLN_E("Error while arming area.");
      this->processStatus = ProcessStatus::loggedOut;
      logout();
      return false;
    }
    this->queueItems.remove(0);
    return true;
  }

  if ((this->processStatus == ProcessStatus::loggedIn) &&
      (item.action == Action::keepAlive)) {
    success = keepAlive();

    if (!success) {
      PRINTLN_E("Error while keeping session alive.");
      this->processStatus = ProcessStatus::loggedOut;
      logout();
      return false;
    }
    this->queueItems.remove(0);
    return true;
  }
}

bool ParadoxControlPanel::logout(bool silentLogOut) {
  if (!silentLogOut) {
    PRINTLN("Paradox: Logging out.");
  }
  String httpResponseBody = httpRequestGet("logout.html");

  if (httpResponseBody.length() != 0) {
    // The response is encoded, so we cannot see it
    this->http.end();

    if (!silentLogOut) {
      this->processStatus = ProcessStatus::loggedOut;
    }
    return true;
  }
  return false;
}

void ParadoxControlPanel::queueActionAdd(QueueItem& item) {
  for (int idx = 0; idx < this->queueItems.size(); ++idx) {
    if (this->queueItems.get(idx).action == item.action) {
      // No need to add it, as item with the same action is already there
      return;
    }
  }
  this->queueItems.add(item);
}

QueueItem ParadoxControlPanel::queueActionGet() {
  if (this->queueItems.size() > 0) {
    return this->queueItems.get(0);
  }
  QueueItem item;
  item.areaName = "";
  item.action   = Action::noAction;
  return item;
}

String ParadoxControlPanel::getLatestAreasInfo() {
  if (areasInfoStrIsTaken) {
    return "";
  }
  areasInfoStrIsTaken = true;
  return this->areasInfoStr;
}

int ParadoxControlPanel::getAreaIndexByName(String areaName) {
  for (int idx = 0; idx < this->areasNameIndex.size(); ++idx) {
    AreaNameIndex areaNameIndex = this->areasNameIndex.get(idx);

    if (areaNameIndex.name.equalsIgnoreCase(areaName)) {
      return areaNameIndex.index;
    }
  }
  return -1;
}

bool ParadoxControlPanel::armArea(QueueItem item,
                                  ArmType   armType) {
  PRINTLN("Paradox: Arming '" + item.areaName + "' area.");
  int areaIndexInt = getAreaIndexByName(item.areaName);

  if (areaIndexInt < 0) {
    PRINT_E("Error armin area '");
    PRINTLN_E(item.areaName + "'. Could not find the index of the area.");
    return false;
  }
  String areaIndex = String(areaIndexInt);

  if (areaIndex.length() <= 1) {
    areaIndex =   "0" + areaIndex; // leading 0 is needed
  }
  String armTypeStr = "";

  switch (armType) {
    case ArmType::force: { armTypeStr = "f"; break; }
    case ArmType::regular: { armTypeStr = "r"; break; }
    case ArmType::stay: { armTypeStr = "s"; break; }
    case ArmType::instant: { armTypeStr = "i"; break; }
    default: { PRINTLN("Unknown arm type."); return false; }
  }

  String url = String("statuslive.html?area=") + areaIndex +
               "&value=" + armTypeStr;

  // FIXME
  String httpResponseBody = httpRequestGet(url);

  return true;
}

bool ParadoxControlPanel::keepAlive() {
  PRINTLN("Paradox: Keeping session alive.");

  //   GET /keep_alive.html?msgid=1&7886282355404697 HTTP/1.1
  // Host: 192.168.1.123
  // Connection: keep-alive
  // User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_13_2)
  // AppleWebKit/537.36 (KHTML, like Gecko)
  // Chrome/63.0.3239.132 Safari/537.36
  // Accept: */*
  // Referer: http://192.168.1.123/menu.html
  // Accept-Encoding: gzip, deflate
  // Accept-Language: bg,en-US;q=0.9,en;q=0.8
  //
  // HTTP/1.0 200 OK
  // Server: lwIP/1.4.0 (http://savannah.nongnu.org/projects/lwip)
  // Content-type: text/html
  // Cache-control: no-cache
  //
  // { "ack":[{ "msgtype":"1", "action":"keepalive"}]}
  return true;
}

bool ParadoxControlPanel::getTerminology() {
  PRINTLN("Paradox: Getting control pannel terminology.");

  if (this->areasNameIndex.size() != 0) {
    // Terminology is already loaded
    return true;
  }
  String httpResponseBody = httpRequestGet("index.html");

  // Get the names of the areas
  String areasNameStr = Utils::getSubString(httpResponseBody,
                                            "tbl_areanam = new Array(",
                                            ");");

  if (areasNameStr.length() <= 0) {
    PRINTLN_E("Could not find the array with the area names.");
    return false;
  }
  LinkedList<String> areaNames = Utils::splitStringToList(areasNameStr);

  for (int idx = 0; idx < areaNames.size(); ++idx) {
    String name = areaNames.get(idx);
    AreaNameIndex area;
    area.name  = areaNames.get(idx);
    area.index = idx;
    this->areasNameIndex.add(area);
  }

  // Get the names of the zones
  String zonesNameStr = Utils::getSubString(httpResponseBody,
                                            "tbl_zone = new Array(",
                                            ");");

  if (areasNameStr.length() <= 0) {
    PRINTLN_E("Could not find the array with the zone names.");
    return false;
  }
  this->zonesNameStr = zonesNameStr;

  return true;
}

bool ParadoxControlPanel::getStatus() {
  PRINTLN("Paradox: Getting control pannel status.");

  // Get the statuses
  String httpResponseBody = httpRequestGet("statuslive.html");

  // Get the status of the zones
  String areasStatusStr = Utils::getSubString(httpResponseBody,
                                              "tbl_useraccess = new Array(",
                                              ");");

  if (areasStatusStr.length() <= 0) {
    PRINTLN_E("Could not find the array with the zone status.");
    return false;
  }
  this->areasStatusStr = areasStatusStr;

  // Get the status of the zones
  String zonesStatusStr = Utils::getSubString(httpResponseBody,
                                              "tbl_statuszone = new Array(",
                                              ");");

  if (zonesStatusStr.length() <= 0) {
    PRINTLN_E("Could not find the array with the zone status.");
    return false;
  }
  this->zonesStatusStr = zonesStatusStr;

  this->zonesNameStr.replace("\"", "");

  const size_t bufferSize = JSON_ARRAY_SIZE(2) + JSON_ARRAY_SIZE(3) +
                            JSON_ARRAY_SIZE(6) + JSON_OBJECT_SIZE(1) + 9 *
                            JSON_OBJECT_SIZE(4) + 2 * JSON_OBJECT_SIZE(5);

  DynamicJsonBuffer jsonBuffer(bufferSize);
  JsonObject& root        = jsonBuffer.createObject();
  JsonObject& status      = root.createNestedObject("status");
  JsonArray & areasStatus = status.createNestedArray("areasStatus");

  // tuka
  int zonesCount = Utils::countElementsInString(this->zonesNameStr);

  for (int idx = 0; idx < zonesCount; idx = idx + 2) {
    String areaIndexStr = Utils::getValue(this->zonesNameStr, idx);

    if (areaIndexStr == "0") {
      // This zone is not used, so we will ignore it
      continue;
    }

    int areaIndex   = areaIndexStr.toInt();
    String zoneName = Utils::getValue(this->zonesNameStr, idx + 1);

    JsonArray  *zonesInfo;
    JsonObject *area;
    bool currentAreaExists = false;

    for (int i = 0; i < this->areasNameIndex.size(); ++i) {
      area =
        &status.get<JsonVariant>("areasStatus").as<JsonArray>().get<JsonVariant>(i).as<JsonObject&>();
      String idStr = area->get<String>("id");

      currentAreaExists = false;

      if (idStr.toInt() == areaIndex) {
        currentAreaExists = true;
        break;
      }
    }

    if (currentAreaExists) {
      zonesInfo = &area->get<JsonVariant>("zonesInfo").as<JsonArray>();
    } else {
      int areaStatus   = Utils::getValueInt(this->areasStatusStr, areaIndex - 1);
      JsonObject& area = areasStatus.createNestedObject();
      area["name"]       = this->areasNameIndex.get(areaIndex - 1).name;
      area["id"]         = areaIndex;
      area["status"]     = areaStatus;
      area["statusName"] = ParadoxControlPanel::getAreaStatusFriendlyName(
        areaStatus);
      zonesInfo = &area.createNestedArray("zonesInfo");
    }
    int zoneId           = (int)(idx / 2) + 1;
    uint8_t zoneStatus   = Utils::getValueInt(this->zonesStatusStr, zoneId - 1);
    JsonObject& zoneInfo = zonesInfo->createNestedObject();
    zoneInfo["name"]       = zoneName;
    zoneInfo["id"]         = zoneId;
    zoneInfo["status"]     = zoneStatus;
    zoneInfo["statusName"] = ParadoxControlPanel::getZoneStatusFriendlyName(
      zoneStatus);
  }

  this->areasInfoStr = "";
  root.printTo(this->areasInfoStr);
  return true;
}

String ParadoxControlPanel::httpRequestGet(String location,
                                           int    delayBeforeRequest) {
  String url = String("http://") + this->moduleHostname + "/" +
               location;

  this->http.begin(url);
  this->http.setReuse(true);
  this->http.addHeader("Accept-Encoding", "identity");
  this->http.addHeader("Accept",
                       "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
  this->http.addHeader("Accept-Language", "en-US,en;q=0.5");

  if (this->lastUrl.length() != 0) {
    this->http.addHeader("Referer", this->lastUrl);
  }

  delay(delayBeforeRequest); // FIXME: remove me
  int httpCode = this->http.GET();

  if (httpCode != HTTP_CODE_OK) {
    PRINT_E("Unexpected HTTP error code. Code: ");
    PRINTLN_E(httpCode);
    return "";
  }

  this->lastUrl = url;
  String payload = http.getString();
  PRINTLN_D(payload);
  return payload;
}

bool ParadoxControlPanel::httpLoginGetSessionId() {
  PRINTLN("Paradox:\tGetting session ID.");
  String httpResponseBody = httpRequestGet("login_page.html");

  // FIXME: add more error cases
  if (httpResponseBody.indexOf("Maximum number of attempts reached.") > 0) {
    PRINTLN_E(
      "Could not get session ID as the maximum number of attemtps is reached.");
    return false;
  }
  String loggedUser = Utils::getSubString(httpResponseBody, "top.cant('", "');");

  if (loggedUser.length() != 0) {
    PRINT_E("User '");
    PRINT_E(loggedUser);
    PRINTLN_E("' already logged in.");
    return false;
  }
  String sessionId = Utils::getSubString(httpResponseBody, "loginaff(\"", "\",", true);

  if (sessionId.length() != 16) {
    PRINTLN_E("Invalid Session ID. ");
    PRINTLN_E(sessionId);
    return false;
  }

  this->sessionId = sessionId;
  PRINT_D("Session ID: ");
  PRINTLN_D(this->sessionId);
  return true;
}

bool ParadoxControlPanel::httpLoginAuthenticate() {
  PRINTLN("Paradox:\tAuthenticating.");

  String modulePassHash = CryptUtil::md5SumHex(this->modulePassword);
  PRINT_D("Module Password Hash: ");
  PRINTLN_D(modulePassHash);

  String sessionPass = modulePassHash + this->sessionId;
  PRINT_D("Session Password: ");
  PRINTLN_D(sessionPass);

  String paradoxPass = CryptUtil::md5SumHex(sessionPass);
  PRINT_D("Paradox Password: ");
  PRINTLN_D(paradoxPass);

  String pardoxUser = CryptUtil::rc4Paradox(this->userPin, sessionPass);
  PRINT_D("Paradox User: ");
  PRINTLN_D(pardoxUser);

  String httpResponseBody = httpRequestGet(
    "default.html?u=" + pardoxUser + "&p=" +  paradoxPass);

  if (httpResponseBody.length() != 0) {
    // The response is encoded, so we cannot see it. The actual verification
    // will
    // happen in the next method
    return true;
  }
  return false;
}

bool ParadoxControlPanel::httpLoginWaitForModuleInit(int timeout,
                                                     int poolDelay) {
  PRINTLN("Paradox:\tWaiting for IP module initialization.");
  long endTime    = millis() + timeout;
  int  loginStage = 0;

  while (millis() < endTime) {
    String httpResponseBody = httpRequestGet("waitlive.html");
    String loginStageStr    = Utils::getSubString(httpResponseBody, "var prg=", ";");

    if (httpResponseBody.length() <= 0) {
      PRINTLN_E("The session seems not authenticated.");
      return false;
    }
    loginStage = loginStageStr.toInt();

    PRINT_D("Stage: ");
    PRINTLN_D(loginStage);

    if (loginStage == 4) {
      return true;
    }

    // FIXME: Do not use delay, instead use the loop
    delay(poolDelay);
  }
  PRINTLN_E("Timeout reached. Last state: ");
  PRINTLN_E(loginStage)
  return false;
}

String ParadoxControlPanel::getAreaStatusFriendlyName(uint8_t status) {
  switch (status) {
    case 1:
      return "disarmed";
    case 2:
      return "armed";
    case 3:
      return "inAlarm";
    case 4:
      return "sleep";
    case 5:
      return "stay";
    case 6:
      return "entryDelay";
    case 7:
      return "exitDelay";
    case 8:
      return "ready";
    case 9:
      return "notReady";
    case 10:
      return "instant";
    case 99: // buttype = "allarea";???
    default:
      return "unknown";
  }
}

String ParadoxControlPanel::getZoneStatusFriendlyName(uint8_t status) {
  switch (status) {
    case 0:
      return "closed";
    case 1:
      return "opened";
    default:
      return "unknown";
  }
}
