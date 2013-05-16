
#include <Wire.h>
#include <EEPROM.h>

#include <I2CUtils.h>

#define PIN_STATUS_LED 12
#define PIN_DEBUG_LED  13

#define PACKET_OFF 'F'
#define PACKET_ON  'N'



void setup()
{
  Serial.begin(9600);

  Wire.begin(); // Start I2C Bus as Master

  pinMode(PIN_STATUS_LED, OUTPUT);
  digitalWrite(PIN_STATUS_LED, HIGH);

  pinMode(PIN_DEBUG_LED, OUTPUT);
  digitalWrite(PIN_DEBUG_LED, HIGH);
}

#define NUM_SLAVES 6
boolean led_on[NUM_SLAVES] = {false, false, false, false, false, false};
int slave_id[NUM_SLAVES] = {0, 3, 2, 4, 1, 5};

boolean debug_led = false;

int cycle = 0;

#define MODE   2
#define PERIOD 250

void loop()
{
  long start = millis();
  int slave;

  update_state();

  for (slave = 0; slave < NUM_SLAVES; slave++) {
    send_state(slave);
  }

  debug_led = !debug_led;
  if (debug_led) digitalWrite(PIN_DEBUG_LED, HIGH);
  else digitalWrite(PIN_DEBUG_LED, LOW);

  int elapsed = millis() - start;
  delay(PERIOD - (elapsed > 0 ? elapsed : 0));
  cycle++;
}

void update_state() 
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
        case 2:
          led_on[slave] = (slave == (cycle % NUM_SLAVES));
          break;
    }
  }
}

void send_state(int slave) 
{
  Wire.beginTransmission(slave_id[slave]);

  switch (MODE) {
      case 0:
      case 1:
        Wire.write(PIN_STATUS_LED);               // Pin to set
        if (led_on[slave]) Wire.write(PACKET_ON); // Pin value
        else Wire.write(PACKET_OFF);
        break;
      case 2:
        Wire.write(12);               // Pin to set
        if (led_on[slave]) Wire.write(PACKET_ON); // Pin value
        else Wire.write(PACKET_OFF);
        Wire.write(11);               // Pin to set
        if (led_on[slave]) Wire.write(PACKET_ON); // Pin value
        else Wire.write(PACKET_OFF);

        break;
  }

  Wire.endTransmission();
}
