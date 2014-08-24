#define NUM_SWITCHES 4
const int switch_pins[NUM_SWITCHES] = { 5, 6, 9, 10 };

#define LED_RED   11
#define LED_GREEN 13

void setup() {
  Serial.begin(9600);

  for (int i = 0; i < NUM_SWITCHES; i++) {
    pinMode(switch_pins[i], INPUT);
  }

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  
  Serial.println("Starting fire control bringup");
}

void loop() {
  for (int i = 0; i < NUM_SWITCHES; i++) {
    if (digitalRead(switch_pins[i]) == HIGH) {
      Serial.print("X");
    } else {
      Serial.print("-");
    }
  }
  Serial.println();

  delay(250);
}
