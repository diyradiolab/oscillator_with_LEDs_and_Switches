/************************************************************************/
/*                                                                      */
/*                      Debouncing 8 Keys				*/
/*			Sampling 4 Times				*/
/*                                                                      */
/*              Author: Peter Dannegger                                 */
/*                                                                      */
/************************************************************************/

#include <util\atomic.h>		// need "--std=c99"


typedef unsigned char	u8;
typedef signed short	s16;

#define	XTAL		8e6		// 8MHz

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

void timer_setup(int f1, int f2){

	}

void enable_ocr1a(){

	}

void enable_ocr2a(){

	}

void advance_output(){
	  if(frequency_led_state==0b00000000){
		LED_PORT = 0B00000001;
		frequency_led_state = LED_PORT;
		
		}
		
      else if(frequency_led_state==0b00000001){
		LED_PORT = 0B00000010;
		frequency_led_state = LED_PORT;
		}

	 else if(frequency_led_state==0b00000010){
		LED_PORT = 0B00000100;
		frequency_led_state = LED_PORT;
		}

	   else if(frequency_led_state==0b00000100){
		LED_PORT = 0B00001000;
		frequency_led_state = LED_PORT;
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
		LED_PORT = 0B1000000;
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
		LED_PORT = 0B00100000;
		frequency_led_state = LED_PORT;
		}


	   else if(frequency_led_state==0b00100000){
		LED_PORT = 0B00010000;
		frequency_led_state = LED_PORT;
		}

	 else if(frequency_led_state==0b00010000){
		LED_PORT = 0B00001000;
		frequency_led_state = LED_PORT;
		}
	 else if(frequency_led_state==0b00001000){
		LED_PORT = 0B00000100;
		frequency_led_state = LED_PORT;
		}
	 else if(frequency_led_state==0b00000100){
		LED_PORT = 0B00000010;
		frequency_led_state = LED_PORT;
		}		
	
	else if(frequency_led_state==0b00000010){ //this is is the overflow value > change the frequency_led_state  value here to overflow at a different value
		LED_PORT = 0B00000001;
		frequency_led_state = LED_PORT;
		}	

	else if(frequency_led_state==0b00000001){ //this is is the overflow value > change the frequency_led_state  value here to overflow at a different value
		LED_PORT = 0B10000000;
		frequency_led_state = LED_PORT;
		}
}

void zero_output(){
		
		LED_PORT = 0B00000000;
		frequency_led_state = LED_PORT;
	
	}

void set_timers(){

if(frequency_led_state==0b00000000){
	//disable timer1 and timer2 output compare register bits
	}


if(frequency_led_state==0b00000001){
	
	//setup timer0 for 3000 hz
	//setup timer1 for 2300 hz	
	timer_setup(3000, 2300); //make function to calculate appropriate values for timer0 and timer1 and set registers
	enable_ocr1a();
	enable_ocr2a();
	}

if(frequency_led_state==0b00000010){
	//setup timer1 for 39300 hz
	//setup timer0 for 40000 hz
	//enable ocr bits

	}


if(frequency_led_state==0b00000100){
	//setup timer1 for 999300 hz
	//setup timer0 for 1000000 hz
	//enable ocr bits

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
  KEY_DDR |= (1<<PB0) | (1<<PB1);	

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
