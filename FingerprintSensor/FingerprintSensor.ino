#include <SoftwareSerial.h>

// Comments side notes for me to talk about the changes:p

// Define new pins for SoftwareSerial (avoid pins 0 and 1)
// Sofware series allos to use other digital pins for communication
SoftwareSerial mySerial(2, 3);  // RX, TX pins for communication with the fingerprint sensor

const int touchPin = 8;         // Pin connected to TP_high (Pin 5 on the sensor)

void setup() {
  pinMode(touchPin, INPUT);     // Set pin for touch detection
  Serial.begin(9600);           // Start Serial Monitor
  mySerial.begin(115200);       // Start communication with the fingerprint sensor (adjust baud rate if necessary)
  delay(1000);                  // Allow sensor some time for setup
  
  Serial.println("Initializing Fingerprint Sensor...");
  sendCommand(0xA0);            // Open the sensor
  
  if (receiveResponse()) {
    Serial.println("Sensor initialized successfully.");
  } else {
    Serial.println("Failed to initialize sensor.");
  }
}

void loop() {
  // Check if finger is detected using the touch pin
  if (digitalRead(touchPin) == HIGH) {
    Serial.println("Finger detected. Enrolling...");
    enrollFingerprint(1);       // Enroll the fingerprint with ID 1
    delay(2000);
  }
  delay(1000);
}

// Enroll fingerprint with given ID
void enrollFingerprint(uint8_t id) {
  sendCommand(0x01, id);         // Command to start enrollment with specific ID
  if (receiveResponse()) {
    Serial.println("Place your finger on the sensor...");
    waitForFinger();
    
    // After placing the finger, capture the fingerprint and store it
    sendCommand(0x01, id);  // Continue with the enrollment steps
    if (receiveResponse()) {
      Serial.println("Fingerprint enrolled successfully.");
    } else {
      Serial.println("Failed to enroll fingerprint.");
    }
  } else {
    Serial.println("Failed to start enrollment.");
  }
}

// Wait for finger placement
void waitForFinger() {
  while (digitalRead(touchPin) != HIGH) {
    delay(100);
  }
}

// Send a command to the fingerprint sensor
void sendCommand(byte command, byte param = 0x00) {
  byte packet[8] = {0xF5, command, param, 0x00, 0x00, 0x00, (byte)(command ^ param), 0xF5};
  for (int i = 0; i < 8; i++) {
    mySerial.write(packet[i]);
  }
}

// Receive and verify a response from the fingerprint sensor
bool receiveResponse() {
  byte response[8];
  int index = 0;
  unsigned long startTime = millis();
  
  // Add a timeout to avoid hanging if no data is received
  while (index < 8 && (millis() - startTime) < 500) {  // 500ms timeout
    if (mySerial.available()) {
      response[index++] = mySerial.read();
    }
  }
  
  if (index == 8 && response[1] == response[6]) {
    return true;  // Valid response
  } else {
    return false; // Invalid or incomplete response
  }
}
