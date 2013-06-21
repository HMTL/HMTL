#include <SoftwareSerial.h>
#include <RS485_non_blocking.h>

SoftwareSerial rs485 (2, 3);  // receive pin, transmit pin
RS485 myChannel (fRead, fAvailable, NULL, 20);

#define ENABLE_PIN 4
#define LED_PIN 13

void setup()
{
  Serial.begin(9600);
  
  rs485.begin(28800);
  myChannel.begin ();

  pinMode (LED_PIN, OUTPUT);  // driver output enable

  pinMode (ENABLE_PIN, OUTPUT);  // driver output enable
  digitalWrite(ENABLE_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
}

byte i = 0;
boolean b = false;

void loop() {
  if (myChannel.update ()) {
    i = *(myChannel.getData ());

    Serial.print("recvd:");
    Serial.print(myChannel.getLength ());
    Serial.print(" = ");
    Serial.println(i);
    b = !b;
    digitalWrite(LED_PIN, b);
  }
}

void fWrite (const byte what)
  {
  rs485.write (what);  
  }
  
int fAvailable ()
  {
  return rs485.available ();  
  }

int fRead ()
  {
  return rs485.read ();  
  }

