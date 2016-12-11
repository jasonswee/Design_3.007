
/*  Pulse Sensor Amped 1.4    by Joel Murphy and Yury Gitman   http://www.pulsesensor.com

----------------------  Notes ----------------------  ---------------------- 
This code:
1) Blinks an LED to User's Live Heartbeat   PIN 13
2) Fades an LED to User's Live HeartBeat
3) Determines BPM
4) Prints All of the Above to Serial

Read Me:
https://github.com/WorldFamousElectronics/PulseSensor_Amped_Arduino/blob/master/README.md   
 ----------------------       ----------------------  ----------------------
*/
#include <Wire.h>
#include <LSM6.h>
#include "pitches.h"

LSM6 imu;


//  Pulse Variables
int pulsePin = A7;                 // Pulse Sensor purple wire connected to analog pin 7
//int blinkPin = 13;                // pin to blink led at each beat
//int fadePin = 5;                  // pin to do fancy classy fading blink at each beat
//int fadeRate = 0;                 // used to fade LED on with PWM on fadePin

// Volatile Variables, used in the interrupt service routine!
volatile int BPM;                   // int that holds raw Analog in 0. updated every 2mS
volatile int Signal;                // holds the incoming raw data
volatile int IBI = 600;             // int that holds the time interval between beats! Must be seeded! 
volatile boolean Pulse = false;     // "True" when User's live heartbeat is detected. "False" when not a "live beat". 
volatile boolean QS = false;        // becomes true when Arduoino finds a beat.

// Regards Serial OutPut  -- Set This Up to your needs
static boolean serialVisual = true;   // Set to 'false' by Default.  Re-set to 'true' to see Arduino Serial Monitor ASCII Visual Pulse 

//Debugging variables
int count = 0;

//Detection of BPM Progress 1
boolean checkAwake = false;
boolean checkOpBPMRange = false;
boolean checkPreviousOpBPMRange = false;
//int lowBPMRate = 200;
const int opLowBPM = 40;
const int opHighBPM = 100;
const int sleepBPM = 50;
int recordedBPM = 0;

//Led for LED stimulus Progress 3
const int ledRPin = A3;
const int ledBPin = A2;
const int ledGPin = A1;
int ledIntensity = 150; //150-254

//Vibrator motor for vibration stimulus Progress 4
const int motorPin = A0;
int motorIntensity = 254; //150-254

//Jerking motion for feedback Progress 6
char report[80];
volatile int reading[3]={0, 0, 0};
int total=0;
int pastTotal = 0;
//Accelerometer
const int rawLow = -32768;
const int rawHigh = 32767;
const int remapLow = 0;
const int remapHigh = 10;
int accelCount = 0;
const int accDiffThreshold = 0;

//Flex Sensor
int flexVoltageThreshold = 5000;
const int flexPin = A6;    
const int rawAnalogLow = 0;
const int rawAnalogHigh = 1023;
const int remapFlexLow = 0;
const int remapFlexHigh = 10000;

// notes in the melody:
const int buzzerPin = 13;
int melody[] = {
  NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4
};
int melody_jjb[] = {
  NOTE_E4, NOTE_E4, NOTE_E4, NOTE_E4, NOTE_E4, NOTE_E4, NOTE_E4, NOTE_G4,NOTE_C4, NOTE_D4, NOTE_E4  , NOTE_F4, NOTE_F4, NOTE_F4, NOTE_F4, NOTE_F4,NOTE_E4, NOTE_E4, NOTE_E4, NOTE_E4, NOTE_D4, NOTE_D4,NOTE_E4, NOTE_D4,NOTE_G4      //NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4
};
// note durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {
  4, 8, 8, 4, 4, 4, 4, 4
};
/*int noteDurations_jjb[] = {
2, 2, 4, 2, 2, 4, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 4
};*/
int noteDurations_jjb[] = {
8, 8, 4, 8, 8, 4, 8, 8, 8, 8, 4, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 4, 4
};

int tempo = 200;
char notes[] = "eeeeeeegcde fffffeeeeddedg";
int duration[] = {1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2};
void setup(){
  Serial.begin(115200);             // we agree to talk fast!
  interruptSetup();                 // sets up to read Pulse Sensor signal every 2mS 
  
  pinMode(ledRPin, OUTPUT);
  pinMode(ledGPin, OUTPUT);
  pinMode(ledBPin, OUTPUT);
  pinMode(motorPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(flexPin, INPUT);
  Wire.begin();
  if (!imu.init()){
    Serial.println("Failed to detect and initialize IMU!");
    offAllStimulus();
    checkLightStimulus(true, 'B');
    while (1);
  }
  imu.enableDefault();

  int delayms = 1000;
  
  for(int i = 2; i<31;i=i){ //Start up light
    analogWrite(ledRPin, ledIntensity);
    delay(delayms/i);
    analogWrite(ledRPin, 0);
    delay(delayms/(i++));
    analogWrite(ledGPin, ledIntensity);
    delay(delayms/(i++));
    analogWrite(ledGPin, 0);
    delay(delayms/(i++));
    analogWrite(ledBPin, ledIntensity);
    delay(delayms/(i++));
    analogWrite(ledBPin, 0);
    delay(delayms/(i++));
  }
  playMelody(true,1);
}

void loop(){
  //tone(13, 200);

checkLightStimulus(true, 'B');
      checkLightStimulus(true, 'G');
      delay(3000);
  
  for(int i = 10; checkAwake == false; i++){//exit when require t
    recordedBPM = checkHeartBPMWithinOperatingRange(45);
    delay(1000);
    if(i==15){
      checkLightStimulus(true, 'B');
      checkLightStimulus(true, 'G');
      //delay(200);
      //offAllStimulus();
      i=10;
    }
    
    if(checkFlexStatus() == false ){
      i=0;
      offAllStimulus();
    }
    
    if(recordedBPM != -1){
      checkAwake = checkBPMBelowThreshold(recordedBPM);
    }
    
  }
  offAllStimulus();
  checkUserAwake();
  delay(5000); //take 5 seconds break before rechecking

  //Test cases
  //delay(1000);
  //testCases(5);//1 test light. 2 test Vibration. 3 test Heart Rate Sensor. 4 test Accelerometer. 5 Flex Sensor. 6 Test Melody
}
boolean checkFlexStatus(){
  if(map(analogRead(flexPin), rawAnalogLow, rawAnalogHigh, remapFlexLow, remapFlexHigh)<flexVoltageThreshold){
    Serial.println("--Flex sensor flexed--");
    Serial.println(map(analogRead(flexPin), rawAnalogLow, rawAnalogHigh, remapFlexLow, remapFlexHigh));
    return false;
  }
  Serial.println("Flex sensor is un-flexed! Check awake now!");
  Serial.println(map(analogRead(flexPin), rawAnalogLow, rawAnalogHigh, remapFlexLow, remapFlexHigh));
  return true;
}
int checkHeartBPMWithinOperatingRange(int registeredBPM){
  if(registeredBPM<opHighBPM&&registeredBPM>opLowBPM ){
    Serial.print("Heart beat detected! BPM: ");  
    Serial.println(registeredBPM);
    return registeredBPM;
  }
  return -1; //Not within range thus negative value
}
boolean checkBPMBelowThreshold(int registeredBPM){
  if(registeredBPM < sleepBPM && checkFlexStatus() == true){
    Serial.print("You are sleepy! ");
    Serial.print("Sleepy heartrate: ");
    Serial.println(registeredBPM);
    Serial.println("Jerk off to stop Stimulus!!");
    checkAwake=true;    
    return true;
  }
  //Serial.print("Healthy BPM!!");
  //Serial.println(registeredBPM);
  return false;
}


void checkUserAwake(){
  if(checkAwake==true){
    accelCount = 0;
    for(int i = 0; i < 1500;i++){//pre-warning 
      checkLightStimulus(true, 'R');
      checkLightStimulus(true, 'G');
      checkAwake = accelJerkFeedback();
      Serial.print("Check pre-warning count: ");
      Serial.println(i);
      if(checkAwake == false){
        offAllStimulus();
        awokenMsg();
        return;
      }
    }
    offAllStimulus();
    while(checkAwake==true){
      checkVibrationStimulus(true);
      playMelody(true,2);
      checkLightStimulus(true, 'R');
      delay(400);
      checkVibrationStimulus(false);
      playMelody(false,1);
      checkLightStimulus(false, 'R');
      delay(300);
      checkAwake = accelJerkFeedback();
    }
    offAllStimulus();
    awokenMsg();
  }  
}
void awokenMsg(){
  Serial.println("Sally Beauty has awoken!!");
}
void checkLightStimulus(boolean onOff, char color){
  if(onOff == true){
    if(color == 'R'){
      analogWrite(ledRPin, ledIntensity);
    }
    else if(color == 'G'){
      analogWrite(ledGPin, ledIntensity);
    }
    else{
      analogWrite(ledBPin, ledIntensity);
    }
    Serial.println("LED switched on!");
  }
  else{
    analogWrite(ledRPin, 0);
    analogWrite(ledGPin, 0);
    analogWrite(ledBPin, 0);
  }
}
void testCases(int test){
  if(test == 1){
    checkLightStimulus(true, 'R');
    delay(200);
    analogWrite(ledRPin, 0);
    checkLightStimulus(true, 'G');
    delay(200);
    analogWrite(ledGPin, 0);
    checkLightStimulus(true, 'B');
    delay(200);
    analogWrite(ledBPin, 0);
  }
  else if(test == 2){
    checkVibrationStimulus(true);
    delay(200);
    checkVibrationStimulus(false);
    delay(200);
  }
  else if(test == 3){
    //Serial.print("BPM detected: ");
    Serial.println(BPM);
  }
  else if(test == 4){
    imu.read();
    /*snprintf(report, sizeof(report), "A: %6d %6d %6d    G: %6d %6d %6d",
    imu.a.x, imu.a.y, imu.a.z,
    imu.g.x, imu.g.y, imu.g.z);
    snprintf(report, sizeof(report), "Accelerometer: x: %6d y: %6d z: %6d",
    imu.a.x, imu.a.y, imu.a.z);*/
    
    total = (map(imu.a.x,rawLow, rawHigh,remapLow,remapHigh) + map(imu.a.y,rawLow, rawHigh,remapLow,remapHigh) + map(imu.a.z,rawLow, rawHigh,remapLow,remapHigh) )/3;
    Serial.print("Remapped Acceleration reads: ");
    analogWrite(motorPin, motorIntensity);
    Serial.print(total);
    Serial.print(" Remapped difference in Acceleration reads: ");
    Serial.println(abs(pastTotal-total));
    pastTotal=total;
    analogWrite(motorPin, motorIntensity);
    //Serial.println(report);
    delay(400);
    analogWrite(motorPin, 0);
    delay(300);
  }
  else if(test == 5){
    Serial.print("Flex reading: ");
    Serial.println(map(analogRead(flexPin), rawAnalogLow, rawAnalogHigh, remapFlexLow, remapFlexHigh));
    checkFlexStatus();
  }
  else if(test == 6){
    playMelody(true,1);
  }
}
void checkVibrationStimulus(boolean onOff){
  if(onOff == true){
    analogWrite(motorPin, motorIntensity);
    Serial.println("Motor is On!");
  }
  else{
    analogWrite(motorPin, 0);
    Serial.println("Motor is OFF!");
  }
}
boolean accelJerkFeedback(){
  imu.read();
  
  total = (map(imu.a.x,rawLow, rawHigh,remapLow,remapHigh) + map(imu.a.y,rawLow, rawHigh,remapLow,remapHigh) + map(imu.a.z,rawLow, rawHigh,remapLow,remapHigh) )/3;
  if(accelCount == 0){
    pastTotal = total;
    accelCount++;
  }
  else if(abs(pastTotal-total)>accDiffThreshold){
    Serial.println("Acceleration: ");
    Serial.println(abs(pastTotal-total));
    Serial.println("You jerked!");
    pastTotal = total;
    checkVibrationStimulus(false);
    return false;
  }
  //Serial.print(total);
  pastTotal = total;
  return true;
}
void offAllStimulus(){
  checkLightStimulus(false, 'R');
  checkLightStimulus(false, 'G');
  checkLightStimulus(false, 'B');
  checkVibrationStimulus(false);
  playMelody(false,1);
}
void playMelody(boolean onOff,int choice){
  if(onOff == true && choice == 1){
    for (int thisNote = 0; thisNote < 8; thisNote++) {
  
      // to calculate the note duration, take one second
      // divided by the note type.
      //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
      int noteDuration = 1000 / noteDurations[thisNote];
      tone(buzzerPin, melody[thisNote], noteDuration);
  
      // to distinguish the notes, set a minimum time between them.
      // the note's duration + 30% seems to work well:
      int pauseBetweenNotes = noteDuration * 1.30;
      delay(pauseBetweenNotes);
      // stop the tone playing:
      noTone(buzzerPin);
      Serial.println("Melody is played!");
    }
  }
  else if(onOff == true && choice == 2){
    tone(buzzerPin, NOTE_DS8);
    Serial.println("Buzzer is On!");
  }
  else if(onOff == false){
    noTone(buzzerPin);
    Serial.println("Buzzer is Off!");
  }
  else if(onOff == true && choice == 3){
    /*for (int i = 0; i < sizeof(notes)-1; i++) {
    if (notes[i] == ' ') {
      // If find a space it rests
      delay(duration[i] * tempo);
    } else {
      playTheShit(notes[i], duration[i] * tempo);
    }

    // Pauses between notes
    delay((tempo*2)*duration[i]);
  }*/
  for (int thisNote = 0; thisNote < 26; thisNote++) {
  
      // to calculate the note duration, take one second
      // divided by the note type.
      //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
      int noteDuration_jjb = 1000 / noteDurations_jjb[thisNote]*2;
      tone(buzzerPin, melody_jjb[thisNote], noteDuration_jjb);
  
      // to distinguish the notes, set a minimum time between them.
      // the note's duration + 30% seems to work well:
      int pauseBetweenNotes = noteDuration_jjb * 1.3;
      delay(pauseBetweenNotes);
      // stop the tone playing:
      noTone(buzzerPin);
      Serial.println("Melody is played!");
    }
    }//end else if
    
}
void playTheShit(char note, int duration) {
  char notesName[] = { 'c', 'd', 'e', 'f', 'g' };
  int tones[] = { 261, 293, 329, 349, 392 };

  for (int i = 0; i < sizeof(tones); i++) {
    // Bind the note took from the char array to the array notesName
    if (note == notesName[i]) {
      // Bind the notesName to tones
      tone(buzzerPin, tones[i], duration);
    }
  }
}

