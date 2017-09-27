/************************************************************************/
/*                                                                      */
/*                      Debouncing 8 Keys				*/
/*			Sampling 4 Times				*/
/*                                                                      */
/*              Author: Peter Dannegger                                 */
/*                                                                      */
/************************************************************************/

#include <util\atomic.h>		// need "--std=c99"
#include <avr/io.h>


typedef unsigned char	u8;
typedef signed short	s16;

#define	XTAL		16e6		// 16MHz

#define KEY_PIN		PINB
#define KEY_PORT	PORTB
#define KEY_DDR		DDRB
#define KEY0		0
#define KEY1		1
#define KEY2		2
#define KEY3		3
#define KEY4		4
#define KEY5		5
#define KEY6		6
#define KEY7		7

#define LED_DDR		DDRD
#define LED_PORT	PORTD
#define LED0		0
#define LED1		1
#define LED2		2
#define LED3		3
#define LED4		4
#define LED5		5
#define LED6		6
#define LED7		7


u8 key_state;				// debounced and inverted key state:
					// bit = 1: key pressed
u8 key_press;				// key press detect

u8 frequency_led_state = 0b00000000; //these leds are turned on sequentially on portd0 1 2 3 4 5 7 8


ISR( TIMER0_COMPA_vect )		// every 10ms
{
  static u8 ct0 = 0xFF, ct1 = 0xFF;	// 8 * 2bit counters
  u8 i;

  i = ~KEY_PIN;				// read keys (low active)
  i ^= key_state;			// key changed ?
  ct0 = ~( ct0 & i );			// reset or count ct0
  ct1 = ct0 ^ (ct1 & i);		// reset or count ct1
  i &= ct0 & ct1;			// count until roll over ?
  key_state ^= i;			// then toggle debounced state
  key_press |= key_state & i;		// 0->1: key press detect
}


u8 get_key_press( u8 key_mask )
{
  ATOMIC_BLOCK(ATOMIC_FORCEON){		// read and clear atomic !
    key_mask &= key_press;		// read key(s)
    key_press ^= key_mask;		// clear key(s)
  }
  return key_mask;
}

void zero_output(){
	
	LED_PORT = 0B00000000;
	frequency_led_state = LED_PORT;
	
}

void set_timer1_2300(){
	TCCR1B |= (1 << CS10); // set prescaler to 0
	TCCR1B |= (1 << WGM12); //put timer 1 in ctc mode a mode where the top is defined in register OCR1A
	TCCR1A |= (1 <<COM1A0); // turn on bits in compare match to toggle.
	OCR1A = 3478; // 2300 Hz
	
	
}

void set_timer1_40000(){
	TCCR1B |= (1 << CS10); // set prescaler to 0
	TCCR1B |= (1 << WGM12); //put timer 1 in ctc mode a mode where the top is defined in register OCR1A
	TCCR1A |= (1 <<COM1A0); // turn on bits in compare match to toggle.
	OCR1A = 200; // 40000 Hz
}

void set_timer1_39215(){
	TCCR1B |= (1 << CS10); // set prescaler to 0
	TCCR1B |= (1 << WGM12); //put timer 1 in ctc mode a mode where the top is defined in register OCR1A
	TCCR1A |= (1 <<COM1A0); // turn on bits in compare match to toggle.
	OCR1A = 204; //39215.5
}

void set_timer1_1000000(){
	TCCR1B |= (1 << CS10); // set prescaler to 0
	TCCR1B |= (1 << WGM12); //put timer 1 in ctc mode a mode where the top is defined in register OCR1A
	TCCR1A |= (1 <<COM1A0); // turn on bits in compare match to toggle.
	OCR1A = 8; //1000000
	
}

void set_timer2_3012(){
	TCCR2B |= (1 << CS21) |(1 << CS20); // set prescaler to 32
	TCCR2A |= (1 << WGM21); //put timer 2 in ctc mode a mode where the top is defined in register OCR2A
	TCCR2A |= (1 <<COM2A0); // turn on bits in compare match to toggle.
	OCR2A = 83; //set value to trigger compare match. this determines the frequency
}

void set_timer2_40000(){
	TCCR2B |= (1 << CS21); // set prescaler to 8
	TCCR2A |= (1 << WGM21); //put timer 2 in ctc mode a mode where the top is defined in register OCR2A
	TCCR2A |= (1 <<COM2A0); // turn on bits in compare match to toggle.
	OCR2A = 24; //set value to trigger compare match. this determines the frequency
}

void set_timer2_1000000(){
	TCCR2B |= (1 << CS20); // set prescaler to 0
	TCCR2A |= (1 << WGM21); //put timer 2 in ctc mode a mode where the top is defined in register OCR2A
	TCCR2A |= (1 <<COM2A0); // turn on bits in compare match to toggle.
	OCR2A = 8; //set value to trigger compare match. this determines the frequency
	
}

void advance_output(){
	  if(frequency_led_state==0b00000000){ //both are at 40kHz
		LED_PORT = 0B00000001;
		frequency_led_state = LED_PORT;
		set_timer1_40000();
		set_timer2_40000();
		}
		
      else if(frequency_led_state==0b00000001){ //40kHz + -700 offset
		LED_PORT = 0B00000010;
		frequency_led_state = LED_PORT;
		set_timer1_39215();
		set_timer2_40000();
		}

	 else if(frequency_led_state==0b00000010){ //3kHz + -700 offset
		LED_PORT = 0B00000100;
		frequency_led_state = LED_PORT;
		set_timer1_2300();
		set_timer2_3012();
		
		}

	   else if(frequency_led_state==0b00000100){ //both at 1 MHz
		LED_PORT = 0B00001000;
		frequency_led_state = LED_PORT;
		set_timer1_1000000();
		set_timer2_1000000();
		
		}

	 else if(frequency_led_state==0b00001000){
		LED_PORT = 0B00010000;
		frequency_led_state = LED_PORT;
		}
	 else if(frequency_led_state==0b00010000){
		LED_PORT = 0B00100000;
		frequency_led_state = LED_PORT;
		}
	else if(frequency_led_state==0b00100000){
		LED_PORT = 0B01000000;
		frequency_led_state = LED_PORT;
		}


	 else if(frequency_led_state==0b01000000){
		LED_PORT = 0B10000000;
		frequency_led_state = LED_PORT;
	    }	
		
	else if(frequency_led_state==0b10000000){ //this is is the overflow value > change the frequency_led_state  value here to overflow at a different value
		LED_PORT = 0B00000001;
		frequency_led_state = LED_PORT;
		}	
}

void reverse_output(){
     	  if(frequency_led_state==0b00000000){
		LED_PORT = 0B10000000;
		frequency_led_state = LED_PORT;
		
		}
		
      else if(frequency_led_state==0b10000000){
		LED_PORT = 0B01000000;
		frequency_led_state = LED_PORT;
		}
	 

     else if(frequency_led_state==0b01000000){
	    LED_PORT = 0B00100000;
		frequency_led_state = LED_PORT;
		}

	   else if(frequency_led_state==0b00100000){
		LED_PORT = 0B00010000;
		frequency_led_state = LED_PORT;
		}

	 else if(frequency_led_state==0b00010000){ //both at 1 MHz
		LED_PORT = 0B00001000;
		frequency_led_state = LED_PORT;
		set_timer1_1000000();
		set_timer2_1000000();
		
		}
		
	 else if(frequency_led_state==0b00001000){ //3kHz + -700 offset
		LED_PORT = 0B00000100;
		frequency_led_state = LED_PORT;
		set_timer1_2300();
		set_timer2_3012();		
		}
		
	 else if(frequency_led_state==0b00000100){ //40kHz + -700 offset
		LED_PORT = 0B00000010;
		frequency_led_state = LED_PORT;
		set_timer1_39215();
		set_timer2_40000();		
		}		
	
	else if(frequency_led_state==0b00000010){ //both at 40kHz
		LED_PORT = 0B00000001;
		frequency_led_state = LED_PORT;
		set_timer1_40000();
		set_timer2_40000();
		}	

	else if(frequency_led_state==0b00000001){ //this is is the overflow value > change the frequency_led_state  value here to overflow at a different value
		LED_PORT = 0B10000000;
		frequency_led_state = LED_PORT;
		}
}







int main( void )
{
  TCCR0A = 1<<WGM01;			// T0 Mode 2: CTC
  TCCR0B = 1<<CS02^1<<CS00;		// divide by 1024
  OCR0A = XTAL / 1024.0 * 10e-3 -1;	// 10ms
  TIMSK0 = 1<<OCIE0A;			// enable T0 interrupt

  //KEY_DDR = 0;				// input
  //SET INPUTS
 // KEY_DDR |= (1<<PB0) | (1<<PB1);	
   KEY_DDR |= (0<<PB0) | (0<<PB2) | (0<<PB7);

  KEY_PORT = 0xFF;			// pullups on
  LED_PORT = 0x00;			// LEDs off (low active)
  
	//LED_DDR = 0xFF;			// LED output
    //SET OUTPUTS
  LED_DDR |= (1<<PD0) | (1<<PD1) | (1<<PD2) | (1<<PD3) | (1<<PD4) | (1 <<PD5) | (1<<PD6) | (1<<PD7);
  KEY_DDR |= (1<<PB1) | (1<<PB3); //set pb1 and pb3 outputs for oc1a and oc2a

   
    
key_state = ~KEY_PIN;			// no action on keypress during reset
  sei();




  for(;;){					// main loop
   
	 if( get_key_press( 1<<KEY0 ))
	
		{
	
			advance_output();

		}


    if( get_key_press( 1<<KEY1 ))
		
		{
			
		}

    if( get_key_press( 1<<KEY2 ))

		{
			zero_output();
		}


/*
    if( get_key_press( 1<<KEY3 ))

		{
			LED_PORT ^= 1<<LED3;

		}
*/

    if( get_key_press( 1<<KEY4 ))

    {

	    reverse_output();
	
		}

/*    if( get_key_press( 1<<KEY5 ))
      LED_PORT ^= 1<<LED5;

    if( get_key_press( 1<<KEY6 ))
      LED_PORT ^= 1<<LED6;

    if( get_key_press( 1<<KEY7 ))
      LED_PORT ^= 1<<LED7;
*/
  }
}
