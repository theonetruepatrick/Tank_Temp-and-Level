/* NOTE TO SELF
 *  for TWILIO SMS see:
 *  https://github.com/tigerfarm/arduino/blob/master/instructables/HttpTwSendSms/HttpTwSendSms.ino  
 *  and project 8266.101-TempSensor-SMS-ver2-Final
 *  
 *SMS References
 *  SMS Reference Site: https://www.instructables.com/Send-an-SMS-Using-an-ESP8266/
 *  SMS Code: https://github.com/tigerfarm/arduino/blob/master/instructables/HttpTwSendSms/HttpTwSendSms.ino
 *  
 *  
 *  Base code is from:
 *    https://github.com/TwilioDevEd/twilio_esp8266_arduino_example
 *  Documentation:
 *    https://www.twilio.com/docs/sms/tutorials/how-to-send-sms-messages-esp8266-cpp
 *  Fingerprint note,
 *    https://github.com/TwilioDevEd/twilio_esp8266_arduino_example/issues/1
 *    echo | openssl s_client -connect api.twilio.com:443 | openssl x509 -fingerprint
 *    ...
 *    SHA1 Fingerprint=BC:B0:1A:32:80:5D:E6:E4:A2:29:66:2B:08:C8:E0:4C:45:29:3F:D0
 *    ...
 *  
 */


bool disableSMS = true;  // when true disables SMS function for troubleshooting


#include "core_iot.h"
#include <arduino_secrets.h>  // Local library to store credentials
                              //  D:\Program Files (x86)\Arduino\libraries\SecretWifi




//User-set device variables
      const char* ssid     = secret_ssid;     //handled by arduino_secrets.h
      const char* password = secret_password; //handled by arduino_secrets.h
      
      int tz = -18000;        //EASTERN time zone (-5GMT) delta in seconds from GMT - set appropriately for your timezone
      bool dstObserved= true; //some time zones do not observe Daylight Savings, set to false for those 

//WLAN Connection Settings
    // NETWORK CLIENT Config
      #include <ESP8266WiFi.h>
      #include <ESP8266HTTPClient.h>
      #include <WiFiClient.h>




    //reference variables
      String MACAddy ;  //network MAC address
      String IPAddy;    //network IP (DHCP) address
      String HostName;  //ESP Hostname (hard coded)
      
// WEB SERVER (on demand) Config
      #include <ESP8266WebServer.h>
      ESP8266WebServer server;
      String html;

//Get TIME
      #include <NTPClient.h>
      #include <WiFiUdp.h>
      WiFiUDP ntpUDP;
      NTPClient timeClient(ntpUDP, "north-america.pool.ntp.org", 0);

//SMS
      #include <WiFiClientSecure.h>
      #include "base64.h"
      WiFiClientSecure client;

  //SMS info - credentials
      const char* account_sid = secret_account_sid;
      const char* auth_token  = secret_auth_token;
      const char fingerprint[]= secret_fingerprint;
      String from_number      = secret_from_number;
      String to_number        = secret_to_number;
      
      extern String SMS_messagePrefix = "(Tank Sensor): ";
            
float TempF;

//constants
    extern const int constSeconds = 1000;  //ms per Second
    extern const int constMinutes = 60000; //ms per Minute
    extern const int constHours = 3600000; //ms per Hour
    extern const int constDays = 86400000; //ms per Day 
      
void RunOnce_WLANSetup(){
  //Connect to wireless & get IP address    
      Serial.println("WiFi: ");
      WiFi.begin(ssid, password);             //supply network credentials
      Serial.println("\t Connecting ");
      Serial.print("\t");
      
      while(WiFi.status() != WL_CONNECTED) {  //keeps trying to connect until successful
        Serial.print(".");  //progress indicator
        delay(500);
        if (millis()> 180000){  //troubleshooting: if can't acquire a signal after 3 min, try rebooting
          reboot();
        }
        
      }

  //reconnect automatically when the connection is lost
      //https://randomnerdtutorials.com/solved-reconnect-esp8266-nodemcu-to-wifi/
      WiFi.setAutoReconnect(true);
      WiFi.persistent(true);
      
      Serial.println("\n\t Connected");  
      MACAddy = WiFi.macAddress();            //assigns the MAC address to a variable
      IPAddy = WiFi.localIP().toString();     //assigns the IP (DHCP) address to a variable
      HostName = WiFi.hostname();
      
      Serial.print("\t hostname:\t");
        Serial.println(HostName);
      Serial.print("\t IP Address:\t");
        Serial.println(IPAddy);
      Serial.print("\t MAC Address:\t");
        Serial.println(MACAddy);
      Serial.printf("\t Signal:\t%d dBm ", WiFi.RSSI());  //Signal Strength
        Serial.print("(");
        Serial.print(signalStrengthQuality());
        Serial.println(")");

//for SMS
    SMS_messagePrefix= HostName + SMS_messagePrefix;
   
}
                              

void RunOnce_HttpRequest(){
      // enable HTTP request handling
    server.on("/", handleRoot);  //default location when serving http request
    server.begin();
        Serial.println();
    Serial.println("....STARTING HTTP SERVER....");
        Serial.println();
}

void RunOnce_StartNetworkTimeClient(){
    timeClient.begin();
    timeClient.setTimeOffset(0);
    timeClient.update();
}

void httpRequestHandling(){      //executed when http request rec'd
  server.handleClient();         //should work in main void Loop(), but only work in .cpp
                                 //   because the "server" is defined here.  not sure yet
}  

void reboot() {
  ESP.restart();
}

String timeStampNTP(int formatType){
   
    
    String daysOfTheWeek_Short[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    String daysOfTheWeek_Long[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
    String monthOfTheYear_Short[12] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    String monthOfTheYear_Long[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "Dececember"};
 
     //Get a time structure:
       /*   The NTP Client doesn’t come with functions to get the date. So, 
            need to create a time structure (struct tm) and then, 
            access its elements to get information about the date.
            https://randomnerdtutorials.com/esp8266-nodemcu-date-time-ntp-client-server-arduino/
            http://www.cplusplus.com/reference/ctime/tm/
       */
      time_t epochTime = timeClient.getEpochTime();
      struct tm *ptm = gmtime ((time_t *)&epochTime);
   
      timeClient.update();    //synch up to time server 
      //adjusts for DST at Eastern Timezone
      
      if (DST_flag(timeClient.getEpochTime()) != 1){
        timeClient.setTimeOffset(tz);
      } else {
        timeClient.setTimeOffset(tz - (-3600));
      }
   
    //figure out Daylight Savings Time
    //DST starts on the second Sunday of March and ends on the first Sunday of November, at 2:AM both times 
      int dow = timeClient.getDay();
      int currentMonth = ptm->tm_mon+1;
      int monthDay = ptm->tm_mday;

    //returns the current time in different formats
      String timeOutput="";  //the formatted response to return
    
    switch (formatType){
      case 1: //epoch time GMT
        {               
          //unsigned long epochTime = timeClient.getEpochTime();
          timeClient.setTimeOffset(0);
          timeOutput = String(timeClient.getEpochTime());
          break;
        }
      case 2: //epoch time LOCAL
        {
          timeOutput = String(timeClient.getEpochTime());
          break;
        }
      case 3: // mm/dd/yy (local)
        {
         if ((ptm->tm_mon+1)<10){timeOutput +="0";}
          timeOutput += ptm->tm_mon+1;
            timeOutput +="/";
          
          if ((ptm->tm_mday<10)){timeOutput +="0";}
          timeOutput += ptm->tm_mday;
          timeOutput +="/";
          timeOutput += (ptm->tm_year+1900)%2000;
          break;
        }
        
      case 4: //   hh:mm:ss (local)
        {
          timeOutput +=timeClient.getFormattedTime();
          break;
        }
      case 5: //   mm/dd/yy hh:mm:ss (local)
        {
          
          if ((ptm->tm_mon+1)<10){timeOutput +="0";}
          timeOutput += ptm->tm_mon+1;
            timeOutput +="/";
          
          if ((ptm->tm_mday<10)){timeOutput +="0";}
          timeOutput += ptm->tm_mday;
          timeOutput +="/";
           
          timeOutput += (ptm->tm_year+1900)%2000;
          
          timeOutput +=" ";

          timeOutput +=timeClient.getFormattedTime();
          break;
        }
      case 6: // 4 digit year
        {
          timeOutput = String(ptm->tm_year+1900);
          break;
        }
      case 7: //  Month
        {
          timeOutput = monthOfTheYear_Short[ptm->tm_mon];
          break;
        }
      case 8: //fully spelled out Month
        {
          timeOutput = monthOfTheYear_Long[ptm->tm_mon];
          break;
        }
      case 9: //Week day short
        {
          timeOutput = daysOfTheWeek_Short[ptm->tm_mday-1];
          break;
        }
      case 10: //Week day long
        {
         timeOutput = daysOfTheWeek_Long[ptm->tm_mday-1];
         break;
        }
        
      default: //yy:mm:dd hh:mm:ss
        {
          timeOutput = daysOfTheWeek_Long[ptm->tm_mday-1];
          timeOutput += ": ";
          timeOutput += monthOfTheYear_Long[ptm->tm_mon];
          timeOutput += " ";
          timeOutput += ptm->tm_mon+1;
          timeOutput += ", ";
          timeOutput += String(ptm->tm_year+1900);
          timeOutput += " ";

          if (timeClient.getHours() >0 && timeClient.getHours()<= 12){
                  timeOutput += (timeClient.getHours());
                  timeOutput += ":";
                  if (timeClient.getMinutes()<10){
                    timeOutput +="0";
                  }
                  timeOutput +=timeClient.getMinutes();
                  
            
            } else  {
            
            timeOutput += (timeClient.getHours()-12);
            timeOutput += ":";
              if (timeClient.getMinutes()<10){
                timeOutput +="0";
              }
             timeOutput +=timeClient.getMinutes();
             
          }

        if (timeClient.getHours() >=12 && timeClient.getHours()< 24){
            timeOutput +="PM";
        } else {
          timeOutput +="AM";
        }
        
        break;
         }
    }
     return timeOutput;
}

bool DST_flag (unsigned long ts){  //ts is TimeStamp
   //test for timeStamp year
     //Timestamps for January 1, 20xx 00:00:00 for each year 2022 through 2033
          unsigned long DST_newYearGMT[12] = {1640995200, 1672531200, 1704067200, 1735689600, 1767225600, 1798761600, 1830297600, 1861920000, 1893456000, 1924992000, 1956528000, 1988150400};
    //Timestamp (date and time) of START of each Daylight Savings Time 2022 through 2032
          unsigned long DST_startGMT[11] = {1647154800, 1678604400, 1710054000, 1741503600, 1772953200, 1805007600, 1836457200, 1867906800, 1899356400, 1930201200, 1962860400};  
    //Timestamp (date and time) of END of each Daylight Savings Time 2022 through 2032
          unsigned long DST_endGMT[11] = {1667718000, 1699167600, 1730617200, 1762066800, 1793516400, 1825570800, 1857020400, 1888470000, 1919919600, 1951369200, 1983423600};

    int ts_yearIndex =0;  //used to select array entries for comparison
    
    //Step 1: cycle through new year timestamp to find year index
    for (int i=0;i<=11;i++) {         
        //check to see if ts is between the two new year date
        if ((ts>=DST_newYearGMT[i]) && (ts<DST_newYearGMT[i+1])){
            ts_yearIndex = i; //sets index for upcoming searches
            break;            //no need to continue the loop
        }
    }

   //Step 2: look to see if ts falls between the start and end of DST for the year
        if((ts>=DST_startGMT[ts_yearIndex]) && (ts<=DST_endGMT[ts_yearIndex]) && (dstObserved!=false)){
            return true;
        } else {
            return false;
        }
     }

String tsMillisHuman(unsigned long timeVal){
    
    String tsUptime="";     //string used to compile the uptime
    int tsDays = timeVal/constDays;
    int tsHours = (timeVal%constDays)/constHours;
    int tsMinutes = ((timeVal%constDays)%constHours)/constMinutes;
    int tsSeconds = (((timeVal%constDays)%constHours)%constMinutes)/constSeconds;

  
    if (tsDays<10){tsUptime+="0";}          //adds leading zero if needed
    tsUptime +=String(tsDays);
    tsUptime +=":";
    
      if (tsHours<10){tsUptime+="0";}       //adds leading zero if needed
      tsUptime +=String(tsHours);
      tsUptime +=":";
    
        if (tsMinutes<10){tsUptime+="0";}   //adds leading zero if needed
        tsUptime +=String(tsMinutes);
        tsUptime +=":";
    
          if (tsSeconds<10){tsUptime+="0";} //adds leading zero if needed
          tsUptime +=String(tsSeconds);
  
  return tsUptime;
  
}



int signalStrengthValue(){
   return WiFi.RSSI();  //numeric value of signal strength
}

String signalStrengthQuality(){
   int ss = WiFi.RSSI();  //value of signal strength
    
   if (-29 >= ss && ss >=-31) {
      return "Best";
   } else if (-32 >= ss && ss >= -50){
      return "Excellent";
   }else if (-51 >= ss && ss >= -60){
      return "Ideal";
   }else if (-61 >= ss && ss >= -67){
      return "Acceptable";
   }else if (-68 >= ss && ss >= -70){
      return "Poor";
   }else if (-71 >= ss && ss >= -80){
      return "Unreliable";
   }else if (-81 >= ss && ss >= -90){
      return "Bad";
   } else {
      return "Issue";
  }
}

String urlencode(String str) {      //USED WITH SMS
    String encodedString = "";
    char c;
    char code0;
    char code1;
    char code2;
    for (int i = 0; i < str.length(); i++) {
      c = str.charAt(i);
      if (c == ' ') {
        encodedString += '+';
      } else if (isalnum(c)) {
        encodedString += c;
      } else {
        code1 = (c & 0xf) + '0';
        if ((c & 0xf) > 9) {
          code1 = (c & 0xf) - 10 + 'A';
        }
        c = (c >> 4) & 0xf;
        code0 = c + '0';
        if (c > 9) {
          code0 = c - 10 + 'A';
        }
        code2 = '\0';
        encodedString += '%';
        encodedString += code0;
        encodedString += code1;
      }
      yield();
    }
    return encodedString;
  }

String get_auth_header(const String& user, const String& password) {
  int i; //added this to fix "std::string::size_type i = 0;" error below
  size_t toencodeLen = user.length() + password.length() + 2;
  char toencode[toencodeLen];
  memset(toencode, 0, toencodeLen);
  snprintf(toencode, toencodeLen, "%s:%s", user.c_str(), password.c_str());
  String encoded = base64::encode((uint8_t*)toencode, toencodeLen - 1);
  String encoded_string = String(encoded);
  //std::string::size_type i = 0;
  // Strip newlines (after every 72 characters in spec)
  while (i < encoded_string.length()) {
    i = encoded_string.indexOf('\n', i);
    if (i == -1) {
      break;
    }
    encoded_string.remove(i, 1);
  }
  return "Authorization: Basic " + encoded_string;
}

void sendSms(String theMessage) {
      //will not send SMS if disableSMS is set to TRUE
      if (disableSMS){ 
        Serial.print("\nSMS Send is intentionally disabled but would execute here.\n\n");
        return; 
        }  
      
      // Use WiFiClientSecure to create a TLS 1.2 connection.
      client.setFingerprint(fingerprint);
      const char* host = "api.twilio.com";
      const int   httpsPort = 443;
      
      Serial.print("..Connecting to ");
      Serial.println(host);
      
      if (!client.connect(host, httpsPort)) {
        Serial.println("***Connection failed***");
        char buf[200];
        int err = client.getLastSSLError(buf, 199);
        buf[199] = '\0';
        Serial.print("...Error code: ");
        Serial.print(err);
        Serial.print(", ");
        Serial.println(buf);
        return; // quits void execution and returns
      }
      
      
      Serial.println("...Connected.");
      Serial.println("....Post SMS request.");
      String post_data = "To=" + urlencode(to_number)
                         + "&From=" + urlencode(from_number)
                         + "&Body=" + urlencode(SMS_messagePrefix) + (theMessage);
      String auth_header = get_auth_header(account_sid, auth_token);
      
      String http_request = "POST /2010-04-01/Accounts/" + String(account_sid) + "/Messages HTTP/1.1\r\n"
                            + auth_header + "\r\n"
                            + "Host: " + host + "\r\n"
                            + "Cache-control: no-cache\r\n"
                            + "User-Agent: ESP8266 Twilio Example\r\n"
                            + "Content-Type: application/x-www-form-urlencoded\r\n"
                            + "Content-Length: " + post_data.length() + "\r\n"
                            + "Connection: close\r\n"
                            + "\r\n"
                            + post_data
                            + "\r\n";
      client.println(http_request);
      Serial.println(".....Message request sent.");
      //
      // Read the response.
      // Comment out the following, if response is not required. Saves time waiting.
      Serial.println("......Waiting for response.");
//      String response = "";
//      while (client.connected()) {
//        String line = client.readStringUntil('\n');
//        response += (line);
//        response += ("\r\n");
//      }
      Serial.println(".......Connection is closed.\n");
//      Serial.println("+ Response:");
//      Serial.println(response);
      
    }

void PrintBoardStats(){
        Serial.print("\t hostname:\t");
        Serial.println(HostName);
      Serial.print("\t IP Address:\t");
        Serial.println(IPAddy);
      Serial.print("\t MAC Address:\t");
        Serial.println(MACAddy);
      Serial.printf("\t Signal:\t%d dBm ", WiFi.RSSI());  //Signal Strength
        Serial.print("(");
        Serial.print(signalStrengthQuality());
        Serial.println(")");
}

void handleRoot(){
   //user interface HTML code----------------
      html = "<!DOCTYPE html><html><head>";
      html += "<title>Tank Temperature and Level Sensor</title>";
      html += "<meta http-equiv='refresh' content='5'></head><body>";
      html += "<h1>Tank Temperature and Level Sensor</h1>";
      html +="<h2>Current Tank Temp: ";
      html += TempF; 
      html += "&degF </h2>";

      html +="<table>";
 
      html +="<tr><td> Now: </td><td>";
      html +=timeStampNTP(99);
      html +="</td></tr>";
    
      html +="<tr><td> Uptime: </td><td>";
      html +=tsMillisHuman(millis());
      html +="</td></tr>";

      html +="<tr><td> </td><td> </td></tr>";
   
      html +="<tr><td> Device IP: </td><td><strong>";
      html +=IPAddy;
      html +="</strong></td></tr>";
    
      html +="<tr><td> Device Hostname: </td><td>";
      html +=HostName;
      html +="</td></tr>";

      html +="<tr><td> MAC address: </td><td>";
      html +=MACAddy;
      html +="</td></tr>";

      html +="<td> Signal Strength: </td><td>";
      html +=WiFi.RSSI();
      html +=" dBm (" + signalStrengthQuality() + ")";
      html +="</td></tr>";
      
     

      html +="</table>";
     
      html +="</body></html>";
      
    
   server.send(200,"text/html", html);
}
