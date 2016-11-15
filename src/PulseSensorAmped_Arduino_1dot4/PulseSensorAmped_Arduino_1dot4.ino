
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


//  Variables
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
const int opLowBPM = 60;
const int opHighBPM = 120;
const int sleepBPM = 100;
int recordedBPM = 0;


//Button for feedback of Button Progress 2
const int buttonPin = 12;     // the number of the pushbutton pin
int buttonState = 0;         // variable for reading the pushbutton status
boolean checkLEDStatus = false;

//Led for LED stimulus Progress 3
int ledPin = 11;

//Buzzer for sound stimulus Progress 4
const int buzzerPin = 1;

//Buzzer for vibration stimulus Progress 4
const int motorPin = 10;

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
volatile int remapDetLow = 0;
volatile int remapDetHigh = 200;
volatile int accelCount = 0;
volatile int accelDisplayCount = 0;

//Implement lack of motion as sleeping trend Progress 7

void setup(){
  //pinMode(blinkPin,OUTPUT);         // pin that will blink to your heartbeat!
  //pinMode(fadePin,OUTPUT);          // pin that will fade to your heartbeat!
  Serial.begin(115200);             // we agree to talk fast!
  interruptSetup();                 // sets up to read Pulse Sensor signal every 2mS 
   // IF YOU ARE POWERING The Pulse Sensor AT VOLTAGE LESS THAN THE BOARD VOLTAGE, 
   // UN-COMMENT THE NEXT LINE AND APPLY THAT VOLTAGE TO THE A-REF PIN
//   analogReference(EXTERNAL);   
  // initialize the pushbutton pin as an input:
  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(motorPin, OUTPUT);
  //tone(buzzerPin,1000);

  Wire.begin();
  if (!imu.init()){
    Serial.println("Failed to detect and initialize IMU!");
    while (1);
  }
  imu.enableDefault();
}


//  Where the Magic Happens
void loop(){
  
//    serialOutput() ;      
  if (QS == true){     // A Heartbeat Was Found
                       // BPM and IBI have been Determined
                       // Quantified Self "QS" true when arduino finds a heartbeat
        //fadeRate = 255;         // Makes the LED Fade Effect Happen
                                // Set 'fadeRate' Variable to 255 to fade LED with pulse
        //serialOutputWhenBeatHappens();   // A Beat Happened, Output that to serial.     
        QS = false;                      // reset the Quantified Self flag for next time    
  
  }//End QS
  checkHeartBPMCondition();
  checkUserAsleep();
  checkUserAwake();
  delay(20);                             //  take a break
  
}
void checkHeartBPMCondition(){
  debugTool();
  recordedBPM = BPM;//80;//BPM;
  if(recordedBPM<opHighBPM&&recordedBPM>opLowBPM ){ //Check BPM at operating range
    checkOpBPMRange = true;
    if(checkOpBPMRange == true && checkPreviousOpBPMRange == false){//Check UPDATED BPM is at operating range
      Serial.print("Heart beat detected!!! <3 <3 <3 BPM: ");  
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
}
void checkUserAsleep(){
  if(checkOpBPMRange == true){//Only perform task when deteced user is within OP range first
    if(recordedBPM < sleepBPM){
      Serial.print("You are sleepy, tired bitch! ");
      Serial.print("Sleepy heartrate: ");
      Serial.println(recordedBPM);
      Serial.println("Press button to stop Stimulus! Dick head!");
      checkAwake=true;    
      checklightStimulus();
      checkBuzzerStimulus();
      checkVibrationStimulus(); 
    }
    
  }
}
void checkUserAwake(){
  
  if(checkAwake==true){
    accelCount = 0;
    while(checkAwake==true){
        //buttonFeedback();
        accelJerkFeedback();
        
    }
    checklightStimulus();
    checkBuzzerStimulus();
    checkVibrationStimulus();
    Serial.println("You are finally awake! noobshit!");
    delay(5000);//prevent loop immediately exit
  }  
  
  
}
void checklightStimulus(){
  if(checkAwake == true){
    digitalWrite(ledPin, HIGH);
    checkLEDStatus = true; //debug
    Serial.println("LED switched on!");
  }
  else{
    digitalWrite(ledPin, LOW);
    checkLEDStatus = false;//debug
  }
}
void debugTool(){
  if(count < 200){
    count++;
  }
  else{
    Serial.println(BPM);
    count = 0;
    if(checkLEDStatus == true)
    {
      Serial.println("LED is On!");
    }
  }
}

void checkBuzzerStimulus(){
  if(checkAwake == true){
    analogWrite(buzzerPin, 244);
    Serial.println("BUZZER is On!");
  }
  else{
    analogWrite(buzzerPin,0);
    Serial.println("Buzzer is OFF!");
  }
}
void checkVibrationStimulus(){
  if(checkAwake == true){
    digitalWrite(motorPin, HIGH);
    Serial.println("Motor is On!");
  }
  else{
    digitalWrite(motorPin, LOW);
    Serial.println("Motor is OFF!");
    //analogWrite(buzzerPin,0);
  }
}
void buttonFeedback(){
  buttonState = digitalRead(buttonPin);//read button pres
  if(buttonState == HIGH){
    checkAwake = false;
    Serial.println("Button Pressed!");
    delay(50); //delay to debounce noise
  }
}
void accelJerkFeedback(){
imu.read();

  /*snprintf(report, sizeof(report), "A: %6d %6d %6d    G: %6%6d %6d",
    imu.a.x, imu.a.y, imu.a.z,
    imu.g.x, imu.g.y, imu.g.z);
  //snprintf(report, sizeof(report), "Accelerometer: x: %6d y: %6d z: %6d",
  //imu.a.x, imu.a.y, imu.a.z);
  //Serial.println(report);

  total = (map(imu.a.x,rawLow, rawHigh,remapLow,remapHigh) + map(imu.a.y,rawLow, rawHigh,remapLow,remapHigh) + map(imu.a.z,rawLow, rawHigh,remapLow,remapHigh) )/3;
  Serial.println(total);*/
  /*int fdPastTotal = pastTotal;
  int fdTotal = total;
  Serial.println("You jerked!");
  if(abs(fdPastTotal-pastTotal)>2){
    Serial.println(total);
    Serial.println("You jerked!");
    checkAwake = false;
  }
  pastTotal = total;
  //Serial.println(readX);*/
  if(accelCount ==0){
    pastTotal = total;
    //pastTotalDet = totalDet;
    count++;
  }
  total = (map(imu.a.x,rawLow, rawHigh,remapLow,remapHigh) + map(imu.a.y,rawLow, rawHigh,remapLow,remapHigh) + map(imu.a.z,rawLow, rawHigh,remapLow,remapHigh) )/3;
  //Serial.println("Acceleration");
  //Serial.println(total);
  if(abs(pastTotal-total)>5){
    Serial.println("Acceleration: ");
    Serial.println(pastTotal-total);
    Serial.println("You jerked!");
    checkAwake = false;
  }
  pastTotal = total;
}
/*void ledFadeToBeat(){
    fadeRate -= 15;                         //  set LED fade value
    fadeRate = constrain(fadeRate,0,255);   //  keep LED fade value from going into negative numbers!
    analogWrite(fadePin,fadeRate);          //  fade LED
  }*/




