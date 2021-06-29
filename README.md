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

There is another version of the code which uses the multi tasking features of the RTOS on the ESP32  - to use a seperate task to update ThingSpeak, and another to run the buzzer
