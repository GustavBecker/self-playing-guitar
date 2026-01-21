// Gustav Becker
// 21/01/2026
// gustavbecker8@gmail.com
// This is still a very basic idea code. I just couldn't continue since I wasn't able to test it without the hardware.

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// LIMIT SWITCHES
const uint8_t LIMIT_PINS[6] = {1, 2, 3, 4, 5, 6};

// TB6600 STEPPERS
const uint8_t STEP_PINS[6] = {39, 44, 46, 36, 34, 33};
const uint8_t DIR_PINS[6]  = {38, 45, 47, 37, 35, 32};

// DIR = HIGH → toward nut (homing direction)

// PCA9685 setup
Adafruit_PWMServoDriver chordServos(0x40); // A0/A1
Adafruit_PWMServoDriver pickServos(0x41);  // A2/A3

#define SERVO_FREQ 50

#define SERVO_MIN 120
#define SERVO_MAX 600

// Data structures
struct Chord {
  const char* name;
  int8_t fret[6];   // -1 = muted, 0–8 frets
};

struct SongEvent {
  uint32_t time_ms;
  uint8_t chordID;
  uint8_t strumMask; // bitmask for strings to play
};


// Chord list
const Chord chords[] = {
  { "G", {3, 2, 0, 0, 0, 3} },
  { "D", {-1,-1, 0, 2, 3, 2} },
  { "C", {-1, 3, 2, 0, 1, 0} },
};
const uint8_t NUM_CHORDS = sizeof(chords)/sizeof(chords[0]);

// Happy birthday chords
const SongEvent song[] = {
  {0,    0, 0b111111},
  {900,  0, 0b111111},
  {1800, 1, 0b111111},
  {2700, 0, 0b111111},

  {3600, 2, 0b111111},
  {4500, 0, 0b111111},
  {5400, 1, 0b111111},
  {6300, 0, 0b111111},

  {7200, 0, 0b111111},
  {8100, 2, 0b111111},
  {9000, 0, 0b111111},
  {9900, 1, 0b111111},

  {10800, 2, 0b111111},
  {11700, 0, 0b111111},
  {12600, 1, 0b111111},
  {13500, 0, 0b111111},
};
const uint16_t SONG_LEN = sizeof(song)/sizeof(song[0]);

// Stepper positioning map
const int STEPS_PER_FRET = 200; // adjust mechanically
int currentFret[6] = {0,0,0,0,0,0};

// Homing sequence
void homeSteppers() {
  for (int i = 0; i < 6; i++) {
    pinMode(LIMIT_PINS[i], INPUT_PULLUP);
    pinMode(STEP_PINS[i], OUTPUT);
    pinMode(DIR_PINS[i], OUTPUT);

    digitalWrite(DIR_PINS[i], HIGH); // toward nut

    while (digitalRead(LIMIT_PINS[i]) == LOW) {
      digitalWrite(STEP_PINS[i], HIGH);
      delayMicroseconds(800);
      digitalWrite(STEP_PINS[i], LOW);
      delayMicroseconds(800);
    }
    currentFret[i] = 0;
  }
}

// Move steppers to chord
void moveToChord(uint8_t chordID) {
  for (int s = 0; s < 6; s++) {
    int target = chords[chordID].fret[s];
    if (target < 0) continue;

    int delta = target - currentFret[s];
    digitalWrite(DIR_PINS[s], delta > 0 ? LOW : HIGH);

    for (int i = 0; i < abs(delta) * STEPS_PER_FRET; i++) {
      digitalWrite(STEP_PINS[s], HIGH);
      delayMicroseconds(600);
      digitalWrite(STEP_PINS[s], LOW);
      delayMicroseconds(600);
    }
    currentFret[s] = target;
  }
}


// Press chord servos
void pressChord(bool press) {
  uint16_t pulse = press ? SERVO_MIN + 80 : SERVO_MIN;

  for (int i = 0; i < 6; i++) {
    chordServos.setPWM(i, 0, pulse);
  }
}

// Strum with position tracking
bool pickLeft[6] = {true,true,true,true,true,true};

void strum(uint8_t mask) {
  for (int i = 0; i < 6; i++) {
    if (!(mask & (1 << i))) continue;

    uint16_t pos = pickLeft[i] ? SERVO_MIN : SERVO_MAX;
    pickServos.setPWM(i, 0, pos);
    pickLeft[i] = !pickLeft[i];
  }
}


// Main playback
uint32_t startTime;
uint16_t eventIndex = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  chordServos.begin();
  pickServos.begin();
  chordServos.setPWMFreq(SERVO_FREQ);
  pickServos.setPWMFreq(SERVO_FREQ);

  delay(500);
  homeSteppers();
  startTime = millis();
}

void loop() {
  if (eventIndex >= SONG_LEN) return;

  uint32_t now = millis() - startTime;
  if (now >= song[eventIndex].time_ms) {
    moveToChord(song[eventIndex].chordID);
    pressChord(true);
    delay(80);
    strum(song[eventIndex].strumMask);
    delay(120);
    pressChord(false);

    eventIndex++;
  }
}






