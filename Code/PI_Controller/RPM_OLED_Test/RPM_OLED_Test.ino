#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ===== OLED =====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ===== TB6612 Motor Pins =====
const int PWMA = 25;
const int AIN1 = 26;
const int AIN2 = 27;
const int STBY = 14;

// ===== Encoder Pins =====
const int encoderA = 32;
const int encoderB = 33;

// ===== Encoder Calibration =====
const float COUNTS_PER_REV = 816.0;

volatile long encoderCount = 0;

void IRAM_ATTR readEncoder() {
  if (digitalRead(encoderA) == digitalRead(encoderB)) {
    encoderCount++;
  } else {
    encoderCount--;
  }
}

void setup() {
  Serial.begin(115200);

  // I2C / OLED
  Wire.begin(21, 22);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
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

  // Encoder
  pinMode(encoderA, INPUT);
  pinMode(encoderB, INPUT);

  attachInterrupt(
    digitalPinToInterrupt(encoderA),
    readEncoder,
    CHANGE
  );

  digitalWrite(STBY, HIGH);

  // Motor forward
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);
  analogWrite(PWMA, 120);

  Serial.println("RPM + OLED Test");
}

void loop() {
  long previousCount;

  noInterrupts();
  previousCount = encoderCount;
  interrupts();

  delay(500);

  long currentCount;

  noInterrupts();
  currentCount = encoderCount;
  interrupts();

  long deltaCounts = currentCount - previousCount;

  float rpm =
    (abs(deltaCounts) / COUNTS_PER_REV) * 120.0;

  // ===== Serial Monitor =====
  Serial.print("Delta Counts: ");
  Serial.print(deltaCounts);
  Serial.print("   RPM: ");
  Serial.println(rpm, 2);

  // ===== OLED =====
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("DC MOTOR CONTROL");

  display.setTextSize(2);
  display.setCursor(0, 20);
  display.print("RPM:");
  display.setCursor(0, 42);
  display.print(rpm, 1);

  display.display();
}