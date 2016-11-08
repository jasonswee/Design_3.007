/*
The sensor outputs provided by the library are the raw
16-bit values obtained by concatenating the 8-bit high and
low accelerometer and gyro data registers. They can be
converted to units of g and dps (degrees per second) using
the conversion factors specified in the datasheet for your
particular device and full scale setting (gain).

Example: An LSM6DS33 gives an accelerometer Z axis reading
of 16276 with its default full scale setting of +/- 2 g. The
LA_So specification in the LSM6DS33 datasheet (page 11)
states a conversion factor of 0.061 mg/LSB (least
significant bit) at this FS setting, so the raw reading of
16276 corresponds to 16276 * 0.061 = 992.8 mg = 0.9928 g.
*/

#include <Wire.h>
#include <LSM6.h>

LSM6 imu;

char report[80];
int reading[3]={0, 0, 0};
int total=0;
int pastTotal = 0;
int readX = 0;
int pastX = 0;

int rawLow = -32768;
int rawHigh = 32767;
int remapLow = 0;
int remapHigh = 10;

void setup()
{
  Serial.begin(9600);
  Wire.begin();

  if (!imu.init())
  {
    Serial.println("Failed to detect and initialize IMU!");
    while (1);
  }
  imu.enableDefault();
}

void loop()
{
  imu.read();

  /*snprintf(report, sizeof(report), "A: %6d %6d %6d    G: %6d %6d %6d",
    imu.a.x, imu.a.y, imu.a.z,
    imu.g.x, imu.g.y, imu.g.z);*/
  snprintf(report, sizeof(report), "Accelerometer: x: %6d y: %6d z: %6d",
  imu.a.x, imu.a.y, imu.a.z);
  //Serial.println(report);

  total = (map(imu.a.x,rawLow, rawHigh,remapLow,remapHigh) + map(imu.a.y,rawLow, rawHigh,remapLow,remapHigh) + map(imu.a.z,rawLow, rawHigh,remapLow,remapHigh) )/3;
  
  if(abs(pastTotal-total)>2){
  Serial.println(total);
  Serial.println("You jerked!");
  }
  pastTotal = total;
  //Serial.println(readX);

  delay(100);
}
