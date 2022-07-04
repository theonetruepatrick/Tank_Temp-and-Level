#ifndef CORE_IOT_H
#define CORE_IOT_H
#include <Arduino.h>

void RunOnce_WLANSetup();
void RunOnce_HttpRequest();
void RunOnce_StartNetworkTimeClient();



void httpRequestHandling();
void handleRoot();
void reboot();
void PrintBoardStats();

String timeStampNTP(int formatType);  /*returns the LOCAL time in various formats
                                      *   formatType:
                                      *          1 = epoch time format; GMT
                                      *          2= epoch time format 
                                      *          3 = mm/dd/yy 
                                      *          4 = hh:mm:ss 
                                      *          5= mm/dd/yy hh:mm:ss
                                      *          6= 4 digit year
                                      *          7= Short (3-letter) Month Name
                                      *          8= Long Month Name
                                      *          9= Short name (3-letter) day of week
                                      *         10= Long name day of week
                                      *          (any other number) = yy:mm:dd hh:mm:ss
                                      *
                                      */        
                                      
                                      
bool DST_flag (unsigned long ts);   //pass the locally adjusted epoch time and get true/false for DST
int signalStrengthValue();          //returns the numeric signal strength value
String signalStrengthQuality();     //returns the qualitative signal strength value

String tsMillisHuman(unsigned long timeVal);
                                    //tsMillisHuman converts milliseconds to DD:HH:MM:SS format as string


String urlencode(String str);                                       //SMS funtionality
String get_auth_header(const String& user, const String& password); //SMS Functionality
void sendSms(String theMessage);                                    //the actual SEND code for SMS

extern float TempF;  //Temperature

#endif
