/*
 * Board: ESP8266
 *    Temp Meter: DS18B20
 *    Depth Meter: JSN-SR04T ultrasonic distance sensor
 *    
 *    
 *Config:
 *    GPIO2 => D4 => Data (DS18B20)
 *    GPIO0 => D3 => Echo TX (JSN-SR04T)
 *    GPIO4 => D2 => Trig DX (JSN-SR04T)
 * 
 * 
 * 
 * 
 * 
 */

//Core Coding
  #include "core_iot.h"

// Pin Connections
#define TempSensor 5  //GPIO5 => D1 => Data (DS18B20)
#define trigPin 0        //GPIO4 => D3 => Trig DX (JSN-SR04T)
#define echoPin 2        //GPIO2 => D4 => Trig DX (JSN-SR04T)

//Temperature Reading Config
  #include <OneWire.h>
  #include <DallasTemperature.h>
  OneWire oneWire(TempSensor);
  DallasTemperature sensors(&oneWire); 

  // Define variables for sensor:
    float duration;               //Sensor pulse
    float distance;               //Sensor pulse
    float tankLevel;              //the % of the tank level
  
  //Variables to set specific to each tank
    float tankFull = 10;          //minimum distance (inches) between sensor and water level aka FULL
    float tankEmpty = 21;         //maximum distance (inches) between sensor and water level aka EMPTY
    float tankLevelMax = 100;     //highest value (%) allowed for tank level
    float tankLevelMin = 0;       //lowest value (%) allowed for tank level
    float distanceOffset = 1.0;   //compensates for reading in INCHES - not sure if this is common across all sensors
    float tankAlarm = 30;         //Threshold for alerting if level falls below this %
    
    const float speedOfSound = 0.0135;  //speed of sound: inches per MICROsecond
    float durationTimeout = (tankEmpty*1.5*2)/speedOfSound;  //used to shorten wait time for pulse 
  
  // Define variables for Data Analysis
    //sampling factors
      int readingDelayMS = 60000;  //time gap between readings; default is 60 seconds
      int readingDelayAdjustment;  //me being WAY too anal retentive
      const int numReadings = 15;  //number of readings to be averaged/smoothed
      int readingCount=0;          //current total number of readings
      
    //Data Smoothing
      unsigned long readingTimestamp = 0;    //timestamp [milliseconds] variable for reading cycle
      int readings[numReadings];   //array that stores readings
      int readIndex = 0;           //index for readings variable
      float readingSumTotal = 0;
      float readingAverage = 0;

    //Rate of Change calcuation
      float readingDelta = 0;
      int ROCIndex = 0;           // "ROC" = "Rate of Change"
      float ROCSumTotal = 0;
      float readingROC[numReadings];  //array that stores deltas of the running averages
      float ROCTrend = 0;

    //Time To Empty
      float hrsToEmpty=0;
      String lowSampleFlag;





 
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

   // Define inputs and outputs
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
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
    Serial.println();
    Serial.println();
    Serial.println("-----------");
    Serial.print("\tTemperature: \t");
      Serial.print(TempF,1); 
      Serial.println("°F");
      Serial.println();
    
    Serial.print("\t Uptime: \t");
    Serial.println(tsMillisHuman(millis()));
    Serial.print("\t Now: \t\t");
    Serial.println (timeStampNTP(99));
    

   PrintBoardStats();
  
    
    
}
