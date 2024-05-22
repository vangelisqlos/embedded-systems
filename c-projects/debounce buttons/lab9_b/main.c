/*
 * HPY411_Lab_9_b.c
 * HPY 411 Lab 9 code in C for ATmega16
 * External Interrupt solution
 * Created: 12/12/2020 7:53:17 μμ
 * Author : Evangelos Kioulos 2016030056
 */ 

/************************************************************************************
 *
 * This program implements SPDT switch debouncing using External Interrupt 0
 * and External Interrupt 1.  
 * 
 * Pin A of the switch is plugged in PD2(INT0) and pin A' the switch is 
 * plugged in PD3(INT1).
 *
 * Pin PA2 of PORTA is the output.
 *
 * Both interrupts are sensitive to rising edge.
 *
 * When INT0 occurs PA2 is '1'.
 * When INT1 occurs PA2 is '0'.
 *
 ************************************************************************************/

#include <avr/io.h>
#include <avr/interrupt.h>

/* Function Declaration */
void ExternalIntertupt_init(void);

int main(void)
{	
	DDRA = (1 << PA2); // Sets pin 2 of Port A as an output
	
	// Initialize External interrupts
	ExternalIntertupt_init();
	
	sei(); // Global Interrupt enable
	
	while (1)
	{
		asm("nop"); // No operation. Non necessary, used only for debugging purposes
	}
}


/*
 * External Interrupt Initialization.
 * INT0 and INT1 are enabled.
 * INT0 and INT1 are sensitive to rising edge.
 */
void ExternalIntertupt_init(void){
	
	// Sets INT0 and INT1 as rising edge sensitive
	MCUCR = (1 << ISC11)|(1 << ISC10)|(1 << ISC01)|(1 << ISC00);
	
	// Enables INT1 and INT0 request
	GICR = (1 << INT1)|(1 << INT0);
}


/*
 * Interrupt service routine for INT0.
 * This interrupt triggers when the value of PD2 changes
 * from '0' to '1'. This means that pin PD2 is bouncing.
 * Makes output PA2 '1'.
 */
ISR(INT0_vect){
	PORTA = (1 << PA2);	// Pin 2 of Port A becomes '1'
}

/*
 * Interrupt service routine for INT1.
 * This interrupt triggers when the value of PD3 changes
 * from '0' to '1'. This means that pin PD3 is bouncing.
 * Makes output PA2 '0'.
 */
ISR(INT1_vect){
	PORTA = (0 << PA2); // Pin 2 of Port A becomes '0'
}