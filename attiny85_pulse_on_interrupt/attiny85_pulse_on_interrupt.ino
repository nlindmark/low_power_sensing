
// ATMEL ATTINY 25/45/85 / ARDUINO
//
//                  +-\/-+
// Ain0 (D 5) PB5  1|    |8  Vcc
// Ain3 (D 3) PB3  2|    |7  PB2 (D 2) Ain1
// Ain2 (D 4) PB4  3|    |6  PB1 (D 1) pwm1
//            GND  4|    |5  PB0 (D 0) pwm0
//                  +----+

// React on PIR activation and output a reset pulse to the ESP8266
// if (and only if) the ESP8266 is in sleep


#include <avr/sleep.h>    // Sleep Modes
#include <avr/power.h>    // Power management

#define NOT_ALIVE 1
#define PIR_ACTIVATED HIGH
#define PULSE_WIDTH 50  // Length in ms
#define ESP8266_BOOT_TIME 500

const byte PULSE_PIN = PB0;     // pin 5 - This is the output pulse to reset the ESP8266
const byte ALIVE_PIN = PB1;     // pin 6 - This is the input pin that signals if the ESP8266 is alive
const byte PIR_PIN   = PB4;     // pin 3 (PCINT4) - This is the PIR input pin

ISR (PCINT0_vect) // Note: this is the ISR for all pin change interrupt (0-5)
 {
 // do something interesting here
 }
 
void setup (){

  pinMode(PULSE_PIN, OUTPUT);
  pinMode(ALIVE_PIN, INPUT);
  pinMode(PIR_PIN,   INPUT);

  digitalWrite(PULSE_PIN, HIGH);  // Inital state
  digitalWrite(ALIVE_PIN, HIGH);  // internal pull-up
  
  // pin change interrupt (example for PB4)
  PCMSK  |= bit (PCINT4);  // want pin PB4 / pin 3
  GIFR   |= bit (PCIF);    // clear any outstanding interrupts
  GIMSK  |= bit (PCIE);    // enable pin change interrupts 
  
}  // end of setup

void loop (){

  delay(10); // Just to make sure pins are stable
  
  // We have been woken up from deep sleep
  // Check if it is due to PIR activated, if so wake the ESP8266
  if(digitalRead(PIR_PIN) == PIR_ACTIVATED){


    // ESP8266 Boot from deepsleep sometimes causes false PIR triggering so
    // I want to wait until ESP8266 is booted until I check the alive pin
    delay(ESP8266_BOOT_TIME);  
     
    // If the ESP8266 is not alive we have to wake it up
    if(digitalRead(ALIVE_PIN) == NOT_ALIVE){
      digitalWrite(PULSE_PIN, LOW);
      delay(PULSE_WIDTH); 
      digitalWrite(PULSE_PIN, HIGH); 
    }
  }

  // Wait until ESP has gone to sleep. I had trouble with ESP powerups and downs disturbing the PIR
  while((digitalRead(ALIVE_PIN) != NOT_ALIVE)){
      delay(500);
  }    
  delay(2000);
  goToSleep();
}  // end of loop
  
  
void goToSleep(){
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  ADCSRA = 0;            // turn off ADC
  power_all_disable ();  // power off ADC, Timer 0 and 1, serial interface
  sleep_enable();
  sleep_cpu();                             
  sleep_disable();   
  power_all_enable();    // power everything back on
}  // end of goToSleep 
