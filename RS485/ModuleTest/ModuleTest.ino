/*
 * This code is intended to verify that a module is functioning properly
 */

#include "SPI.h"
#include "Adafruit_WS2801.h"

#define DEBUG_LEVEL DEBUG_HIGH
#include "Debug.h"
#include "PixelUtil.h"

// PWM outputs
#define PWM_PINS 6
#define MAX_VAL 64
uint8_t pwm_pins[PWM_PINS] = { 3, 5, 6, 9, 10, 11 };
uint8_t pwm_values[PWM_PINS] = {
  0,
  MAX_VAL * 1 / 4,
  MAX_VAL * 2 / 4,
  MAX_VAL * 3 / 4,
  MAX_VAL,
  0
};
#define PWM_STEP 2

// Pixel strand outputs
#define PIXEL_DATA 8 // Blue
#define PIXEL_CLOCK 12 // Green
#define PIXEL_NUM 50
Adafruit_WS2801 pixels = Adafruit_WS2801(PIXEL_NUM, PIXEL_DATA, PIXEL_CLOCK);


void setup() {
  Serial.begin(9600);

  for (int i = 0; i < PWM_PINS; i++) {
    pinMode(pwm_pins[i], OUTPUT);
  }

  pixels.begin();
  pixels.show();
}

#define DELAY 10
void loop() {

  /* Increment all 6 PWM outputs */
  for (int i = 0; i < PWM_PINS; i++) {
    pwm_values[i] = (pwm_values[i] + PWM_STEP) % MAX_VAL;
    analogWrite(pwm_pins[i], pwm_values[i]);
  }

  /* Shift through the pixels */
  static int currentPixel = 0;
  pixels.setPixelColor(currentPixel, 0);
  currentPixel = (currentPixel + 1) % pixels.numPixels();
  pixels.setPixelColor(currentPixel, pixel_color(255, 0, 0));
  pixels.show();

  delay(DELAY);
}
