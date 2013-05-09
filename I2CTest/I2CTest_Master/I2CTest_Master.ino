
#include <Wire.h>

#define PIN_STATUS_LED 12
#define PIN_DEBUG_LED  13

#define PACKET_OFF 'F'
#define PACKET_ON  'N'



void setup()
{
  Wire.begin(); // Start I2C Bus as Master

  pinMode(PIN_STATUS_LED, OUTPUT);
  digitalWrite(PIN_STATUS_LED, LOW);

  pinMode(PIN_DEBUG_LED, OUTPUT);
  digitalWrite(PIN_DEBUG_LED, LOW);
}


boolean led_on = true;

void loop()
{  
  led_on = !led_on;

  Wire.beginTransmission(0xF);
  Wire.write(PIN_STATUS_LED);
  if (led_on) Wire.write(PACKET_ON);
  else Wire.write(PACKET_OFF);
  Wire.endTransmission();

  if (led_on) digitalWrite(PIN_DEBUG_LED, HIGH);
  else digitalWrite(PIN_DEBUG_LED, LOW);
  
  delay(1000);
}
