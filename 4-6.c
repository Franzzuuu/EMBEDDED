#include <xc.h>
#include<stdio.h> 


#pragma config FOSC = XT
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config BOREN = ON
#pragma config LVP = OFF
#pragma config CPD = OFF
#pragma config WRT = OFF 	
#pragma config CP = OFF

int period = 0;

void interrupt ISR(void)
{
	GIE = 0; 		// disable all unmasked interrupts (INTCON reg) 
	if(CCP1IF==1) 	// checks CCP1 interrupt flag 
	{
		CCP1IF = 0; // clears interrupt flag
		TMR1 = 0; 	// resets TMR1 
		period = CCPR1/1000; 	// transfers captured TMR1 value
		// normalize the value (make the number smaller)
		period = period*8;		 // multiply by the normalized TMR1 timeout
		
		RA0 = RA0 ^ 1;
	}
	GIE = 1; // enable all unmasked interrupts (INTCON reg)
}

void delay (int cnt) {
	int i, j;
	for (i = cnt; i != 0; i--) // loop until i=0
	    for (j = 0; j < 1000; j++); // loop until j=999
}

void dataCtrl( unsigned char DATA) {
	PORTB = DATA; // load data to PORTB
	RD0 = 1;	// set RS to 1 (data reg)
	RD1 = 0;	// set RW to 0 (write)
	RD2 = 1;	// set E to 1
	delay(2);	// call delay
	RD2 = 0;	// set E to 0 (strobe)
}

void instCtrl (unsigned char INST) {
	PORTB = INST;	 // load instruction to PORTB
	RD0 = 0;	// set RS to 0 (instruction reg)
	RD1 = 0;	// set RW to 0 (write)
	RD2 = 1;	// set E to 1
	delay(1);	// call delay
	RD2 = 0;	// set E to 0 (strobe)
}	
	
	
void initLCD() {
	delay(10);
	instCtrl(0x38); // set function to 8 bits, 2 line display, 5x7
	instCtrl(0x08);
	instCtrl(0x01); // Display clear
	instCtrl(0x06); // entry mode set: increase, display is not shifted
	instCtrl(0x0E); // display on, cursor on, blink off
}	

void dPeriod() {
 	// convert and store hex values
    char val[4];				
    val[3] = ' ';
    sprintf(val, "%d", period);
   
	// display values
    dataCtrl(val[0]);	
    dataCtrl(val[1]);
    dataCtrl(val[2]);
    dataCtrl('m');
    dataCtrl('s');
}

void main(void)
{
	OPTION_REG = 0xC0;
	ADCON1 = 0X06;
	TRISA = 0x00;
	RA0 = 0;
	TRISC = 0x04; 	// set RC2 to input
	PEIE = 1; 		// enable all peripheral interrupt (INTCON reg)
	GIE = 1; 		// enable all unmasked interrupts (INTCON reg)
	T1CON = 0x30; 	// 1:8 prescaler, Timer1 off
	CCP1CON = 0x05; // capture mode: every rising edge
	CCP1IE = 1; 	// enable TMR1/CCP1 match interrupt (PIE1 reg)
	CCP1IF = 0; 	// reset interrupt flag (PIR1 reg)
	TMR1ON = 1;		// Turns on Timer1 (T1CON reg)
	
	TRISB = 0x00;
	TRISD = 0x00;	// sets PORTB as output
	initLCD();		// go to initialize lcd function
	unsigned char bin, lim;
	lim = 0x00;
	
	// display period
	dataCtrl('P');
    dataCtrl('E');
    dataCtrl('R');
    dataCtrl('I');
    dataCtrl('O');
    dataCtrl('D');
    dataCtrl(':');
	
	for(;;) // foreground routine
	{
		instCtrl(0x87);	// reset postion of outputed value
		dPeriod();		// display period
		delay(10);		// short delay
	}
}
