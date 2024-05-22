/*
 * HPY411_Lab_4.c
 * HPY411 - Lab 4 code in C for ATmega16
 * Created: 3/11/2020 6:44:20 μμ
 * Author : Evangelos Kioulos 2016030056
 */ 

/************************************************************************************************
 *	This program receives USART commands formed with ASCII characters,
 *	through USART serial port. After each command, the microprocessor
 *	transmits OK<CR><LF>.
 *
 *	Commands:
 *
 *	AT<CR><LF>: microprocessor responds with OK<CR><LF>.
 *
 *	C<CR><LF>: stores character 0x0A in all allocated RAM space.
 *
 *	N<X><CR><LF>: stores character 0x0A in all allocated RAM space and
 *	then stores <X> characters in allocated RAM space and displays them
 *	in a 7-segment display.
 *
 *	Main program and initialization is in C. Interrupt service routines are in Assembly
 *
 ************************************************************************************************/

#include <avr/io.h>
#include <avr/interrupt.h>

#define FOSC 10000000	// clock speed at 10MHz
#define BAUD 9600		// baud rate
#define MYUBRR FOSC/16/BAUD-1	// register ubrr value

/* Function Declaration */
void USART_init(unsigned int ubrr);
void TIMER0_init(void);
void clear_RAM(void);
void USART_Transmit(unsigned char data);
void send_OK(void);
/* Assembly routines */
extern void ISR_timer(void);
extern void ISR_uart(void);

int main(void){
	
	/*memory location of register that points to the digit shown*/
	unsigned char *digitShown = (unsigned char*) 0x68;
	*digitShown = (unsigned char)0x00;
	
	/* Initialize USART */
	USART_init(MYUBRR);

	/* Insert char 0x0A in allocated Memory */
	clear_RAM();
	
	// Set Port A and Port C as outputs
	DDRA = 0xFF;
	DDRC = 0xFF;
	
	/* Initialize Timer/Counter0 */
	TIMER0_init();
	
	sei();	// Global Interrupt Enable
	
	while(1){
		asm("nop");	/* No Operation */
	}
}

/**************************************************************
 *
 *	Function implementations and Interrupt Service Routines
 *
 **************************************************************/

/*
 *	Initialize USART:
 *
 *	USART frame format is in Asynchronous mode with no parity, 
 *	1 stop bit and 8-bit character size.
 *	Reciever and transmiter are enabled.
 *	RXC and TXC interrupts are enabled.
 */

void USART_init(unsigned int ubrr){

	// init baud rate
	UBRRH = (unsigned char)(ubrr >> 8);
	UBRRL = (unsigned char)ubrr;

	// Enable reciever and transmiter, enable RXC interrupt 
	UCSRB = (1 << RXCIE)|(1 << RXEN)|(1 << TXEN);

	// Set USART frame format
	UCSRC = (1 << UCSZ1)|(1 <<UCSZ0);

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
 * This function fills address 0x0060 to 0x0067 of RAM with char 0x0A
 */
void clear_RAM(void){
	unsigned char *ram_pos = (unsigned char*)0x60;
	int i;
	for(i=0; i<8; i++){
		ram_pos[i] = 0x0A;
	}
}

/*
 *	Usart Transmit
 *
 *	Wait until UDRE bit in register UCSRA is off, and then transmit
 *	data from register r16.
 */

void USART_Transmit( unsigned char data ){
	//Wait for empty transmit buffer
	while ( !( UCSRA & (1<<UDRE)) )
	;
	//Put data into buffer, sends the data 
	//UDR = data;
	TCNT2 = data;
}

/*
 *	This function transmits chars "OK"<CR><LF>
 *	It is called in the assembly ISR_uart routine
 */
void send_OK(){
	USART_Transmit(0x4F);	// send "O"
	USART_Transmit(0x4B);	// send "K"
	USART_Transmit(0x0D);	// send <CR>
	USART_Transmit(0x0A);	// send <LF>
}

/*
 * Interrupt Service Routine for TIMER0 Overflow interrupt.
 * Calls TIMER0 Overflow assembly routine
 * TIMER0_OVF_vect is the interrupt vector for TIMER0 Overflow interrupt.
 */
ISR(TIMER0_OVF_vect){
	ISR_timer();
}

/*
 * Interrupt Service Routine for RXC interrupt.
 * Calls USART RXC interrupt assembly routine
 * USART_RXC_vect is the interrupt vector for RXC interrupt.
 */
ISR(USART_RXC_vect){
	ISR_uart();
}