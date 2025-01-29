#include <Adafruit_NeoPixel.h> 
#include <SD.h>

#define D2 2
#define NUMPIXELS 16


// global variables for the FSRs
int FORCE_THRESHOLD = 200; 
const int NUM_FSR = 4;
const int FSR_PINS[4] = {A0, A1, A2, A3};
const int FSR_INDICES[4][3] = {
  {0, 1, 2},
  {4, 5, 6},
  {8, 9, 10},
  {12, 13, 14}
};


// global variables for the neopixel
Adafruit_NeoPixel strip(NUMPIXELS, D2, NEO_GRB + NEO_KHZ800);


// global variables for the microSD card reader
const int SD_CS_PIN = 10; 
File outFile;


// global variables for the buzzers
const int BUZZER_PINS[4] = {6, 7, 8, 9};
const int RED_TONE = 440;
const int GREEN_TONE = 660;


// global variables for incrementing internal time
unsigned long previousLogTime = 0;                  // Tracks time passed for logging
const unsigned long logInterval = 1000;             // Interval for logging (in milliseconds)


// Traffic Light States
enum TrafficStates { RED, GREEN };                  // CHECK THIS OUT LATER (!)
TrafficStates currentState[4];


// global time variables
const unsigned long waitThreshold = 10000; 
unsigned long stateChangeTime[4] = {0, 0, 0, 0}; // Tracks when state was last changed
unsigned long waitTimes[4] = {0, 0, 0, 0};



void setup() {

  Serial.begin(9600);

  // SD Card
  pinMode(SD_CS_PIN, OUTPUT);
  // Serial.println("Initializing SD Card...");

  // check if it begins right
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Something went wrong with the SD Card!");
    while (1); // Stops running if SD card fails to initialize
  }
  // Serial.println("SD card initialized successfully.");

  // check if the file is fine
  outFile = SD.open("results.txt", FILE_WRITE);
  if (!outFile) {
    Serial.println("Something went wrong with the SD Card File!");
    while (1); // Stops running if file doesn't open
  }
  // Serial.println("Log file opened successfully.");


  //NeoPixel Ring Section
  strip.begin();
  strip.show();
  strip.setBrightness(25);

  // FSR Pins
  for (int i = 0; i < NUM_FSR; i++) {
    pinMode(FSR_PINS[i], INPUT);
  }

  // Buzzer Pins
  for (int i = 0; i < NUM_FSR; i++) {
    pinMode(BUZZER_PINS[i], OUTPUT);
    noTone(BUZZER_PINS[i]);
  }

  // Traffic Light States
  for (int i = 0; i < NUM_FSR; i++) {
    currentState[i] = RED; // Starts with RED light
    setGroupPixelColor(i, strip.Color(255, 0, 0)); // Sets Neopixels to Red
  }
  strip.show();

}

void loop() {

  // Var to track if any FSR exceeds the threshold value
  // bool anyForceDetected = false; (x)

  bool forceDetected[NUM_FSR] = {false, false, false, false};                            // !!!!!!

  // Array to store FSR readings, an array of 4
  int fsrReadings[NUM_FSR];

  // Serial.println("FSR Readings:");

  // getting the FSR value for each FSR
  for (int i = 0; i < NUM_FSR; i++) {

    // update the applicable pressure reading
    fsrReadings[i] = analogRead(FSR_PINS[i]);

    // Serial.print("FSR ");
    // Serial.print(i + 1);
    // Serial.print(": ");
    // Serial.println(fsrReadings[i]); 

    // if there is pressure detected in ONE fsr
    if (fsrReadings[i] > FORCE_THRESHOLD) {

      Serial.println("going green");
      Serial.print("index of FSR detecting pressure :");
      Serial.println(i);

      // indicate pressure detected
      forceDetected[i] = true;

      // change the light to green
      handleTrafficState(i, GREEN, forceDetected[i]);

    // if there is no pressure detected
    } else {

      // change the light to red
      handleTrafficState(i, RED, forceDetected[i]);
    }
  }

  // Serial.println("------------------------");

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
      // Serial.println("FSR readings logged to SD card.");

  }  
  delay(200);
}

void setGroupPixelColor(int groupIndex, int color) { // 0 for red, 1 for green for int color

    // for every index in groupIndex (0, 1, 2; 4, 5, 6; ...)
    for(int i = 0; i < 3; i++) {

      Serial.print("indices we're changing for the light :");
      Serial.println(FSR_INDICES[groupIndex][i]);

        // grabbing the index of the physical light to change
        int pixelIndex = FSR_INDICES[groupIndex][i]; 

        // change the light color

        // if the light is green
        if (color == 0) {

          // change it to red
          strip.setPixelColor(pixelIndex, strip.Color(255, 0, 0));

          // update the lights
          strip.show();

        }

        // if the light is red
        else if (color == 1) {
          
          // change it to green
          strip.setPixelColor(pixelIndex, strip.Color(0, 255, 0));

          // update the lights
          strip.show();
        }

        else {

          // change it to blue, something is up
          strip.setPixelColor(pixelIndex, strip.Color(0, 0, 255));

          // update the lights
          strip.show();

        }
        
    }
}

void handleTrafficState(int groupIndex, TrafficStates newState, bool force) {


  // if the lights are currently red
  if (currentState[groupIndex] == RED) {

    // if they've waited more time than the threshold AND there's force on the FSR (someone is there)
    if ((millis() - stateChangeTime[groupIndex] >= waitThreshold) && (force)) {

      // update currentState array to update the light representation internally
      currentState[groupIndex] = GREEN;

      // actually should be setting the color to green
      setGroupPixelColor(groupIndex, 1);

      // update the internal timer for returning to the sdCard      (!)
      waitTimes[groupIndex] = millis() - stateChangeTime[groupIndex];

      // update the internal timer indicating the last time the color changed
      stateChangeTime[groupIndex] = millis();


    }
  }

  // if the lights are currently green
  if (currentState[groupIndex] == GREEN) {

    // CHANGE THIS !!! / does this make sense ???
    if (millis() - stateChangeTime[groupIndex] >= waitThreshold) {

      // update currentState to change the representation to red internally
      currentState[groupIndex] = RED;

      // actually should be setting the color to red
      setGroupPixelColor(groupIndex, 0);

      // update the internal timer indicating the last time the color changed
      stateChangeTime[groupIndex] = millis();
    }
  }
}
