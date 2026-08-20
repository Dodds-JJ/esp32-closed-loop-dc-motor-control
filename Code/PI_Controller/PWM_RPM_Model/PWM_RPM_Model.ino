#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ===============================
// OLED
// ===============================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  -1
);

// ===============================
// MOTOR DRIVER
// ===============================

const int PWMA = 25;
const int AIN1 = 26;
const int AIN2 = 27;
const int STBY = 14;

// ===============================
// ENCODER
// ===============================

const int encoderA = 32;
const int encoderB = 33;

const float COUNTS_PER_REV = 816.0;

volatile long encoderCount = 0;

// ===============================
// TEST SETTINGS
// ===============================

int pwmValues[] = {
  80, 100, 120, 140, 160, 180
};

const int numPWMValues =
  sizeof(pwmValues) / sizeof(pwmValues[0]);

const int samplesPerPWM = 8;
const int sampleDelayMs = 500;

// ===============================
// ENCODER INTERRUPT
// ===============================

void IRAM_ATTR readEncoder() {
  if (digitalRead(encoderA) ==
      digitalRead(encoderB)) {
    encoderCount++;
  } else {
    encoderCount--;
  }
}

// ===============================
// SETUP
// ===============================

void setup() {
  Serial.begin(115200);

  Wire.begin(21, 22);

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        0x3C)) {
    Serial.println("OLED failed");
    while (true);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  pinMode(PWMA, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(STBY, OUTPUT);

  pinMode(encoderA, INPUT);
  pinMode(encoderB, INPUT);

  attachInterrupt(
    digitalPinToInterrupt(encoderA),
    readEncoder,
    CHANGE
  );

  digitalWrite(STBY, HIGH);

  // Forward direction
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);

  Serial.println("PWM,RPM");
}

// ===============================
// MAIN LOOP
// ===============================

void loop() {

  for (int i = 0; i < numPWMValues; i++) {

    int pwm = pwmValues[i];

    analogWrite(PWMA, pwm);

    // Let motor settle
    delay(3000);

    float rpmSum = 0.0;

    for (int sample = 0;
         sample < samplesPerPWM;
         sample++) {

      long previousCount;

      noInterrupts();
      previousCount = encoderCount;
      interrupts();

      delay(sampleDelayMs);

      long currentCount;

      noInterrupts();
      currentCount = encoderCount;
      interrupts();

      long deltaCounts =
        currentCount - previousCount;

      float rpm =
        (abs(deltaCounts) /
         COUNTS_PER_REV) * 120.0;

      rpmSum += rpm;

      // Serial output
      Serial.print(pwm);
      Serial.print(",");
      Serial.println(rpm, 2);

      // OLED
      display.clearDisplay();

      display.setTextSize(1);

      display.setCursor(0, 0);
      display.print("OPEN LOOP TEST");

      display.setCursor(0, 20);
      display.print("PWM: ");
      display.print(pwm);

      display.setCursor(0, 40);
      display.print("RPM: ");
      display.print(rpm, 1);

      display.display();
    }

    float averageRPM =
      rpmSum / samplesPerPWM;

    Serial.print("AVG,");
    Serial.print(pwm);
    Serial.print(",");
    Serial.println(averageRPM, 2);

    analogWrite(PWMA, 0);

    delay(2000);
  }

  // Stop after one complete sweep
  analogWrite(PWMA, 0);

  Serial.println("TEST COMPLETE");

  while (true) {
    delay(1000);
  }
}