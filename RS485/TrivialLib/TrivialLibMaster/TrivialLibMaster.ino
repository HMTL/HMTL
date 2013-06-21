#include <SoftwareSerial.h>
#include <RS485_non_blocking.h>

SoftwareSerial rs485 (2, 3);  // receive pin, transmit pin

RS485 myChannel (NULL, NULL, fWrite, 0);

#define ENABLE_PIN 4
#define LED_PIN 13

void setup()
{
  Serial.begin(9600);
  
  rs485.begin (28800);
  myChannel.begin ();

  pinMode (LED_PIN, OUTPUT);  // driver output enable

  pinMode (ENABLE_PIN, OUTPUT);  // driver output enable
}

byte i = 0;
boolean b = false;

void loop() {
  i++;

  digitalWrite(ENABLE_PIN, HIGH);
//  rs485.println(i);
//  sendMsg (fWrite, &i, sizeof i);
  myChannel.sendMsg(&i, sizeof i);
  digitalWrite(ENABLE_PIN, LOW);

  Serial.print("Sent ");
  Serial.println(i);

  b = !b;
  digitalWrite(LED_PIN, b);

  delay(1000);
}

size_t fWrite (const byte what)
  {
      Serial.print("write:");
  Serial.println(what);
  return rs485.write (what);  
  }
  
int fAvailable ()
  {
  return rs485.available ();  
  }

int fRead ()
  {
  return rs485.read ();  
  }

