#include <SoftwareSerial.h>

SoftwareSerial mySerial(2, 3);  // RX, TX pins for fingerprint sensor          ////////////// Softwarerial help to use serial communication using different ports from 0 and 1 (just try it)
const int touchPin = 8;         // Pin connected to the fingerprint touch signal

// System Authentification for the fingerprint sensor, for now there is limited MaxAttempts
// Variables for user management
int maxAttempts = 3;            // Maximum allowed attempts before locking
int currentAttempts = 0;        // Track failed attempts
bool systemLocked = false;      // Lock the system after too many failed attempts
int userID = 1;                 // Start with User ID 1

void setup() {
  pinMode(touchPin, INPUT);
  Serial.begin(9600);
  mySerial.begin(115200);      // Baud rate (adjust if necessary) ///////// Changed from 9600 to 115200 just because this value is more common in serial running
  Serial.println("Fingerprint Security System Initialized.");
}

// System Locked loop to handle maximum attempts for the finger scan
void loop() {
  if (systemLocked) { 
    // If true (system is locked)
    Serial.println("System is locked. Too many failed attempts.");
    delay(5000); // Wait before allowing more actions
    return;
  }
  // if false then just run it...
  // Check for a finger touch
  if (digitalRead(touchPin) == HIGH) {
    Serial.println("Finger detected. Starting verification process...");
    bool verified = verifyUser();
    if (verified) {
      Serial.println("User verified successfully.");
      // Reset failed attempts after successful verification
      currentAttempts = 0;
    } else {
      currentAttempts++;
      Serial.println("Verification failed.");
      if (currentAttempts >= maxAttempts) {
        Serial.println("Too many failed attempts. Locking system.");
        systemLocked = true;  // Lock the system
      }
    }
    delay(2000);  // Avoid spamming checks
  }

  // Allow new user enrollment
  if (Serial.available() > 0) {
    char command = Serial.read();
    if (command == 'e') {  // Press 'e' to enroll a new user
      enrollUser(userID);
      userID++;  // Increment User ID for the next enrollment
    } else if (command == 'r') {  // Press 'r' to reset system lock
      resetSystem();
    }
  }
}

// Function to enroll a new user
void enrollUser(int id) {
  Serial.print("Enrolling new user with ID: ");
  Serial.println(id);
  sendCommand(0x01, id);  // Send command to start enrollment with this ID
  if (receiveResponse()) {
    Serial.println("Place your finger on the sensor...");
    waitForFinger();
    sendCommand(0x01, id);  // Capture the fingerprint and save
    if (receiveResponse()) {
      Serial.println("Fingerprint enrolled successfully.");
    } else {
      Serial.println("Failed to enroll fingerprint.");
    }
  } else {
    Serial.println("Failed to start enrollment.");
  }
}

// Function to verify the user
bool verifyUser() {
  sendCommand(0x10);  // Command for verification
  if (receiveResponse()) {
    return true;  // User verified
  } else {
    return false;  // Verification failed
  }
}

// Reset system lock
void resetSystem() {
  Serial.println("System reset. Lock cleared.");
  systemLocked = false;
  currentAttempts = 0;
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
  while (index < 8 && (millis() - startTime) < 500) {  // Timeout after 500ms
    if (mySerial.available()) {
      response[index++] = mySerial.read();
    }
  }
  return (index == 8 && response[1] == response[6]);
}
