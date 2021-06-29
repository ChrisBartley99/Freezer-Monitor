/////////////////////////////////////
//   Change to 0 to for test code  or 1 for live code
#define LIVE_CODE 0
////////////////////////////////////

#include <WiFi.h>
#include "secrets.h"

// include library to read and write from flash memory
// https://www.electronics-lab.com/project/using-esp32s-flash-memory-for-data-storage/
#include <EEPROM.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#include "cb_ssd1306_text.h"    //  OLED display 

#include "GetNTPtime.h"         // Internet Time protocol

#include "ESPtone.h"            // Buzzer stuff

#include "SendGmail.h"          // GMAIL handling

// #####################
// Includes for web server OTA code
// https://randomnerdtutorials.com/esp32-ota-over-the-air-arduino/
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
// ####################'


#include "ThingSpeak.h"         // always include thingspeak header file after other header files and custom macros


// -----------------------------------------------

#define ThingSpeakUpdateInterval 10   // every 15 mins ( approx)

#define FreezerDoorSwitchPin 16          

#define LED_BUILTIN  2        // ESP32 DoItAll board

#define DOOR_OPEN   1
#define DOOR_CLOSED 2

#define MinsBeforePanic  1.5

// define the number of bytes you want to access
#define EEPROM_SIZE 1

#define EEPROM_REBOOTED_CODE 0x55  

//------------------------------------------------

long DaysRunning = 0;

bool DoorIsOpen = false;

int ThisDay;
int DayofYear;


int DoorStatus;

char buffer[80];
int loop_count;

float temperatureC;
float MaxtemperatureC;
float MintemperatureC;
float LongMaxtemperatureC = -100;
float LongMintemperatureC = 100;


int ThingSpeakUpdateTime;

unsigned long openDoorCountMillis;
unsigned long LastThinkSpeakUpdateMillis;
unsigned long currMillis;


//  Email stuff

bool SentPanicEmailFlag;

char ToAddress[] = EMAILRECEIPIENT;

char Subject1[] =   "Freezer Monitor : DOOR LEFT OPEN !!!";

char Subject2[] =   "Freezer Monitor : Working OK";

char Subject3[] =   "Freezer Monitor : Door Closed";

char Subject4[] =   "ThinkSpeak update Problem";

char Subject5[] =   "Freezer Monitor : REBOOTED";



// ---------------------------

// Set up the Wi_Fi params
char ssid[] = SECRET_SSID;   // your network SSID (name)  ff
char pass[] = SECRET_PASS;   // your network password
int keyIndex = 0;            // your network key Index number (needed only for WEP)


// Handle the ThingSpeak channel
unsigned long myChannelNumber = SECRET_CH_ID;
const char * myWriteAPIKey = SECRET_WRITE_APIKEY;

// Handle the DS18B20 Temp sensor
// GPIO where the DS18B20 is connected to
const int oneWireBus = 4;

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensors(&oneWire);


int first_temp_read;

// Initialize our values
int number1 = 1;
int number2 = 0;
int number3 = 0;
int number4 = 0;
String myStatus = "";



WiFiClient  client;


//  == Start the WebServer to allow OTA ===
AsyncWebServer server(80);







void setup() {

  
  StartSSD1306VCC();    // set up the voltage for the display
  display.clearDisplay();
  ScreenPrint("Booting...",5,20,2);
  display.display();


  Serial.begin(115200);  //Initialize serial


  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(FreezerDoorSwitchPin, INPUT_PULLUP);


  WiFi.mode(WIFI_STA);

  
  ThingSpeak.begin(client);  // Initialize ThingSpeak

    
  // Start the DS18B20 sensor
  sensors.begin();

  // initialize EEPROM with predefined size
  EEPROM.begin(EEPROM_SIZE);


  ConnectToWiFi();              


  // #### Set up the OTA update stuff 
  //=================================
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", WebSignon().c_str());
  });



  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server started");


  ThingSpeakUpdateTime = ThingSpeakUpdateInterval;    // Set the defaultupdate frequency for ThinkSpeak

  LastThinkSpeakUpdateMillis = millis();

  DoorIsOpen = -1;
  DoorStatus = DOOR_CLOSED;

  SentPanicEmailFlag = false;


  printLocalTime();       // Get the NTP time
  ThisDay = 0;
  DayofYear = timeinfo.tm_yday;    // make note of Day number so we can send keep Gmail alive email


  ResetMaxMin();



  if (EEPROM.read(0) == EEPROM_REBOOTED_CODE )
  {
       SendNotificationEmail(Subject5);
       EEPROM.write(0, 0); 
       EEPROM.commit();    
       
  }

  InitScreen();       // My Splash Screen
  delay(500);
  
}



void __loop()
// Test loop to test out the Reboot reporting email stuff
{
  Serial.print("Loop : ");
  Serial.println(loop_count);
  
  AsyncElegantOTA.loop();
  
  display.clearDisplay();

  ScreenPrint(String(loop_count), 5, 10, 2);

  display.display();

  delay(500);
  loop_count++;
  
  if (loop_count > 99)
  {
        EEPROM.write( 0 ,  EEPROM_REBOOTED_CODE) ;
        EEPROM.commit();  
          
        ESP.restart();        
    
  }
}



void loop()
{

//  Serial.print("Loop : ");
//  Serial.println(loop_count);
//  Serial.println(millis());

  AsyncElegantOTA.loop();

  ReadTemperature();

  display.clearDisplay();
  
  DisplayTemperature();
  
  DisplayTime();


  // See if fridge door is open
  if (digitalRead(FreezerDoorSwitchPin) == HIGH)
  {

    if (DoorIsOpen == false)            // Log this is the 1st time we noticed door is open
    {
      // Door has just been opened
      Serial.println(" //// Door opened");
      DoorIsOpen = true ;
      DoorStatus = DOOR_OPEN;
      openDoorCountMillis = millis();   // Note Time when door was opened

      UpdateThingSpeak(1);              // Update ThingSpeak NOW to report Door open
    }

      //  Serial.println("Freezer Door is open");
      display.clearDisplay();
      DisplayTemperature();
      if (loop_count % 2 == 0)
      {
          ScreenPrint("Door Open!", 5, 50, 2);
          sound_short_warning();
      }

    currMillis = millis();
    if (currMillis > (openDoorCountMillis + ( (MinsBeforePanic * 60)  * 1000))) {

      // Door open tooo long !!
      //   Serial.print(" Door Open too long");
     display.clearDisplay();
     DisplayTemperature();
      if (loop_count % 2 == 0)
      {
          ScreenPrint("PANIC", 5, 50, 2);
      }
      else
      {    
          ScreenPrint("PANIC", 60, 50, 2);
      }    

      sound_long_warning();

     if (SentPanicEmailFlag == false)   // Only want to send the Panic email oncer per event
     {
        SendNotificationEmail(Subject1);
        SentPanicEmailFlag = true;
     }
      
    }

  }
  else
  {

    if (DoorIsOpen == true) 
    {
        // Door has just been closed
        //==========================
        Serial.println("\\\\\\Door Closed");

        DoorStatus = DOOR_CLOSED;

        sound_tune();

        display.clearDisplay();
          ScreenPrint("DOOR", 32, 5, 3);
          ScreenPrint("CLOSED", 12, 40, 3);
        display.display();
        delay(2000);       
              
       if (SentPanicEmailFlag == true)   // Email : confirm door now closed after PANIC event
       {
          SendNotificationEmail(Subject3);  // 
       }

        SentPanicEmailFlag = false;       // Clear the One time only Email gate 
        UpdateThingSpeak(2);             // Update ThingSpeak NOW to report Door now closed 

    }


    // Reset
    DoorIsOpen = false ;
  }


  if (loop_count %20 == 0)
  {
        InitScreen();         //  Tell them what we are 
  }
  
  
  // Every 10 mins or so update ThinkSpeak

   if (loop_count >  ThingSpeakUpdateTime * 60 ) {    // connect to wi-fi and ThingSpeak

      UpdateThingSpeak(3);

      loop_count = 0;                 // Start counting till next thinkspeak


  }
  


  if (loop_count % 2 == 0)
      digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)   
  else
      digitalWrite(LED_BUILTIN, LOW);   // turn the LED off (LOW is the voltage level)   


  CheckGmailAwake();
   
  display.display();
  delay(500);
  loop_count++;
}




//
// ====================================================
//   SUB-ROUTINES BELOW HERE
// ====================================================
//


String WebSignon(){
//
//   Message used by web server to report various tempratures etc
//   also used as the body of any notification emails
//
String buff;

    buff = "Hi! This is Freezer Monitor.\n";
    if (LIVE_CODE == 0)
    {
        buff = buff + "Testbed version";
    }
    buff = buff + "\n\nThe current Freezer Temperature : " + String(temperatureC);
    buff = buff + "\n\nTodays Max/Min temps : " + String(MaxtemperatureC)  + " ~ " + String(MintemperatureC);
    buff = buff + "\n\nLong term Max/Min temps : " + String(LongMaxtemperatureC) + " ~ " + String(LongMintemperatureC);
    buff = buff + "\n\n\nFor online update ";
    buff = buff + "\nhttp://" +  WiFi.localIP().toString() + "/update";
    buff = buff + "\n\nView long term history at ThingSpeak";
    buff = buff + "\nhttps://thingspeak.com/channels/" + String(SECRET_CH_ID) + "/private_show";
    buff = buff + "\n\n\nDays running = " + DaysRunning;
    
    return String(buff);
}





void CheckGmailAwake()
{

  if ( DayofYear > ThisDay )        //   These things only happen once per day 
  {
 
        SendNotificationEmail(Subject2);

        DaysRunning += 1;
        
        ResetMaxMin();

        ThisDay = DayofYear;

        

  }
  
}



void SendNotificationEmail(char *whichSubject)
{
    char tmp[350];

    WebSignon().toCharArray(tmp,sizeof(tmp));     // Use the server signon msg as the body of the email
 
    SendGmail(ToAddress, whichSubject , tmp );

}





void InitScreen()
{
  
    if (DoorIsOpen == true) 
    {
       return;              // more important messages to display than this
    }
    display.clearDisplay();
    if (LIVE_CODE ==1 ) {
        ScreenPrint("FREEZER", 5, 0, 2 );
    }
    else
    {
    ScreenPrint("Freezer", 5, 0, 2 );    
    }
    ScreenPrint("Monitor", 5, 20, 2 );
    ScreenPrint("By Chris Bartley", 5, 38, 1 );
    ScreenPrint(WiFi.localIP().toString(),5,50,1);
    display.display();
    delay(500);
}




//   ===========================================
//
//   Temperature Measurment  + Max/Min handling
//
//   ===========================================





void ResetMaxMin()
//
//   Force the ReadTemprature() code to reset the daily Max and Min temp variables
//
{
    first_temp_read = true;
}



void  ReadTemperature() {
  // Get the temperature
  //Serial.print("Read Temp = ");

  sensors.requestTemperatures();
  temperatureC = sensors.getTempCByIndex(0);


  if (first_temp_read == true)
  {
      MaxtemperatureC = temperatureC;
      MintemperatureC = temperatureC;
      first_temp_read = false;
  }
  else
  {
    if (temperatureC > MaxtemperatureC )
      MaxtemperatureC = temperatureC;

    if (temperatureC < MintemperatureC )
      MintemperatureC = temperatureC;
  }   

    if (temperatureC > LongMaxtemperatureC )
      LongMaxtemperatureC = temperatureC;

    if (temperatureC < LongMintemperatureC )
      LongMintemperatureC = temperatureC;

     

//  Serial.print(temperatureC);
//  Serial.print(" degC  Max : ");
//  Serial.print(MaxtemperatureC);
//  Serial.print("  - Min : ");
//  Serial.println(MintemperatureC);

  
}




//   =======================================
//
//   Sending information to the OLED display 
//
//   =======================================



void DisplayTime() {

  int x;

  if (loop_count % 50 == 0 )
  {
      printLocalTime();       // Get the NTP time into 'timeinfo' structure
      Serial.println("Getting NTP");
    
  }
  strftime(buffer, 80, "%H:%M  %a %b %e", &timeinfo);
  ScreenPrint(buffer, 5, 45, 1 );

  //  TEMP - report the wi-fi strength
  ScreenPrint(String(WiFi.RSSI()) , 5, 55, 1 );
  
  
  
  
  DayofYear = timeinfo.tm_yday;           // make a note of the Day - so we can detect Midnight to do once a night stuff

  
  x = timeinfo.tm_isdst;
//  Serial.print("dst : ");
//  Serial.println(x);

}



void DisplayTemperature() {

  // Output it to the display
  ScreenPrint(String(temperatureC), 5, 10, 3 );
  
  ScreenPrint("C", 115, 0, 2);
  switch (loop_count % 4) {             //  Display a rotating line to prove system is active
    case 0:
      ScreenPrint("-", 115, 25, 2);
      break;
    case 1:
      ScreenPrint("\\", 115, 25, 2);
      break;
    case 2:
      ScreenPrint("|", 115, 25, 2);
      break;
    case 3:
      ScreenPrint("/", 115, 25, 2);
      break;
  }

  //ScreenPrint(String(MaxtemperatureC) ,0, 50, 2 );
  //ScreenPrint(String(MintemperatureC) ,70, 50, 2 );


}



//   =============================================
//
//   Wi-Fi connection and ThingSpeak handling code
//
//   =============================================




void _ConnectToWiFi() {

  // Connect or reconnect to WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(SECRET_SSID);
    while (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid, pass);  // Connect to WPA/WPA2 network. Change this line if using open or WEP network
      Serial.print(".");
      delay(5000);
    }
    Serial.println("\nConnected.");
  }

}



void ConnectToWiFi() {
long wifi_wait;

  // Connect or reconnect to WiFi

  wifi_wait = 0;
  
  if (WiFi.status() != WL_CONNECTED) {
    
    
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(SECRET_SSID);
    
    while (WiFi.status() != WL_CONNECTED) {

      display.clearDisplay();
      ScreenPrint("Wi-Fi", 5, 0, 2 );
      ScreenPrint("Connecting", 5, 20, 2 );
      ScreenPrint(String(wifi_wait), 5, 50, 2 );
      display.display();

      WiFi.mode(WIFI_STA);        // from https://rntlab.com/question/wifi-connection-drops-auto-reconnect/
      WiFi.disconnect();
      
      delay(100);
      
      WiFi.mode(WIFI_STA);        // from https://rntlab.com/question/wifi-connection-drops-auto-reconnect/

      WiFi.begin(ssid, pass);  // Connect to WPA/WPA2 network. Change this line if using open or WEP network 
      Serial.print(".");
      delay(35000);
      wifi_wait +=1;

      if (wifi_wait > 10 )
      {
        EEPROM.write( 0 ,  EEPROM_REBOOTED_CODE) ;
        EEPROM.commit();  
          
        ESP.restart();        
      }
    }
    
    Serial.println("\nConnected.");

  }
}







void    UpdateThingSpeak(int caller) {

  // Comunicate with ThingSpeak servers
  // Send Field 1 - just an incremneting counter
  // Send Field 2 - The Freezer Temp in deg C
  // Send Field 3 - A Door Open (0) / Closed flag (100) 
  // Send Field 4 - The Max Temprature
  // Send Field 5 - The Min temprature
  // Send Status  - Door state in text form

  
  int sizeofdelay;      // 15 sec delay padding
  
  Serial.print("Update ThingSpeak=");
  Serial.println(caller);

  Serial.print("Loop_Count=");
  Serial.println(loop_count);
  
  // Update display to redisplay Temp & indication that we are updating ThingSpeak
  //
  display.clearDisplay();
  DisplayTemperature();
  ScreenPrint("Updating ThingSpeak", 5, 50, 1);
  display.display();

  // Double check we have a wi-fi connection
  //
  ConnectToWiFi();          // If we are not already then re-connect to wi-fi

    
  // ThingSpeak can only received updates as a mas of every 15 seconds
  // There is a chance with the Door Open/Close that we might report to quickly for ThinkSpeak
  // So check when last update was sent - 
  // Check if last update was less than 15 seconds ago - if so wait a variable num of seconds < 15
  //
  currMillis = millis();          // note current timestamp
  //Serial.println(currMillis);
  //Serial.println((LastThinkSpeakUpdateMillis + (15  * 1000)));
  
  // See if this update is with the 15 sec window of the last update otherwise ThinkSpeak will choke
  if (currMillis < (LastThinkSpeakUpdateMillis + (15  * 1000))) 
  {
      //  Force a delay size to pad it out so that the update is not inside 15 second window
      //
      sizeofdelay = (currMillis - LastThinkSpeakUpdateMillis) / 1000;      // in seconds
  //    Serial.println(sizeofdelay);
      sizeofdelay = 16 - sizeofdelay;  // we need a delay to pad it up to 15 seconds from last update
  //    Serial.println(sizeofdelay);
      
      Serial.print("Sizeofdelay=");
      Serial.println(sizeofdelay);

      if (sizeofdelay > 20)           // CRASH AFTER  n days FIX ? - ensure delay can never be way too big 
          sizeofdelay = 20;
  
      delay(sizeofdelay * 1000);      // ok - so now we do the padding delay
  }    

 
  
  // set the fields with the values
  //
  ThingSpeak.setField(1, number1);                      // the Incrementing counter
  ThingSpeak.setField(2, String(temperatureC));         // the  Temperature falue

  ThingSpeak.setField(4, String(MaxtemperatureC));      // the Max Temperature falue
  ThingSpeak.setField(5, String(MintemperatureC));      // the Min Temperature falue

  
  Serial.print("door status=");
  Serial.println(DoorStatus);
  
  switch (DoorStatus)                        // and the position of the door  open/closed
  {
    case DOOR_OPEN:
      // set the status
      ThingSpeak.setStatus("DoorOpen !");
      ThingSpeak.setField(3, 100);
      break;
    case DOOR_CLOSED:
      ThingSpeak.setStatus("DoorClosed ! - Wifi-Sig:" + String(WiFi.RSSI()));
      ThingSpeak.setField(3, 1);
      break;
  }

  // write to the ThingSpeak channel
  int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
  Serial.println("Write Channel\n");
  if (x == 200) {
    Serial.print(number1);
    Serial.println(" - Channel update successful.");
  }
  else {
    Serial.println("Problem updating channel. HTTP error code " + String(x));
    SendNotificationEmail(Subject4);  // 
  }

  //  bump the incrementing count number
  number1++;
  if (number1 > 99) {
    number1 = 0;
  }

 LastThinkSpeakUpdateMillis = millis();       // record timestamp of the latest update



 Serial.println("-------");


}






void FlashLed()
{
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(500);                        // wait for a bit
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
}
