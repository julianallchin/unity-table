#include <Trill.h>

Trill rings[4];                                       // Array to hold the four Trill ring objects
float volumes[4] = { 0.0, 0.0, 0.0, 0.0 };            // Array to store volume levels
float lastTouchLocation[4] = { 0.0, 0.0, 0.0, 0.0 };  // Array to store the last touch location
bool isTouching[4] = { false, false, false, false };  // Array to store touch states
float sensitivity = 2;

void setup() {
  // Initialize the serial communication
  Serial.begin(115200);
  int ret;

  // Initialize the Trill ring sensors
  for (int i = 0; i < 4; i++) {
    ret = rings[i].setup(Trill::TRILL_RING, 56 + i);

    if (ret != 0) {
      Serial.print("Failed to initialise Trill ring ");
      Serial.print(i);
      Serial.print(" with error code: ");
      Serial.println(ret);
    }
  }
}

void loop() {
  delay(50);  // Read sensor 20 times per second

  // Read the touch data from each sensor and update the volumes array
  for (int i = 0; i < 4; i++) {
    rings[i].read();
    int numTouches = rings[i].getNumTouches();

    if (numTouches > 0) {
      float touchLocation = rings[i].touchLocation(0) / 3600.0;  // Get the angle of the first touch

      if (!isTouching[i]) {
        // First touch: initialize lastTouchLocation
        lastTouchLocation[i] = touchLocation;
        volumes[i] = 0.0;  // Reset volume on first touch
        isTouching[i] = true;
      }

      // Calculate the delta (change in angle)
      float delta = touchLocation - lastTouchLocation[i];

      // Handle wrapping around the boundary
      if (delta > 0.5) {
        delta -= 1.0;
      } else if (delta < -0.5) {
        delta += 1.0;
      }

      // if (i == 0) {
      //   Serial.println(delta);
      // }

      // Update the volume, ensuring it stays within the range [0, 1]
      volumes[i] += delta * sensitivity;
      if (volumes[i] > 1.0) {
        volumes[i] = 1.0;
      } else if (volumes[i] < 0) {
        volumes[i] = 0;
      }

      // Update the last touch location
      lastTouchLocation[i] = touchLocation;
    } else {
      // No touch means volume is zero and reset the touch state
      volumes[i] = 0.0;
      isTouching[i] = false;
    }
  }

   // Send volume levels as MIDI Control Change messages
  for (int i = 0; i < 4; i++) {
    int midiValue = map(volumes[i], 0, 1, 0, 127);  // Scale volume to MIDI range [0, 127]
    usbMIDI.sendControlChange(21 + i, midiValue, 1);  // Sending on CC 21-24, Channel 1
  }

  usbMIDI.send_now();

  // Format and send volume levels
  for (int i = 0; i < 4; i++) {
    Serial.print(volumes[i]);
    if (i < 3) {
      Serial.print(", ");
    }
  }
  Serial.println("");
}
