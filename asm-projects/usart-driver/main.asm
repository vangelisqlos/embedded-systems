;
; HRY411_Lab_3.asm
; HPY411 - Lab 3 code in AVR assembly
; For ATmega16
; Created: 22/10/2020 10:37:30 μμ
; Author : Evangelos Kioulos 2016030056
;


; ----------------------------------------------------------------------
;
;	This program recieves USART commands formed with ASCII characters,
;	through USART serial port. After each command, the microprocessor
;	transimts OK<CR><LF>. 
;
;	Commands:
;
;	AT<CR><LF>: microprocessor responds with OK<CR><LF>.
;
;	C<CR><LF>: stores character 0x0A in all allocated RAM space. 
;
;	N<X><CR><LF>: stores character 0x0A in all allocated RAM space and 
;	then stores <X> characters in allocated RAM space and displays them 
;	in a 7-segment display.
;
; ----------------------------------------------------------------------


.dseg
.org SRAM_START

; allocate 8 bytes of sram
bcd_num:	.byte 8

.cseg				; start of code segment
.org 0x00			; We specify the address 0x00 to put our code in 
					; the beginning of the flash memory.

rjmp start			; Jump to label "start" which starts the initialization
					; of registers we need and the main program.

; TIMER/COUNTER0 interrupt address.
.org OVF0addr
rjmp ISR_timer

; USART RXD interrupt address.
.org URXCaddr
rjmp ISR_uart

; Baud rate prescale
.equ bps = 64			

.def next_state = r20
.def current_state = r21

; States needed
.equ state_read = 1
.equ state_clear = 2
.equ state_store = 3
.equ state_OK = 4
.equ state_end = 5



start:
; Set stack pointer at the top of RAM
ldi r17, high(RAMEND)			
out SPH, r17
ldi r17, low(RAMEND)
out SPL, r17

; Initilize USART
rcall USART_init

; Global Interrupt Enable
sei		

;clear all digits from display
rcall clear_RAM

; set port A as an output
ldi r16, 0xFF
out DDRA, r16

; set port C as an output
ldi r16, 0xFF
out DDRC, r16

; initilize TIMER/COUNTER0
rcall init_tcnt0

; r17 is used only for ring counter in port C
ldi r17, 0b00000000
out PORTC, r17

; main loop
main:
nop
rjmp main

; ---------------------------------------------------------------
;
;	Initialize USART:
;
;	USART frame format is in Asynchronous mode with no parity, 
;	1 stop bit and 8-bit character size.
;	Reciever and transmiter are enabled.
;	RXC and TXC interrupts are enabled.
;
; ---------------------------------------------------------------

USART_init:
; insert baud rate prescale in registers r16, r17
ldi r16, LOW(bps)
ldi r17, HIGH(bps)

; init baud rate
out UBRRL, r16
out UBRRH, r17

; Enable reciever and transmiter, 
; enable RXC interrupt 
ldi r16, 0b10011000
out UCSRB, r16

; Set USART frame format
ldi r16, 0b00000110
out UCSRC, r16

ret

; ---------------------------------------------------------------
;
;	Initialize TIMER/COUNTER 0
;
;	TCNT0 Starts counting from 100. 
;	TCNT0 Overflow interrupt is enabled
;	Prescaler is set in Fclk/64
;
; ---------------------------------------------------------------

init_tcnt0:
ldi r16,0x64					; Initialize TCNT0 to 256-2*78=100. This is because 256-78=156
out TCNT0, r16					; overflows TCNT0 in 5000 cycles and we need 10000 cycles.
								
ldi r16,(1<<TOIE0)				; insert 0x00000001 in register r16.
out TIMSK, r16					; Makes Value of bit TOIE0 '1' which enables Timer/Counter 0
								; Overflow interrupt

ldi r16,(1<<CS01)|(1<<CS00)		; insert 0x00000011 in register TCCR0
out TCCR0, r16					; Sets prescaler fclk/64
ret

; ---------------------------------------------------------------
;
;	Clear RAM
;
;	Fills all allocated RAM space(8 bytes) with character 0x0A.
;	Character 0x0A decodes to 0xFF in 7-segment. After this 
;	routine is executed display is cleared(all segments are off).
;
; ---------------------------------------------------------------

clear_RAM:
; Initialize X register to point at the start of data 
; we want to store in the allocated memory.
ldi XL, low(bcd_num)			
ldi XH, high(bcd_num)

; initialize memory for bcd numbers with value 0x0A,
; so that every segment if off in the begining.
ldi r18, 8				
init:
ldi r16, 0x0A
st X+, r16
dec r18
brne init

; Reset X register to point at the start of allocated space
ldi XL, low(bcd_num)
ldi XH, high(bcd_num)

ret

; ------------------------------------------------------------------
;
;	Shift RAM
;
;	Shifts bytes stored in allocated ram space to the right by one
;
; ------------------------------------------------------------------

shift_RAM:
	; Initialize X register to point at the start of data 
	; we want to store in the allocated memory.
	ldi XL, low(bcd_num)
	ldi XH, high(bcd_num)

	ldi r16, 7
	shift_loop:
		mov r19, r18	; move value of r18 to r19.
		ld r18, X		; load value from the address that X points to, to r18.
		st X+, r19		; store value of r19 in the address that X points to and inrement X.
		dec r16
		brne shift_loop

	; Reset X register to point at the start of allocated space
	ldi XL, low(bcd_num)
	ldi XH, high(bcd_num)
ret

; ------------------------------------------------------------------
;
;	Usart Transmit
;
;	Wait until UDRE bit in register UCSRA is off, and then transmit
;	data from register r16.
;
; ------------------------------------------------------------------

USART_transmit:
; Wait until UDR is empty
sbis UCSRA, UDRE			
rjmp USART_transmit

; Transmit value of r16
;out UDR, r16			
out TCNT2, r16
ret


; ------------------------------------------------------------------
;
;	Routine for decoding BCD to 7-segment 
;
;	This routine decodes BCD digit loaded from SRAM into 7-segment, 
;	through a series of compares.
;
; ------------------------------------------------------------------

bcd_to_7_segment:
	cpi r16, 0
	brne eq1
	ldi r16, 0b11000000
	rjmp end
eq1:
	cpi r16, 1
	brne eq2
	ldi r16, 0b11111001
	rjmp end
eq2:
	cpi r16, 2
	brne eq3
	ldi r16, 0b10100100
	rjmp end
eq3:
	cpi r16, 3
	brne eq4
	ldi r16, 0b10110000
	rjmp end
eq4:
	cpi r16, 4
	brne eq5
	ldi r16, 0b10010010
	rjmp end
eq5:
	cpi r16, 5
	brne eq6
	ldi r16, 0b10010010
	rjmp end
eq6:
	cpi r16, 6
	brne eq7
	ldi r16, 0b10000010
	rjmp end
eq7:
	cpi r16, 7
	brne eq8
	ldi r16, 0b11111000
	rjmp end
eq8:
	cpi r16, 8
	brne eq9
	ldi r16, 0b10000000
	rjmp end
eq9:
	cpi r16, 9
	brne eqff
	ldi r16, 0b10010000
	rjmp end
eqff:
	cpi r16, 0x0A
	brne end
	ldi r16, 0xFF
	rjmp end
end:
	out PORTA, r16
ret

; ------------------------------------------------------------------
;
;	TIMER/COUNTER0 Interrupt Service Routine
;
;	Rolls ring counter, reads digit from ram, decodes to 7-segment
;	and displays digit.
;
; ------------------------------------------------------------------

; Programm comes here only from the interrupt handler
ISR_timer:								; Timer Interrupt service routine
	ldi r16, 0xFF
	out PORTA, r16

	; shift an '1' through zeros
	rol r17							
	; check if value is 0x00
	cpi r17, 0						
	brne cont
	; Reset ring counter to "00000001" if r17 is "00000000"
	ldi r17, 0b00000001				
	; output the value of r17 to port C
	cont:
	out PORTC, r17

	ld r16, X+					; Load BCD value from the address that X points to
	cpi XL, low(bcd_num+9)		; check if address is out of bounds
	brne cont2
	ldi XL, low(bcd_num)		; reset pointer if address is out of bounds
	ldi XH, high(bcd_num)
	ld r16, X+					; Load BCD value from the address that X points to

	cont2:
	rcall bcd_to_7_segment		; Decode BCD to 7-segment and display digit in port A

	ldi r16,0x64
	out TCNT0, r16					; Reset TCNT0 to 256-2*78=100
reti								; Return interrupt, returns to the last PC address
									; saved in the stack pointer before the interrupt

; -------------------------------------------------------------------
;
;	USART Interrupt Service Routine
;
;	Reads chars from USART and executes usart commands repectively.
;
; -------------------------------------------------------------------

; Programm comes here only from the interrupt handler
ISR_uart:					; USART Interrupt service routine
	in r16, UDR		;read from udr
	in r16, UDR		;read from udr
	mov r16, r15	; in .stim file we use r15 for simulating recieved data
	read:				; read state
		ldi current_state, state_read
		if_A:
			cpi r16, 0x41	; compare read data with A
			brne if_T		; if it is A return from interrupt, else check for if its another char
			ldi next_state, state_end
			jmp rec_end		
		if_T:
			cpi r16, 0x54	; compare read data with T
			brne if_C		; if it is T return from interrupt, else check for if its another char
			ldi next_state, state_end
			jmp rec_end
		if_C:
			cpi r16, 0x43	; compare read data with C
			brne if_N		; if it is C go to clear state, else check for if its another char
			ldi next_state, state_clear
			jmp clear
		if_N:
			cpi r16, 0x4E	; compare read data with N
			brne if_CR		; if it is N go to clear state, else check for if its another char
			ldi next_state, state_clear
			jmp clear
		if_CR:
			cpi r16, 0x0D	; compare read data with <CR>
			brne if_LF		; if it is <CR> return from interrupt, else check for if its another char
			ldi next_state, state_end
			jmp rec_end
		if_LF:
			cpi r16, 0x0A	; compare read data with <LF>
			brne else		; if it is <LF> transmit ok, else check for if its another char
			ldi next_state, state_OK
			jmp trans_OK
		else:				; if others go to store state
			ldi next_state, state_store
			jmp store
	clear:				; Clear state. fills allocated ram with 0x0A and returns
		ldi current_state, state_clear
		rcall clear_RAM
		ldi next_state, state_end
		jmp rec_end
	store:				; Store state. Shifts data in ram and stores masked data to ram then returns.
		ldi current_state, state_store
		; shift data in ram
		rcall shift_RAM
		mov r16, r15
		; masks 4 upper bits, turns ascii into bcd
		ldi r18, 0x30
		eor r16, r18	; XOR read data with 0x30. turns 4 upper bits of read data to 0. for example 0x32 xor 0x30 => 0x02
		; store bcd in ram
		st X+, r16
		ldi XL, low(bcd_num)			
		ldi XH, high(bcd_num)
		ldi next_state, state_end
		jmp rec_end
	trans_OK:			; Transmit OK state. Sends OK<CR><LF> and returns
		ldi current_state, state_OK
		ldi r16, 0x4F
		rcall USART_transmit	; Send char "O"
		ldi r16, 0x4B
		rcall USART_transmit	; Send char "K"
		ldi r16, 0x0D
		rcall USART_transmit	; Send char "<CR>"
		ldi r16, 0x0A
		rcall USART_transmit	; Send char "<LF>"
		ldi next_state, state_end
		jmp rec_end
	rec_end:
		ldi current_state, state_end
		ldi next_state, state_read
reti