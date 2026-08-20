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
// PI CONTROLLER
// ==================================

float targetRPM = 30.0;

float Kp = 2.0;
float Ki = 2.0;

float integral = 0.0;

// Baseline PWM near 40 RPM
float baselinePWM = 145.0;

const float sampleTime = 0.5;

// ==================================
// STEP TEST SETTINGS
// ==================================

unsigned long testStartTime;

const unsigned long STEP_TIME_MS = 10000;

const float LOW_TARGET = 30.0;
const float HIGH_TARGET = 50.0;

// ==================================
// ENCODER INTERRUPT
// ==================================

void IRAM_ATTR readEncoder() {

  if (digitalRead(encoderA) ==
      digitalRead(encoderB)) {

    encoderCount++;

  } else {

    encoderCount--;
  }
}

// ==================================
// SETUP
// ==================================

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

  testStartTime = millis();

  // CSV header
  Serial.println(
    "Time_s,Target_RPM,Actual_RPM,Error,P_Term,I_Term,PWM"
  );
}

// ==================================
// MAIN LOOP
// ==================================

void loop() {

  // ----------------------------------
  // Set target based on elapsed time
  // ----------------------------------

  unsigned long elapsedMs =
      millis() - testStartTime;

  float elapsedSeconds =
      elapsedMs / 1000.0;

  if (elapsedMs < STEP_TIME_MS) {

    targetRPM = LOW_TARGET;

  } else {

    targetRPM = HIGH_TARGET;
  }

  // ----------------------------------
  // Measure encoder counts
  // ----------------------------------

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

  // ----------------------------------
  // Calculate RPM
  // ----------------------------------

  float rpm =
      (abs(deltaCounts)
       / COUNTS_PER_REV)
       * 120.0;

  // ----------------------------------
  // PI CONTROL
  // ----------------------------------

  float error =
      targetRPM - rpm;

  integral +=
      error * sampleTime;

  // Anti-windup limit
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
      constrain(
        pwmCommand,
        0.0,
        255.0
      );

  analogWrite(
    PWMA,
    (int)pwmCommand
  );

  // ----------------------------------
  // SERIAL OUTPUT — CSV FORMAT
  // ----------------------------------

  Serial.print(elapsedSeconds, 2);
  Serial.print(",");

  Serial.print(targetRPM, 2);
  Serial.print(",");

  Serial.print(rpm, 2);
  Serial.print(",");

  Serial.print(error, 2);
  Serial.print(",");

  Serial.print(proportionalTerm, 2);
  Serial.print(",");

  Serial.print(integralTerm, 2);
  Serial.print(",");

  Serial.println(pwmCommand, 2);

  // ----------------------------------
  // OLED
  // ----------------------------------

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