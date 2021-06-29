
//  https://techtutorialsx.com/2017/07/01/esp32-arduino-controlling-a-buzzer-with-pwm/
//  https://forum.arduino.cc/index.php?topic=583513.15

#include "pitches.h"

#define BUZZER_PIN          13

#ifdef    ARDUINO_ARCH_ESP32

#define SOUND_PWM_CHANNEL   0
#define SOUND_RESOLUTION    8 // 8 bit resolution
#define SOUND_ON            (1<<(SOUND_RESOLUTION-1)) // 50% duty cycle
#define SOUND_OFF           0                         // 0% duty cycle

 
// notes in the melody:
int melody[] = {
  NOTE_C5, NOTE_D5, NOTE_E5, NOTE_F5, NOTE_G5, NOTE_A5, NOTE_B5, NOTE_C6};
int duration = 500;  // 500 miliseconds
 

 void ESPtone(int pin, int frequency, int duration)
{
  ledcSetup(SOUND_PWM_CHANNEL, frequency, SOUND_RESOLUTION);  // Set up PWM channel
  ledcAttachPin(pin, SOUND_PWM_CHANNEL);                      // Attach channel to pin
  ledcWrite(SOUND_PWM_CHANNEL, SOUND_ON);
  delay(duration);
  ledcWrite(SOUND_PWM_CHANNEL, SOUND_OFF);
}

void sound_long_warning()
{
    ESPtone(BUZZER_PIN, NOTE_B5, duration * 2);
  
}


void sound_short_warning()
{
    ESPtone(BUZZER_PIN, NOTE_C5, duration / 10);
  
}


void sound_tune()
{
        ESPtone(BUZZER_PIN, NOTE_C5, duration);
        delay(500);
        ESPtone(BUZZER_PIN, NOTE_B5, duration);
        delay(200);
        ESPtone(BUZZER_PIN, NOTE_A5, duration);
        delay(100);
}


void scale()
{
    for (int thisNote = 0; thisNote < 8; thisNote++) {
    // pin n output the voice, every scale is 0.5 sencond
    ESPtone(BUZZER_PIN, melody[thisNote], duration);
     
    // Output the voice after several minutes
    delay(1000);
  }
   
  // restart after two seconds 
  delay(2000);

}





#endif
