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

void serOut(const char* str);

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

const char * msg_ON = "Turning on LED \r\n";
const char * msg_OFF = "Turning off LED \r\n";

#define LOW 0
#define HIGH 1

class Flasher {
	// Class Member Variables
	// These are initialized at startup
	int ledPin;      // the number of the LED pin
	unsigned long OnTime;     // milliseconds of on-time
	unsigned long OffTime;    // milliseconds of off-time

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
  DDRB |= _BV(LEDPIN4);

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
  // Disable Interrupts
  cli();
  // Enable General Interrupt MasK
  GIMSK |= (1 << PCIE);   // enable PCINT interrupt in the general interrupt mask //SBI
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

  // Enable global interrupts
  sei();

}

int count = 54;

ISR(PCINT_VECTOR2){
  serOut("INTERRUPT RUN");
  char buffer [24];
  itoa(count,buffer,10);
  serOut(buffer);
  if(PINB & _BV(INTERRUPTPIN2)){
    serOut("12V is seen");
  }

  serOut("This is a test\r\n");
}

void serOut(const char* str) {
   while (*str) TxByte (*str++);
}

void loop(){

  led1.Update();
  _delay_ms(300);
  serOut("Running through update\r\n");;
  if(PINB & _BV(INTERRUPTPIN2)){
    serOut("12V is seen");
  }

}

int main(void){
  setup();
  while(1){
    loop();
  }
}
