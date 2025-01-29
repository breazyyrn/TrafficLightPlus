//Including needed Libraries
#include <Adafruit_NeoPixel.h> //NeoPixel Ring
#include <SD.h> //SD card

#define D2 2
#define NUMPIXELS 16

//FSR Global Vars
const int FSR_PINS[4] = {A0, A1, A2, A3};
//A0 -> indices 0-2
//A1 -> indices 4-6
//A2 -> indices 8-10
// A3 -> indices 12-14

const int NUM_FSR = 4;
int FORCE_THRESHOLD = 200; // Detects whether force is significant

// Var for Arduino's built-in LED Pin
const int LED_PIN = 13;

//NeoPixel Ring Global Vars
int index = 0;
Adafruit_NeoPixel strip(NUMPIXELS, D2, NEO_GRB + NEO_KHZ800);

// SD Card Vars
const int SD_CS_PIN = 10; // CS Pin for SD Card
File outFile; // File Object for logging data

// Buzzer Definitions
const int BUZZER_PINS[4] = {6, 7, 8, 9};

// Sound frequencies for each color
const int RED_TONE = 440;
const int GREEN_TONE = 660;

// Timing definitions 
unsigned long previousLogTime = 0; // Tracks time passed for logging
const unsigned long logInterval = 1000; // Interval for logging (in milliseconds)

// Array for FSR to Neopixel mapping (each FSR controls 3 Neopixels)
const int FSR_INDICES[4][3] = {
  {0, 1, 2},
  {4, 5, 6},
  {8, 9, 10},
  {12, 13, 14}
};

// Traffic Light States
enum TrafficStates { RED, GREEN};
TrafficStates currentState[4];

// Timer Definitions for each FSR group
unsigned long stateChangeTime[4] = {0, 0, 0, 0}; // Tracks when state was last changed
const unsigned long waitThreshold = 30000; // 20 seconds threshold

void setup() {

  Serial.begin(9600);

  // SD Card
  pinMode(SD_CS_PIN, OUTPUT);
  Serial.println("Initializing SD Card...");

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Something went wrong with the SD Card!");
    while (1); // Stops running if SD card fails to initialize
  }
  Serial.println("SD card initialized successfully.");

  outFile = SD.open("results.txt", FILE_WRITE);
  if (!outFile) {
    Serial.println("Something went wrong with the SD Card File!");
    while (1); // Stops running if file doesn't open
  }
  Serial.println("Log file opened successfully.");


  //NeoPixel Ring Section
  strip.begin();
  strip.show();
  strip.setBrightness(25);

  // FSR Pins
  for (int i = 0; i < NUM_FSR; i++) {
    pinMode(FSR_PINS[i], INPUT);
  }

  // Built-in LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Buzzer Pins
  for (int i = 0; i < NUM_FSR; i++) {
    pinMode(BUZZER_PINS[i], OUTPUT);
    noTone(BUZZER_PINS[i]);
  }

  // Traffic Light States
  for (int i = 0; i < NUM_FSR; i++) {
    currentState[i] = RED; // Starts with RED light
    setPixelColor(i, strip.Color(255, 0, 0)); // Sets Neopixels to Red
  }
  strip.show();

}

void loop() {
  // Var to track if any FSR exceeds the threshold value
  bool anyForceDetected = false;
  // Array to store FSR readings
  int fsrReadings[NUM_FSR];

  Serial.println("FSR Readings:");
  for (int i = 0; i < NUM_FSR; i++) {
      fsrReadings[i] = analogRead(FSR_PINS[i]);
      Serial.print("FSR ");
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.println(fsrReadings[i]); 

      // Checks if current FSR value exceeds the force thresold value
      if (fsrReadings[i] > FORCE_THRESHOLD) {
          anyForceDetected = true;
          handleTrafficState(i, GREEN);
      } else {
          handleTrafficState(i, RED);
      }
  }

  // Built-in LED is controlled based on whether it detects any force
  if (anyForceDetected) {
      // Turns on
      digitalWrite(LED_PIN, HIGH); 
      Serial.println("Force was detected on at least one FSR!");
  } else {
      // Turns off
      digitalWrite(LED_PIN, LOW);
  }

  Serial.println("------------------------");

  // Gets current time for logging
  unsigned long currentMillis = millis();

  // Checks to see if it's time to log data
  if (currentMillis - previousLogTime >= logInterval) {
      // Updates last log time
      previousLogTime = currentMillis;

      // Creates a timestamp
      unsigned long timestamp = currentMillis;

      // Sends FSR readings to SD card
      outFile.print("Timestamp: ");
      outFile.print(timestamp);
      outFile.print(" ms, FSR Readings: ");
      for (int i = 0; i < NUM_FSR; i++) {
          outFile.print(fsrReadings[i]);
          if (i < NUM_FSR - 1) {
              outFile.print(", ");
          }
      }
      outFile.println();

      // data is written to SD card
      outFile.flush(); // ensures
      Serial.println("FSR readings logged to SD card.");

  }  
  delay(200);
}

void setGroupPixelColor(int groupIndex, uint32_t color) { //I think the issue is that neopixel already has a setPixelColor function. So imma change the name here
    for(int i = 0; i < 3; i++) {
        int pixelIndex = FSR_INDICES[groupIndex][i]; //Grabs the index value of the current FSR's ith index
        if (pixelIndex >= 0 && pixelIndex < NUMPIXELS) {
            strip.setPixelColor(pixelIndex, color);
        }
    }
}

void handleTrafficState(int groupIndex, TrafficStates newState) {
    if (currentState[groupIndex] == RED) {
      if (fsrReading > FORCE_THRESHOLD && (millis() - stateChangeTime[groupIndex] >= waitThreshold)) {
        currentState[groupIndex] = GREEN;
        setGroupPixelColor(groupIndex, strip.Color(0, 255, 0));
        stateChangeTime[groupIndex] = millis();
      }
  }

      if (currentState[groupIndex] == GREEN) {
        if (millis() - stateChangeTime[groupIndex] >= waitThreshold) {
          currentState[groupIndex] = RED;
          setGroupPixelColor(groupIndex, strip.Color(255, 0, 0));
          stateChangeTime[groupIndex] = millis();
        }
      }
}
