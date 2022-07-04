/*
 * Board: ESP8266
 *    Temp Meter: DS18B20
 *    
 *    
 *Config:
 *   GPIO2 => D4 => Data (DS18B20
 * 
 * 
 * 
 * 
 */

//Core Coding
  #include "core_iot.h"



//Temperature Reading Config
  #include <OneWire.h>
  #include <DallasTemperature.h>
  #define TempSensor_01 2  //Temp Sensor DATA 
  OneWire oneWire(TempSensor_01);
  DallasTemperature sensors(&oneWire); 

//Declare Variables
  //extern float TempF = 0;  //Temperature
 
void setup(void)
{
  // start serial port
  Serial.begin(115200);
  Serial.println("booting");

    RunOnce_WLANSetup();                //connects board to network
    RunOnce_HttpRequest();              //configures the handling of HTTP requests
    RunOnce_StartNetworkTimeClient();   //start the NTP time service
    sendSms("Sensor has rebooted");



  
  // Start up the Temperature Sensor library
  sensors.begin();
}
 
 
void loop(void)
{
    //Required to capture and direct HTTP requests
      httpRequestHandling();
      
  sensors.requestTemperatures(); // Send the command to get temperatures
  TempF=(round(sensors.getTempFByIndex(0)*10))/10;
  
  serialMonitorDisplay();
  
  
  
    
    delay(5000);
}


void serialMonitorDisplay(){
    Serial.print("** ");
    Serial.println(millis());
    Serial.print("\t Uptime: \t");
      Serial.println(tsMillisHuman(millis()));
    Serial.print("\t Now: \t\t");
      Serial.println (timeStampNTP(99));
    

   PrintBoardStats();

    Serial.print("Temperature: ");
    Serial.print(TempF,1); 
    Serial.println("°F");
    
}
