;
; HRY411_lab_1.asm
; Lab 1 - HPY411	
; Atmel Studio 7
; Avr Assembler
; Created: 7/10/2020 7:59:05 μμ
; Author : Evangelos Kioulos 2016030056
;
; This program implements a simple 1msec timer using a nested loop
; with a clock of 10 Mhz.
; Pin 0 of port B is set as an output in the beginning and becomes '1' 
; when the timer ends.
;

; set pin 0 of port b as output.
ldi r16,(1<< PINB0)
out DDRB, r16
								
; Inner loop (loop2) costs 4 cycles and loops 250 times.
; Outer loop (loop1) iterrates loop2 10 times
; Both loops take 10000 cycles.
ldi r18,10
loop1:
ldi r19, 250
loop2:
nop
dec r19
brne loop2
dec r18
brne loop1

out PORTB, r16

stay:
	rjmp stay
	


