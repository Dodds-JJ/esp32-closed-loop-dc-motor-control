// TB6612FNG open-loop motor test

const int PWMA = 25;
const int AIN1 = 26;
const int AIN2 = 27;
const int STBY = 14;

void setup() {
  pinMode(PWMA, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(STBY, OUTPUT);

  digitalWrite(STBY, HIGH); // enable driver
}

void loop() {
  // Forward
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);
  analogWrite(PWMA, 120);   // about 47% duty
  delay(3000);

  // Stop
  analogWrite(PWMA, 0);
  delay(3000);

  // Reverse
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  analogWrite(PWMA, 120);
  delay(3000);

  // Stop
  analogWrite(PWMA, 0);
  delay(3000);
}