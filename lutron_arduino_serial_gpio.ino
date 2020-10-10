const int readTimeoutMillis = 500;
const int baudRate = 9600;

const int inputButtonStart = 1; // 3rd column in 24 button keypad view
const int outputButtonStart = 9; // 2nd column in 24 button keypad view

#ifdef ESP32
const byte inputPins[] = { 13, 14, 15, 18 };
const byte outputPins[] = { 19, 22, 23, 25, 26 };
const byte processorAddressPins[] = { 4 };
const byte keypadAddressPins[] = { 5, 12 };

const int transmitPin = 21;

const int ledPin = 27;
const int ledOnLevel = LOW; // cmon man
const int ledOffLevel = HIGH;
#else
const byte inputPins[] = { 4, 5, 6, 7 };
const byte outputPins[] = { 8, 9, 10, 11 };
const byte processorAddressPins[] = { 12 };
const byte keypadAddressPins[] = { 13 };

const int transmitPin = 3;

const int ledPin = LED_BUILTIN;
const int ledOnLevel = HIGH;
const int ledOffLevel = LOW;
#endif

// these are the base lutron IDs which we offset
// so we don't have to read as many inputs
const int processorAddressOffset = 1;
const int keypadAddressOffset = 28;

const int keypadLedStateLength = 41;
const char keypadLedStateStart[] = "KLS";
const int keypadLedStateButtonZeroIndex = 16;

const int keypadLinkAddress = 6;

const int bufferLength = 128;
char readBuffer[bufferLength];
char writeBuffer[bufferLength];

int processorAddress;
int keypadAddress;

bool isLogEnabled;

void setup() {
  { // configure pins
    for (auto i = 0; i < sizeof(inputPins); i++)
      pinMode(inputPins[i], INPUT_PULLUP);
    
    for (auto i = 0; i < sizeof(processorAddressPins); i++)
      pinMode(processorAddressPins[i], INPUT_PULLUP);
    
    for (auto i = 0; i < sizeof(keypadAddressPins); i++)
      pinMode(keypadAddressPins[i], INPUT_PULLUP);
  
    pinMode(ledPin, OUTPUT);
    
    pinMode(transmitPin, OUTPUT);
    digitalWrite(transmitPin, HIGH);
  }

  { // calculate processor and keypad addresses from pins that are grounded
    processorAddress = processorAddressOffset;
    for (auto i = 0; i < sizeof(processorAddressPins); i++)
      processorAddress += digitalRead(processorAddressPins[i]) == LOW ? 1 << i : 0;
   
    keypadAddress = keypadAddressOffset;
    for (auto i = 0; i < sizeof(keypadAddressPins); i++)
      keypadAddress += digitalRead(keypadAddressPins[i]) == LOW ? 1 << i : 0;
  }

  Serial.begin(baudRate);
  Serial.setTimeout(readTimeoutMillis);

  isLogEnabled = false;
  //isLogEnabled = digitalRead(inputPins[0]) == LOW;

  //if (!isLogEnabled)
  //  writeLogToSerial();
}

void loop() {
  writeLineToSerial(writeBuffer, bufferLength, "RKLS,[%d:%d:%d]", processorAddress, keypadLinkAddress, keypadAddress);

  auto startTickCount = millis();

  while (true) {
    auto lineLength = readLineFromSerial(readBuffer, bufferLength);

    if (lineLength == 0 && millis() - startTickCount > readTimeoutMillis)
      break;

    if (lineLength == keypadLedStateLength && strncmp(readBuffer, keypadLedStateStart, strlen(keypadLedStateStart)) == 0) {
      digitalWrite(ledPin, ledOnLevel);

      // send keypad button press for any input pin not matching keypad button led state
      for (auto i = 0; i < sizeof(inputPins); i++)
        if ((digitalRead(inputPins[i]) == LOW) != (readBuffer[keypadLedStateButtonZeroIndex + inputButtonStart + i] == '1'))
          writeLineToSerial(writeBuffer, bufferLength, "KBP,[%d:%d:%d],%d", processorAddress, keypadLinkAddress, keypadAddress, inputButtonStart + i);
      
      // set output pin for every keypad led
      for (auto i = 0; i < sizeof(outputPins); i++) {
        auto isLedOn = readBuffer[keypadLedStateButtonZeroIndex + outputButtonStart + i] == '1';
        pinMode(outputPins[i], isLedOn ? OUTPUT : INPUT); // OUTPUT,LOW=grounded
        digitalWrite(outputPins[i], isLedOn ? LOW : HIGH); // INPUT,HIGH=ungrounded
      }
  
      digitalWrite(ledPin, ledOffLevel);
    }
  }
}

void writeLineToSerial(char* buffer, int length, char* format, ...) {
  va_list args;
  va_start(args, format);
  auto size = vsnprintf(buffer, length, format, args);
  digitalWrite(transmitPin, HIGH);
  Serial.write((byte*)buffer, size);
  Serial.write('\r');
  Serial.flush();
  if (isLogEnabled)
    logString('W', buffer, size);
  digitalWrite(transmitPin, LOW);
}

int readLineFromSerial(char* buffer, int length) {
  auto readCount = Serial.readBytesUntil('\r', buffer, length);
  if (isLogEnabled)
    logString('R', buffer, readCount);
  return readCount;
}
