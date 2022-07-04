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

// Clear the trigPin by setting it LOW:
  digitalWrite(trigPin, LOW);
  
  if (millis()-readingTimestamp >= readingDelayMS){  // will execute only after pre-defined time period
    getReading();
  }


  
  serialMonitorDisplay();
  
  
  
    
    delay(5000);
}


void serialMonitorDisplay(){
    if (readingCount < numReadings) {
          lowSampleFlag = "**";     //visual flag to indicate that current average is not a full set of data yet
        } else {
          lowSampleFlag = "";
        }
    if (tankLevel<=tankAlarm){
        Serial.println ("  !!!ALERT!!!    Low Tank Level    !!!ALERT!!!");
      }
    
    
    Serial.println();
    Serial.println();
    Serial.println("-----------");
    Serial.print("\tTemperature: \t");
      Serial.print(TempF,1); 
      Serial.println("°F");
      Serial.println();
  
Serial.print ("\t Pulse duration (round trip):\t");
      Serial.print (duration,0);
      Serial.println (" µs");
      
      Serial.print("\t Distance to liquid surface:\t");
        Serial.print(distance,1);
        Serial.println (" in");
     
      Serial.print ("\t Tank level - Current:\t\t");
        Serial.print(tankLevel,1);
        Serial.println("%");

      Serial.print ("\t Tank level - Running Avg:\t");
        Serial.print(readingAverage,1);
        Serial.print("%");
        Serial.print(lowSampleFlag);     
        Serial.println();
        
      Serial.print ("\t Tank level change trend:\t");
        Serial.print(ROCTrend,1);
        Serial.print("%");
        Serial.print(lowSampleFlag);
        Serial.println();

       Serial.print ("\t Approx time to empty:\t\t");
        if (hrsToEmpty <= 0){
          Serial.print("n/a");
        } else {
          Serial.print(hrsToEmpty,0);
          Serial.print(" hrs");
        }
        
        Serial.print(lowSampleFlag);
        Serial.println();
      
    
    Serial.print("\t Uptime: \t");
    Serial.println(tsMillisHuman(millis()));
    Serial.print("\t Now: \t\t");
    Serial.println (timeStampNTP(99));
    

   PrintBoardStats();
 }

 void getReading(){
   // Trigger the sensor by setting the trigPin high for 10 microseconds:
      digitalWrite(trigPin, HIGH);
      delayMicroseconds(10);
      digitalWrite(trigPin, LOW);  //
   
      duration = pulseIn(echoPin, HIGH, durationTimeout); // Read the echoPin for round-trip pulse time.

      if (duration > 10){  //sometimes echo fails; returns zero
        readingTimestamp=millis();    //timestamp this loop
        readingDelayAdjustment= readingTimestamp % readingDelayMS;
        readingTimestamp -= readingDelayAdjustment;
     
       // Calculate the distance 
          distance = duration*speedOfSound;     //converts the duration to INCHES
          distance /= 2;                //halves distance for round trip
          distance += distanceOffset; //adds compensation to measurement
       
      // converts distance to % of tank    
          tankLevel = round(100*(1-((distance-tankFull)/(tankEmpty-tankFull))));
          tankLevel = max(tankLevel, tankLevelMin);   //doesn't allow reading to go below 0
          tankLevel = min(tankLevel, tankLevelMax);   //doesn't allow reading to go above 100
      
      // Call the smoothing formula
          dataAnalysis();
          
//      // display output
//          serialMonitorOutput();
       
     } else {   
        readingTimestamp=millis()-readingDelayMS;     //if distance returns a "0", re-run reading
        Serial.print (readingTimestamp);
        Serial.println(" : Error: zero value returned, redoing reading....");
     }
      
}

void dataAnalysis(){
    readingDelta=readingAverage;  //set initial value for calculating the rate of change
    
    readingSumTotal -= readings[readIndex]; // subtract the last reading from the total
    readings[readIndex] = tankLevel;        // assigns current level to array
    readingSumTotal += readings[readIndex]; // add the currentreading to the total
    
    readIndex = readIndex + 1;              // advance to the next position in the array:
    if (readIndex >= numReadings) {         // if at the end of the array, wrap around to the beginning
      readIndex = 0;
    }
    
    if (readingCount < numReadings){        //accomodates initial lack of data until array is fully populated 
      readingCount++;
      readingAverage = readingSumTotal / readingCount;
                                          
    } else {
      readingAverage = readingSumTotal / numReadings;
    }

    if (readingCount < 2){
      readingDelta=0; //eliminates skewed analysis with first reading
    } else {
      readingDelta-=readingAverage; //subtracts updated average from previous average
    }

    
  //this will work essentially like the dataAnalysis but tracks the trend of the average change over time
    ROCSumTotal -= readingROC[ROCIndex];  // subtract the last reading from the total
    readingROC[ROCIndex] = readingDelta;  // assigns current level to array
    ROCSumTotal += readingROC[ROCIndex];  // add the currentreading to the total
    ROCIndex = ROCIndex + 1;              // advance to the next position in the array:
    if (ROCIndex >= numReadings) {        // if at the end of the array, wrap around to the beginning
      ROCIndex = 0;
    }   
   
   if (readingCount < numReadings){       //accomodates initial lack of data until array is fully populated 
      ROCTrend = ROCSumTotal / readingCount;
    } else {
      ROCTrend = ROCSumTotal / numReadings;
    }


  //this will estimate the number of hours until tank is empty
      hrsToEmpty = ((tankLevel/ROCTrend)/60)*(-1);  
    

}
