#include "BLEDevice.h"
#include "BLEHIDDevice.h"
#include "HIDTypes.h"
#include "HIDKeyboardTypes.h"
#include <math.h>

// BLE objects
BLEHIDDevice* hid;
BLECharacteristic* input;
BLECharacteristic* output;

// Mouse state
bool connected = false;
uint8_t mouseButtons = 0;
int8_t mouseX = 0;
int8_t mouseY = 0;
int8_t mouseWheel = 0;

// HID Report Descriptor for Mouse
const uint8_t hidReportDescriptor[] = {
  USAGE_PAGE(1),       0x01, // Generic Desktop Ctrls
  USAGE(1),            0x02, // Mouse
  COLLECTION(1),       0x01, // Application
  USAGE(1),            0x01, //  Pointer
  COLLECTION(1),       0x00, //  Physical
  USAGE_PAGE(1),       0x09, //   Buttons
  USAGE_MINIMUM(1),    0x01, //   Button 1
  USAGE_MAXIMUM(1),    0x03, //   Button 3
  LOGICAL_MINIMUM(1),  0x00, //   0
  LOGICAL_MAXIMUM(1),  0x01, //   1
  REPORT_COUNT(1),     0x03, //   3 Buttons
  REPORT_SIZE(1),      0x01, //   1 bit each
  HIDINPUT(1),         0x02, //   Data,Var,Abs
  REPORT_COUNT(1),     0x01, //   1 byte
  REPORT_SIZE(1),      0x05, //   5 bits
  HIDINPUT(1),         0x03, //   Cnst,Var,Abs (padding)
  USAGE_PAGE(1),       0x01, //   Generic Desktop
  USAGE(1),            0x30, //   X
  USAGE(1),            0x31, //   Y
  LOGICAL_MINIMUM(1),  0x81, //   -127
  LOGICAL_MAXIMUM(1),  0x7f, //   127
  REPORT_SIZE(1),      0x08, //   8 bits
  REPORT_COUNT(1),     0x02, //   2 bytes (X,Y)
  HIDINPUT(1),         0x06, //   Data,Var,Rel
  USAGE(1),            0x38, //   Wheel
  LOGICAL_MINIMUM(1),  0x81, //   -127
  LOGICAL_MAXIMUM(1),  0x7f, //   127
  REPORT_SIZE(1),      0x08, //   8 bits
  REPORT_COUNT(1),     0x01, //   1 byte
  HIDINPUT(1),         0x06, //   Data,Var,Rel
  END_COLLECTION(0),
  END_COLLECTION(0)
};

// Connection status callback
class MyCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    connected = true;
    BLE2902* desc = (BLE2902*)input->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
    desc->setNotifications(true);
    Serial.println("AirMouse connected!");
  }

  void onDisconnect(BLEServer* pServer) {
    connected = false;
    Serial.println("AirMouse disconnected!");
    BLEDevice::startAdvertising();
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting AirMouse BLE HID...");

  // Initialize BLE
  BLEDevice::init("AirMouse");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyCallbacks());

  // Create HID Device
  hid = new BLEHIDDevice(pServer);
  input = hid->inputReport(1); // Report ID 1
  output = hid->outputReport(1);

  // Set HID info
  hid->manufacturer()->setValue("AirMouse Corp");
  hid->pnp(0x02, 0xe502, 0xa111, 0x0210);
  hid->hidInfo(0x00, 0x02);

  // Set report map (HID descriptor)
  hid->reportMap((uint8_t*)hidReportDescriptor, sizeof(hidReportDescriptor));
  hid->startServices();

  // Configure advertising
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->setAppearance(HID_MOUSE);
  pAdvertising->addServiceUUID(hid->hidService()->getUUID());
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  
  // Set device name in advertising data
  BLEAdvertisementData advertisementData;
  advertisementData.setName("AirMouse");
  advertisementData.setCompleteServices(BLEUUID(hid->hidService()->getUUID()));
  pAdvertising->setAdvertisementData(advertisementData);

  // Start advertising
  pAdvertising->start();
  Serial.println("AirMouse advertising started!");
  
  hid->setBatteryLevel(100);
}

void loop() {
  if (connected) {
    // Example mouse movements - replace with your sensor data
    // You can read from accelerometer, gyroscope, or other sensors here
    
    // Simple demo: move mouse in a circle pattern
    static unsigned long lastUpdate = 0;
    static float angle = 0;
    
    if (millis() - lastUpdate > 50) { // Update every 50ms
      lastUpdate = millis();
      
      // Calculate circular movement (demo purposes)
      mouseX = (int8_t)(5 * cos(angle));
      mouseY = (int8_t)(5 * sin(angle));
      angle += 0.1;
      
      if (angle > 2 * PI) angle = 0;
      
      // Send mouse report
      sendMouseReport(mouseButtons, mouseX, mouseY, mouseWheel);
      
      // Reset movement after sending
      mouseX = 0;
      mouseY = 0;
      mouseWheel = 0;
    }
    
    // Check for button presses (example using built-in button)
    // Replace with your actual button reading logic
    static bool lastButtonState = false;
    bool currentButtonState = digitalRead(0) == LOW; // Boot button on most ESP32 boards
    
    if (currentButtonState != lastButtonState) {
      lastButtonState = currentButtonState;
      
      if (currentButtonState) {
        mouseButtons |= 0x01; // Left click
      } else {
        mouseButtons &= ~0x01; // Release left click
      }
      
      sendMouseReport(mouseButtons, 0, 0, 0);
    }
  }
  
  delay(10);
}

void sendMouseReport(uint8_t buttons, int8_t x, int8_t y, int8_t wheel) {
  if (connected) {
    uint8_t report[4];
    report[0] = buttons;
    report[1] = x;
    report[2] = y;
    report[3] = wheel;
    
    input->setValue(report, 4);
    input->notify();
  }
}

// Function to move mouse cursor
void moveMouse(int8_t x, int8_t y) {
  mouseX = x;
  mouseY = y;
  sendMouseReport(mouseButtons, mouseX, mouseY, mouseWheel);
}

// Function to click mouse button
void clickMouse(uint8_t button, bool press) {
  if (press) {
    mouseButtons |= button;
  } else {
    mouseButtons &= ~button;
  }
  sendMouseReport(mouseButtons, 0, 0, 0);
}

// Function to scroll wheel
void scrollWheel(int8_t direction) {
  mouseWheel = direction;
  sendMouseReport(mouseButtons, 0, 0, mouseWheel);
}

// Convenience functions for common mouse actions
void leftClick() {
  clickMouse(0x01, true);
  delay(50);
  clickMouse(0x01, false);
}

void rightClick() {
  clickMouse(0x02, true);
  delay(50);
  clickMouse(0x02, false);
}

void middleClick() {
  clickMouse(0x04, true);
  delay(50);
  clickMouse(0x04, false);
}