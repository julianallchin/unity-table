#include <Trill.h>
#include <FastLED.h>

#define NUM_RINGS  4
#define LED_PIN     11
#define LEDS_PER_RING 12
#define NUM_LEDS (LEDS_PER_RING * NUM_RINGS)
#define BRIGHTNESS  64
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

Trill rings[NUM_RINGS];                                // Array to hold the Trill ring objects
float volumes[NUM_RINGS] = { 0.0, 0.0, 0.0, 0.0 };      // Array to store volume levels
float lastTouchLocation[NUM_RINGS] = { 0.0, 0.0, 0.0, 0.0 };  // Array to store the last touch location
bool isTouching[NUM_RINGS] = { false, false, false, false };  // Array to store touch states
float sensitivity = 4;

void updateLEDs(float value, int ringIndex) {
  int startLED = ringIndex * LEDS_PER_RING;
  int endLED = startLED + LEDS_PER_RING;
  int numFullLEDs = (int)(value * LEDS_PER_RING); // Calculate the number of full LEDs
  float fractionalLED = (value * LEDS_PER_RING) - numFullLEDs; // Calculate the fractional part

  // Turn on the required number of LEDs for the specified ring
  for (int i = startLED; i < endLED; i++) {
    int hue = map(i, startLED, endLED, 0, 255); // Map the LED index to a hue value

    if (i < startLED + numFullLEDs) {
      leds[i] = CHSV(hue, 255, 255); // Full brightness for full LEDs with rainbow color
    } else if (i == startLED + numFullLEDs) {
      leds[i] = CHSV(hue, 255, 255);
      leds[i].fadeLightBy(255 - (fractionalLED * 255)); // Set brightness proportionally for the fractional LED
    } else {
      leds[i] = CRGB::Black; // Turn off the remaining LEDs
    }
  }

  FastLED.show(); // Update the LEDs to the new settings
}

void setup() {
  delay(3000);
  // Initialize the serial communication
  Serial.begin(115200);

  // Setup LEDS
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness( BRIGHTNESS );
  int ret;

  // Initialize the Trill ring sensors
  for (int i = 0; i < NUM_RINGS; i++) {
    ret = rings[i].setup(Trill::TRILL_RING, 56 + i);

    if (ret != 0) {
      Serial.print("Failed to initialise Trill ring ");
      Serial.print(i);
      Serial.print(" with error code: ");
      Serial.println(ret);
    } else {
      rings[i].setNoiseThreshold(220);
    }
  }
}

void loop() {
  delay(50);  // Read sensor 20 times per second

  // Read the touch data from each sensor and update the volumes array
  for (int i = 0; i < NUM_RINGS; i++) {
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
  for (int i = 0; i < NUM_RINGS; i++) {
    int midiValue = map(volumes[i], 0, 1, 0, 127);  // Scale volume to MIDI range [0, 127]
    usbMIDI.sendControlChange(21 + i, midiValue, 1);  // Sending on CC 21-24, Channel 1
  }

  usbMIDI.send_now();

  // Update LEDs based on the volume level for each ring
  for (int i = 0; i < NUM_RINGS; i++) {
    updateLEDs(volumes[i], i);
  }

  // Format and send volume levels
  for (int i = 0; i < NUM_RINGS; i++) {
    Serial.print(volumes[i]);
    if (i < NUM_RINGS - 1) {
      Serial.print(", ");
    }
  }
  Serial.println("");
}
