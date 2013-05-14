
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

#define NUM_SLAVES 4
boolean led_on[NUM_SLAVES] = {false, true, false, true};
int slave_id[NUM_SLAVES] = {0, 1, 2, 3};

boolean debug_led = false;

int cycle = 0;

#define MODE   1
#define PERIOD 500

void loop()
{
  long start = millis();
  int slave;

  update();
  
  for (slave = 0; slave < NUM_SLAVES; slave++) {
    Wire.beginTransmission(slave_id[slave]);
    Wire.write(PIN_STATUS_LED);
    if (led_on[slave]) Wire.write(PACKET_ON);
    else Wire.write(PACKET_OFF);
    Wire.endTransmission();
  }

  debug_led = !debug_led;
  if (debug_led) digitalWrite(PIN_DEBUG_LED, HIGH);
  else digitalWrite(PIN_DEBUG_LED, LOW);

  int elapsed = millis() - start;
  delay(PERIOD - (elapsed > 0 ? elapsed : 0));
  cycle++;
}

void update() 
{
  int slave;
  for (slave = 0; slave < NUM_SLAVES; slave++) {

    switch (MODE) {
        case 0:
          led_on[slave] = !led_on[slave];
          break;
        case 1:
          led_on[slave] = (slave == (cycle % NUM_SLAVES));
          break;
    }
  }
}
