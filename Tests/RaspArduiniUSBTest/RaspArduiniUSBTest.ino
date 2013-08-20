/*
 * This is a trivial test program for communicating with an Arduino over
 * a USB serial connection.
 */

const int ledPin = 11;

void setup(){
  pinMode(ledPin, OUTPUT);
  Serial.begin(9600);
}

void loop(){
  if (Serial.available())  {
    byte value = Serial.read();
    Serial.print((char)value);
    if ((value > '0') && (value <= '9')) {
      light(value - '0');
    }
  }
  delay(500);
}

void light(int n){
  for (int i = 0; i < n; i++)  {
    digitalWrite(ledPin, HIGH);
    delay(100);
    digitalWrite(ledPin, LOW);
    delay(100);
  }
}
