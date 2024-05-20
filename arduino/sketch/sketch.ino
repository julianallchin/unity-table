#include <Trill.h>

Trill trillSensor;
boolean isTouched = false;

void setup() {
  // Initialise the touch sensor
  Serial.begin(115200);
  int ret = trillSensor.setup(Trill::TRILL_RING);
  if(ret != 0) {
    Serial.println("Failed to initialise trillSensor");
    Serial.print("Error code: ");
    Serial.println(ret);
  }
}

void loop() {
  delay(50); // Read sensor 20 times per second
  trillSensor.read();

  // Check if there are any touches
  if(trillSensor.getNumTouches() > 0) {
    isTouched = true;
  } else {
    isTouched = false;
  }
  Serial.print("<");
  Serial.print(isTouched);
  Serial.println(">");
}
