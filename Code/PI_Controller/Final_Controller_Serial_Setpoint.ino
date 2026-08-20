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
// MOTOR DRIVER - TB6612FNG
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
// POTENTIOMETER
// ==================================

const int POT_PIN = 34;

const float MIN_RPM = 25.0;
const float MAX_RPM = 50.0;

// ==================================
// CONTROLLER
// ==================================

float targetRPM = 25.0;

const float Kp = 0.5;
const float Ki = 0.5;

float integral = 0.0;

// Nominal measurement period
const float sampleTime = 0.5;

// Experimentally derived feedforward model
// PWM = 3.245 * Target RPM + 6.53

const float FF_SLOPE = 3.245;
const float FF_OFFSET = 6.53;

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

  // ------------------------------
  // OLED
  // ------------------------------

  Wire.begin(21, 22);

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        0x3C)) {

    Serial.println("OLED initialization failed");

    while (true);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // ------------------------------
  // MOTOR DRIVER
  // ------------------------------

  pinMode(PWMA, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(STBY, OUTPUT);

  digitalWrite(STBY, HIGH);

  // Forward direction
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);

  // ------------------------------
  // ENCODER
  // ------------------------------

  pinMode(encoderA, INPUT);
  pinMode(encoderB, INPUT);

  attachInterrupt(
    digitalPinToInterrupt(encoderA),
    readEncoder,
    CHANGE
  );

  // ------------------------------
  // POTENTIOMETER
  // ------------------------------

  pinMode(POT_PIN, INPUT);

  int initialPotValue =
      analogRead(POT_PIN);

  targetRPM =
      MIN_RPM +
      (initialPotValue / 4095.0) *
      (MAX_RPM - MIN_RPM);

  Serial.println();
  Serial.println(
    "Standalone Closed-Loop Motor Controller"
  );
}

// ==================================
// MAIN LOOP
// ==================================

void loop() {

  // =================================
  // READ POTENTIOMETER
  // =================================

  int potValue =
      analogRead(POT_PIN);

  float requestedRPM =
      MIN_RPM +
      (potValue / 4095.0) *
      (MAX_RPM - MIN_RPM);

  // Smooth potentiometer command
  targetRPM =
      0.90 * targetRPM +
      0.10 * requestedRPM;


  // =================================
  // MEASURE MOTOR SPEED
  // =================================

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
      (abs(deltaCounts) /
       COUNTS_PER_REV) * 120.0;


  // =================================
  // CONTROL ERROR
  // =================================

  float error =
      targetRPM - rpm;


  // =================================
  // FEEDFORWARD
  // =================================

  float feedforwardPWM =
      FF_SLOPE * targetRPM
      + FF_OFFSET;


  // =================================
  // PI FEEDBACK
  // =================================

  integral +=
      error * sampleTime;

  // Anti-windup
  integral = constrain(
      integral,
      -50.0,
      50.0
  );

  float proportionalTerm =
      Kp * error;

  float integralTerm =
      Ki * integral;


  // =================================
  // FINAL PWM COMMAND
  // =================================

  float pwmCommand =
      feedforwardPWM
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


  // =================================
  // SERIAL TELEMETRY
  // =================================

  Serial.print("Pot: ");
  Serial.print(potValue);

  Serial.print("  Target: ");
  Serial.print(targetRPM, 1);

  Serial.print("  Actual: ");
  Serial.print(rpm, 1);

  Serial.print("  Error: ");
  Serial.print(error, 1);

  Serial.print("  PWM: ");
  Serial.println(pwmCommand, 1);


  // =================================
  // OLED TELEMETRY
  // =================================

  display.clearDisplay();

  display.setTextSize(1);

  display.setCursor(0, 0);
  display.print("TARGET: ");
  display.print(targetRPM, 1);
  display.print(" RPM");

  display.setCursor(0, 16);
  display.print("ACTUAL: ");
  display.print(rpm, 1);
  display.print(" RPM");

  display.setCursor(0, 32);
  display.print("ERROR:  ");
  display.print(error, 1);

  display.setCursor(0, 48);
  display.print("PWM:    ");
  display.print(pwmCommand, 0);

  display.display();
}