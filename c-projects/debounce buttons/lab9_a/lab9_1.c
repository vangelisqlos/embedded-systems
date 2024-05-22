/*
 * HPY411_Lab_9.c
 * HPY 411 Lab 9 code in C for ATmega16
 * Polling solution
 * Created: 12/12/2020 6:52:09 μμ
 * Author : Evangelos Kioulos 2016030056
 */ 

/******************************************************************************************
 *
 * This program implements SPDT switch debouncing using Polling.
 * 
 * Pin A of the switch is plugged in PA0 and pin A' the switch is 
 * plugged in PA1 of PORTA.
 *
 * Pin PA2 of PORTA is the output. Input PA0 defines the output.
 *
 * 8-bit TIMER/COUNTER0 is used for sampling the input every 1 msec.
 * Timer/Counter is set in Normal mode with prescaler fclk/64 and starting value 100.
 * TCNT0 Overflow interrupt is enabled.
 *
 * New output becomes set 10 msec after the input starts bouncing.
 *
 * The program uses 3 bytes of ram.
 * In address 0x0060: Current state of the switch is stored. If value is '0' the switch
 *                    is bouncing. If value is '1' the switch is set.
 * In address 0x0061: The previous set value of the switch is stored.
 * In address 0x0062: Value of counter used for counting 10 msec is stored.
 *
 ******************************************************************************************/

#include <avr/io.h>
#include <avr/interrupt.h>

/* Function declaration */
void TIMER0_init(void);

/* States */
#define STATE_BOUNCING 0
#define STATE_SET 1

int main(void)
{
	// Current state address
	unsigned char *curr_state = (unsigned char *) 0x60;
	*curr_state = (unsigned char)STATE_SET;
	// Previous stable value of the switch address
	unsigned char *prev_value = (unsigned char *) 0x61;
	*prev_value = (unsigned char)0x00;
	
	// Set Pin 2 of PORTA as an output.
	DDRA = (1 << PA2);
	
	// Initialize Timer/Counter0
	TIMER0_init();
	
	sei(); // Global Interrupt Enable
	
    while (1) 
    {
		asm("nop"); // No operation. Not necessary. used for debugging purposes 
    }
}

/*
 *	Initialize TIMER/COUNTER 0
 *
 *	TCNT0 Starts counting from 100. 
 *	TCNT0 Overflow interrupt is enabled
 *	Prescaler is set in Fclk/64
 */
void TIMER0_init(void){

	/*
	 * Initialize TCNT0 to 256-2*78=100. This is because 256-78=156
	 * overflows TCNT0 in 5000 cycles and we need 10000 cycles.
	 */
	TCNT0 = 100;

	/* 
	 * insert 0x00000001 in register r16.
	 * Makes Value of bit TOIE0 '1' which enables Timer/Counter 0
	 * Overflow interrupt
	 */
	TIMSK = (1 << TOIE0);

	/*
	 * insert 0x00000011 in register TCCR0
	 * Sets prescaler fclk/64
	 */
	TCCR0 = (1 << CS01)|(1 << CS00);
}


/*
 * Interrupt Service Routine for TCNT0 Overflow.
 * This interrupt occurs every 1 msec.
 * If val of A = previous value and A' != previous value the switch is set. else the switch is bouncing.
 * 10 msec after the switch starts bouncing the new output is set.
 */
ISR(TIMER0_OVF_vect){
	
	unsigned char *curr_state = (unsigned char *) 0x60;
	unsigned char *prev_value = (unsigned char *) 0x61;
	unsigned char *counter = (unsigned char *) 0x62;
	unsigned char alpha = 0, alpha_bar = 0;
	alpha = (PINA >> PINA0) & 0x01; // Value of bit PA0 corresponds to A
	alpha_bar = (PINA >> PINA1) & 0x01; // Value of bit PA1 corresponds to A'
	
	switch(*curr_state){
		case STATE_SET:
			// If state is set and the new value of A = prev value of A and A' != prev value of A,
			// The switch is set. Else it is bouncing.
			if((alpha == *prev_value) && (alpha_bar != *prev_value)){
				*curr_state = STATE_SET;
			}
			else{
				*curr_state = STATE_BOUNCING;
			}
			break;
		case STATE_BOUNCING:
			// If the state is bouncing start counting 10 msecs.
			// After 10 msec output new value reset counter and go to state set.
			if(*counter < 10){
				*counter = *counter + 1;
			}
			else{
				*prev_value = alpha;
				PORTA = (alpha << PA2);
				*counter = 0;
				*curr_state = STATE_SET;
			}
			break;
	}

	TCNT0 = 100;
}
