#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ==================================
// OLED
// ==================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  -1
);

// ==================================
// TB6612 MOTOR DRIVER
// ==================================

const int PWMA = 25;
const int AIN1 = 26;
const int AIN2 = 27;
const int STBY = 14;

// ==================================
// ENCODER
// ==================================

const int encoderA = 32;
const int encoderB = 33;

const float COUNTS_PER_REV = 816.0;

volatile long encoderCount = 0;

// ==================================
// CONTROL SETTINGS
// ==================================

float targetRPM = 40.0;

// Approximate PWM needed for 40 RPM
float baselinePWM = 145.0;

// Controller gains
float Kp = 2.0;
float Ki = 1.0;

// Integral accumulator
float integral = 0.0;

// Control-loop period
const float sampleTime = 0.5;   // seconds

void IRAM_ATTR readEncoder() {

  if (digitalRead(encoderA) ==
      digitalRead(encoderB)) {

    encoderCount++;

  } else {

    encoderCount--;
  }
}

void setup() {

  Serial.begin(115200);

  // OLED
  Wire.begin(21, 22);

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        0x3C)) {

    Serial.println("OLED initialization failed");
    while (true);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Motor driver
  pinMode(PWMA, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(STBY, OUTPUT);

  digitalWrite(STBY, HIGH);

  // Forward direction
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);

  // Encoder
  pinMode(encoderA, INPUT);
  pinMode(encoderB, INPUT);

  attachInterrupt(
    digitalPinToInterrupt(encoderA),
    readEncoder,
    CHANGE
  );

  Serial.println("PI Controller Starting");
}

void loop() {

  // ==================================
  // Measure speed
  // ==================================

  long previousCount;

  noInterrupts();
  previousCount = encoderCount;
  interrupts();

  delay(500);

  long currentCount;

  noInterrupts();
  currentCount = encoderCount;
  interrupts();

  long deltaCounts =
      currentCount - previousCount;

  float rpm =
      (abs(deltaCounts) / COUNTS_PER_REV)
      * 120.0;

  // ==================================
  // PI CONTROLLER
  // ==================================

  float error =
      targetRPM - rpm;

  // Integral term
  integral += error * sampleTime;

  // Limit integral to prevent windup
  integral = constrain(
      integral,
      -50.0,
      50.0
  );

  float proportionalTerm =
      Kp * error;

  float integralTerm =
      Ki * integral;

  float pwmCommand =
      baselinePWM
      + proportionalTerm
      + integralTerm;

  pwmCommand =
      constrain(pwmCommand, 0, 255);

  analogWrite(
      PWMA,
      (int)pwmCommand
  );

  // ==================================
  // SERIAL DATA
  // ==================================

  Serial.print("Target: ");
  Serial.print(targetRPM);

  Serial.print("  RPM: ");
  Serial.print(rpm);

  Serial.print("  Error: ");
  Serial.print(error);

  Serial.print("  P: ");
  Serial.print(proportionalTerm);

  Serial.print("  I: ");
  Serial.print(integralTerm);

  Serial.print("  PWM: ");
  Serial.println(pwmCommand);

  // ==================================
  // OLED
  // ==================================

  display.clearDisplay();

  display.setTextSize(1);

  display.setCursor(0, 0);
  display.print("TARGET: ");
  display.print(targetRPM, 0);

  display.setCursor(0, 16);
  display.print("ACTUAL: ");
  display.print(rpm, 1);

  display.setCursor(0, 32);
  display.print("ERROR:  ");
  display.print(error, 1);

  display.setCursor(0, 48);
  display.print("PWM:    ");
  display.print(pwmCommand, 0);

  display.display();
}