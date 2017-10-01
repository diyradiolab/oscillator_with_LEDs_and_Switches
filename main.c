/************************************************************************/
/*
arduino digital pins 0-4 - output LEDs
arduino digital pin 6 - toggle/interrupt signal enable - this can be thought of as interrupting the output signal
arduino digital pin 7 - output and continuous wave enabled
pin 8 - output and continuous wave enabled
pin 9 - output1
pin 10 - toggle/interrupt signal enable - this can be thought of as interrupting the output signal
pin 11 - output2
pin 12 - advance output to next state
pin 13 - reverse output to previous state

outputs are square waves at the following frequencies:
selection	timer1	timer2
0			39215	40000
1			40000	40000
2           2300	3012
3			1MHz	1MHz
4			8MHz	8MHz



future improvements
don't use a toggle fuction with a state variable for the on/off button because then other functions that want to turn off the
on/off button end up toggling the button if called more than once
it's just bad practice

                              */
/*                                                                      */
/************************************************************************/

#include <util/atomic.h>		// need "--std=c99"




#define	XTAL		16e6		// 8MHz

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


//for checking state of output


//define
#define FREQ_2300 0
#define FREQ_3012 1
#define FREQ_40000 2
#define FREQ_39125 3
#define FREQ_1000000 4
#define FREQ_8000000 5

#define FREQ_TIMER1_39215_TIMER2_40000 0
#define FREQ_TIMER1_40000_TIMER2_40000 1
#define FREQ_TIMER1_2300_TIMER2_3012 2
#define FREQ_TIMER1_1000000_TIMER2_1000000 3
#define FREQ_TIMER1_8000000_TIMER2_8000000 4


#define FREQ_TIMER1_39215_TIMER2_40000_LED 0
#define FREQ_TIMER1_40000_TIMER2_40000_LED 1
#define FREQ_TIMER1_2300_TIMER2_3012_LED 2
#define FREQ_TIMER1_1000000_TIMER2_1000000_LED 3
#define FREQ_TIMER1_8000000_TIMER2_8000000_LED 4


#define MIN_LED_THAT_IS_FREQUENCY_INDICATOR  0
#define MAX_LED_THAT_IS_FREQUENCY_INDICATOR  4

#define MIN_OUTPUT_FREQUENCY_SELECTION 0
#define MAX_OUTPUT_FREQUENCY_SELECTION 4

#define DISABLE 0
#define ENABLE 1

#define DISABLED 0
#define ENABLED 1




//globals:
uint8_t output_status = 0; //this will be between 0 and 4
uint8_t output_status_LED = 0;	//this variable will hold the current output state of the LED between 0 and 4
int8_t CURRENT_OUTPUT_FREQUENCY_SELECTION = -1; //set as -1 so no frequency is selected when the device is turned on
int8_t CURRENT_OUTPUT_FREQUENCY_SELECTION_LED = -1;
uint8_t KEY3_OR_KEY4_WAS_PRESSED = 0;
uint8_t toggle_mode = DISABLED;
uint8_t toggle_interrupt_passes_counter = 0;
uint8_t toggle_is_in_high_state = 0;

uint8_t TOGGLE_FREQUENCY = 10; //Multiply this value by 20 (10*2) to get the period of the toggle functionality



uint8_t key_state;				// debounced and inverted key state:
// bit = 1: key pressed
uint8_t key_press;				// key press detect


ISR(TIMER0_COMPA_vect)		// every 10ms
{
    static uint8_t ct0 = 0xFF, ct1 = 0xFF;	// 8 * 2bit counters
    uint8_t i;

    i = ~KEY_PIN;				// read keys (low active)
    i ^= key_state;			// key changed ?
    ct0 = ~( ct0 & i );			// reset or count ct0
    ct1 = ct0 ^ (ct1 & i);		// reset or count ct1
    i &= ct0 & ct1;			// count until roll over ?
    key_state ^= i;			// then toggle debounced state
    key_press |= key_state & i;		// 0->1: key press detect

//this is used by the toggle function that's called by pressing KEY2
    if((toggle_interrupt_passes_counter > TOGGLE_FREQUENCY) && (toggle_mode == ENABLED) && (output_status == ENABLED)) {

        //this switch just toggles the  output on/off each time the interrupt timer counter reaches 50
        switch(toggle_is_in_high_state) {
        case 0:
            toggle_is_in_high_state = 1;
            turn_on_timers();
            toggle_interrupt_passes_counter = 0;
            break;

        case 1:
            toggle_is_in_high_state = 0;
            turn_off_continous_wave();
            toggle_interrupt_passes_counter = 0;
            break;
        }

    }

    toggle_interrupt_passes_counter++;
}


uint8_t get_key_press( uint8_t key_mask )
{
    ATOMIC_BLOCK(ATOMIC_FORCEON) {		// read and clear atomic !
        key_mask &= key_press;		// read key(s)
        key_press ^= key_mask;		// clear key(s)
    }
    return key_mask;
}

void initialize_timer0() {
    TCCR0A = 1<<WGM01;			// T0 Mode 2: CTC
    TCCR0B = 1<<CS02^1<<CS00;		// divide by 1024
    OCR0A = XTAL / 1024.0 * 10e-3 -1;	// 10ms
    TIMSK0 = 1<<OCIE0A;			// enable T0 interrupt

}

void initilize_ports_and_leds() {
    KEY_DDR = 0;				// input
    KEY_PORT = 0xFF;			// pullups on
    LED_PORT &= 0B00000000;			// LEDs off (low active)
    LED_DDR = 0xFF;			// LED output
    key_state = ~KEY_PIN;			// no action on keypress during reset
    KEY_DDR |= (1<<PORTB1) | (1<<PORTB3); //set pb1 and pb3 outputs for oc1a and oc2a
}


void clear_timer1_and_timer2() {
    //clear all the registers
    TCCR1B = 0b00000000;
    TCCR1A = 0b00000000;
    TCCR2B = 0b00000000;
    TCCR2A = 0b00000000;
}

void set_power_indicator_led() {
    switch(output_status_LED) {


    case DISABLED:
        PORTD |= (1 << PORTD7); // PD7 goes high
        output_status_LED = ENABLED; //this sets the output to status to enabled
        break;

    case ENABLED:
        PORTD &= ~(1 << PORTD7); // PD7 goes low
        output_status_LED = DISABLED; //this sets the output status to disabled
        break;
    }
}

void set_output_status() {

    switch(output_status) {
    case DISABLED:
        output_status = ENABLED; //this sets the output to status to enabled
        break;
    case ENABLED:

        output_status = DISABLED; //this sets the output status to disabled
        break;
    }

}

void advance_LED() {
    if(CURRENT_OUTPUT_FREQUENCY_SELECTION_LED < MAX_LED_THAT_IS_FREQUENCY_INDICATOR) {
        CURRENT_OUTPUT_FREQUENCY_SELECTION_LED +=1;
    }
    else {
        CURRENT_OUTPUT_FREQUENCY_SELECTION_LED = CURRENT_OUTPUT_FREQUENCY_SELECTION_LED;
    }
}

void reverse_LED() {
    if(CURRENT_OUTPUT_FREQUENCY_SELECTION_LED > MIN_LED_THAT_IS_FREQUENCY_INDICATOR) {
        CURRENT_OUTPUT_FREQUENCY_SELECTION_LED -=1;
    }
    else {
        CURRENT_OUTPUT_FREQUENCY_SELECTION_LED = CURRENT_OUTPUT_FREQUENCY_SELECTION_LED;
    }
}

void set_timer1() {


    const int dividers[] = {204, 200, 3478, 8, 0}; // f = 39215, 40000, 2300, 1000000, 8000000
    TCCR1B |= (1 << CS10); // set prescaler to 0
    TCCR1B |= (1 << WGM12); //put timer 1 in ctc mode a mode where the top is defined in register OCR1A
    TCCR1A |= (1 <<COM1A0); // turn on bits in compare match to toggle.
    OCR1A = dividers[CURRENT_OUTPUT_FREQUENCY_SELECTION]; // correct divider to use
}

void set_timer2() {

    const int dividers[] = {20, 20, 83, 8, 0}; //f = 40000, 40000, 3012, 1000000, 8000000


    if (dividers[CURRENT_OUTPUT_FREQUENCY_SELECTION] == 83) {

        TCCR2B |= (1 << CS21) |(1 << CS20); // set prescaler to 32
    }

    if (dividers[CURRENT_OUTPUT_FREQUENCY_SELECTION] == 20) {

        TCCR2B |= (1 << CS21); // set prescaler to 8
    }
    else {

        TCCR2B |= (1 << CS20); // set prescaler to 0
    }



    TCCR2A |= (1 << WGM21); //put timer 2 in ctc mode a mode where the top is defined in register OCR2A
    TCCR2A |= (1 <<COM2A0); // turn on bits in compare match to toggle.
    OCR2A = dividers[CURRENT_OUTPUT_FREQUENCY_SELECTION]; // correct divider to use
}

void advance_output() {

    if(CURRENT_OUTPUT_FREQUENCY_SELECTION < MAX_OUTPUT_FREQUENCY_SELECTION) {
        CURRENT_OUTPUT_FREQUENCY_SELECTION +=1;

    }

    if(CURRENT_OUTPUT_FREQUENCY_SELECTION == MAX_OUTPUT_FREQUENCY_SELECTION)
        CURRENT_OUTPUT_FREQUENCY_SELECTION = MAX_OUTPUT_FREQUENCY_SELECTION;
}

void reverse_output() {



    if(CURRENT_OUTPUT_FREQUENCY_SELECTION > MIN_OUTPUT_FREQUENCY_SELECTION) {

        CURRENT_OUTPUT_FREQUENCY_SELECTION -=1;
    }
    else {
        CURRENT_OUTPUT_FREQUENCY_SELECTION = CURRENT_OUTPUT_FREQUENCY_SELECTION;
    }
}



void set_frequency_indicator_LED() {

    switch(CURRENT_OUTPUT_FREQUENCY_SELECTION_LED) {
    case FREQ_TIMER1_39215_TIMER2_40000_LED:
        PORTD &= 0B11000000; //turn off all pins but leave PORTD7 (THE POWER INDICATOR) alone
        PORTD |= (1 << PORTD0); //enable PORTD0
        break;

    case FREQ_TIMER1_40000_TIMER2_40000_LED:
        PORTD &= 0B11000000; //turn off all pins but leave PORTD7 (THE POWER INDICATOR) alone
        PORTD |= (1 << PORTD1); //enable PORTD1
        break;

    case FREQ_TIMER1_2300_TIMER2_3012_LED:
        PORTD &= 0B11000000; //turn off all pins but leave PORTD7 (THE POWER INDICATOR) alone
        PORTD |= (1 << PORTD2); //enable PORTD2
        break;

    case FREQ_TIMER1_1000000_TIMER2_1000000_LED:
        PORTD &= 0B11000000; //turn off all pins but leave PORTD7 (THE POWER INDICATOR) alone
        PORTD |= (1 << PORTD3); //enable PORTD3
        break;

    case FREQ_TIMER1_8000000_TIMER2_8000000_LED:
        PORTD &= 0B11000000; //turn off all pins but leave PORTD7 (THE POWER INDICATOR) alone
        PORTD |= (1 << PORTD4); //enable PORTD4
        break;


    }

}

void advance_continous_wave() {

    clear_timer1_and_timer2();
    advance_LED();
    set_frequency_indicator_LED();
    advance_output();
    if(output_status) {
        turn_on_timers();
    }
}

void reverse_continous_wave() {

    clear_timer1_and_timer2();
    reverse_LED();
    set_frequency_indicator_LED();
    reverse_output();

    if(output_status && CURRENT_OUTPUT_FREQUENCY_SELECTION != -1) {
        turn_on_timers();
    }

}

void turn_on_timers() {

    set_timer1();
    set_timer2();
}

void turn_on_continous_wave_without_advancing_or_reversing_it() {
    //this sets timer1 and timer2 without advancing them if KEY3 or KEY4 haven't been pressed


    turn_on_timers();


}

void turn_off_continous_wave() {
    clear_timer1_and_timer2();
}






int main( void)
{

    initialize_timer0();
    initilize_ports_and_leds();
    sei();

    for(;;) {					// main loop







        if(get_key_press( 1<<KEY0)) {
            KEY3_OR_KEY4_WAS_PRESSED = 0;

            set_power_indicator_led();
            set_output_status();

            //this clears the timer1 timer2 if output is disabled to turn off oscillators
            if(output_status == DISABLED && CURRENT_OUTPUT_FREQUENCY_SELECTION != -1) {
                toggle_mode = DISABLED; //turn off toggle mode if it's running


                //turn of wave
                turn_off_continous_wave();
            }

            if(output_status == ENABLED && CURRENT_OUTPUT_FREQUENCY_SELECTION != -1 && KEY3_OR_KEY4_WAS_PRESSED ==0) {

                //turn on wave if output frequency hasn't been changed via key3 or key4

                turn_on_timers();

            }
        }



        /*if( get_key_press( 1<<KEY1 ))
        LED_PORT ^= 1<<LED1;
        */
        if( get_key_press( 1<<KEY2 ))
        {

            if(output_status == ENABLED && (CURRENT_OUTPUT_FREQUENCY_SELECTION !=-1)) {

                switch(toggle_mode) {

                //turn on toggle mode
                case DISABLED:

                    toggle_mode = ENABLED;
                    PORTD |= (1 << PORTD6);
                    break;

                //restore continuous wave function and disable toggle_mode
                case ENABLED:
                    if(output_status != 0) {
                        turn_on_continous_wave_without_advancing_or_reversing_it();
                    }
                    toggle_mode = DISABLED;
                    PORTD &= ~(1 << PORTD6);
                    break;
                }

            }
        }



        if( get_key_press( 1<<KEY3 ))
        {}

        if( get_key_press( 1<<KEY4 )) {
            KEY3_OR_KEY4_WAS_PRESSED =1; //this flag is referenced by key0 when enabled/disables the output to make sure frequency hasn't changed
            advance_continous_wave();

        }



        if( get_key_press( 1<<KEY5 )) {
            KEY3_OR_KEY4_WAS_PRESSED =1;
            reverse_continous_wave();


        }

        if( get_key_press( 1<<KEY6 ))
            LED_PORT ^= 1<<LED6;

        if( get_key_press( 1<<KEY7 ))
            LED_PORT ^= 1<<LED7;
    }
}
