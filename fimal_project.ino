#include <Arduino.h>
#include <Servo.h>

// Вбудований LED на Arduino Uno (пін 13)
const int LED_PIN = LED_BUILTIN;

// Серво шлагбаума (використовуємо пін 9 з підтримкою PWM)
const int SERVO_PIN = 9;
const int CLOSED_ANGLE = 0;
const int OPEN_ANGLE = 90;

const unsigned long OPEN_TIME = 5000;
const unsigned long COOLDOWN_TIME = 3000;

Servo barrier;

bool barrierOpen = false;
bool hasActed = false;

unsigned long openedAt = 0;
unsigned long lastActionAt = 0;

String inputLine = "";

// Дозволені номери (виправлений синтаксис масиву)
const char* allowedPlates[] = {
  "AA1234BC",
  "KA1234CB"
};

const size_t PLATE_COUNT = sizeof(allowedPlates) / sizeof(allowedPlates[0]);

// Вмикання/вимикання вбудованого LED
void setLed(bool on) {
  digitalWrite(LED_PIN, on ? HIGH : LOW);
}

bool isAllowed(String plate) {
  plate.trim();
  plate.toUpperCase();

  for (size_t i = 0; i < PLATE_COUNT; i++) {
    if (plate == allowedPlates[i]) {
      return true;
    }
  }

  return false;
}

void openBarrier() {
  unsigned long now = millis();

  if (barrierOpen) {
    Serial.println("BUSY");
    return;
  }

  if (hasActed && now - lastActionAt < COOLDOWN_TIME) {
    Serial.println("COOLDOWN");
    return;
  }

  barrier.write(OPEN_ANGLE);
  barrierOpen = true;
  openedAt = now;
  lastActionAt = now;
  hasActed = true;

  setLed(true);
  Serial.println("ACCESS_GRANTED");
}

void processCommand(String command) {
  command.trim();

  if (!command.startsWith("PLATE:")) {
    Serial.println("UNKNOWN_COMMAND");
    return;
  }

  String plate = command.substring(6);
  plate.trim();
  plate.toUpperCase();

  Serial.print("PLATE_RECEIVED:");
  Serial.println(plate);

  if (isAllowed(plate)) {
    openBarrier();
  } else {
    setLed(false);
    Serial.println("ACCESS_DENIED");  
  }
}

void setup() {
  // Швидкість обов'язково 9600
  Serial.begin(9600);

  pinMode(LED_PIN, OUTPUT);
  setLed(false);

  barrier.attach(SERVO_PIN);
  barrier.write(CLOSED_ANGLE);

  Serial.println("ALPR_CONTROLLER_READY");
}

void loop() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();

    if (c == '\n') {
      processCommand(inputLine);
      inputLine = "";
    } else if (c != '\r') {
      if (inputLine.length() < 64) {
        inputLine += c;
      } else {
        inputLine = "";
        setLed(false);
        Serial.println("ERROR_LINE_TOO_LONG");
      }
    }
  }

  if (barrierOpen && millis() - openedAt >= OPEN_TIME) {
    barrier.write(CLOSED_ANGLE);
    barrierOpen = false;

    setLed(false);
    Serial.println("GATE_CLOSED");
  }
}