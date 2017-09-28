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

/*

Hooking up the wires:

LEDs:
Arduino digital pins 0-6 are the outputs of the LEDs
Arduino digital pin 7 has 3 modes - off - on - and toggle --it is an output indicator

Switches:
Arduino digital 12 is the "reverse LED" switch
Arduino digital 10 disables all LEDs
Arduino digital 8 is the "advance LED" switch
Arduino digital 13 is the "toggle output on digital pin 9" switch It doesn't affect digital pin 11


Outputs:
Arduino digital 9 (portb.1) is the output of timer1
Arduino digital 11 (portb.3) is the output of timer2


Configuring Buttons:
The buttons use a basic state machine type configuration to advance and reverse LEDs. 
Functions can be called for each state, configured independently, though probably ideally symmetrically
in advance_output() and reverse_output()
in rewrite the state machine needs to be fixed to ignore multiple on/off presses if an advance/reverse hasn't 
occurred. Right now I'm doing that with a flag.
Also in rewrite to decouple the led output from the frequency output
Also in rewrite resuming after turning off the output should output the same frequency. It does that here
but it's ugly
this is major speghetti code


Feature: 
Can go forward, back, and disable LEDs. pressing one switch advances the lit LED. Pressing the other 
switch results in the previous LED being lit. Pressing the final switch disables all LEDs.
Goal is to be able to use three switches and a bank of up to 8 LEDs to control (via functions) and indicate (via LEDs) the 
output/output state


*/




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
u8 output_enable = 0; //0 means off - 1 means continuous wave enabled - 2 means pulse wave enabled


volatile long toggle_interval =0;



u8 get_key_press( u8 key_mask )
{
  ATOMIC_BLOCK(ATOMIC_FORCEON){		// read and clear atomic !
    key_mask &= key_press;		// read key(s)
    key_press ^= key_mask;		// clear key(s)
  }
  return key_mask;
}



void clear_timer1(){
	LED_PORT = 0B00000000;
	frequency_led_state = LED_PORT;
	TCCR1B = 0x00;
	TCCR1A = 0x00;
	
}

void clear_timer1_and_timer2(){
    //clear all the registers
	TCCR1B = 0b00000000;
	TCCR1A = 0b00000000;
	TCCR2B = 0b00000000;
	TCCR2A = 0b00000000;
	
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

void set_timer1_8000000(){
	TCCR1B |= (1 << CS10); // set prescaler to 0
	TCCR1B |= (1 << WGM12); //put timer 1 in ctc mode a mode where the top is defined in register OCR1A
	TCCR1A |= (1 <<COM1A0); // turn on bits in compare match to toggle.
	OCR1A = 0; //8000000
	
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
	OCR2A = 20; //set value to trigger compare match. this determines the frequency
}

void set_timer2_1000000(){
	TCCR2B |= (1 << CS20); // set prescaler to 0
	TCCR2A |= (1 << WGM21); //put timer 2 in ctc mode a mode where the top is defined in register OCR2A
	TCCR2A |= (1 <<COM2A0); // turn on bits in compare match to toggle.
	OCR2A = 8; //set value to trigger compare match. this determines the frequency
	
}

void set_timer2_8000000(){
	TCCR2B |= (1 << CS20); // set prescaler to 0
	TCCR2A |= (1 << WGM21); //put timer 2 in ctc mode a mode where the top is defined in register OCR2A
	TCCR2A |= (1 <<COM2A0); // turn on bits in compare match to toggle.
	OCR2A = 0; //set value to trigger compare match. this determines the frequency
	
}



void advance_output(){

clear_timer1_and_timer2();
 if(output_enable ==1){
		
	
	switch(frequency_led_state){
		case 0b00000000:
			LED_PORT = 0B00000001;
			frequency_led_state = LED_PORT;
			set_timer1_40000();
			set_timer2_40000();
			break;
		
		
        case 0b00000001: //40kHz + -700 offset
			LED_PORT = 0B00000010;
			frequency_led_state = LED_PORT;
			set_timer1_39215();
			set_timer2_40000();
			break;

	    case 0b00000010: //3kHz + -700 offset
			LED_PORT = 0B00000100;
			frequency_led_state = LED_PORT;
			set_timer1_2300();
			set_timer2_3012();
			break;
		

	    case 0b00000100: //both at 1 MHz
			LED_PORT = 0B00001000;
			frequency_led_state = LED_PORT;
			set_timer1_1000000();
			set_timer2_1000000();
			break;
		

	    case 0b00001000:
			LED_PORT = 0B00010000;
			frequency_led_state = LED_PORT;
			  set_timer1_8000000();
			  set_timer2_8000000();
			  
			
			break;
		
	    case 0b00010000:
			LED_PORT = 0B00100000;
			frequency_led_state = LED_PORT;
			break;
		
	    case 0b00100000:
			LED_PORT = 0B01000000;
			frequency_led_state = LED_PORT;
			break;


	    
		
	    case 0b01000000: //this is is the overflow value > change the frequency_led_state  value here to overflow at a different value
			LED_PORT = 0B00000001;
			frequency_led_state = LED_PORT;
			break;
}
}
}

void reverse_output(){
  
  if(output_enable ==1){

	clear_timer1_and_timer2();
	switch(frequency_led_state){
     
		
        case 0b00000000:
			LED_PORT = 0B01000000;
			frequency_led_state = LED_PORT;
			break;
	 

        case 0b01000000:
			LED_PORT = 0B00100000;
			frequency_led_state = LED_PORT;
			break;

	    case 0b00100000:
			LED_PORT = 0B00010000;
			frequency_led_state = LED_PORT;
			set_timer1_8000000();
			set_timer2_8000000();
			  
			break;

	    case 0b00010000: //both at 1 MHz
			LED_PORT = 0B00001000;
			frequency_led_state = LED_PORT;
			set_timer1_1000000();
			set_timer2_1000000();
			break;
		
	    case 0b00001000: //3kHz + -700 offset
			LED_PORT = 0B00000100;
			frequency_led_state = LED_PORT;
			set_timer1_2300();
			set_timer2_3012();		
			break;
		
	    case 0b00000100: //40kHz + -700 offset
			LED_PORT = 0B00000010;
			frequency_led_state = LED_PORT;
			set_timer1_39215();
			set_timer2_40000();		
			break;		
	
	    case 0b00000010: //both at 40kHz
			LED_PORT = 0B00000001;
			frequency_led_state = LED_PORT;
			set_timer1_40000();
			set_timer2_40000();
			break;

	    case 0b00000001: //this is is the overflow value > change the frequency_led_state  value here to overflow at a different value
			LED_PORT = 0B01000000;
			frequency_led_state = LED_PORT;
			break;
}
}
}

void toggle_output(){
	
	if (toggle_interval >=500){
	   set_timer1_39215(); //this is the output frequency
	   LED_PORT = 0B10000000; //this is specific to the frequency set above - referenced in advance
			
	}

	if (toggle_interval >=1000){
	   clear_timer1();
	   toggle_interval = 0;
	  LED_PORT = 0B00000000;
	  
	}
}


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
  
  
	//this is patched in here badly maybe
	//anyway - when enable_output is 2 this will call a function that toggles the output using the toggle_output function
	if (output_enable ==2){
		toggle_interval += 10;
		toggle_output();
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
   KEY_DDR |= (0<<PB0) | (0<<PB2) | (0<<PB4) |(0<<PB5);

  KEY_PORT = 0xFF;			// pullups on
  LED_PORT = 0x00;			// LEDs off (low active)
  
	//LED_DDR = 0xFF;			// LED output
    //SET OUTPUTS
  LED_DDR |= (1<<PD0) | (1<<PD1) | (1<<PD2) | (1<<PD3) | (1<<PD4) | (1 <<PD5) | (1<<PD6) | (1<<PD7);
  KEY_DDR |= (1<<PB1) | (1<<PB3); //set pb1 and pb3 outputs for oc1a and oc2a

   
    
key_state = ~KEY_PIN;			// no action on keypress during reset
  sei();




  for(;;){					// main loop
	
	//OR and AND arduino pin 7 with the LED output - this keeps led pin 7 lit-OFF-or toggling depending on output_enable's state
	
	if(output_enable ==1){
	LED_PORT |= 0B10000000;
	}
	if(output_enable == 0){
	LED_PORT &= 0B00000000;
	}
	
	
	 if( get_key_press( 1<<KEY0 ))

	
		{
	
			advance_output();

		}


    if( get_key_press( 1<<KEY1 ))
		
		{
			
		}

    if( get_key_press( 1<<KEY2 ))

		{
			switch (output_enable){
				case 0:
						output_enable =1;
					
					//LED_PORT = 0B10000000;
			       // frequency_led_state = LED_PORT;
				
					break;
				case 1:
					clear_timer1_and_timer2();
					output_enable=0;	
					
					//resets output to zero
					LED_PORT = 0B00000000;
			        frequency_led_state = LED_PORT;
				
					break;
					
				case 2:
					clear_timer1_and_timer2();
					output_enable =0;
					
					break;
					
		}
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

   if( get_key_press( 1<<KEY5 )){
      output_enable = 2;
	 
	 
	  
   }
/*
    if( get_key_press( 1<<KEY6 ))
      LED_PORT ^= 1<<LED6;

    if( get_key_press( 1<<KEY7 ))
      LED_PORT ^= 1<<LED7;
*/
  }
}
