// Written By: Ian Berdan and Mason Rakowicz
// Date: 3/18/21
// Description: C program with fully functioning keypad controls. Maintains
// input temperature.

//    Default clock frequency and delay definition 
# define   SYSTEM_CLOCK_FREQUENCY     16000000 
# define   DELAY_VALUE                SYSTEM_CLOCK_FREQUENCY /100 

//    System clock  register 
#define SYSCTL_RCGCGPIO_R	(*((volatile unsigned long *)0x400FE608))
	
//Setup ADC Clock
#define SYSCTL_RCGC_ADC_R  *((volatile unsigned long *)0x400FE638)
#define ADC0_CLOCK_ENABLE  0x00000001 // ADC0 Clock Gating Control
#define CLOCK_GPIOE        0x00000010   //Port E clock control
#define CLOCK_GPIOB 			 0x00000002
#define ADC_SS3_BIT				 0x08		//SS3 set, check, and clear/done bit

// ADC module 0 base address 
#define ADC_BASE              0x40038000

// ADC module configuration registers
#define ADC_ACTIVE_SS_R       *(volatile long *)(ADC_BASE + 0x000)
#define ADC_INT_MASK_R        *(volatile long *)(ADC_BASE + 0x008)
#define ADC_TRIGGER_MUX_R     *(volatile long *)(ADC_BASE + 0x014)
#define ADC_PROC_INIT_SS_R    *(volatile long *)(ADC_BASE + 0x028)
#define ADC_PERI_CONFIG_R     *(volatile long *)(ADC_BASE + 0xFC4)
#define ADC_INT_STATUS_CLR_R  *(volatile long *)(ADC_BASE + 0x00C)
#define ADC_SAMPLE_AVE_R      *(volatile long *)(ADC_BASE + 0x030)
	
// For checking and clearing ADC status (complete):
#define ADC_RIS_R      				*(volatile long *)(ADC_BASE + 0x004)
#define ADC_ISC_R      				*(volatile long *)(ADC_BASE + 0x00C)
	
// ADC sequencer 3 configuration registers
#define ADC_SS3_IN_MUX_R      *(volatile long *)(ADC_BASE + 0x0A0)
#define ADC_SS3_CONTROL_R     *(volatile long *)(ADC_BASE + 0x0A4)
#define ADC_SSF_STAT_R   			*(volatile long *)(ADC_BASE + 0x04C)
#define ADC_SS3_FIFO_DATA_R   *(volatile long *)(ADC_BASE + 0x0A8)

//    GPIO register  definitions 
#define GPIO_PORTC_DEN_R 	(*((volatile unsigned long *)0x4000651C))
#define GPIO_PORTC_DIR_R 	(*((volatile unsigned long *)0x40006400))
#define GPIO_PORTC_DATA_R	(*((volatile unsigned long *)0x400063FC))

#define GPIO_PORTE_AFSEL_R 	*	(( volatile unsigned long *)0x40024420)
#define GPIO_PORTE_AMSEL_R 	*	(( volatile unsigned long *)0x40024528)

//Port E for output, PE0-PE3
#define	GPIO_PORTE_DEN_R    (*((volatile unsigned long *)0x4002451C))
#define GPIO_PORTE_DIR_R    (*((volatile unsigned long *)0x40024400))
#define GPIO_PORTE_DATA_R		(*((volatile unsigned long *)0x400243FC))
#define GPIO_PORTE_PIN2 		(*((volatile unsigned long *)0x40024010))// for a2d


#define	GPIO_PORTB_DEN_R    (*((volatile unsigned long *)0x4000551C))
#define GPIO_PORTB_DIR_R    (*((volatile unsigned long *)0x40005400))

#define GPIO_PORTB_PIN0 		(*((volatile unsigned long *)0x40005004))
#define GPIO_PORTB_PIN1 		(*((volatile unsigned long *)0x40005008))
#define GPIO_PORTB_PIN2 		(*((volatile unsigned long *)0x40005010))// for a2d
#define GPIO_PORTB_PIN3 		(*((volatile unsigned long *)0x40005020))
#define GPIO_PORTB_PIN4 		(*((volatile unsigned long *)0x40005040))
#define GPIO_PORTB_PIN5 		(*((volatile unsigned long *)0x40005080))

#define GPIO_PORTF_PIN1 		(*((volatile unsigned long *)0x40025008))
#define GPIO_PORTF_PIN2 		(*((volatile unsigned long *)0x40025010))
#define GPIO_PORTF_PIN3 		(*((volatile unsigned long *)0x40025020))
#define GPIO_PORTF_PIN4 		(*((volatile unsigned long *)0x40025040))
	
#define	GPIO_PORTF_DEN_R    (*((volatile unsigned long *)0x4002551C))
#define GPIO_PORTF_DIR_R    (*((volatile unsigned long *)0x40025400))

//Possibility for error
#define row0 (GPIO_PORTC_DATA_R & 0x10)
#define row1 (GPIO_PORTC_DATA_R & 0x20)
#define row2 (GPIO_PORTC_DATA_R & 0x40)
#define row3 (GPIO_PORTC_DATA_R & 0x80)
	
#define GPIO_PORTx_PIN6        	0x40       
#define GPIO_PORTx_PIN5        	0x20       
#define GPIO_PORTx_PIN4        	0x10       
#define GPIO_PORTx_PIN3         0x08      
#define GPIO_PORTx_PIN2         0x04
#define GPIO_PORTx_PIN1         0x02   
#define GPIO_PORTx_PIN0         0x01   

#include "TM4C123.h" // Device header
#include <stdint.h>

//Function prototypes 
	void ADC_Init(void);
	void ADC_Start_Sampling(void);
	void  Delay ( unsigned    long counter ); 
	void  Init_Keypad_GPIOs ( void );
	void rsfPutChar(char);
	void initRsfPutChar(void);
	void delayMs(int n);
	void checkPress(void);
	void justMaster(void);

void ADC_Init(void)
{		
	// GPIO configuration for ADC 0 analog input 1 from GPIO pin PE2
		
	// Enable the port clock for the pin that we will be using for the ADC input:
		SYSCTL_RCGCGPIO_R |=  CLOCK_GPIOE;   		// clock to Port E*
		GPIO_PORTE_AFSEL_R |= (GPIO_PORTx_PIN2);	// (o 671), want the alternate function
		GPIO_PORTE_DEN_R &= ~(GPIO_PORTx_PIN2);	// Cleraing, do NOT want digital for A2D  (p 683)
		GPIO_PORTE_AMSEL_R |= GPIO_PORTx_PIN2;		// select analog mode (p 687)

		// Enable the clock for ADC0
		SYSCTL_RCGC_ADC_R |= ADC0_CLOCK_ENABLE;   // ADC0, (p 652)

		Delay(3);   // see * at bottom of file
		ADC_PERI_CONFIG_R |= 0x03;				// 250 Ksps (p 891) 
		
		// Select AN1 (PE2) as the analog input 
		ADC_SS3_IN_MUX_R = 0x01;  // p 875 with explanation on p 851
		
		// 1st sample is end of sequence (so we're done after 1)
		ADC_SS3_CONTROL_R |= 0x06;	// p 876 (need to set bit to poll on)
		
		// 16x oversampling and then averaged
		ADC_SAMPLE_AVE_R |= 0x04;
		
		// Configure ADC0 module for sequencer 3
		ADC_ACTIVE_SS_R = 0x00000008;	
}

/*  This function initialized the GPIOs for key pad  interface . */
void Init_Keypad_GPIOs(void)
{ 
	volatile unsigned long delay;
	
	SYSCTL_RCGCGPIO_R |= 0x06;
	SYSCTL_RCGCGPIO_R |= 0x20;
	delay = SYSCTL_RCGCGPIO_R;
	
	//enable digital function for rows and colums
	GPIO_PORTC_DEN_R |= 0xF0;
	GPIO_PORTB_DEN_R |= 0x3F;
	GPIO_PORTF_DEN_R |= 0x1E;
	
	//1011
	GPIO_PORTC_DIR_R &= ~0xF0; //configures them as inputs
	GPIO_PORTB_DIR_R |= 0x2F; // conficures them as outputs
	GPIO_PORTB_DIR_R &= ~0x10; // master switch
	GPIO_PORTF_DIR_R |= 0x1E;
	
	//do led ports in here as well
} 

/*    This function scans the keypad for any key press and return the  ASCII value for the pressed key .*/ 
unsigned char Scan_Keypad(int *key_num){
	unsigned int i, cols, key = 0;
	const unsigned char key_val[4][4] = 
	{//   0    1    2    3  (columns)
		{'1', '2', '3', 'A'}, //row1
		{'4', '5', '6', 'B'}, //row2
		{'7', '8', '9', 'C'}, //row3
		{'*', '0', '#', 'D'}  //row4
	};
	for(i=0;i<4; i++){
		cols = 0x01 << i;
		GPIO_PORTB_PIN0 = cols;
		GPIO_PORTB_PIN1 = cols;
		GPIO_PORTB_PIN2 = cols;
		GPIO_PORTB_PIN3 = cols;
		if(row0 & 0x10){
			*key_num = (i * 4) + 1;
			while(row0 & 0x10);
			return key_val[key][i];
		}
		if(row1 & 0x20){
			key += 1;
			*key_num = (i * 4) + 2;
			while(row1 & 0x20);
			return key_val[key][i];
		}
		if(row2 & 0x40){
			key += 2;
			*key_num = (i * 4) + 3;
			while(row2 & 0x40);
			return key_val[key][i];
		}
		if(row3 & 0x80){
			key += 3;
			*key_num = (i * 4) + 4;
			while(row3 & 0x80);
			return key_val[key][i];
		}
		key = 0;
	}
	return 0;
}

void initRsfPutChar (void)  
{ 
		SYSCTL->RCGCUART |= 1; SYSCTL->RCGCGPIO |= 1; 		
		UART0->CTL = 0; UART0->IBRD = ((160 <<(4-3)) + 5); 
		UART0->FBRD = (66 >>1);  UART0->CC = 42/2-21; 		
		UART0->LCRH = 3<<(2+3); UART0->CTL = 69 + (1400>>1);
		GPIOA->DEN = 690/230; GPIOA->AFSEL = 48/4>>2; 
		GPIOA->PCTL = 0x7F8/15 >>3; 
		delayMs(1); 
		return;
} 

int main() 
{
	int start = 1;
	int firstPress = 0;
	int running = 0;
	int keyPress = 0;
	int tenDigit = 0;
	int oneDigit = 0;
	int tempIn = 0;
	double a2dData = 0;
	int curTempTen = 0;
	int curTempOne = 0;
	int currentTemp = 0;
	double currentTempDoub = 0;
	int tooHot = 0;
	int secondPress = 0;
	int maxTemp = 0;
	int minTemp = 0;
	Init_Keypad_GPIOs (); //Initialize Keypad we should initialize everything here
	ADC_Init(); //Initialize the ADC
	initRsfPutChar(); //Initialize PutChar
	
	
	while(1){
		int press = 0;
		justMaster();
		if(start){
			checkPress();
			GPIO_PORTB_PIN5 = 0x20;
		}
		keyPress = Scan_Keypad(&press);
		if(start){
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0D); /*carriage return*/
			rsfPutChar('E'); rsfPutChar('n'); rsfPutChar('t'); rsfPutChar('e'); rsfPutChar('r'); rsfPutChar(' ');
			rsfPutChar('T'); rsfPutChar('e');	rsfPutChar('m'); rsfPutChar('p');	rsfPutChar(' ');
			rsfPutChar('I'); rsfPutChar('n'); rsfPutChar(' '); rsfPutChar('C'); rsfPutChar(':'); rsfPutChar(' ');
			start = 0;
		}
		else{
			justMaster();
			if(keyPress != 0 && firstPress == 0 && secondPress == 0){
				rsfPutChar(keyPress);
				firstPress = 1;
				tenDigit = keyPress;
				Delay(DELAY_VALUE*20); //will need to change to prevent multiple presses
				tempIn = ((tenDigit-48)*10 + (oneDigit-48)); //converts from ASCII to int
			}
			else if(keyPress != 0 && firstPress == 1 && secondPress == 0){
				rsfPutChar(keyPress);
				oneDigit = keyPress;
				Delay(DELAY_VALUE*20); //might need to change to prevent multiple presses
				tempIn = ((tenDigit-48)*10 + (oneDigit-48)); //converts from ASCII to int
				secondPress = 1;
			}
		}
		
		if(running){
			checkPress();
		}
		///////////////////////////////////////////////////////////////////////
		if(keyPress == 0x2A && secondPress == 1){
			running = 1;
			maxTemp = tempIn + 2;
			minTemp = tempIn - 2;
		}
		///////////////////////////////////////////////////////////////////////
		if(running){
			delayMs(500);
			checkPress();
			delayMs(500);
			checkPress();
			delayMs(500);
			checkPress();
			delayMs(500);
			checkPress();
			delayMs(500);
			checkPress();
			delayMs(500);
			checkPress();
			delayMs(500);
			checkPress();
			delayMs(500);
			checkPress();
			delayMs(500);
			checkPress();
			delayMs(500);
			//do the sampling stuff
			ADC_PROC_INIT_SS_R |= 0x08;	 //might need to change since we are using a new pin for this
			while((ADC_RIS_R& ADC_SS3_BIT)==0){};   		// wait on status flag
			a2dData = (ADC_SS3_FIFO_DATA_R & 0xFFF);  
			ADC_ISC_R = ADC_SS3_BIT;
			currentTempDoub = a2dData*0.0398 - 18; //probably has a +- offset
			currentTemp = currentTempDoub;
			curTempTen = (currentTemp / 10) + 48;
			curTempOne = (currentTemp % 10) + 48;
			if(currentTemp <= minTemp){ //might want to do a slightly lower value than the tempIn, this is cuz of overshoot possibilities
				GPIO_PORTF_PIN1 = 0;
				GPIO_PORTF_PIN2 = 0x4;
				GPIO_PORTF_PIN3 = 0;
				GPIO_PORTF_PIN4 = 0x10;//turn on
				tooHot = 0;
			}
			else if(currentTemp >= minTemp && currentTemp <= maxTemp){
				GPIO_PORTF_PIN1 = 0x2;
				GPIO_PORTF_PIN2 = 0;
				GPIO_PORTF_PIN3 = 0;
				GPIO_PORTF_PIN4 = 0; //turn off
				tooHot = 0;
			}
			else if(currentTemp >= maxTemp)
				tooHot = 1;
		}
		if(running){
			checkPress();
		}
		///////////////////////////////////////////////////////////////////////
		if(tooHot){ //i am not entirely sure how the relay works. I just dont want to say immediately to turn it off and then have it in a superposition. Maybe we will need to check if it is on or off also
			//do blinky shit
			//turn off relay
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0D); /*carriage return*/
			rsfPutChar('T'); rsfPutChar('O'); rsfPutChar('O'); rsfPutChar(' ');
			rsfPutChar('H'); rsfPutChar('E'); rsfPutChar('K'); rsfPutChar('K'); rsfPutChar('I'); rsfPutChar('N'); rsfPutChar(' ');
			rsfPutChar('H'); rsfPutChar('O'); rsfPutChar('T'); rsfPutChar('!'); rsfPutChar(' ');
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0D); /*carriage return*/
			rsfPutChar('C'); rsfPutChar('u'); rsfPutChar('r'); rsfPutChar('r'); rsfPutChar('e'); rsfPutChar('n'); rsfPutChar('t'); rsfPutChar(' '); 
			rsfPutChar('T'); rsfPutChar('e'); rsfPutChar('m'); rsfPutChar('p'); rsfPutChar(':'); rsfPutChar(' ');
			rsfPutChar(curTempTen); rsfPutChar(curTempOne);
			while(tooHot){
				 // display the desired temp
				// wanna add in a delay
				GPIO_PORTF_PIN1 = 0;
				GPIO_PORTF_PIN2 = 0;
				GPIO_PORTF_PIN3 = 0x8;
				GPIO_PORTF_PIN4 = 0;
				delayMs(500);
				GPIO_PORTF_PIN3 = 0;
				checkPress();
				delayMs(500);
				GPIO_PORTF_PIN3 = 0x8;
				delayMs(500);
				GPIO_PORTF_PIN3 = 0;
				checkPress();
				delayMs(500);
				GPIO_PORTF_PIN3 = 0x8;
				delayMs(500);
				GPIO_PORTF_PIN3 = 0;
				checkPress();
				delayMs(500);
				GPIO_PORTF_PIN3 = 0x8;
				delayMs(500);
				GPIO_PORTF_PIN3 = 0;
				checkPress();
				delayMs(500);
				GPIO_PORTF_PIN3 = 0x8;
				delayMs(500);
				GPIO_PORTF_PIN3 = 0;
				checkPress();
				delayMs(500);
				ADC_PROC_INIT_SS_R |= 0x08;	 //might need to change since we are using a new pin for this
				while((ADC_RIS_R& ADC_SS3_BIT)==0){};   		// wait on status flag
				a2dData = (ADC_SS3_FIFO_DATA_R & 0xFFF);  
				ADC_ISC_R = ADC_SS3_BIT;
				currentTempDoub = a2dData*0.0398 - 18; //probably has a +- offset
				currentTemp = currentTempDoub;
				curTempTen = (currentTemp / 10) + 48;
				curTempOne = (currentTemp % 10) + 48;
				rsfPutChar (0x0A); //line feed
				rsfPutChar (0x0A); //line feed
				rsfPutChar (0x0D); /*carriage return*/
				rsfPutChar('C'); rsfPutChar('u'); rsfPutChar('r'); rsfPutChar('r'); rsfPutChar('e'); rsfPutChar('n'); rsfPutChar('t'); rsfPutChar(' '); 
				rsfPutChar('T'); rsfPutChar('e'); rsfPutChar('m'); rsfPutChar('p'); rsfPutChar(':'); rsfPutChar(' ');
				rsfPutChar(curTempTen); rsfPutChar(curTempOne);
				if(currentTemp <= maxTemp){
					tooHot = 0;
					rsfPutChar (0x0A); //line feed
					rsfPutChar (0x0A); //line feed
					rsfPutChar (0x0D); /*carriage return*/
					rsfPutChar('S'); rsfPutChar('A'); rsfPutChar('F'); rsfPutChar('E'); rsfPutChar('!'); rsfPutChar(' ');
					//do light shits
				}
			}
		}
		///////////////////////////////////////////////////////////////////////
		//cycleCounter++;
		if(running == 1){
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0D); /*carriage return*/
			rsfPutChar('C'); rsfPutChar('u'); rsfPutChar('r'); rsfPutChar('r'); rsfPutChar('e'); rsfPutChar('n'); rsfPutChar('t'); rsfPutChar(' '); 
			rsfPutChar('T'); rsfPutChar('e'); rsfPutChar('m'); rsfPutChar('p'); rsfPutChar(':'); rsfPutChar(' ');
			rsfPutChar(curTempTen); rsfPutChar(curTempOne);
			rsfPutChar(' '); rsfPutChar(' '); rsfPutChar(' '); rsfPutChar('D'); rsfPutChar('e'); rsfPutChar('s'); rsfPutChar('i'); rsfPutChar('r'); rsfPutChar('e'); rsfPutChar('d'); rsfPutChar(' '); 
			rsfPutChar('T'); rsfPutChar('e'); rsfPutChar('m'); rsfPutChar('p'); rsfPutChar(':'); rsfPutChar(' ');
			rsfPutChar(tenDigit); rsfPutChar(oneDigit);
		}
		//maybe add reset shit
	}
}
 
void    Delay ( unsigned  long  counter ) 
{     
	unsigned long i = 0;     
	for (i =0; i< counter ; i ++) ; 
}

void justMaster(){
	if(GPIO_PORTB_PIN4 != 0x10){
		rsfPutChar (0x0A); //line feed
		rsfPutChar (0x0A); //line feed
		rsfPutChar (0x0A); //line feed
		rsfPutChar (0x0A); //line feed
		rsfPutChar (0x0A); //line feed
		rsfPutChar (0x0A); //line feed
		rsfPutChar (0x0A); //line feed
		rsfPutChar (0x0A); //line feed
		rsfPutChar (0x0A); //line feed
		rsfPutChar (0x0A); //line feed
		rsfPutChar (0x0A); //line feed
		rsfPutChar (0x0A); //line feed
		rsfPutChar (0x0A); //line feed
		rsfPutChar (0x0A); //line feed
		rsfPutChar (0x0A); //line feed
		rsfPutChar (0x0A); //line feed
		rsfPutChar (0x0A); //line feed
		rsfPutChar (0x0A); //line feed
		rsfPutChar (0x0A); //line feed
		rsfPutChar (0x0A); //line feed
		rsfPutChar (0x0A); //line feed
		rsfPutChar (0x0A); //line feed
		rsfPutChar (0x0A); //line feed
		rsfPutChar (0x0A); //line feed
		rsfPutChar (0x0A); //line feed
		rsfPutChar (0x0A); //line feed
		rsfPutChar (0x0D); /*carriage return*/
		rsfPutChar('O'); rsfPutChar('f'); rsfPutChar('f');
		while(GPIO_PORTB_PIN4 != 0x10){ //master switch
			GPIO_PORTF_PIN1 = 0;
			GPIO_PORTF_PIN2 = 0;
			GPIO_PORTF_PIN3 = 0;
			GPIO_PORTF_PIN4 = 0;
			GPIO_PORTB_PIN5 = 0;
			delayMs(500);
		}
		main();
	}
}

void checkPress(){
	int keyPress;
	int press;
	if(GPIO_PORTB_PIN4 != 0x10){
		rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0A); //line feed
			rsfPutChar (0x0D); /*carriage return*/
			rsfPutChar('O'); rsfPutChar('f'); rsfPutChar('f');
		while(GPIO_PORTB_PIN4 != 0x10){ //master switch
			GPIO_PORTF_PIN1 = 0;
			GPIO_PORTF_PIN2 = 0;
			GPIO_PORTF_PIN3 = 0;
			GPIO_PORTF_PIN4 = 0;
			GPIO_PORTB_PIN5 = 0;
			delayMs(500);
		}
		main();
	}
	keyPress = Scan_Keypad(&press);
	if(keyPress == 0x23){
		GPIO_PORTF_PIN1 = 0;
		GPIO_PORTF_PIN2 = 0;
		GPIO_PORTF_PIN3 = 0;
		GPIO_PORTF_PIN4 = 0;
		main();
	}
}

void rsfPutChar (char c)
{
	while((UART0->FR & 0x8) != 0); // wait until UART is not busy
	UART0->DR = c;								 // then load your char into the UART
	return;
}

void delayMs(int n){
	int i,j;
	for(i=0;i<n;i++){
		for(j=0;j<3180;j++){
		}
	}
}

