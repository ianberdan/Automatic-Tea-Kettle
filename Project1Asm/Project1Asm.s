;*************************************************************
;
; Written by: Ian Berdan and Mason Rakowicz
; Date: 3/18/21
; Description: Assembly language tea kettle project. Heats the kettle to either a soup,
; simmer, or boil based on which 3 keys you press.
; 


;*************************************************************
	THUMB           ; Marks the THUMB mode of operation  

; Constants are declared in CODE AREA   ; (RSF -- NO, they are 
										; assembly directives)
;    AREA    Constdata, DATA, READONLY  ; so this is not needed)
;
; Peripheral bus (APB)
;
; Defining register addresses and constants (Page 660 has index)

SYSTEM_CLOCK_FREQUENCY 	EQU 16000000 
DELAY_VALUE 			EQU SYSTEM_CLOCK_FREQUENCY /8 

;System clock  register 
SYSCTL_RCGCGPIO_R 		EQU 0x400FE608
	
;Setup ADC Clock
SYSCTL_RCGC_ADC_R 		EQU 0x400FE638
ADC0_CLOCK_ENABLE  		EQU 0x00000001 ; ADC0 Clock Gating Control
CLOCK_GPIOE    			EQU 0x00000010   ;Port E clock control
CLOCK_GPIOB  			EQU 0x00000002
ADC_SS3_BIT	 			EQU 0x08		;SS3 set, check, and clear/done bit

GPIO_PORTB_CLK_EN 		EQU 0x02
GPIO_PORTC_CLK_EN 		EQU 0x04
GPIO_PORTD_CLK_EN 		EQU 0x48    
GPIO_PORTE_CLK_EN 		EQU 0x10
GPIO_PORTF_CLK_EN 		EQU 0x20

;ADC module 0 base address 
ADC_BASE       			EQU 0x40038000

;ADC module configuration registers
ADC_ACTIVE_SS_R   		EQU (ADC_BASE + 0x000)
ADC_INT_MASK_R  		EQU (ADC_BASE + 0x008)
ADC_TRIGGER_MUX_R   	EQU (ADC_BASE + 0x014)
ADC_PROC_INIT_SS_R   	EQU (ADC_BASE + 0x028)
ADC_PERI_CONFIG_R    	EQU (ADC_BASE + 0xFC4)
ADC_INT_STATUS_CLR_R  	EQU (ADC_BASE + 0x00C)
ADC_SAMPLE_AVE_R    	EQU (ADC_BASE + 0x030)
	
;For checking and clearing ADC status (complete):
ADC_RIS_R      	 		EQU (ADC_BASE + 0x004)
ADC_ISC_R      	 		EQU (ADC_BASE + 0x00C)
	
;ADC sequencer 3 configuration registers
ADC_SS3_IN_MUX_R    	EQU (ADC_BASE + 0x0A0)
ADC_SS3_CONTROL_R   	EQU (ADC_BASE + 0x0A4)
ADC_SSF_STAT_R   	 	EQU (ADC_BASE + 0x04C)
ADC_SS3_FIFO_DATA_R   	EQU (ADC_BASE + 0x0A8)

;GPIO register  definitions 
;PORT C --> OUTPUT (ROW)---------------------------
GPIO_PORTC_DEN_R  		EQU 0x4000651C
GPIO_PORTC_DIR_R  		EQU 0x40006400

GPIO_PORTC_PIN4 		EQU 0x40006040 ;it looks like these are for toggling?
GPIO_PORTC_PIN5 		EQU 0x40006080 ;|
GPIO_PORTC_PIN6 		EQU 0x40006100 ;|
GPIO_PORTC_PIN7 		EQU 0x40006200 ;^

PORT_C_PINS_4_7 		EQU 0x400063C0 ;effectively the data (3c0 because only pins 4:7)

;PORT E --> ANALOG INPUT -------------------------
GPIO_PORTE_AFSEL_R 		EQU 0x40024420
GPIO_PORTE_AMSEL_R 		EQU 0x40024528

GPIO_PORTE_DEN_R  		EQU 0x4002451C
GPIO_PORTE_DIR_R   		EQU 0x40024400
GPIO_PORTE_DATA_R 		EQU 0x400243FC
GPIO_PORTE_PIN2  		EQU 0x40024010

;PORT B --> INPUT (COLUMN)------------------------
GPIO_PORTB_DEN_R  		EQU 0x4000551C
GPIO_PORTB_DIR_R  		EQU 0x40005400

GPIO_PORTB_PIN0 		EQU 0x40005004 ;for toggling
GPIO_PORTB_PIN1  		EQU 0x40005008
GPIO_PORTB_PIN2  		EQU 0x40005010
GPIO_PORTB_PIN3  		EQU 0x40005020

PORT_B_PINS_0_3 		EQU 0x4000503C ;like pin c above, this is data

;PORT F --> OUTPUT HEATING LEDs PF1-3--------------------
;PORT F --> OUTPUT RELAY PF4-----------------------------
GPIO_PORTF_PIN1  		EQU 0x40025008
GPIO_PORTF_PIN2  		EQU 0x40025010
GPIO_PORTF_PIN3  		EQU 0x40025020
GPIO_PORTF_PIN4 		EQU 0x40025040

PORT_F_PINS_1_4 		EQU 0x40025078


GPIO_PORTF_DEN_R  		EQU 0x4002551C
GPIO_PORTF_DIR_R  		EQU 0x40025400
	
;PORT D --> OUTPUT KEYPAD LEDs-----------------------------
GPIO_PORTD_DEN_R  		EQU 0x4000751C
GPIO_PORTD_DIR_R  		EQU 0x40007400
	
PORT_D_PINS_0_2 		EQU 0x4000701C
	
;PORT D PIN 3 --> MASTER SWITCH (ACTIVE HIGH)---------------
PORT_D_PIN_3			EQU 0x40007020
	
PORT_D_PIN_6			EQU 0x40007100

	
GPIO_PORTx_PIN7			EQU 0x80
GPIO_PORTx_PIN6     	EQU 0x40     ;  
GPIO_PORTx_PIN5    		EQU 0x20     ;  
GPIO_PORTx_PIN4     	EQU 0x10     ;  
GPIO_PORTx_PIN3     	EQU 0x08     ; 
GPIO_PORTx_PIN2     	EQU 0x04	 ;
GPIO_PORTx_PIN1     	EQU 0x02     ;
GPIO_PORTx_PIN0     	EQU 0x01     ;
	
	AREA    |.text|, CODE, READONLY, ALIGN=2
	ENTRY            ; ENTRY marks the starting point of user code
	EXPORT __main   	

; User Code Starts from the next line. startup.s branches to __main
; (see lines 266-7 in startup.s)
;

__main
  LDR   r1, =SYSCTL_RCGCGPIO_R   
  LDR   R0, [r1]
  ORR   R0, R0, #GPIO_PORTF_CLK_EN
  ORR	R0, R0, #GPIO_PORTC_CLK_EN
  ORR	R0, R0, #GPIO_PORTB_CLK_EN
  ORR 	R0, R0, #GPIO_PORTD_CLK_EN
  STR   R0, [r1]                  
  NOP                            
  NOP

  LDR   r1, =GPIO_PORTF_DIR_R    	; Set direction for PORT F
  LDR   R0, [r1]
  ORR   R0, R0, #GPIO_PORTx_PIN1  	; out to green LED
  ORR   R0, R0, #GPIO_PORTx_PIN2	; out to red ""
  ORR   R0, R0, #GPIO_PORTx_PIN3	; in from SW1
  ORR   R0, R0, #GPIO_PORTx_PIN4	; in from SW1
  STR   R0, [r1]                  

  LDR   r1, =GPIO_PORTB_DIR_R    ; Set direction for PORT E
  LDR   R0, [r1]
  BIC   R0, R0, #GPIO_PORTx_PIN0
  BIC   R0, R0, #GPIO_PORTx_PIN1
  BIC   R0, R0, #GPIO_PORTx_PIN2
  BIC   R0, R0, #GPIO_PORTx_PIN3
  STR   R0, [r1]       
  
  LDR   r1, =GPIO_PORTC_DIR_R    ; Set direction for PORT E
  LDR   R0, [r1]
  ORR   R0, R0, #GPIO_PORTx_PIN4
  ORR   R0, R0, #GPIO_PORTx_PIN5
  ORR   R0, R0, #GPIO_PORTx_PIN6
  ORR   R0, R0, #GPIO_PORTx_PIN7
  STR   R0, [r1] 
  
  LDR 	R1, =GPIO_PORTD_DIR_R
  LDR	R0, [R1]
  ORR 	R0, R0, #GPIO_PORTx_PIN0
  ORR 	R0, R0, #GPIO_PORTx_PIN1
  ORR 	R0, R0, #GPIO_PORTx_PIN2
  BIC 	R0, R0, #GPIO_PORTx_PIN3
  ORR 	R0, R0, #GPIO_PORTx_PIN6
  STR	R0, [R1]
      

  LDR   R1, =GPIO_PORTD_DEN_R 
  LDR	R0, [R1]
  ORR 	R0, R0, #GPIO_PORTx_PIN0
  ORR 	R0, R0, #GPIO_PORTx_PIN1
  ORR 	R0, R0, #GPIO_PORTx_PIN2
  ORR 	R0, R0, #GPIO_PORTx_PIN3
  ORR 	R0, R0, #GPIO_PORTx_PIN6
  STR	R0, [R1]

  LDR   r1, =GPIO_PORTB_DEN_R    ; Set direction for PORT E
  LDR   R0, [r1]
  ORR   R0, R0, #GPIO_PORTx_PIN0
  ORR   R0, R0, #GPIO_PORTx_PIN1
  ORR   R0, R0, #GPIO_PORTx_PIN2
  ORR   R0, R0, #GPIO_PORTx_PIN3
  STR   R0, [r1]    

  LDR   r1, =GPIO_PORTC_DEN_R    ; Set direction for PORT E
  LDR   R0, [r1]
  ORR   R0, R0, #GPIO_PORTx_PIN4
  ORR   R0, R0, #GPIO_PORTx_PIN5
  ORR   R0, R0, #GPIO_PORTx_PIN6
  ORR   R0, R0, #GPIO_PORTx_PIN7
  STR   R0, [r1]     
  
  LDR   r1, =GPIO_PORTF_DEN_R    ; Set direction for PORT E
  LDR   R0, [r1]
  ORR   R0, R0, #GPIO_PORTx_PIN1
  ORR   R0, R0, #GPIO_PORTx_PIN2
  ORR   R0, R0, #GPIO_PORTx_PIN3
  ORR   R0, R0, #GPIO_PORTx_PIN4
  STR   R0, [r1]
  

  LDR R2, =PORT_B_PINS_0_3 ;Columns, inputs
  LDR R3, =PORT_C_PINS_4_7 ;Rows, outputs
  LDR R4, =PORT_F_PINS_1_4 ;LEDs, outputs
  

  LDR R0, [R3]
  ORR R0, #0x10 ;turns on the first row of the keypad, we will only use this so we can keep it powered
  STR R0, [R3]
  B ADC_Init
  LTORG
  
	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	

ADC_Init ;initializes ADC
	LDR R1, =SYSCTL_RCGCGPIO_R ;clock to Port E (set bit)
	LDR R0, [R1]
	ORR R0, #CLOCK_GPIOE
	STR R0, [R1]
	
	LDR R1, =GPIO_PORTE_AFSEL_R ;pg 671, want alternate select (set bit)
	LDR R0, [R1]
	ORR R0, #GPIO_PORTx_PIN2 
	STR R0, [R1]
	
	LDR R1, =GPIO_PORTE_DEN_R ;pg 683, do NOT want digital for A2D (clear bit)
	LDR R0, [R1]
	BIC R0, #GPIO_PORTx_PIN2 
	STR R0, [R1]
	
	LDR R1, =GPIO_PORTE_AMSEL_R ;pg 687, select analog mode (set bit)
	LDR R0, [R1]
	ORR R0, #GPIO_PORTx_PIN2 
	STR R0, [R1]
	
		;Enable the clock for ADC0
	LDR R1, =SYSCTL_RCGC_ADC_R ;pg 652,(set bit)
	LDR R0, [R1]
	ORR R0, #ADC0_CLOCK_ENABLE
	STR R0, [R1]
	NOP ;yes, needs at least this many no ops. shoutout Fick
	NOP
	NOP
	NOP
	NOP
	NOP
	NOP

	LDR R1, =ADC_PERI_CONFIG_R ;pg 891, 250 Ksps(set bit)
	LDR R0, [R1]
	ORR R0, #0x03
	STR R0, [R1]
	
	;select AN1 (PE2) as the analog input
	LDR R1, =ADC_SS3_IN_MUX_R ;pg 875 w/ explanation on p 851
	LDR R0, [R1]
	ORR R0, #0x01
	STR R0, [R1]
	
	;1st sample is end of sequence (so we're done after 1)
	LDR R1, =ADC_SS3_CONTROL_R ;p 876 (need to set bit to poll on)
	LDR R0, [R1]
	ORR R0, #0x06
	STR R0, [R1]
	
	;16x oversampling and then averaged
	LDR R1, =ADC_SAMPLE_AVE_R 
	LDR R0, [R1]
	ORR R0, #0x04
	STR R0, [R1]
	
	;Configure ADC0 module for sequencer 3
	LDR R1, =ADC_ACTIVE_SS_R 
	LDR R0, [R1]
	ORR R0, #0x08
	STR R0, [R1]
	
	BL runADC
	
;*******************************************************************************************
	
main
	;Master Switch
	LDR R12, =PORT_D_PIN_3
	LDR R12, [R12]
	CMP R12, #0
	BEQ mainoff
	LDR R7, =PORT_D_PIN_6 ;Green LED ON to show system is on
	LDR R0, [R7]
	ORR R0, #0x40
	STR R0, [R7]
	BL keypad 
mainNoPress ;this is used for when the keypad switches operation. dont need to run code above ^ to recheck the keypad. R1 should not be trashed
	CMP R1, #1
	BEQ soupB
	CMP R1, #2
	BEQ simmerB
	CMP R1, #3
	BEQ boilB
	B main ;infinit loop until a button is pressed. I dont have anything to update R1 to zero so it should constantly run and skip this when you run the program and click the button

mainoff
	LDR R7, =PORT_D_PIN_6 ;Green LED ON to show system is on
	LDR R0, [R7]
	BIC R0, #0x40
	STR R0, [R7]
	B main
;Function to scan keypad 
keypad
	PUSH {R0} 
	LDR R0, [R2]
	CMP R0, #0 ; no button input, will not update the current button press
	BEQ skipKeypad3
	CMP R0, #1 ; first button pressed
	BNE skipKeypad1
	MOV R1, #1 ; sets a flag for which button was pressed 
	POP {R0}
	BX LR
skipKeypad1
	CMP R0, #2
	BNE skipKeypad2
	MOV R1, #2
	POP {R0}
	BX LR
skipKeypad2
	CMP R0, #4
	BNE skipKeypad3
	MOV R1, #3
	POP {R0}
	BX LR
skipKeypad3
	POP {R0}
	BX LR
	
	
	
runADC ;Takes ADC measurement. Nearly exactly copied and pasted, just with a couple changes
	LDR R11, =ADC_PROC_INIT_SS_R
	LDR R0, [R11]
	ORR R0, #0x08
	STR R0, [R11]
	
addWhile
	LDR R11, =ADC_RIS_R
	LDR R0, [R11]
	AND R0, #ADC_SS3_BIT
	CMP R0, #0
	BEQ addWhile
	NOP
	NOP
	
	;MOV R9, #0xFFF
	LDR R11, =ADC_SS3_FIFO_DATA_R
	LDR R11, [R11]
	MOV R9, #0xFF0
	ADD R9, #0x00F
	AND R9, R11 		;Should be ADC value in R9
	
	LDR R11, =ADC_ISC_R
	LDR R0, [R11]
	ORR R0, #ADC_SS3_BIT
	STR R0, [R11]
	
	BX LR
	
	
;Intermediate branches
soupB
	LDR R7, =PORT_D_PINS_0_2 ;Turns Soup LED on
	LDR R0, [R7]
	ORR R0, #0x01
	STR R0, [R7]
	B soup

simmerB
	LDR R7, =PORT_D_PINS_0_2 ;Turns simmer LED on
	LDR R0, [R7]
	ORR R0, #0x02
	STR R0, [R7]
	B simmer
	
boilB
	LDR R7, =PORT_D_PINS_0_2 ;Turns boil LED on
	LDR R0, [R7]
	ORR R0, #0x04
	STR R0, [R7]
	B boil


;Where the magic happens
soup
	LDR R12, =PORT_D_PIN_3
	LDR R12, [R12]
	CMP R12, #0
	BEQ backwards1
	BL keypad
	CMP R1, #1 ;check keypad change. wont update if not pressed
	BNE backwards ; this is done to branch out of this loop if the keypad changes to a different value
	BL runADC
	;This is weird because the adc value jumps around a lot it seems like, so the too-hot ceiling is set pretty high to 
	;help accound for a wide range of adc samples
	MOV R10, #0x960
	ORR R10, #0x00d
	CMP R9, R10 ;hex value corresponding to 80 degrees C
	BLT tooCold 
	MOV R10, #0xa00
	ORR R10, #0x003
	CMP R9, R10 ;hex value corresponding to 86 degrees C 
	BGT tooHot
	BL relayOff
	LDR R7, =GPIO_PORTF_PIN1 ;Green LED ON to show it is of desired temp
	LDR R0, [R7]
	ORR R0, #0x02
	STR R0, [R7]
	LDR R7, =GPIO_PORTF_PIN2 ;Amber (yellow) LED OFF since the kettle is now off
	LDR R0, [R7]
	BIC R0, #0x04
	STR R0, [R7]
	B soup ;This should go back to the beginning and regulate if neither too hot/cold
tooCold
	LDR R7, =GPIO_PORTF_PIN1 ;Green LED OFF because too cold
	LDR R0, [R7]
	BIC R0, #0x02
	STR R0, [R7]
	BL relayOn
	LDR R7, =GPIO_PORTF_PIN2 ;Amber (yellow) LED ON to show it is heating up
	LDR R0, [R7]
	ORR R0, #0x04
	STR R0, [R7]
	B soup
tooHot
	LDR R7, =GPIO_PORTF_PIN1 ;Green LED OFF because too hot
	LDR R0, [R7]
	BIC R0, #0x02
	STR R0, [R7]
	LDR R7, =GPIO_PORTF_PIN2 ;Amber (yellow) LED OFF since the kettle is now off
	LDR R0, [R7]
	BIC R0, #0x04
	STR R0, [R7]
	BL relayOff
	B flashRed
	B soup

backwards1
	LDR R7, =PORT_D_PIN_6 ;Green LED OFF to show system is off
	LDR R0, [R7]
	BIC R0, #0x40
	STR R0, [R7]

backwards
	MOV R1, #0x00
	LDR R0, [R4] ;with all output pins
	MOV R0, #0x00 ;turn off LEDs PF1-3, off relay PF4
	STR R0, [R4]
	LDR R7, =PORT_D_PINS_0_2 ;Turns all mode LEDs off (soup, simmer, or boil)
	LDR R0, [R7]
	BIC R0, #0x01
	BIC R0, #0x02
	BIC R0, #0x04
	STR R0, [R7]
	B mainNoPress

simmer
	LDR R12, =PORT_D_PIN_3
	LDR R12, [R12]
	CMP R12, #0
	BEQ backwards1
	BL keypad
	CMP R1, #2
	BNE backwards
	BL runADC
	
	MOV R10, #0xa90
	ORR R10, #0x00a
	CMP R9, R10 ;hex value corresponding to 92 degrees C (this is an educated guess for a simmer)
	BLT tooColdSim 
	MOV R10, #0xbe0
	ORR R10, #0x000
	CMP R9, R10 ;hex value corresponding to 105 degrees C (this sets the ceiling high again)
	BGT tooHotSim
	BL relayOff
	LDR R7, =GPIO_PORTF_PIN1 ;Green LED ON to show it is of desired temp
	LDR R0, [R7]
	ORR R0, #0x02
	STR R0, [R7]
	LDR R7, =GPIO_PORTF_PIN2 ;Amber (yellow) LED OFF since the kettle is now off
	LDR R0, [R7]
	BIC R0, #0x04
	STR R0, [R7]
	B simmer ;This should go back to the beginning and regulate if neither too hot/cold
tooColdSim
	LDR R7, =GPIO_PORTF_PIN1 ;Green LED OFF because too cold
	LDR R0, [R7]
	BIC R0, #0x02
	STR R0, [R7]
	BL relayOn
	LDR R7, =GPIO_PORTF_PIN2 ;Amber (yellow) LED ON to show it is heating up
	LDR R0, [R7]
	ORR R0, #0x04
	STR R0, [R7]
	B simmer
tooHotSim
	LDR R7, =GPIO_PORTF_PIN1 ;Green LED OFF because too hot
	LDR R0, [R7]
	BIC R0, #0x02
	STR R0, [R7]
	LDR R7, =GPIO_PORTF_PIN2 ;Amber (yellow) LED OFF since the kettle is now off
	LDR R0, [R7]
	BIC R0, #0x04
	STR R0, [R7]
	BL relayOff
	B flashRedSim
	B simmer
	
boil
	LDR R12, =PORT_D_PIN_3
	LDR R12, [R12]
	CMP R12, #0
	BEQ backwards1
	BL keypad
	CMP R1, #3
	BNE backwards
	BL runADC
	
	MOV R10, #0xb40
	ORR R10, #0x00a
	CMP R9, R10 ;hex value corresponding to 99 degrees C (approximate boiling, seems to start boiling even slightly before due to kettle)
	BLT tooColdBoil
	BL relayOff
	LDR R7, =GPIO_PORTF_PIN1 ;Green LED ON to show it is of desired temp
	LDR R0, [R7]
	ORR R0, #0x02
	STR R0, [R7]
	LDR R7, =GPIO_PORTF_PIN2 ;Amber (yellow) LED OFF since the kettle is now off
	LDR R0, [R7]
	BIC R0, #0x04
	STR R0, [R7]
	B boil
tooColdBoil
	LDR R7, =GPIO_PORTF_PIN1 ;Green LED OFF because too cold
	LDR R0, [R7]
	BIC R0, #0x02
	STR R0, [R7]
	BL relayOn
	LDR R7, =GPIO_PORTF_PIN2 ;Amber (yellow) LED ON to show it is heating up
	LDR R0, [R7]
	ORR R0, #0x04
	STR R0, [R7]
	B boil
	

	
relayOn
	LDR R7, =GPIO_PORTF_PIN4
	LDR R0, [R7]
	ORR R0, #0x10 ;I changed this to #0x10 from #0x08 because PF4 is relay
	STR R0, [R7]
	BX LR
	
	
relayOff
	LDR R7, =GPIO_PORTF_PIN4
	LDR R0, [R7]
	BIC R0, #0x10 ;turns off the relay
	STR R0, [R7]
	BX LR

flashRed
	BL keypad
	CMP R1, #1 ;check keypad change. wont update if not pressed
	BNE backwards
	BL delLoop
	BL keypad
	CMP R1, #1 ;check keypad change. wont update if not pressed
	BNE backwards
	;turn on red (0.5 s)
	LDR R7, =GPIO_PORTF_PIN3
	LDR R0, [R7]
	ORR R0, #0x08
	STR R0, [R7]
	BL keypad
	CMP R1, #1 ;check keypad change. wont update if not pressed
	BNE backwards
	;delay loop
	BL delLoop
	BL keypad
	CMP R1, #1 ;check keypad change. wont update if not pressed
	BNE backwards
	;turn off red
	LDR R7, =GPIO_PORTF_PIN3
	LDR R0, [R7]
	BIC R0, #0x08
	STR R0, [R7]
	B soup
	
flashRedSim
	BL keypad
	CMP R1, #2 ;check keypad change. wont update if not pressed
	BNE backwards
	BL delLoop
	BL keypad
	CMP R1, #1 ;check keypad change. wont update if not pressed
	BNE backwards
	;turn on red (0.5 s)
	LDR R7, =GPIO_PORTF_PIN3
	LDR R0, [R7]
	ORR R0, #0x08
	STR R0, [R7]
	BL keypad
	CMP R1, #1 ;check keypad change. wont update if not pressed
	BNE backwards
	;delay loop
	BL delLoop
	BL keypad
	CMP R1, #1 ;check keypad change. wont update if not pressed
	BNE backwards
	;turn off red
	LDR R7, =GPIO_PORTF_PIN3
	LDR R0, [R7]
	BIC R0, #0x08
	STR R0, [R7]
	B simmer


; Delay loop
delLoop
  PUSH {LR}
  LDR   R11 , =DELAY_VALUE  ;adjusted up top, hopefully is 0.5 seconds
delay
  SUBS  R11, #1			; 1 cycle
  BNE   delay			; 1 + P  (assuming P = 1 here)
  POP {pc}

ALIGN
END
