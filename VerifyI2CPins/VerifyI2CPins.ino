
/*
 * This code is intended to verify that the I2C pins (A4 and A5) are working
 * correctly, for instance after stupid wiring mistakes.
 */
 
// Pin 13 has an LED connected on most Arduino boards.
// give it a name:
int led = 13;

// the setup routine runs once when you press reset:
void setup() {            
  Serial.begin(9600);
  
  // initialize the digital pin as an output.
  pinMode(led, OUTPUT);     
  pinMode(A4, INPUT);
  pinMode(A5, INPUT);
}

// the loop routine runs over and over again forever:
void loop() {
  Serial.print(analogRead(A4));
  Serial.print("-");
  Serial.println(analogRead(A5));
  
  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(500);               // wait for a second
  digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
  delay(500);               // wait for a second
}
