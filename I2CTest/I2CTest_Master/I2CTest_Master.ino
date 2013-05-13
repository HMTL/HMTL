
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

#define NUM_SLAVES 3
boolean led_on[NUM_SLAVES] = {false, true, false};
int slave_id[NUM_SLAVES] = {0, 9, 1};

void loop()
{
  int slave;
  for (slave = 0; slave < NUM_SLAVES; slave++) {
    led_on[slave] = !led_on[slave];
  
    Wire.beginTransmission(slave_id[slave]);
    Wire.write(PIN_STATUS_LED);
    if (led_on[slave]) Wire.write(PACKET_ON);
    else Wire.write(PACKET_OFF);
    Wire.endTransmission();
  }
  
  if (led_on[0]) digitalWrite(PIN_DEBUG_LED, HIGH);
  else digitalWrite(PIN_DEBUG_LED, LOW);
  
  delay(1000);
}
