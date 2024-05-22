
/*
 *	interrupt_routines.S
 *	HPY411 - Lab 4 code in AVR assembly for ATmega16
 *	Created: 3/11/2020 6:50:32 μμ
 *  Author: Evangelos Kioulos 2016030056
 */ 

 ;
 ; This file contains the needed interrupt service routines
 ;

 #include <avr/io.h>

 #define ram_digits 0x0060

 .global ISR_timer

 ; Programm comes here only from the interrupt handler
ISR_timer:								; Timer Interrupt service routine
	ldi XH, hi8(ram_digits)		; set X pointer at address 0x0068
	ldi XL, lo8(ram_digits+8)	
	ld r18, X					; load value from adress 0x0060
	
	ldi XL, lo8(ram_digits)		; set X pointer at address 0x0060
	add XL, r18					; set X pointer at address 0x0060 + "value of r18"(0 to 7)

	ldi r16, 0xFF
	out _SFR_IO_ADDR(PORTA), r16

	; shift an '1' through zeros
	rol r17							
	; check if value is 0x00
	cpi r17, 0						
	brne cont
	; Reset ring counter to "00000001" if r17 is "00000000"
	ldi r17, 0b00000001				
	; output the value of r17 to port C
	cont:
	out _SFR_IO_ADDR(PORTC), r17

	ld r16, X+					; Load BCD value from the address that X points to
	inc r18;						; increment r18
	cpi XL, lo8(ram_digits+9)		; check if address is out of bounds
	brne cont2
	ldi XL, lo8(ram_digits)		; reset pointer if address is out of bounds
	ldi XH, hi8(ram_digits)
	ld r16, X+					; Load BCD value from the address that X points to
	ldi r18,0x01				; reset r18 to 1

	cont2:
		cpi r16, 0
		brne eq1
		ldi r16, 0b11000000				; 0 in 7-segment
		rjmp end
	eq1:
		cpi r16, 1
		brne eq2
		ldi r16, 0b11111001				; 1 in 7-segment
		rjmp end
	eq2:
		cpi r16, 2
		brne eq3
		ldi r16, 0b10100100				; 2 in 7-segment
		rjmp end
	eq3:
		cpi r16, 3
		brne eq4
		ldi r16, 0b10110000				; 3 in 7-segment
		rjmp end
	eq4:
		cpi r16, 4
		brne eq5
		ldi r16, 0b10010010				; 4 in 7-segment
		rjmp end
	eq5:
		cpi r16, 5
		brne eq6
		ldi r16, 0b10010010				; 5 in 7-segment
		rjmp end
	eq6:
		cpi r16, 6
		brne eq7
		ldi r16, 0b10000010				; 6 in 7-segment
		rjmp end
	eq7:
		cpi r16, 7
		brne eq8
		ldi r16, 0b11111000				; 7 in 7-segment
		rjmp end
	eq8:
		cpi r16, 8
		brne eq9
		ldi r16, 0b10000000				; 8 in 7-segment
		rjmp end
	eq9:
		cpi r16, 9
		brne eqff
		ldi r16, 0b10010000				; 9 in 7-segment
		rjmp end
	eqff:
		cpi r16, 0x0A
		brne end
		ldi r16, 0xFF					; if r16 has the 0xff value display 0xFF value
		rjmp end						; turns digit off
	end:
		out _SFR_IO_ADDR(PORTA), r16					; output display digit

	ldi XL, lo8(ram_digits+8)	; store new value of r18 to address 0x0068
	st X, r18 

	ldi r16,0x64
	out _SFR_IO_ADDR(TCNT0), r16					; Reset TCNT0 to 256-2*78=100
reti								; Return interrupt, returns to the last PC address
									; saved in the stack pointer before the interrupt

.global ISR_uart

ISR_uart:					; USART Interrupt service routine
	in r16, _SFR_IO_ADDR(UDR)		;read from udr
	in r16, _SFR_IO_ADDR(UDR)		;read from udr
	mov r16, r15	; in .stim file we use r15 for simulating recieved data
	read:				; read state
		if_A:
			cpi r16, 0x41	; compare read data with A
			brne if_T		; if it is A return from interrupt, else check for if its another char
			jmp rec_end		
		if_T:
			cpi r16, 0x54	; compare read data with T
			brne if_C		; if it is T return from interrupt, else check for if its another char
			jmp rec_end
		if_C:
			cpi r16, 0x43	; compare read data with C
			brne if_N		; if it is C go to clear state, else check for if its another char
			jmp clear
		if_N:
			cpi r16, 0x4E	; compare read data with N
			brne if_CR		; if it is N go to clear state, else check for if its another char
			jmp clear
		if_CR:
			cpi r16, 0x0D	; compare read data with <CR>
			brne if_LF		; if it is <CR> return from interrupt, else check for if its another char
			jmp rec_end
		if_LF:
			cpi r16, 0x0A	; compare read data with <LF>
			brne else		; if it is <LF> transmit ok, else check for if its another char
			jmp trans_OK
		else:				; if others go to store state
			jmp store
	clear:				; Clear state. fills allocated ram with 0x0A and returns
		; Initialize X register to point at the start of data 
		; we want to store in the allocated memory.
		ldi XL, lo8(ram_digits)			
		ldi XH, hi8(ram_digits)

		; initialize memory for bcd numbers with value 0x0A,
		; so that every segment if off in the begining.
		ldi r18, 8
		; This loops 8 times because we have 8 allocated bytes.					
		init:
			ldi r16, 0x0A
			st X+, r16		; After each loop pointer X increments to the next address
			dec r18			; until the loop is over.
			brne init

		; Reset X register to point at the start of allocated space
		ldi XL, lo8(ram_digits)
		ldi XH, hi8(ram_digits)

		jmp rec_end
	store:				; Store state. Shifts data in ram and stores masked data to ram then returns.
		; shift data in ram
		; Initialize X register to point at the start of data 
		; we want to store in the allocated memory.
		ldi XL, lo8(ram_digits)
		ldi XH, hi8(ram_digits)

		ldi r16, 7
		shift_loop:
			mov r19, r18	; move value of r18 to r19.
			ld r18, X		; load value from the address that X points to, to r18.
			st X+, r19		; store value of r19 in the address that X points to and inrement X.
			dec r16
			brne shift_loop

		; Reset X register to point at the start of allocated space
		ldi XL, lo8(ram_digits)
		ldi XH, hi8(ram_digits)

		mov r16, r15
		; masks 4 upper bits, turns ascii into bcd
		ldi r18, 0x30
		eor r16, r18	; XOR read data with 0x30. turns 4 upper bits of read data to 0. for example 0x32 xor 0x30 => 0x02
		; store bcd in ram
		st X+, r16
		ldi XL, lo8(ram_digits)			
		ldi XH, hi8(ram_digits)
		jmp rec_end
	trans_OK:			; Transmit OK state. Sends OK<CR><LF> and returns
		rcall send_OK
		jmp rec_end
	rec_end:
reti
