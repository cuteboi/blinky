//#define DEBUG

#ifdef DEBUG
  #include <BasicSerial.h>
#endif
#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <inttypes.h>
#include <avr/interrupt.h>

// change LEDPIN based on your schematic
#define LEDPIN4  PINB4
// #define #define UART_Tx 3 // PINB3

#define RELAYPIN1 PINB1

#define SHUTDOWNPIN0 PINB0

#define PCINT_VECTOR2 PCINT0_vect  //this step is not necessary
#define DETECTPIN2  PINB2
#define INTERRUPTPIN2 PCINT2
#define DATADIRECTIONPIN2 DDB2

#ifdef DEBUG
  void serOut(const char* str) {
     while (*str) TxByte (*str++);
  }
#endif

// TIMER Functions
/* some vars */
uint64_t _millis = 0;
uint16_t _1000us = 0;
uint64_t old_millis = 0;

/* interrupts routines */
// timer overflow occur every 0.256 ms
ISR(TIM0_OVF_vect) {
  _1000us += 256;
  while (_1000us > 1000) {
    _millis++;
    _1000us -= 1000;
  }
}

// safe access to millis counter
uint64_t millis() {
  uint64_t m;
  cli();
  m = _millis;
  sei();
  return m;
}

void setup(){
#ifdef DEBUG
  serOut("Starting Setup\r\n");
#endif

  DDRB |= _BV(LEDPIN4); // Enable output
  /* interrup setup */
  // prescale timer0 to 1/8th the clock rate
  // overflow timer0 every 0.256 ms
  TCCR0B |= (1<<CS01);
  // enable timer overflow interrupt
  TIMSK  |= 1<<TOIE0;
  sei();
}

int count = 54;

bool is_relay_enabled = false;
bool prepare_shutdown = false;
const int millis_to_turn_off_relay = 5000;
unsigned long start_millis = 0;
bool is_12v_enabled = false;

#ifdef DEBUG
char str[16];
#endif

void loop(){
  unsigned long currentMillis = millis();
  bool is_power_on = PINB & _BV(INTERRUPTPIN2);
  _delay_ms(100);

#ifdef DEBUG
  serOut("DEBUG:");

  serOut(" is_relay_enabled: ");
  itoa(is_relay_enabled, str, 10);
  serOut(str);

  serOut(" prepare_shutdown: ");
  itoa(prepare_shutdown, str, 10);
  serOut(str);

  serOut(" start_millis: ");
  itoa(start_millis, str, 10);
  serOut(str);

  serOut(" is_12v_enabled: ");
  itoa(is_12v_enabled, str, 10);
  serOut(str);

  serOut(" is_power_on: ");
  itoa(is_power_on, str, 10);
  serOut(str);

  serOut(" millis() ");
  itoa(currentMillis, str, 10);
  serOut(str);

  serOut("\r\n");
#endif

  if(is_power_on){
    if(!is_12v_enabled){
#ifdef DEBUG
    serOut("12V is seen - Turning on relay\r\n");
#endif
      // Turn on relay
      is_relay_enabled = true;
      is_12v_enabled = true;
      PORTB |= _BV(LEDPIN4);
      // LEDPIN4 relay location
    }

    if(prepare_shutdown){
      // Power recovered and we were about to shut down
      prepare_shutdown = false;
      start_millis = 0;
  #ifdef DEBUG
      serOut("\r\nCancelling powerdown\r\n");
  #endif
    }
  }

  if(is_12v_enabled && !start_millis && !is_power_on){
    start_millis = millis();
    prepare_shutdown = true;
#ifdef DEBUG
    serOut("12V is gone - ");
    serOut("Countdown Started at ");
    itoa(start_millis, str, 10);
    serOut(str);
    serOut("\r\n");
#endif

  }

  if(is_relay_enabled && prepare_shutdown){

#ifdef DEBUG
    serOut("DEBUG:");
    serOut(" start_millis + millis_to_turn_off_relay: ");
    itoa(((start_millis + millis_to_turn_off_relay)-currentMillis)/1000, str, 10);
    serOut(str);
    serOut(" millis(): ");
    itoa(currentMillis, str, 10);
    serOut(str);
    serOut("\r");
#endif

    if(currentMillis >= start_millis + millis_to_turn_off_relay){

#ifdef DEBUG
      serOut("\r\nTime met, shutting off relay\r\n");
#endif

      PORTB &= ~_BV(LEDPIN4);
      is_relay_enabled = false;
      is_12v_enabled = false;
      prepare_shutdown = false;
      start_millis = 0;
    };
  }

}

int main(void){
  setup();
  while(1){
    loop();
  }
}
