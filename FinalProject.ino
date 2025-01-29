#include <Adafruit_NeoPixel.h> 
#include <SD.h>

// global variables for the FSRs -----------------------------------------------
int forceThreshold = 200;
const int FSR_PINS[4] = {A0, A1, A2, A3};      // 0 and 2, 1 and 3 connected
const int FSR_INDICES[2][6] = {
  {0, 1, 2, 8, 9, 10},
  {4, 5, 6, 12, 13, 14}
};


// global variables for the neopixel -------------------------------------------
Adafruit_NeoPixel strip(16, 2, NEO_GRB + NEO_KHZ800);
enum TrafficStates { RED, GREEN };
int currentState[2];
enum Roads { nSouth, eWest };
bool pressureOn[4] = {false, false, false, false};


// global variables for the microSD --------------------------------------------
File outFile;


// global variables for the buzzers --------------------------------------------
const int BUZZER_PINS[4] = {6, 7, 8, 9};
const int RED_TONE = 440;
const int GREEN_TONE = 660;


// global variables for time
const unsigned long maxWait = 10000;
unsigned long lastForce[4] = {0, 0, 0, 0};


void setup() { // --------------------------------------------------------------

  Serial.begin(9600);

  // init for the microSD
  pinMode(10, OUTPUT);                // pin for sd card

  if (!SD.begin(10)) {
    Serial.println("Something went wrong with the SD Card!");
    while (1);                        // stops the program from continuing
  }

  outFile = SD.open("results.txt", FILE_WRITE);

  if (!outFile) {
    Serial.println("Something went wrong with the SD Card File!");
    while (1); 
  }

  // init for the neopixel
  strip.begin();
  strip.show();
  strip.setBrightness(25);

  // init for the FSRs
  for (int i = 0; i < 4; i++) {
    pinMode(FSR_PINS[i], INPUT);
  }

  // init for buzzers
  for (int i = 0; i < 4; i++) {
    pinMode(BUZZER_PINS[i], OUTPUT);
    noTone(BUZZER_PINS[i]);
  }

  // init the traffic light states to be green and red
  setLight(nSouth, GREEN);
  setLight(eWest, RED);
  currentState[0] = GREEN;
  currentState[1] = RED;

}

void loop() { // ---------------------------------------------------------------

  // variables to store FSR input
  int forceMeasurement[4];
  /* because there's no tangible difference between if one or two FSRs on the
     same road is activated, condense into just two again */
  // bool forceDetected[2] = {false, false};

  // getting the reading from each fsr
  for (int i = 0; i < 4; i++) {

    // update measurements
    forceMeasurement[i] = analogRead(FSR_PINS[i]);

    // if force is detected...
    if (forceMeasurement[i] > forceThreshold) {

      if (pressureOn[i] == false) {

        // record the last time force was exerted
        lastForce[i] = millis();

      }

      pressureOn[i] = true;

      // update boolean to true
      // forceDetected[i % 2] = true;

      determineBehavior(i);


    } else {

      pressureOn[i] = false;

    }
  }
}

void playBuzzerTone(int roadRep, int colorRep) {
    int buzzerPin = BUZZER_PINS[roadRep]; // Select the buzzer for this road's traffic light

    if (colorRep == RED) {
        tone(buzzerPin, RED_TONE, 200);
    } 
    else if (colorRep == GREEN) {
        tone(buzzerPin, GREEN_TONE, 200);
    }

    delay(200);
    noTone(buzzerPin);
}

void setLight(int roadRep, int colorRep) {

  // for loop to get all of the indices for the matching road
  for (int i = 0; i < 6; i++) {

    int neoIndex = FSR_INDICES[roadRep][i];

    if (colorRep == 0) {                               // change it to red

      strip.setPixelColor(neoIndex, strip.Color(255, 0, 0));
      currentState[roadRep] = RED;

    } else if (colorRep == 1) {                       // change it to green

      strip.setPixelColor(neoIndex, strip.Color(0, 255, 0));
      currentState[roadRep] = GREEN;


    } else {                                     // something is wrong, blue

      strip.setPixelColor(neoIndex, strip.Color(0, 255, 0));
      currentState[roadRep] = 3;
    }
  }
  
  playBuzzerTone(roadRep, colorRep);
  
}


int getOpp(int index) {

  if (index == 0 || index == 2) {
    return 1;

  } else {
    return 0;

  }

}

void determineBehavior(int fsrRep) {      // fsrRep = fsr that got the force

  // so we know that one fsr got the force, now is it a green or red light

  // if the current state is green and has force just dont do anything
  if (currentState[fsrRep % 2] == 1) {

    Serial.println("green stays green");

    return;

  } else if (currentState[fsrRep % 2] == 0) {     // red and force, DO SOMETHING

    // get the current time
    unsigned long curTime = millis();

    // get the opposing FSR indices
    int opposing = getOpp(fsrRep);

    // if pressure has been on both within the past 5 seconds
    if (curTime - lastForce[opposing] < 5000 || curTime - lastForce[opposing + 2] < 5000) {

      Serial.println("im not freaking out!");

      // the light shouldn't change yet, there's still motion
      return;


      // if there's been no pressure on opposing within the past 5 seconds
    } else if (curTime - lastForce[opposing] > 5000 || curTime - lastForce[opposing + 2] > 5000) {

      Serial.println("clear roads for 5 seconds, changing lights");

      // change the lights, the roads have been clear
      setLight((fsrRep % 2), GREEN);
      setLight((1 - (fsrRep % 2)), RED);

      // make it so the pressure flag resets (a car moves away)
      pressureOn[fsrRep] = false;

    }

    // have they been waiting for too long
    if (curTime - lastForce[fsrRep] > maxWait) {

      Serial.println("waited too long, changing lights");

      // change the lights regardless
      setLight((fsrRep % 2), GREEN);
      setLight((1 - (fsrRep % 2)), RED);

      pressureOn[fsrRep] = false;

      Serial.println("red, waiting TOO LONG");


    }
  }
}
