
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
const int sleepBPM = 90;
int recordedBPM = 0;


//Button for feedback of Button Progress 2
//const int buttonPin = 12;     // the number of the pushbutton pin
//int buttonState = 0;         // variable for reading the pushbutton status
boolean checkLEDStatus = false;

//Led for LED stimulus Progress 3
//int ledPin = 11;

//Vibrator motor for vibration stimulus Progress 4
const int motorPin = 5;

//Jerking motion for feedback Progress 6
volatile char report[80];
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
volatile int remapHigh = 10;
volatile int accelCount = 0;

//Flex Sensor
int flexVoltageThreshold = 80;
const int flexPin = 6;    

void setup(){
  Serial.begin(115200);             // we agree to talk fast!
  interruptSetup();                 // sets up to read Pulse Sensor signal every 2mS 
  
  //pinMode(ledPin, OUTPUT);
  pinMode(motorPin, OUTPUT);
  pinMode(flexPin, INPUT);
  Wire.begin();
  if (!imu.init()){
    Serial.println("Failed to detect and initialize IMU!");
    while (1);
  }
  imu.enableDefault();
}

void loop(){
  //while(checkAwake == false){//exit when require t
    recordedBPM = checkHeartBPMWithinOperatingRange(BPM);
    //checkAwake = checkBPMBelowThreshold(recordedBPM);
  //}
  //checkUserAwake();
  delay(5000); //take 5 seconds break before rechecking
}
boolean checkFlexStatus(){
  boolean flexStatus = false;
  flexRegisteredValue = analogRead(flexPin);
  if(flexRegisteredValue<flexVoltageThreshold){
    return true;
  }
  return false;
}
int checkHeartBPMWithinOperatingRange(int registeredBPM){
  if(registeredBPM<opHighBPM&&registeredBPM>opLowBPM ){
    Serial.print("Heart beat detected! BPM: ");  
    Serial.println(registeredBPM);
  }
}
boolean checkBPMBelowThreshold(int registeredBPM){
  if(registeredBPM < sleepBPM ){//&& checkFlexStatus() == true){
    Serial.print("You are sleepy! ");
    Serial.print("Sleepy heartrate: ");
    Serial.println(registeredBPM);
    Serial.println("Jerk off to stop Stimulus!!");
    checkAwake=true;    
    checkVibrationStimulus(true);
    return true;
  }
  
  return false;
}
/*void checkHeartBPMCondition(){
  //debugTool();
  recordedBPM = 80;//BPM;
  if(recordedBPM<opHighBPM&&recordedBPM>opLowBPM ){ //Check BPM at operating range
    checkOpBPMRange = true;
    if(checkOpBPMRange == true && checkPreviousOpBPMRange == false){//Check UPDATED BPM is at operating range
      Serial.print("Heart beat detected! BPM: ");  
      Serial.println(recordedBPM);
      checkPreviousOpBPMRange = true;
    }
  }
  else if(checkPreviousOpBPMRange == true){//Updated BPM not within range now
    Serial.println("You got no heart rate now, dead bitch!");
    checkPreviousOpBPMRange = false;
  }
  else{
    checkOpBPMRange = false;//Not within OP BPM Range
  }
}*/
void checkUserAwake(){
  if(checkAwake==true){
    accelCount = 0;
    while(checkAwake==true){
        checkAwake = accelJerkFeedback();
    }
    checkVibrationStimulus(false);
    Serial.println("Sally Beauty has awoken!");
    //delay(5000);//prevent loop immediately exit
  }  
}
/*void checklightStimulus(){
  if(checkAwake == true){
    digitalWrite(ledPin, HIGH);
    checkLEDStatus = true; //debug
    Serial.println("LED switched on!");
  }
  else{
    digitalWrite(ledPin, LOW);
    checkLEDStatus = false;//debug
  }
}*/

void checkVibrationStimulus(boolean onOff){
  if(onOff == true){
    digitalWrite(motorPin, HIGH);
    Serial.println("Motor is On!");
  }
  else{
    digitalWrite(motorPin, LOW);
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
  if(abs(pastTotal-total)>4){
    Serial.println("Acceleration: ");
    Serial.println(pastTotal-total);
    Serial.println("You jerked!");
    checkVibrationStimulus(false);
    return false;
  }
  pastTotal = total;
  return true;
}



