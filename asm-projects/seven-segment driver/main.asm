;
; HPY411_Lab_2.asm
; Lab 2
; AVR Assembly for ATmega16
; Basic 7-segment LED display with BCD encoding
; Created: 14/10/2020 9:10:25 μμ
; Author : Evangelos Kioulos 2016030056
;

; --------------------------------------------------------------------
;
; This program implements a simple 7-segment display dirver
; with time multiplexity, using timer 0 ovf interrupt.
; Digits we want to display are stored in SRAM in BCD form.
;
; --------------------------------------------------------------------

.dseg
.org SRAM_START

; allocate 8 bytes of sram
bcd_num:	.byte 8				

.cseg				
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

; Initialize X register to point at the
; start of data we want to store in the
; allocated memory
ldi XL, low(bcd_num)			
ldi XH, high(bcd_num)

; initialize memory for bcd numbers with value 0xff,
; so that every segment if off in the begining.
ldi r18, 8
; This loops 8 times because we have 8 allocated bytes.

init:
ldi r16, 0xFF
st X+, r16
dec r18
brne init

; Reset X register to point at the start of allocated space
ldi XL, low(bcd_num)
ldi XH, high(bcd_num)

;insert bcd digits we want to display
ldi r16, 0x05
st X+, r16
ldi r16, 0x03
st X+, r16
ldi r16, 0x07
st X+, r16

; Reset X register to point at the start of allocated space
ldi XL, low(bcd_num)
ldi XH, high(bcd_num)

; set port A as an output
ldi r16, 0xFF
out DDRA, r16

; set port C as an output
ldi r16, 0xFF					
out DDRC, r16

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

ldi r17, 0b00000000
out PORTC, r17

main:
nop
rjmp main

;
; This routine decodes BCD digit loaded 
; from SRAM into 7-segment, through a 
; series of compares.
;
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
	cpi r16, 0xFF
	brne end
	ldi r16, 0xFF					
	rjmp end						
end:
	out PORTA, r16
ret

; Interrupt service routine
ISR:								
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

; Reset TCNT0 to 256-2*78=100
ldi r16,0x64
out TCNT0, r16					
reti