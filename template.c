#include <xc.h>

#pragma config FOSC = XT
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config BOREN = ON
#pragma config LVP = OFF
#pragma config CPD = OFF
#pragma config WRT = OFF
#pragma config CP = OFF

void delay(unsigned char MUL){
	int i,j;

	for(i=MUL; i!=0; i--){ //loop until i=0
	for(j=0; j<256; j++); { //loop until j=0
	}
}
}

void instCtrl(unsigned char INST){
	PORTB = INST; //load to portb
	RC0 = 0;	  //set RS to 0
	RC2 = 0;	  //set RW to 0
	RC1 = 1;	  //set E to 1
	delay(1);
	RC1 = 0;	  //set E to 0(strobe)
}

void dataCtrl(unsigned char DATA){
	PORTB = DATA;
	RC0 = 1;
	RC2 = 0;
	RC1 = 1;
	delay(1);
	RC1 = 0;
}

void initLCD() {
	delay(1); //lcd start
	instCtrl(0x38); //function set: 8-bit
	instCtrl(0x08); //display off
	instCtrl(0x01); //display on
	instCtrl(0x06); //entry mode: increment; shift off
	instCtrl(0x0E); //display on; cursor on; blink off(e) on(f)
}

void displayString(const char *str){
	while (*str) {
		dataCtrl(*str);
		str++;
}
}

void main(){
	unsigned char data;
	OPTION_REG = 0xC0; // configuring the OPTION register
	TRISC = 0x00; // set all of PORTB as output
	TRISB = 0x00;

	initLCD();
	
	
	
	while(1) {
	
	}
}