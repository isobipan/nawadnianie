// Test płytki ESP32 ideaspark v4.1
// Faza 1: Weryfikacja połączenia i miganie LED

void setup() {
  Serial.begin(115200);
  pinMode(2, OUTPUT);  // LED wbudowany ESP32
  Serial.println("ESP32 dziala!");
}

void loop() {
  digitalWrite(2, HIGH);
  delay(500);
  digitalWrite(2, LOW);
  delay(500);
}
