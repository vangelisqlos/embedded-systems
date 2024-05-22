/*
 * HPY411_Lab_7_8.c
 * HPY411 Lab 8 code for ATmega16 in C
 * Created: 30/11/2020 2:32:40 μμ
 * Author : Evangelos Kioulos 2016030056
 */ 

/************************************************************************************************************
 *	This program implements a simple scheduler with 3 simple processes. 
 *	All processes use PORT A for output. The time slice of each process is 100ms.
 *	To start a process, command S<X><CR><LF>, where X is 1,2 or 3, has to be received
 *	from the USART.
 *	To quit a process, command Q<X><CR><LF>, where X is 1,2 or 3, has to be received
 *	from the USART.
 *	
 *	RAM address 0x006B: process_1() status flag, if '0' process 1 is disabled, if '1' process is enabled 
 *	RAM address 0x006C: process_2() status flag, if '0' process 2 is disabled, if '1' process is enabled
 *	RAM address 0x006D: process_3() status flag, if '0' process 3 is disabled, if '1' process is enabled
 *
 *	RAM address 0x006E: Current Process running. 
 *						If '0', no process is running(endless loop with no operation).
 *						If '1', process 1 is running.
 *						if '2', process 2 is running. 
 *						if '3', process 3 is running. 
 *
 *	RAM address 0x0080: process_1() data.
 *	RAM address 0x0081: process_2() data.
 *	RAM address 0x0082: process_3() data.
 *	
 *	Processes change circularly.
 *	Time slice is implemented using 16-bit TIMER/COUNTER1.
 *	All USART commands and functionality from previous labs is supported.
 *	No seven segment display driver.
 *
************************************************************************************************************/

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
#define STATE_SET_PROCESS 5
#define STATE_QUIT_PROCESS 6 
#define STATE_END 7

/* Process Status */
#define STOPPED 0	// Process has quit
#define RUNNING 1	// Process has started

/* Running Process */
#define NO_PROCESS_FLAG 0	// no process running
#define PROCESS1_FLAG 1		// process 1 running
#define PROCESS2_FLAG 2		// process 2 running
#define PROCESS3_FLAG 3		// process 3 running

/* Function Declaration */
void USART_init(unsigned int ubrr);
void TIMER1_init(void);
void clear_RAM(void);
void shift_RAM(void);
void USART_Transmit(unsigned char data);
void send_OK(void);
void init_7_seg(void);
void process_1(void);
void process_2(void);
void process_3(void);

int main(void){
	
	/* memory location of current state */
	unsigned char *curr_state = (unsigned char *)0x6A;
	*curr_state = STATE_IDLE; //initial state is IDLE
	/* Initialize ram data for process 2*/
	unsigned char *proc2_init = (unsigned char *)0x81;
	*proc2_init = 0b10101010; //initial value of address 0x81
	/* Initial Process running */
	unsigned char *process_flag = (unsigned char *)0x6E;
	*process_flag = (unsigned char)NO_PROCESS_FLAG;
	
	/* Initialize USART */
	USART_init(MYUBRR);

	/* Insert char 0x0A in allocated Memory (0x0060 - 0x0067) */
	clear_RAM();
	
	/* Insert 7-segment digits in allocated Memory (0x0070 - 0x007A) */
	init_7_seg();
	
	// Set Port A as output
	DDRA = 0xFF;

	/* Initialize Timer/Counter1 */
	TIMER1_init();
	
	sei();	// Global Interrupt Enable
	
	while(1){
		unsigned char *process_running = (unsigned char *)0x6E;
		switch(*process_running){
			// If no process running, do nothing
			case NO_PROCESS_FLAG:
				asm("nop");
				break;
			// If process 1 flag is enabled, execute process 1
			case PROCESS1_FLAG:
				process_1();
				break;
			// If process 2 flag is enabled, execute process 2
			case PROCESS2_FLAG:
				process_2();
				break;
			// If process 3 flag is enabled, execute process 3
			case PROCESS3_FLAG:
				process_3();
				break;
			// Else break
			default:
				break;
		}
	}
}

/**************************************************************
 *
 *	Process subroutines
 *
 **************************************************************/


/*
 * Process 1: This function implements a simple BCD counter.
 * Every time that it is called it adds 1 to the value of address 0x80.
 * When the value becomes "00010000" it resets to "00000000"
 */
void process_1(void){
	unsigned char *process_data = (unsigned char*) 0x80;
	
	// if value is "00010000" reset to "00000000"
	if(*process_data == 0x10){
		*process_data = 0x00;
	}
	
	// output value of address 0x80 to port B
	PORTA = *process_data;
	
	// add 1 to value of address 0x80 and store new value
	*process_data = *process_data + 1;
	
}

/*
 * Process 2: This function toggles value "10101010" to value "01010101"
 * and vice versa. This value is stored in address 0x81.
 */
void process_2(void){
	unsigned char *process_data = (unsigned char*)0x81;
	
	// XOR value of address 0x81 with 0xFF and store new value
	*process_data = *process_data^(0xFF);
	
	// output to port B
	 PORTA = *process_data;
	
}

/*
 * Process 3: Simple ring counter.
 * The value of the ring counter is stored in address 0x82.
 */
void process_3(void){
	unsigned char *process_data = (unsigned char*) 0x82;
	
	*process_data = *process_data << 1; // shift ring counter and store new value
	
	// if value of ring counter is 0x00, reset it to 0x01
	if(*process_data == 0x00){
		*process_data = 0b00000001;
	}
	
	// output ring counter
	PORTA = *process_data;
	
}

/**************************************************************
 *
 *	Initialization routines
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
 *	Initialize TIMER/COUNTER 1(CTC mode)
 *
 *	TCNT1 counts until OCR1A is 15265. 
 *	TCNT1 Compare Match A interrupt is enabled
 *	Prescaler is set in Fclk/64
 */
void TIMER1_init(void){
	
	/*
	 * Enabling bit WGM12 runs timer/counter1 at CTC mode
	 * Enabling bits CS11 and CS10 sets prescaler at fclk/64
	 */
	TCCR1B |= (1 << WGM12)|(1 << CS11)|(1 << CS10);
	
	/*
	 * Count until OCR1A is 15625, then reset timer
	 */
	OCR1A = 15625;
	
	/*
	 * Bit OCIE1A enables Compare Match A interrupt
	 */
	TIMSK = (1 << OCIE1A);
}

/**************************************************************
 *
 *	Helper Functions
 *
 **************************************************************/

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

/**************************************************************
 *
 *	Interrupt Service Routines
 *
 **************************************************************/

/*
 * Interrupt Service Routine for TIMER1 Compare Match A interrupt.
 * TIMER1_COMPA_vect is the interrupt vector for TIMER1 Compare Match A interrupt.
 * This routine is called when the time-slice ends. Time-slice is 100ms.
 */
ISR(TIMER1_COMPA_vect){
	// Current process flag value memory address
	unsigned char *process_running = (unsigned char*) 0x6E;
	// Process 1 status flag memory address
	unsigned char *proc1_flag = (unsigned char*) 0x6B;
	// Process 2 status flag memory address
	unsigned char *proc2_flag = (unsigned char*) 0x6C;
	// Process 3 status flag memory address
	unsigned char *proc3_flag = (unsigned char*) 0x6D;
	
	switch(*process_running){
		// If no process running, execute the next available process
		case NO_PROCESS_FLAG:
			if(*proc1_flag == RUNNING){
				*process_running = PROCESS1_FLAG;
			}
			else if(*proc2_flag == RUNNING){
				*process_running = PROCESS2_FLAG;
			}
			else if(*proc3_flag == RUNNING){
				*process_running = PROCESS3_FLAG;
			}
			else{
				*process_running = NO_PROCESS_FLAG;
			}
			break;
		// If process 1 is running, execute the next available process
		// If process 1 is running, and after the time-slice there is no available process return in main and execute nothing
		case PROCESS1_FLAG:
			if(*proc2_flag == RUNNING){
				*process_running = PROCESS2_FLAG;
			}
			else if(*proc3_flag == RUNNING){
				*process_running = PROCESS3_FLAG;
			}
			else if((*proc1_flag == STOPPED) && (*proc2_flag == STOPPED) && (*proc3_flag == STOPPED)){
				*process_running = NO_PROCESS_FLAG;
			}
			else{
				*process_running = PROCESS1_FLAG;
			}
			break;
		// If process 2 is running, execute the next available process
		// If process 2 is running, and after the time-slice there is no available process return in main and execute nothing
		case PROCESS2_FLAG:
			if(*proc3_flag == RUNNING){
				*process_running = PROCESS3_FLAG;
			}
			else if(*proc1_flag == RUNNING){
				*process_running = PROCESS1_FLAG;
			}
			else if((*proc1_flag == STOPPED) && (*proc2_flag == STOPPED) && (*proc3_flag == STOPPED)){
				*process_running = NO_PROCESS_FLAG;
			}
			else{
				*process_running = PROCESS2_FLAG;
			}
			break;
		// If process 3 is running, execute the next available process
		// If process 3 is running, and after the time-slice there is no available process return in main and execute nothing
		case PROCESS3_FLAG:
			if(*proc1_flag == RUNNING){
				*process_running = PROCESS1_FLAG;
			}
			else if(*proc2_flag == RUNNING){
				*process_running = PROCESS2_FLAG;
			}
			else if((*proc1_flag == STOPPED) && (*proc2_flag == STOPPED) && (*proc3_flag == STOPPED)){
				*process_running = NO_PROCESS_FLAG;
			}
			else{
				*process_running = PROCESS3_FLAG;
			}
			break;
		// Else break
		default:
			break;
	}
	
}

/*
 * Interrupt Service Routine for RXC interrupt.
 * USART_RXC_vect is the interrupt vector for RXC interrupt.
 */
ISR(USART_RXC_vect){
	
	// first address of bytes we want to display
	unsigned char *store_to_ram = (unsigned char *)0x60;
	// address where state is stored
	unsigned char *curr_state = (unsigned char *)0x6A;
	// address for process flags
	unsigned char *process1_en_flag = (unsigned char *)0x6B;
	unsigned char *process2_en_flag = (unsigned char *)0x6C;
	unsigned char *process3_en_flag = (unsigned char *)0x6D;
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
		// If state is IDLE and input is S, next state is STATE_SET_PROCESS
		// If state is IDLE and input is Q, next state is STATE_QUIT_PROCESS
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
			else if(readChar == 0x53){
				*curr_state = (unsigned char) STATE_SET_PROCESS;
			}
			else if(readChar == 0x51){
				*curr_state = (unsigned char) STATE_QUIT_PROCESS;
			}
			else{
				*curr_state = (unsigned char) STATE_IDLE;
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
			}
			break;
		case STATE_SET_PROCESS:
		// If state is SET PROCESS and input is "0x31"(char 1 in ascii) enable process 1 and stay in this state
		// If state is SET PROCESS and input is "0x32"(char 2 in ascii) enable process 2 and stay in this state
		// If state is SET PROCESS and input is "0x33"(char 3 in ascii) enable process 3 and stay in this state
		// If state is SET PROCESS and input is <CR>, next state is STATE_END
		// Else stay on this state
			if(readChar == 0x31){
				*process1_en_flag = RUNNING;
				*curr_state = (unsigned char) STATE_SET_PROCESS;
			}
			else if(readChar == 0x32){
				*process2_en_flag = RUNNING;
				*curr_state = (unsigned char) STATE_SET_PROCESS;
			}
			else if(readChar == 0x33){
				*process3_en_flag = RUNNING;
				*curr_state = (unsigned char) STATE_SET_PROCESS;
			}
			else if(readChar == 0x0D){
				*curr_state = (unsigned char) STATE_END;
			}
			else{
				*curr_state = (unsigned char) STATE_SET_PROCESS;
			}
			break;
		case STATE_QUIT_PROCESS:
		// If state is QUIT PROCESS and input is "0x31"(char 1 in ascii) disable process 1 and stay in this state
		// If state is QUIT PROCESS and input is "0x32"(char 2 in ascii) disable process 2 and stay in this state
		// If state is QUIT PROCESS and input is "0x33"(char 3 in ascii) disable process 3 and stay in this state
		// If state is QUIT PROCESS and input is <CR>, next state is STATE_END
		// Else stay on this state
			if(readChar == 0x31){
				*process1_en_flag = STOPPED;
				*curr_state = (unsigned char) STATE_QUIT_PROCESS;
			}
			else if(readChar == 0x32){
				*process2_en_flag = STOPPED;
				*curr_state = (unsigned char) STATE_QUIT_PROCESS;
			}
			else if(readChar == 0x33){
				*process3_en_flag = STOPPED;
				*curr_state = (unsigned char) STATE_QUIT_PROCESS;
			}
			else if(readChar == 0x0D){
				*curr_state = (unsigned char) STATE_END;
			}
			else{
				*curr_state = (unsigned char) STATE_QUIT_PROCESS;
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
			}
			break;
	}
}

