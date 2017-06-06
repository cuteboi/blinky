#include <BasicSerial.h>
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

void serOut(const char* str) {
   while (*str) TxByte (*str++);
}

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

#define LOW 0
#define HIGH 1

class Flasher {
	// Class Member Variables
	// These are initialized at startup
	int ledPin;      // the number of the LED pin
	unsigned long OnTime;     // milliseconds of on-time
	unsigned long OffTime;    // milliseconds of off-time

  const char * msg_ON = "Turning on LED \r\n";
  const char * msg_OFF = "Turning off LED \r\n";
	// These maintain the current state
	int ledState;             		// ledState used to set the LED
	unsigned long previousMillis;  	// will store last time LED was updated

  // Constructor - creates a Flasher
  // and initializes the member variables and state
  public:
  Flasher(int pin, long on, long off)
  {
	ledPin = pin;
	//pinMode(ledPin, OUTPUT);
  DDRB |= _BV(ledPin);

	OnTime = on;
	OffTime = off;

	ledState = LOW;
	previousMillis = 0;
  }

  void Update()
  {
    // check to see if it's time to change the state of the LED
    unsigned long currentMillis = millis();

    if((ledState == HIGH) && (currentMillis - previousMillis >= OnTime))
    {
    	ledState = LOW;  // Turn it off
      previousMillis = currentMillis;  // Remember the time
      serOut(msg_OFF);
      PORTB &= ~_BV(ledPin);
      //digitalWrite(ledPin, ledState);  // Update the actual LED
    }
    else if ((ledState == LOW) && (currentMillis - previousMillis >= OffTime))
    {
      ledState = HIGH;  // turn it on
      previousMillis = currentMillis;   // Remember the time
      //digitalWrite(ledPin, ledState);	  // Update the actual LED
      serOut(msg_ON);
      PORTB |= _BV(ledPin);
    }
  }
};





Flasher led1(LEDPIN4, 3000, 5000);

void setup(){
  serOut("Starting Setup\r\n");
  // Pin Change MaSK
  PCMSK |= (1 << INTERRUPTPIN2); // Button or 12v detect
  // Port B Data Direction
  DDRB &= ~(1 << DATADIRECTIONPIN2); //cbi(DDRB, DATADIRECTIONPIN);//  set up as input  - pin2 clear bit  - set to zero
  // Port B Data Register
  PORTB |= (1<< DETECTPIN2); //cbi(PORTB, PORTPIN);// disable pull-up. hook up pulldown resistor. - set to zero


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
int millis_to_turn_off_relay = 10000;
unsigned long start_millis = 0;
bool is_12v_enabled = false;
char str[16];


void loop(){
  unsigned long currentMillis = millis();
  bool is_power_on = PINB & _BV(INTERRUPTPIN2);

  led1.Update();
  _delay_ms(100);
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



  if( is_power_on && !is_12v_enabled){
    serOut("12V is seen - Turning on relay\r\n");
    // Turn on relay
    is_relay_enabled = true;
    is_12v_enabled = true;
  }


  if(is_12v_enabled && !start_millis && !is_power_on){
    serOut("12V is gone - ");
    if(!start_millis){
      serOut("Countdown Started at ");
      start_millis = millis();
      itoa(start_millis, str, 10);
      serOut(str);
      serOut("\r\n");
      prepare_shutdown = true;
    }
  }

  if(is_power_on && prepare_shutdown){
    // Power recovered and we were about to shut down
    serOut("Cancelling powerdown");
    prepare_shutdown = false;
    start_millis = 0;

  }

  if(!is_power_on && !is_12v_enabled){
    // Power turning off
    is_12v_enabled = false;
  }

  if(is_relay_enabled && prepare_shutdown){
    serOut("DEBUG:");

    serOut(" start_millis + millis_to_turn_off_relay: ");
    itoa(start_millis + millis_to_turn_off_relay, str, 10);
    serOut(str);

    serOut(" millis(): ");
    itoa(currentMillis, str, 10);
    serOut(str);

    serOut("\r\n");

    if(currentMillis >= start_millis + millis_to_turn_off_relay){
      serOut("Time met, shutting off relay\r\n");
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
