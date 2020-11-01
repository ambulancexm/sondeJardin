#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>


Adafruit_BMP280 runBmp280(){

  Adafruit_BMP280 bmp;
  while(!bmp.begin(0x76)){
    Serial.println("marche pas!!!!");
    delay(1500);
  }
  Serial.println("c'est cool");
  return bmp;
}

float getPressure(char * str){
  float pressure = 0;
  Adafruit_BMP280 bmp = runBmp280();
 
  if(strcmp(str,"pa")== 0){
    pressure = bmp.readPressure();  
  }
  else {
    pressure = (bmp.readPressure())/100;
  }
  
  return pressure;
  
}
