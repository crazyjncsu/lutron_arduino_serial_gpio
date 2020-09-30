const int pollIntervalMillisecondCount = 500;
const int readTimeout = 2000;
const int baudRate = 9600;

const int inputPinCount = 4;
const int outputPinCount = 4;
const int processorAddressPinCount = 1; // number of values is "1 << pinCount" for these two, offset by the *Offset variables beloew
const int keypadAddressPinCount = 1;

const int inputButtonStart = 1; // 3rd column in 24 button keypad view
const int outputButtonStart = 9; // 2nd column in 24 button keypad view

// esp32 board I have is kind funny with this
// no LED_BUILTIN was defined ... and the levels here
const int ledPin = 27; // 27
const int ledOnLevel = LOW; // cmon man
const int ledOffLevel = HIGH;

// these are the base lutron IDs which we offset
// so we don't have to read as many inputs
const int processorAddressOffset = 1;
const int keypadAddressOffset = 28;

const int keypadLedStateLength = 41;
const char keypadLedStateStart[] = "KLS";
const int keypadLedStateButtonZeroIndex = 16;

const int pinStart = 12;
const int inputPinStart = pinStart;
const int outputPinStart = inputPinStart + inputPinCount;
const int processorAddressPinStart = outputPinStart + outputPinCount;
const int keypadAddressPinStart = processorAddressPinStart + processorAddressPinCount;

const int keypadLinkAddress = 6;

const int bufferLength = 128;
char readBuffer[bufferLength];
char writeBuffer[bufferLength];

int processorAddress;
int keypadAddress;

void setup() {
  { // configure pins
    for (auto i = 0; i < inputPinCount; i++)
      pinMode(inputPinStart + i, INPUT_PULLUP);
    
    for (auto i = 0; i < processorAddressPinCount; i++)
      pinMode(processorAddressPinStart + i, INPUT_PULLUP);
    
    for (auto i = 0; i < keypadAddressPinCount; i++)
      pinMode(keypadAddressPinStart + i, INPUT_PULLUP);
  
    pinMode(ledPin, OUTPUT);
  }

  { // calculate processor and keypad addresses from pins that are grounded
    processorAddress = processorAddressOffset;
    for (auto i = 0; i < processorAddressPinCount; i++)
      processorAddress += digitalRead(processorAddressPinStart + i) == LOW ? 1 << i : 0;
   
    keypadAddress = keypadAddressOffset;
    for (auto i = 0; i < keypadAddressPinCount; i++)
      keypadAddress += digitalRead(keypadAddressPinStart + i) == LOW ? 1 << i : 0;
  }

  Serial.begin(baudRate);
  Serial.setTimeout(readTimeout);
}

void loop() {
  digitalWrite(ledPin, ledOnLevel);

  writeLineToSerial(writeBuffer, bufferLength, "RKLS,[%d:%d:%d]", processorAddress, keypadLinkAddress, keypadAddress);
  
  auto lineLength = readLineFromSerial(readBuffer, bufferLength);

  if (lineLength == keypadLedStateLength && strncmp(readBuffer, keypadLedStateStart, strlen(keypadLedStateStart)) == 0) {
    // send keypad button press for any input pin not matching keypad button led state
    for (auto i = 0; i < inputPinCount; i++)
      if ((digitalRead(inputPinStart + 1) == LOW) != (readBuffer[keypadLedStateButtonZeroIndex + inputButtonStart + i] == '1'))
        writeLineToSerial(writeBuffer, bufferLength, "KBP,[%d:%d:%d],%d", processorAddress, keypadLinkAddress, keypadAddress, inputButtonStart + i);

    // set output pin for every keypad led
    for (auto i = 0; i < outputPinCount; i++) {
      auto isLedOn = readBuffer[keypadLedStateButtonZeroIndex + inputButtonStart + i] == '1';
      pinMode(outputPinStart + 1, isLedOn ? OUTPUT : INPUT); // OUTPUT,LOW=grounded
      digitalWrite(outputPinStart + i, isLedOn ? LOW : HIGH); // INPUT,HIGH=ungrounded
    }
  }
  
  digitalWrite(ledPin, ledOffLevel);
  
  delay(pollIntervalMillisecondCount);
}

void writeLineToSerial(char* buffer, int length, char* format, ...) {
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, length, format, args);
  Serial.println(buffer);
}

int readLineFromSerial(char* buffer, int length) {
  auto byteCount = Serial.readBytesUntil('\r', buffer, length);
  
  if (byteCount > 0 && Serial.read() != '\n')
    return -1;

  return byteCount;
}
