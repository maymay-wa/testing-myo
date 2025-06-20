#include <BleMouse.h>

BleMouse bleMouse("uMyo Airmouse", "YourCompany", 100);  // Name, manufacturer, battery %

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE Mouse...");

  bleMouse.begin();
}

void loop() {
  if (bleMouse.isConnected()) {
    // Example movement: small move to the right
    bleMouse.move(5, 0);  // x=5, y=0
    delay(500);

    // You can replace this with real sensor input
  } else {
    Serial.println("Waiting for connection...");
    delay(1000);
  }
}