;
; lab1_b.asm
; Lab 1 -HPY411
; Atmel Studio 7
; AVR Assembler
; Created: 8/10/2020 7:46:30 ??
; Author: Evangelos Kioulos 2016030056 
; 
; This program implements a simple 1msec timer using Timer0 Overflow interrupt
; with a clock of 10 Mhz.
; Pin 0 of port B is set as an output in the beginning and becomes '1' 
; after the interrupt service routine is executed
;

	.org 0x00

	rjmp start

	; Timer/Counter 0 Overflow interrupt address.
	.org OVF0addr
	rjmp ISR

start:
	; Set stack pointer at the top of RAM
	ldi r17, high(RAMEND)			
	out SPH, r17
	ldi r17, low(RAMEND)
	out SPL, r17
	; Global Interrupt Enable
	sei

	; makes pin 0 of port b an output.
	ldi r16,(1<<PINB0)
	out DDRB, r16

	; Initialize port B to 0.
	ldi r16, 0x00
	out PORTB, r16

	; Initialize TCNT0 to 256-2*78=100. This is because 256-78=156
	; overflows TCNT0 in 5000 cycles and we need 10000 cycles.
	ldi r16,0x64					
	out TCNT0, r16					

	; Makes Value of bit TOIE0 '1' which enables Timer/Counter 0
	; Overflow interrupt		
	ldi r16,(1<<TOIE0)
	out TIMSK, r16					

	; Sets prescaler fclk/64
	ldi r16,(1<<CS01)|(1<<CS00)
	out TCCR0, r16					
main:
	nop
	rjmp main

; Interrupt service routine
ISR:		
	ldi r16,(1<<PINB0)
	out PORTB, r16
	ldi r16,0x64
	; Reset TCNT0 to 256-2*78=100
	out TCNT0, r16					
	reti