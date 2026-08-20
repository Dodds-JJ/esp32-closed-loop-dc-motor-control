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
// TB6612 MOTOR DRIVER
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
// CONTROLLER SETTINGS
// ===============================

float targetRPM = 40.0;

// Proportional gain
float Kp = 2.0;

// Starting PWM
float pwmCommand = 120.0;


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


  // ----- OLED -----

  Wire.begin(21, 22);

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        0x3C)) {

    Serial.println(
      "OLED initialization failed");

    while (true);
  }

  display.clearDisplay();
  display.setTextColor(
    SSD1306_WHITE);


  // ----- MOTOR -----

  pinMode(PWMA, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(STBY, OUTPUT);

  digitalWrite(STBY, HIGH);

  // Forward direction
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);


  // ----- ENCODER -----

  pinMode(encoderA, INPUT);
  pinMode(encoderB, INPUT);

  attachInterrupt(
    digitalPinToInterrupt(encoderA),
    readEncoder,
    CHANGE
  );


  Serial.println(
    "P-Control Test Starting");
}


// ===============================
// MAIN CONTROL LOOP
// ===============================

void loop() {

  // -------------------------------
  // Measure encoder counts
  // -------------------------------

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


  // -------------------------------
  // Calculate RPM
  // -------------------------------

  float rpm =
    (abs(deltaCounts)
     / COUNTS_PER_REV)
     * 120.0;


  // -------------------------------
  // Calculate control error
  // -------------------------------

  float error =
    targetRPM - rpm;


  // -------------------------------
  // P CONTROL
  // -------------------------------

  pwmCommand =
    pwmCommand + Kp * error;


  // Prevent invalid PWM values

  if (pwmCommand > 255)
    pwmCommand = 255;

  if (pwmCommand < 0)
    pwmCommand = 0;


  // Send PWM to motor

  analogWrite(
    PWMA,
    (int)pwmCommand
  );


  // -------------------------------
  // SERIAL MONITOR
  // -------------------------------

  Serial.print("Target: ");
  Serial.print(targetRPM);

  Serial.print("   RPM: ");
  Serial.print(rpm);

  Serial.print("   Error: ");
  Serial.print(error);

  Serial.print("   PWM: ");
  Serial.println(pwmCommand);


  // -------------------------------
  // OLED
  // -------------------------------

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