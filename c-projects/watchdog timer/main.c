/*
 * HPY411_Lab_6.c
 * Lab 6 code in C for ATmega16.
 * Clock speed at 10Mhz
 * Created: 20/11/2020 7:07:23 μμ
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
 *	Main program, initialization and interrupt service routines are in C.
 *
 *	The program uses Watchdog timer with a prescaler of 32K cycles. 
 *	The WDT gets reset every time we get an input from usart.
 *	In case we receive a wrong instruction, WDT cold starts the program after 32K cycles.
 *	When USART is not used WDT times out after 32K cycles and cold starts the program.
 *
 ************************************************************************************************/

#include <avr/io.h>
#include <avr/interrupt.h>

#define FOSC 10000000	// clock speed at 10MHz
#define BAUD 9600		// baud rate
#define MYUBRR FOSC/16/BAUD-1	// register ubrr value

/* FSM States */
#define STATE_IDLE 0
#define STATE_A 1
#define STATE_T 2
#define STATE_CLEAR 3
#define STATE_STORE 4
#define STATE_END 5

/* Function Declaration */
void USART_init(unsigned int ubrr);
void TIMER0_init(void);
void clear_RAM(void);
void shift_RAM(void);
void USART_Transmit(unsigned char data);
void send_OK(void);
void init_7_seg(void);
void WDT_init(void);

int main(void){
	
	/* memory location of register that points to the digit shown */
	unsigned char *digitShown = (unsigned char*) 0x68;
	*digitShown = (unsigned char)0x00; // initial value is 0x00
	/* memory location of ring counter value */
	unsigned char *ring_counter = (unsigned char*) 0x69;
	*ring_counter = (unsigned char) 0x00; // initial value is 0x00
	/* memory location of current state */
	unsigned char *curr_state = (unsigned char *)0x6A;
	*curr_state = STATE_IDLE; //initial state is IDLE
	
	/* Insert char 0x0A in allocated Memory (0x0060 - 0x0067) */
	clear_RAM();
	
	/* Initialize USART */
	USART_init(MYUBRR);

	/* Insert 7-segment digits in allocated Memory (0x0070 - 0x007A) */
	init_7_seg();
	
	// Set Port A and Port C as outputs
	DDRA = 0xFF;
	DDRC = 0xFF;
	
	/* Initialize Timer/Counter0 */
	TIMER0_init();
	
	/* Initialize Watchdog Timer */
	WDT_init();
	
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
 *	Receiver and transmitter are enabled.
 *	RXC and TXC interrupts are enabled.
 */

void USART_init(unsigned int ubrr){

	// initialize baud rate
	UBRRH = (unsigned char)(ubrr >> 8);
	UBRRL = (unsigned char)ubrr;

	// Enable receiver and transmitter, enable RXC interrupt 
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
 * Initialize watchdog timer.
 */
void WDT_init(void){
	
	/*
	 * Bit WDE enables watchdog timer.
	 * Bits WDP0, WDP1, WDP2 set the prescaler for watchdog timer.
	 * Here, bit WDT0 is '1' and bits WDT1,WDT2 are '0'.
	 * So the watchdog timer runs for 32k cycles.
	 */
	WDTCR |= (1 << WDE) | (1 << WDP0);
	
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
 * This function shifts bytes in addresses 0x0060 - 0x0067 of RAM one address right.
 * Old value of address 0x0067 is lost
 */
void shift_RAM(void){
	unsigned char *ram_pos = (unsigned char*)0x60;
	int size = 7;
	int i = 0;
	
	for(i=size; i > 0; i--){
		ram_pos[i] = ram_pos[i-1];
	}
	
	ram_pos[0] = 0x00;
	
}

/*
 * This function stores 7-segment display digits in addresses
 * 0x0070 - 0x007A.
 */
void init_7_seg(void){
	unsigned char *seven_seg_digit = (unsigned char*)0x70;
	seven_seg_digit[0] = 0b11000000;	// 0 in 7-seg stored in address 0x0070
	seven_seg_digit[1] = 0b11111001;	// 1 in 7-seg stored in address 0x0071
	seven_seg_digit[2] = 0b10100100;	// 2 in 7-seg stored in address 0x0072
	seven_seg_digit[3] = 0b10110000;	// 3 in 7-seg stored in address 0x0073
	seven_seg_digit[4] = 0b10011001;	// 4 in 7-seg stored in address 0x0074
	seven_seg_digit[5] = 0b10010010;	// 5 in 7-seg stored in address 0x0075
	seven_seg_digit[6] = 0b10000010;	// 6 in 7-seg stored in address 0x0076
	seven_seg_digit[7] = 0b11111000;	// 7 in 7-seg stored in address 0x0077
	seven_seg_digit[8] = 0b10000000;	// 8 in 7-seg stored in address 0x0078
	seven_seg_digit[9] = 0b10010000;	// 9 in 7-seg stored in address 0x0079
	seven_seg_digit[10] = 0b11111111;	// A in 7-seg stored in address 0x007A
}

/*
 *	USART Transmit
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
	
	// points at RAM START
	unsigned char *ram_pos = (unsigned char *)0x60;
	// points at address 0x68, used for storing which digit is shown
	unsigned char *digitShown = (unsigned char*) 0x68;
	// points at address 0x69, used for storing the value of the ring counter
	unsigned char *ring_counter = (unsigned char*) 0x69;
	// points at address 0x70, the first address of the stored seven segment digits
	unsigned char *seven_seg = (unsigned char*) 0x70;
	
	// used for reading data we want to display	
	unsigned char data = ram_pos[(unsigned char)*digitShown];
	
	PORTA = 0xFF;	// turn of display
	
	*ring_counter = *ring_counter << 1; // shift ring counter and store new value
	
	// if value of ring counter is 0x00, reset it to 0x01
	if(*ring_counter == 0x00){
		*ring_counter = 0b00000001;
	}
	
	// show next digit
	*digitShown = (unsigned char)(*digitShown + 1);
	
	// if value of digitShown is 0x08, reset it to 0x00
	if(*digitShown == (unsigned char)0x08){
		ram_pos = (unsigned char *)0x60;
		*digitShown = 0x00;
	}
	
	// decode from BCD to 7-segment
	data = seven_seg[data];
	// display digit
	PORTA = data;
	// display ring counter
	PORTC = *ring_counter;
	// reset TIMER/COUNTER0
	TCNT0 = 100;
}

/*
 * Interrupt Service Routine for RXC interrupt.
 * Calls USART RXC interrupt assembly routine
 * USART_RXC_vect is the interrupt vector for RXC interrupt.
 */
ISR(USART_RXC_vect){
	
	// Initialize watchdog timer
	//WDT_init();
	asm("WDR");
	// first address of bytes we want to display
	unsigned char *store_to_ram = (unsigned char *)0x60;
	// address where state is stored
	unsigned char *curr_state = (unsigned char *)0x6A;
	// used for reading input
	unsigned char readChar;
	
	//read input
	readChar = UDR;
	readChar = UDR;	// consume RXC
	readChar = PORTB; // read input from stimuli file
	
	switch(*curr_state){
		case STATE_IDLE:
		// If state is IDLE and input is A, next state is STATE_A
		// If state is IDLE and input is N, clear display (store char 0x0A in RAM) and next state is STORE
		// If state is IDLE and input is C, clear display (store char 0x0A in RAM) and next state is CLEAR
		// Else stay in IDLE
			if(readChar == 0x41){
				*curr_state = (unsigned char) STATE_A;
			}
			else if(readChar == 0x43){
				*curr_state = (unsigned char) STATE_CLEAR;
				clear_RAM();
			}
			else if(readChar == 0x4E){
				*curr_state = (unsigned char) STATE_STORE;
				clear_RAM();
			}
			else{
				*curr_state = (unsigned char) STATE_IDLE;
				while(1){
					asm("nop");	/* No Operation */
				}
			}
			break;
		case STATE_A:
		// If state is STATE_A and input is T, next state is STATE_T
		// Else stay in STATE_A
			if(readChar == 0x54){
				*curr_state = (unsigned char) STATE_T;
			}
			else{
				*curr_state = (unsigned char) STATE_A;
				while(1){
					asm("nop");	/* No Operation */
				}
			}
			break;
		case STATE_T:
		// If state is STATE_T and input is <CR>, next state is STATE_END
		// Else stay in STATE_T
			if(readChar == 0x0D){
				*curr_state = (unsigned char) STATE_END;
			}
			else{
				*curr_state = (unsigned char) STATE_T;
				while(1){
					asm("nop");	/* No Operation */
				}
			}
			break;
		case STATE_STORE:
		// If state is STORE and input is a char in range "0x30 - 0x39", next state is STORE
		// If state is STORE and input is <CR>, next state is STATE_END
		// Else stay in STORE
			if((readChar >= 0x30) && (readChar <= 0x39)){
				*curr_state = (unsigned char) STATE_STORE;
				shift_RAM();	// shift bytes in RAM
				store_to_ram[0] = readChar^0x30;	// Mask the first 4 bits of input and store in address 0x0060 of RAM.
				// Example, if input is 0x35, then in RAM we store 0x35 XOR 0x30 => 0x05
			}
			else if(readChar == 0x0D){
				*curr_state = (unsigned char) STATE_END;
			}
			else{
				*curr_state = (unsigned char) STATE_STORE;
				while(1){
					asm("nop");	/* No Operation */
				}
			}
			break;
		case STATE_CLEAR:
		// If state is CLEAR and input is <CR>, next state is STATE_END
		// Else stay in CLEAR
			if(readChar == 0x0D){
				*curr_state = (unsigned char) STATE_END;
			}
			else{
				*curr_state = (unsigned char) STATE_CLEAR;
				while(1){
					asm("nop");	/* No Operation */
				}
			}
			break;
		case STATE_END:
		// If state is STATE_END and input is <LF>, send "OK<CR><LF>" and next state is IDLE
		// Else stay in STATE_END
			if(readChar == 0x0A){
				*curr_state = (unsigned char) STATE_IDLE;
				send_OK();
			}
			else{
				*curr_state = (unsigned char) STATE_END;
				while(1){
					asm("nop");	/* No Operation */
				}
			}
			break;
	}
}