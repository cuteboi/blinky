//
// Serials communications (TX only) with ATtiny84
//
// electronut.in
//
//#define DEBUG

// bool, true, false
#include <stdbool.h>
#include <inttypes.h>


// Power savings Watchdog/sleep macros
#include <avr/wdt.h>
#include <avr/sleep.h>

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#ifdef DEBUG // Serial output
#include <stdlib.h>
#include "BasicSerial.h"
#endif

#define DATA_PORT PORTA
#define DDR_PORT  DDRA
#define INPUT_PORT PINA

#define PIN_RELAY PINA0
#define PIN_12VDETECT PINA1


void init_timer(){
  // set up timer0:
  /* interrup setup */
  // prescale timer0 to 1/8th the clock rate
  // overflow timer0 every 0.256 ms
  TCCR0B |= _BV(CS01);
  // enable timer overflow interrupt
  TIMSK0  |= _BV(TOIE0);
}

uint64_t _millis = 0;
uint16_t _1000us = 0;
uint64_t old_millis = 0;

ISR(TIM0_OVF_vect)
{
  _1000us += 256;
  while (_1000us > 1000) {
    _millis++;
    _1000us -= 1000;
  }
}

uint64_t millis() {
  uint64_t m;
  cli();
  m = _millis;
  sei();
  return m;
}

#ifdef DEBUG
// write null terminated string
void send_str(const char* str)
{
  while(*str) TxByte(*str++);
}
#endif


bool prepare_shutdown = false;
int millis_to_turn_off_relay = 5000;
unsigned long start_millis = 0;
bool is_12v_enabled = false;
char str[24];


// WatchDogTimer event, must be defined
ISR(WDT_vect){}

void setup(){
  init_timer();
  // Check if this is a reset caused by Watchdog
  if(MCUSR & _BV(WDRF)){            // If a reset was caused by the Watchdog Timer...
    MCUSR &= ~_BV(WDRF);                 // Clear the WDT reset flag
    WDTCSR |= (_BV(WDCE) | _BV(WDE));   // Enable the WD Change Bit
    WDTCSR = 0x00;                      // Disable the WDT
  }

  // Set input pin
  DDR_PORT &= _BV(PIN_12VDETECT);

  // Set up Watch Dog Timer for Inactivity
  cli();
  WDTCSR |= (_BV(WDCE) | _BV(WDE));   // Enable the WD Change Bit
  WDTCSR =   _BV(WDIE) |              // Enable WDT Interrupt
             _BV(WDP2) | _BV(WDP1);   // Set Timeout to ~0.5 seconds
    // Enable Sleep Mode for Power Down
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);    // Set Sleep Mode: Power Down
  sleep_enable();                     // Enable Sleep Mode
  sei();
}

void loop(){
  bool is_power_on = INPUT_PORT & _BV(PIN_12VDETECT);
  uint64_t currentMillis = millis();

  if (MCUCR & _BV(SE)){    // If Sleep is Enabled...
    cli();                 // Disable Interrupts
    sleep_bod_disable();   // Disable BOD
    sei();                 // Enable Interrupts
    if(!is_12v_enabled && !is_power_on){
      #ifdef DEBUG
      itoa(currentMillis, str, 10);
      send_str("Going to sleep, nothing to do ");
      send_str(str);
      send_str("\r\n");
      #endif
      sleep_cpu();           // Go to Sleep
      return;
    }

/****************************
 *   Sleep Until WDT Times Out
 *   -> Go to WDT ISR
 ****************************/

  }
  // Sensor resolution
  _delay_ms(100);

  #ifdef DEBUG
    send_str("DEBUG:");

    send_str(" prepare_shutdown: ");
    itoa(prepare_shutdown, str, 10);
    send_str(str);

    send_str(" start_millis: ");
    itoa(start_millis, str, 10);
    send_str(str);

    send_str(" is_12v_enabled: ");
    itoa(is_12v_enabled, str, 10);
    send_str(str);

    send_str(" is_power_on: ");
    itoa(is_power_on, str, 10);
    send_str(str);

    send_str(" millis() ");
    itoa(currentMillis, str, 10);
    send_str(str);

    send_str("\r\n");
  #endif
  if( is_power_on && !is_12v_enabled){
    #ifdef DEBUG
    send_str("12V is seen - Turning on relay\r\n");
    #endif
    // Turn on relay
    is_12v_enabled = true;
    DATA_PORT |= _BV(PIN_RELAY);
  }


  if(is_12v_enabled && !start_millis && !is_power_on){
    start_millis = millis();
    prepare_shutdown = true;
#ifdef DEBUG
    send_str("12V is gone - ");
    send_str("Countdown Started at ");
    itoa(start_millis, str, 10);
    send_str(str);
    send_str("\r\n");
#endif

  }

  if(is_power_on && prepare_shutdown){
    // Power recovered and we were about to shut down
    prepare_shutdown = false;
    start_millis = 0;
#ifdef DEBUG
    send_str("\r\nCancelling powerdown\r\n");
#endif
  }

  if(!is_power_on && !is_12v_enabled){
    // Power turning off
    is_12v_enabled = false;
  }

  if(is_12v_enabled && prepare_shutdown){

#ifdef DEBUG
    send_str("DEBUG:");
    send_str(" start_millis + millis_to_turn_off_relay: ");
    itoa(((start_millis + millis_to_turn_off_relay)-currentMillis)/1000, str, 10);
    send_str(str);
    send_str(" millis(): ");
    itoa(currentMillis, str, 10);
    send_str(str);
    send_str("\r");
#endif

    if(currentMillis >= start_millis + millis_to_turn_off_relay){

#ifdef DEBUG
      send_str("\r\nTime met, shutting off relay\r\n");
#endif

      DATA_PORT &= ~_BV(PIN_RELAY);
      is_12v_enabled = false;
      prepare_shutdown = false;
      start_millis = 0;
    };
  }
}


int main (void)
{
  // Enable Interrupts
  setup();

  // loop
  for(;;) {
    loop();
  }

  return 1;
}
