const uint8_t PIN_R = 3;
const uint8_t PIN_G = 5;
const uint8_t PIN_B = 6;
const uint8_t PIN_W1 = 9;  // Alb Rece (Firul Mov)
const uint8_t PIN_W2 = 10; // Alb Cald / Galben (Firul Galben stânga)

void stingeTot() {
  analogWrite(PIN_R, 0);
  analogWrite(PIN_G, 0);
  analogWrite(PIN_B, 0);
  analogWrite(PIN_W1, 0);
  analogWrite(PIN_W2, 0);
}

void setup() {
  pinMode(PIN_R, OUTPUT);
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_B, OUTPUT);
  pinMode(PIN_W1, OUTPUT);
  pinMode(PIN_W2, OUTPUT);
  
  stingeTot();
}

void loop() {
  // 1. Test Rosu
  stingeTot();
  analogWrite(PIN_R, 255);
  delay(10000);

  // 2. Test Verde
  stingeTot();
  analogWrite(PIN_G, 255);
  delay(10000);

  // 3. Test Albastru
  stingeTot();
  analogWrite(PIN_B, 255);
  delay(10000);

  // 4. Test Alb Rece
  stingeTot();
  analogWrite(PIN_W1, 255);
  delay(10000);

  // 5. Test Alb Cald (Galben)
  stingeTot();
  analogWrite(PIN_W2, 255);
  delay(10000);
}