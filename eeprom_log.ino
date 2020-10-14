#include <EEPROM.h>

const int maxLogLength = 2048;
int currentLogLength = 0;

void writeLogToSerial() {
#if ESP32
  EEPROM.begin(maxLogLength);
#endif

  for (auto i = 0; i < maxLogLength; i++) {
    auto value = EEPROM.read(i);
    Serial.write(value);
    Serial.flush();
  }

#if ESP32
  EEPROM.end();
#endif
}

void logChar(char c) {
#if ESP32
  if (currentLogLength == 0)
    EEPROM.begin(maxLogLength);
  else if (currentLogLength == maxLogLength)
    EEPROM.end();
#endif

  if (currentLogLength < maxLogLength)
    EEPROM.write(currentLogLength++, c);
}

void logStringFormat(char operation, char* buffer, int length, char* format, ...) {
  va_list args;
  va_start(args, format);
  auto size = vsnprintf(buffer, length, format, args);
  logString(operation, buffer, size);
}

void logString(char operation, char* buffer, int length) {
  logChar(operation);
  logChar(':');

  for (auto i = 0; i < length; i++)
    logChar(buffer[i]);

  logChar('\n');
}
