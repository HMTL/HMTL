#include <Arduino.h>

#define DEBUG_PIN 13

void setup()
{
  Serial.begin(9600);
  pinMode(DEBUG_PIN, OUTPUT);

  Serial.println("Ready");
}

void loop()
{
  if (handle_command()) {
    digitalWrite(DEBUG_PIN, HIGH);
  } else {
    delay(500);
    digitalWrite(DEBUG_PIN, LOW);
  }
}

#define BUFF_LEN 128
boolean handle_command()
{
  static char buff[BUFF_LEN];
  static byte offset = 0;
  boolean received_command = false;

  while (Serial.available()) {
    buff[offset] = Serial.read();
    
    offset++;
    if (buff[offset - 1] == '\n') {
      handle_command(buff, offset);
      offset = 0;
      received_command = true;
    }

    if (offset >= BUFF_LEN) offset = 0;
  }

  return received_command;
}

void handle_command(char *cmd, byte len) {
  Serial.println("ok");
}
