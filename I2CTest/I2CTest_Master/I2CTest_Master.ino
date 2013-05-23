
#include <Wire.h>
#include <EEPROM.h>

#include <I2CUtils.h>

#define PIN_STATUS_LED 12
#define PIN_DEBUG_LED  13

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
#define NUM_PINS 4
boolean led_value[NUM_SLAVES][NUM_PINS] = {
  {0, 0, 0, 0},
  {0, 0, 0, 0},
  {0, 0, 0, 0},
  {0, 0, 0, 0},
  {0, 0, 0, 0},
  {0, 0, 0, 0}
};
int slave_id[NUM_SLAVES] = {0, 3, 2, 4, 1, 5};
int slave_pin[NUM_SLAVES][NUM_PINS] = {
  {12, 9, 10, 11},
  {12, 9, 10, 11},
  {12, 9, 10, 11},
  {12, 9, 10, 11},
  {12, 9, 10, 11},
  {12, 9, 10, 11},
};

boolean debug_led = false;

int cycle = 0;

#define MODE   3
#define PERIOD 100

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

/* Update hte state of all devices */
void update_state() 
{
  int slave;
  int pin;

  switch (MODE) {
      case 0:
        for (slave = 0; slave < NUM_SLAVES; slave++) {
          led_value[slave][0] = (led_value[slave][0] == 0 ? 255 : 0);
        }
        break;
      case 1:
        for (slave = 0; slave < NUM_SLAVES; slave++) {
          led_value[slave][0] = ((slave == (cycle % NUM_SLAVES)) ? 255 : 0);
        }
        break;
      case 2:
        for (slave = 0; slave < NUM_SLAVES; slave++) {
          led_value[slave][0] = ((slave == (cycle % NUM_SLAVES)) ? 255 : 0);
          led_value[slave][1] = ((slave == (cycle % NUM_SLAVES)) ? 255 : 0);
        }
        break;
      case 3:
        static int current_pin = 0;
        static int current_slave = 0;
        static boolean current_on = true;

        led_value[current_slave][current_pin] = (current_on ? 255 : 0);

        if (!current_on) {
          current_pin = (current_pin + 1) % NUM_PINS;
          if (current_pin == 0) current_slave = (current_slave + 1) % NUM_SLAVES;
        }
        current_on = !current_on;
        break;
      case 4:
        for (slave = 0; slave < NUM_SLAVES; slave++) {
          for (pin = 0; pin < NUM_PINS; pin++) {
            led_value[slave][pin] = (led_value[slave][pin] + 10) % 256;
          }
        }

        break;
  }
}

/* Send a state update to the indicated device */
void send_state(int slave) 
{
  Wire.beginTransmission(slave_id[slave]);

  switch (MODE) {
      case 0:
      case 1:
        Wire.write(slave_pin[slave][0]);               // Pin to set
        Wire.write(led_value[slave][0]);               // Pin value        
        break;
      case 2:
        Wire.write(slave_pin[slave][0]);               // Pin to set
        Wire.write(led_value[slave][0]);               // Pin value

        Wire.write(slave_pin[slave][1]);               // Pin to set
        Wire.write(led_value[slave][1]);               // Pin value
        break;
      case 3:
      case 4:
        for (int pin = 0; pin < NUM_PINS; pin++) {
          message_t msg = {slave_pin[slave][pin],
                           led_value[slave][pin]
          };
          I2C_write_struct((byte *)&msg, sizeof (msg));
        }
        break;
  }

  Wire.endTransmission();
}
