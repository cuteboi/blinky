// bool, true, false
#include <stdbool.h>
#include <inttypes.h>


// Power savings Watchdog/sleep macros
#include <avr/wdt.h>
#include <avr/sleep.h>
#include <avr/power.h>

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

#define PIN_RELAY PINA1
#define PIN_14V PINA0


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

ISR(TIM0_OVF_vect){
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
bool is_relay_latched = false;
char str[24];
bool is_power_on = false;

// WatchDogTimer event, must be defined
ISR(WDT_vect){}

void setup(){
  init_timer();

  // TUrn off everything, except the clock
  PRR = 0b1011;
  // Check if this is a reset caused by Watchdog
  if(MCUSR & _BV(WDRF)){            // If a reset was caused by the Watchdog Timer...
    MCUSR &= ~_BV(WDRF);                 // Clear the WDT reset flag
    WDTCSR |= (_BV(WDCE) | _BV(WDE));   // Enable the WD Change Bit
    WDTCSR = 0x00;                      // Disable the WDT
  }

  // Set input pin
  DDR_PORT &= _BV(PIN_14V);

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

#define send_debug(x) send_str(" " #x ": "); itoa(x,str,10); send_str(str);

void loop(){
  is_power_on = INPUT_PORT & _BV(PIN_14V);
  uint64_t current_millis = millis();

  if (MCUCR & _BV(SE)){    // If Sleep is Enabled...
    cli();                 // Disable Interrupts
    sleep_bod_disable();   // Disable BOD
    sei();                 // Enable Interrupts
    if(!is_relay_latched && !is_power_on){
    #ifdef DEBUG
      send_str("Going to sleep, nothing to do ");
      send_debug(current_millis);
      send_str("\r\n");
    #endif
      sleep_cpu();           // Go to Sleep
      return;
    }
  }
  // Sensor resolution
  _delay_ms(500);

#ifdef DEBUG
  send_str("DEBUG:");
  send_debug(prepare_shutdown);
  send_debug(start_millis);
  send_debug(is_relay_latched);
  send_debug(is_power_on);
  send_debug(current_millis);
  send_str("\r\n");
#endif
  if( is_power_on && !is_relay_latched){
    #ifdef DEBUG
    send_str("\r\n12V is seen - Turning on relay\r\n");
    #endif
    // Turn on relay
    is_relay_latched = true;
    DATA_PORT |= _BV(PIN_RELAY);
  }


  if(is_relay_latched && !start_millis && !is_power_on){
    start_millis = millis();
    prepare_shutdown = true;
#ifdef DEBUG
    send_str("12V is gone - ");
    send_str("Countdown Started at ");
    send_debug(start_millis);
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

  if(!is_power_on && !is_relay_latched){
    // Power turning off
    is_relay_latched = false;
  }

  if(is_relay_latched && prepare_shutdown){
#ifdef DEBUG
    send_str("DEBUG:");
    send_str(" start_millis + millis_to_turn_off_relay: ");
    itoa(((start_millis + millis_to_turn_off_relay)-current_millis)/1000, str, 10);
    send_str(str);

    send_debug(current_millis);
    send_str("\r\n");
#endif
    if(current_millis >= start_millis + millis_to_turn_off_relay){
#ifdef DEBUG
      send_str("\r\nTime met, shutting off relay\r\n");
#endif
      DATA_PORT &= ~_BV(PIN_RELAY);
      is_relay_latched = false;
      prepare_shutdown = false;
      start_millis = 0;
    };
  }
}


int main (void){
  // Enable Interrupts
  setup();

  // loop
  for(;;) {
    loop();
  }

  return 1;
}
