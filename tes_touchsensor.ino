#define TOUCH1_PIN 4
#define TOUCH2_PIN 15

void setup() {
  Serial.begin(115200);
  Serial.println("Tes Touch Sensor di GPIO 4 dan 15");
}

void loop() {
  int val1 = touchRead(TOUCH1_PIN);
  int val2 = touchRead(TOUCH2_PIN);

  Serial.printf("Touch1: %d | Touch2: %d\n", val1, val2);
  delay(300);
}
