#include <SoftwareSerial.h>

SoftwareSerial mySerial(2, 3);  // RX, TX pins for fingerprint sensor
const int touchPin = 8;         // Pin connected to the fingerprint touch signal

// System Authentication for the fingerprint sensor, for now there is limited MaxAttempts
int maxAttempts = 3;            // Maximum allowed attempts before locking
int currentAttempts = 0;        // Track failed attempts
bool systemLocked = false;      // Lock the system after too many failed attempts
int userID = 1;                 // Start with User ID 1

void sendCommand(byte command, byte param = 0x00);
bool receiveResponse();

void setup() {
  pinMode(touchPin, INPUT);
  Serial.begin(115200);  // Communication with Serial Monitor
  mySerial.begin(115200);  // Communication with the fingerprint sensor based on documentation
  Serial.println("Fingerprint Security System Initialized.");
}

void loop() {
  if (systemLocked) { 
    // If true (system is locked)
    Serial.println("System is locked. Too many failed attempts.");
    delay(5000); // Wait before allowing more actions
    return;
  }

  // Check for a finger touch
  if (digitalRead(touchPin) == HIGH) {
    Serial.println("Finger detected. Starting verification process...");
    bool verified = verifyUser();
    if (verified) {
      Serial.println("User verified successfully.");
      currentAttempts = 0;  // Reset failed attempts
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
      Serial.println("Failed to enroll fingerprint. No response after capturing.");
    }
  } else {
    Serial.println("Failed to start enrollment. No initial response.");
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
  Serial.print("Sending command: ");
  for (int i = 0; i < 8; i++) {
    mySerial.write(packet[i]);
    Serial.print(packet[i], HEX);  // Print command being sent
    Serial.print(" ");
  }
  Serial.println();
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

  if (index == 8) {
    Serial.print("Response received: ");
    for (int i = 0; i < 8; i++) {
      Serial.print(response[i], HEX);  // Print received response
      Serial.print(" ");
    }
    Serial.println();
  } else {
    Serial.println("No valid response received.");
  }

  return (index == 8 && response[1] == response[6]);  // Basic checksum for response validation
}