#include "time.h"


// based on 
// https://randomnerdtutorials.com/esp32-date-time-ntp-client-server-arduino/   + look at comments re DST

const char* defaultTimezone = "CET+0CEST,M3.5.0/2:00:00,M10.5.0/3:00:00";   // hopefully this corrects the DST issue 24/4/2021
const char* ntpServer = "uk.pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 0;


struct tm timeinfo;


void printLocalTime()
{

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); 
  configTzTime(defaultTimezone, ntpServer);                   // hopefully this corrects the DST issue  24/4/2021
  
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }

  //Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  //Serial.println(&timeinfo);
}
