//#include "pitches.h"
#include <Wire.h>
int VibrationMotor = 1;
int PushButton = 12;
int Accelerometer = 15;
int LEDPin = 14;
int PulseSensor = 20;
int Buzzer = 21;

int HeartRate_i;
int HeartRate_f;
boolean TapButton;

void setup() {
  Serial.begin(115200);

  pinMode(Accelerometer, INPUT);
  pinMode(PulseSensor, INPUT);
  pinMode(PushButton, INPUT);
  pinMode(LEDPin, OUTPUT);
  pinMode(Buzzer, OUTPUT);
  pinMode(VibrationMotor, OUTPUT);
  TapButton = LOW;

}

void loop() {
  // put your main code here, to run repeatedly:
  analogWrite(PulseSensor, HeartRate_i);
  delay (1000);
  analogWrite(PulseSensor, HeartRate_f);
  while ((HeartRate_f - HeartRate_i) >= 20)
  {
    analogWrite(LEDPin, 200);
    digitalWrite(VibrationMotor, HIGH);
    if (TapButton = LOW)
    {
      digitalWrite(Buzzer, HIGH);
    }
  }
}
