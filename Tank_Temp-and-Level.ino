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

//Includes for Temperature Reading
#include <OneWire.h>
#include <DallasTemperature.h>
 

#define TempSensor_01 2  //Temp Sensor DATA 
OneWire oneWire(TempSensor_01);
 

DallasTemperature sensors(&oneWire);  // Pass our oneWire reference to Dallas Temperature.

//Declare Variables
float TempF = 0;  //Temperature in F
 
void setup(void)
{
  // start serial port
  Serial.begin(115200);
  Serial.println("Dallas Temperature IC Control Library Demo");

  // Start up the library
  sensors.begin();
}
 
 
void loop(void)
{
  // call sensors.requestTemperatures() to issue a global temperature
  // request to all devices on the bus
  
  sensors.requestTemperatures(); // Send the command to get temperatures
  
  Serial.print("Temperature is: ");
  TempF=(round(sensors.getTempFByIndex(0)*10))/10;
  
  
  Serial.print(TempF,1); 
  Serial.println("°F");
    delay(1000);
}
