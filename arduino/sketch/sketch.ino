#include <Trill.h>
#include <FastLED.h>
#include <Wire.h>
#include "TCA9548.h"

#define NUM_ROWS 4
#define NUM_RINGS_PER_ROW 5
#define NUM_RINGS (NUM_ROWS * NUM_RINGS_PER_ROW)
#define LED_PIN 11
#define LEDS_PER_RING 12
#define NUM_LEDS (LEDS_PER_RING * NUM_RINGS)
#define BRIGHTNESS 64
#define LED_TYPE WS2811
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

Trill rings[NUM_RINGS];                                // Array to hold the Trill ring objects
float volumes[NUM_RINGS] = { 0.0 };                    // Array to store volume levels
float lastTouchLocation[NUM_RINGS] = { 0.0 };          // Array to store the last touch location
bool isTouching[NUM_RINGS] = { false };                // Array to store touch states
float sensitivity = 4;


#define TCAADDR 0x70

TCA9548 tca(TCAADDR, &Wire1);

void updateLEDs(float value, int ringIndex) {
  int startLED = ringIndex * LEDS_PER_RING;
  int endLED = startLED + LEDS_PER_RING;
  int numFullLEDs = (int)(value * LEDS_PER_RING);
  float fractionalLED = (value * LEDS_PER_RING) - numFullLEDs;

  for (int i = startLED; i < endLED; i++) {
    int hue = map(i, startLED, endLED, 0, 255);

    if (i < startLED + numFullLEDs) {
      leds[i] = CHSV(hue, 255, 255);
    } else if (i == startLED + numFullLEDs) {
      leds[i] = CHSV(hue, 255, 255);
      leds[i].fadeLightBy(255 - (fractionalLED * 255));
    } else {
      leds[i] = CRGB::Black;
    }
  }

  FastLED.show();
}

void setup() {
  delay(6000);
  Serial.begin(9600);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  Wire1.begin();
  Wire1.setClock( 400000UL);

  if (!tca.begin()) {
      Serial.println("Failed to initialize TCA9548. Check connections!");
      while (1);
  }

  for (int row = 0; row < NUM_ROWS; row++) {
    int port;
    if (row == 0) port = 7;
    if (row == 1) port = 6;
    if (row == 2) port = 4;
    if (row == 3) port = 3;
    if (!tca.selectChannel(port)) {
        Serial.print("Failed to select TCA channel "); Serial.println(port);
        continue;
    }
    for (int i = 0; i < NUM_RINGS_PER_ROW; i++) {
      int ringIndex = row * NUM_RINGS_PER_ROW + i;
      int ret = rings[ringIndex].setup(Trill::TRILL_RING, 56 + i, &Wire1);
      if (ret != 0) {
        Serial.print("Failed to initialise Trill ring ");
        Serial.print(ringIndex);
        Serial.print(" with error code: ");
        Serial.println(ret);
      } else {
        
        rings[ringIndex].setNoiseThreshold(150);
        // rings[ringIndex].setScanSettings(3, 16);
      }
    }
  }

  Serial.println("Initialized!");
}

void loop() {
  // delay(50);
  for (int row = 0; row < NUM_ROWS; row++) {
    
    int port;
    if (row == 0) port = 7;
    if (row == 1) port = 6;
    if (row == 2) port = 4;
    if (row == 3) port = 3;
    if (!tca.selectChannel(port)) {
        Serial.print("Failed to select TCA channel "); Serial.println(port);
        continue;
    }

    for (int i = 0; i < NUM_RINGS_PER_ROW; i++) {
      int ringIndex = row * NUM_RINGS_PER_ROW + i;
      rings[ringIndex].read();
      int numTouches = rings[ringIndex].getNumTouches();

      if (numTouches > 0) {
        float touchLocation = rings[ringIndex].touchLocation(0) / 3600.0;
        if (!isTouching[ringIndex]) {
          lastTouchLocation[ringIndex] = touchLocation;
          volumes[ringIndex] = 0.0;
          isTouching[ringIndex] = true;
        }

        float delta = touchLocation - lastTouchLocation[ringIndex];
        if (delta > 0.5) delta -= 1.0;
        else if (delta < -0.5) delta += 1.0;

        volumes[ringIndex] += delta * sensitivity;
        volumes[ringIndex] = constrain(volumes[ringIndex], 0.0, 1.0);
        lastTouchLocation[ringIndex] = touchLocation;

      } else {
        volumes[ringIndex] = 0.0;
        isTouching[ringIndex] = false;
      }

      int midiValue = map(volumes[ringIndex], 0, 1, 0, 127);  // Scale volume to MIDI range [0, 127]
      usbMIDI.sendControlChange(21 + ringIndex, midiValue, 1);  // Sending on CC 21-24, Channel 1
    }
  }

  usbMIDI.send_now();

  // // Format and send volume levels
  // for (int i = 0; i < NUM_RINGS; i++) {
  //   Serial.print(volumes[i]);
  //   if (i < NUM_RINGS - 1) {
  //     Serial.print(", ");
  //   }
  // }
  // Serial.println("");
}
