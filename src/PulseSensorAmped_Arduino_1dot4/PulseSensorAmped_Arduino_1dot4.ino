
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

LSM6 imu;


//  Pulse Variables
int pulsePin = 7;                 // Pulse Sensor purple wire connected to analog pin 0
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
const int opLowBPM = 30;
const int opHighBPM = 180;
const int sleepBPM = 70;
int recordedBPM = 0;


//Button for feedback of Button Progress 2
//const int buttonPin = 12;     // the number of the pushbutton pin
//int buttonState = 0;         // variable for reading the pushbutton status
boolean checkLEDStatus = false;

//Led for LED stimulus Progress 3
const int ledRPin = A3;
const int ledBPin = A2;
const int ledGPin = A1;
int ledIntensity = 150; //1-254

//Vibrator motor for vibration stimulus Progress 4
const int motorPin = A0;
int motorIntensity = 140; //0-255

//Jerking motion for feedback Progress 6
char report[80];
volatile int reading[3]={0, 0, 0};
volatile int total=0;
volatile int pastTotal = 0;
volatile int totalDet=0;
volatile int pastTotalDet = 0;
volatile int readX = 0;
volatile int pastX = 0;

volatile int rawLow = -32768;
volatile int rawHigh = 32767;
volatile int remapLow = 0;
volatile int remapHigh = 20;
volatile int accelCount = 0;

//Flex Sensor
int flexVoltageThreshold = 6900;
const int flexPin = A6;    
const int rawAnalogLow = 0;
const int rawAnalogHigh = 1023;
const int remapFlexLow = 0;
const int remapFlexHigh = 10000;

void setup(){
  Serial.begin(115200);             // we agree to talk fast!
  interruptSetup();                 // sets up to read Pulse Sensor signal every 2mS 
  
  pinMode(ledRPin, OUTPUT);
  pinMode(ledGPin, OUTPUT);
  pinMode(ledBPin, OUTPUT);
  pinMode(motorPin, OUTPUT);
  pinMode(flexPin, INPUT);
  Wire.begin();
  if (!imu.init()){
    Serial.println("Failed to detect and initialize IMU!");
    while (1);
  }
  imu.enableDefault();
  for(int i = 2; i<31;i=i){ //Start up light
    analogWrite(ledRPin, ledIntensity);
    delay(1000/i);
    analogWrite(ledRPin, 0);
    delay(1000/(i++));
    analogWrite(ledGPin, ledIntensity);
    delay(1000/(i++));
    analogWrite(ledGPin, 0);
    delay(1000/(i++));
    analogWrite(ledBPin, ledIntensity);
    delay(1000/(i++));
    analogWrite(ledBPin, 0);
    delay(1000/(i++));
  }
  
}

void loop(){
  while(checkAwake == false){//exit when require t
    recordedBPM = checkHeartBPMWithinOperatingRange(BPM);
    checkAwake = checkBPMBelowThreshold(recordedBPM);
  }
  checkUserAwake();
  delay(1000); //take 5 seconds break before rechecking

  //Test cases
  //testCases(1);//1 test light. 2 test Vibration. 3 test Heart Rate Sensor. 4 test Accelerometer. 5 Flex Sensor
}
boolean checkFlexStatus(){
  if(map(analogRead(flexPin), rawAnalogLow, rawAnalogHigh, remapFlexLow, remapFlexHigh)<flexVoltageThreshold){
    Serial.println("--Flex sensor flexed--");
    return false;
  }
  Serial.println("Flex sensor is un-flexed! Check awake now!");
  return true;
}
int checkHeartBPMWithinOperatingRange(int registeredBPM){
  if(registeredBPM<opHighBPM&&registeredBPM>opLowBPM ){
    Serial.print("Heart beat detected! BPM: ");  
    Serial.println(registeredBPM);
  }
}
boolean checkBPMBelowThreshold(int registeredBPM){
  if(registeredBPM < sleepBPM && checkFlexStatus() == true){
    Serial.print("You are sleepy! ");
    Serial.print("Sleepy heartrate: ");
    Serial.println(registeredBPM);
    Serial.println("Jerk off to stop Stimulus!!");
    checkAwake=true;    
    checkVibrationStimulus(true);
    checkLightStimulus(true, 'R');
    return true;
  }
  
  return false;
}

void checkUserAwake(){
  if(checkAwake==true){
    accelCount = 0;
    while(checkAwake==true){
        checkAwake = accelJerkFeedback();
    }
    checkVibrationStimulus(false);
    checkLightStimulus(false,'R');
    Serial.println("Sally Beauty has awoken!");
    //delay(5000);//prevent loop immediately exit
  }  
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
    Serial.print("BPM detected: ");
    Serial.println(BPM);
  }
  else if(test == 4){
    imu.read();
    /*snprintf(report, sizeof(report), "A: %6d %6d %6d    G: %6d %6d %6d",
    imu.a.x, imu.a.y, imu.a.z,
    imu.g.x, imu.g.y, imu.g.z);
    snprintf(report, sizeof(report), "Accelerometer: x: %6d y: %6d z: %6d",
    imu.a.x, imu.a.y, imu.a.z);
    Serial.println(report);*/
    total = (map(imu.a.x,rawLow, rawHigh,remapLow,remapHigh) + map(imu.a.y,rawLow, rawHigh,remapLow,remapHigh) + map(imu.a.z,rawLow, rawHigh,remapLow,remapHigh) )/3;
    Serial.print("Remapped Acceleration reads: ");
    Serial.println(total);
  }
  else if(test == 5){
    Serial.print("Flex reading: ");
    Serial.println(map(analogRead(flexPin), rawAnalogLow, rawAnalogHigh, remapFlexLow, remapFlexHigh));
    checkFlexStatus();
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
    //analogWrite(buzzerPin,0);
  }
}
boolean accelJerkFeedback(){
imu.read();
  if(accelCount == 0){
    pastTotal = total;
    count++;
  }
  total = (map(imu.a.x,rawLow, rawHigh,remapLow,remapHigh) + map(imu.a.y,rawLow, rawHigh,remapLow,remapHigh) + map(imu.a.z,rawLow, rawHigh,remapLow,remapHigh) )/3;
  if(abs(pastTotal-total)>13){
    Serial.println("Acceleration: ");
    Serial.println(pastTotal-total);
    Serial.println("You jerked!");
    checkVibrationStimulus(false);
    return false;
  }
  //Serial.print(total);
  pastTotal = total;
  return true;
}



