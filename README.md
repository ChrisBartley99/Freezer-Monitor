# Freezer-Monitor
An ESP32 based Freezer Monitor

Started as a simple project to learn ESP32 
Originally intended just to alert me if the door was left open , but suffered a bit of mission creep as I experimented with ESP32 features

Sends Email if Door is left open too long
Now also includes 
An OLED display to display internal temp of freezer
Reports data to a ThingSpeak account - 3rd party widget on phone can monitor ThingSpeak channel and alert if temp out of range
Code is updatable via OTA 

This version of the code doesn't use any timers or interuppts, just performs various actions after a set number of times round the loop

This is getting its IP address via DHCP - which does seem to cause some problems for the ESP32 if it loses the signal it can have the devils own job trying to reconnect - so have resorted to re-booting the whole thing in such cases
Since been informed that its more if you use fixed IP addressing - still testing that 

To switch to Fixed IP addressing
Add these variables
===================
// Set your Static IP address

IPAddress local_IP(192, 168, x, xxx);

// Set your Gateway IP address

IPAddress gateway(192, 168, x, 1);

IPAddress subnet(255, 255, 1, 0);

IPAddress primaryDNS(8, 8, 8, 8); // optional   - NOTE : These are NOT optional!

IPAddress secondaryDNS(8, 8, 4, 4); // optional - NOTE : These are NOT optional !



Add a variation of this code at about line 175 (before the connect to Wi-Fi) to switch to fixed IP addressing

======================

#if (FIXED_IP == 1)

  // Set up for FIXED IP address

  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
  
    Serial.println("STA Failed to configure");
    
  }
  
#endif  
=====================

There is another version of the code which I was experimenting with the multi tasking features of the RTOS on the ESP32 
This uses a seperate task to update ThingSpeak, and another to run the buzzer
But I ran into various crashng problems mainly due to overrunning the stack and a couple of other odd errors 
