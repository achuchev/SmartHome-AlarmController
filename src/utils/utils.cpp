#include <utils/utils.h>
#include <LinkedList.h>
#include <Arduino.h>
#include <RemotePrint.h>


String Utils::getValue(String data, int index, char separator) {
  int found      = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex   = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; ++i) {
    if ((data.charAt(i) == separator) || (i == maxIndex)) {
      ++found;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

int Utils::getValueInt(String data, int index, char separator) {
  String value = getValue(data, index, separator);

  return value.toInt();
}

size_t Utils::countElementsInString(String data, char separator) {
  const char *dataC = data.c_str();
  size_t dataCount;

  for (dataCount = 0; dataC[dataCount];
       dataC[dataCount] == separator ? dataCount++ : *dataC++);

  // It's allways +1 as we have less commas
  ++dataCount;
  return dataCount;
}

String Utils::getSubString(String data,
                           String prefix,
                           String suffix,
                           bool   toUpper) {
  int startPosition = data.indexOf(prefix);

  if (startPosition <= 0) {
    return "";
  }

  int endPosition = data.indexOf(suffix, startPosition);

  if (endPosition <= 0) {
    return "";
  }
  String result = data.substring(startPosition + prefix.length(), endPosition);

  if (toUpper) {
    result.toUpperCase();
  }
  return result;
}

LinkedList<String>Utils::splitStringToList(String data, char separator) {
  data.replace("\"", "");

  size_t dataCount            = countElementsInString(data);
  LinkedList<String> dataList =  LinkedList<String>();

  for (int i = 0; i < dataCount; ++i) {
    dataList.add(getValue(data, i));
  }
  return dataList;
}
