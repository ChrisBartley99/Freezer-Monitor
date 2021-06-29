// Use this file to store all of the private credentials 
// and connection details

#define SECRET_SSID "your_wifi ssid"        	       // replace MySSID with your WiFi network name
#define SECRET_PASS "your wifi password"               // replace MyPassword with your WiFi password

//=======================
//
//  Control whether creating Test or Live code
//
//  use '#define LIVE_CODE 1' in main module    1 = make live   0 = test code
//
#if (LIVE_CODE==1)
  // Live ThingSpeak channel 
  #define SECRET_CH_ID nnnnnn                      // replace nnnnnn with your channel number
  #define SECRET_WRITE_APIKEY "XYZ"   // replace XYZ with your channel write API Key
#else
  // TestBed ThingSpeak channel 
  #define SECRET_CH_ID nnnnnnn                     // replace nnnnn with your channel number
  #define SECRET_WRITE_APIKEY "XYZ"   // replace XYZ with your channel write API Key
#endif
//===========================


// Google GMAIL
#define  EMAIL_SENDER_ACCOUNT    "your_email_address@gmail.com"
#define  EMAIL_SENDER_PASSWORD   "your email account password"
#define  SMTPSERVER              "smtp.gmail.com"
#define  SMTP_PORT                465
#define  EMAILRECEIPIENT         "your chosen reciepient@gmail.com"
