#define DEBUG_LEVEL DEBUG_HIGH
#include "Debug.h"

#include "SPI.h"
#include "Adafruit_WS2801.h"

#define HMTL_VERSION 2
#if HMTL_VERSION == 1
#define RED_LED    9
#define GREEN_LED 10
#define BLUE_LED  11

#define DEBUG_LED 13
#elif HMTL_VERSION == 2
#define RED_LED   10
#define GREEN_LED 11
#define BLUE_LED  13
#endif

#define DATA_PIN 12
#define CLOCK_PIN 8
Adafruit_WS2801 strip = Adafruit_WS2801(45, DATA_PIN, CLOCK_PIN);

void setup() {
  Serial.begin(9600);

  pinMode(RED_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

#ifdef DEBUG_LED
  pinMode(DEBUG_LED, OUTPUT);
#endif

  strip.begin();
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, 0);
  }
  strip.show();
}

void loop() {

#ifdef DEBUG_LED
  for (int i=0; i < strip.numPixels(); i++) 
    strip.setPixelColor(i, 255, 255, 255);  
  strip.show();
  digitalWrite(DEBUG_LED, HIGH);

  delay(1000);
  digitalWrite(DEBUG_LED, LOW);
#endif

  digitalWrite(RED_LED, HIGH);
  for (int i=0; i < strip.numPixels(); i++) 
    strip.setPixelColor(i, 255, 0, 0);  
  strip.show();

  delay(1000);

  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, HIGH);
  for (int i=0; i < strip.numPixels(); i++) 
    strip.setPixelColor(i, 0, 255, 0);  
  strip.show();

  delay(1000);

  digitalWrite(GREEN_LED, LOW);
  digitalWrite(BLUE_LED, HIGH);
  for (int i=0; i < strip.numPixels(); i++) 
    strip.setPixelColor(i, 0, 0, 255);  
  strip.show();

  delay(1000);

  digitalWrite(BLUE_LED, LOW);
}
