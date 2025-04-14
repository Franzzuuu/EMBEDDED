#include<xc.h> // include file for the XC8 compiler
#pragma config FOSC = XT
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config BOREN = ON
#pragma config LVP = OFF
#pragma config CPD = OFF
#pragma config WRT = OFF
#pragma config CP = OFF

//void USART_char(char ch);
//void display(const unsigned char* input);

void delay(int MUL) {
    int i, j;
    for (i = MUL; i != 0; i--) {
        for (j = 0; j < 256; j++) {} 
    }
}

void interrupt ISR()
{
	GIE = 0;
	if(ADIF){
	
	}

	GIE = 1;

}

void USART_char(char ch){
	while(!TXIF);
	TXREG = ch;
}

void display(const unsigned char* input){
	int i;
	while(*input != '\0'){
		USART_char(*input++);
	}
	USART_char('\r'); // Carriage return
    USART_char('\n'); // New line
}

void main()
{	
	SPBRG = 0x19; 		// 9.6K baud rate @ FOSC=4MHz, asynchronous high speed
	// (see formula in Table 10-1)
	SYNC = 0; 			// asynchronous mode (TXSTA reg)
	SPEN = 1; 			// enable serial port (RCSTA reg)
	TX9 = 0; 			// 8-bit transmission (TXSTA reg)
	BRGH = 1; 			// asynchronous high-speed (TXSTA reg)
	TXEN = 1;			// transmit enable (TXSTA reg)
	//CREN = 1;
	PEIE = 1;
	GIE = 1;
	TRISC6 = 0;
	TRISC7 = 1;
	
	TRISD = 0xFF;
	int DATA;
	
	while (1) {
        while(!RD4){}
		while(RD4){
			DATA = PORTD & 0x0F;
			switch(DATA){
				case 0x00:
					display("You pressed 1.");
					break;
					
				case 0x01:
					display("You pressed 2.");
					break;

				case 0x02:
					display("You pressed 3.");
					break;

				case 0x04:
					display("You pressed 4.");
					break;
					
				case 0x05:
					display("You pressed 5.");
					break;
				
				case 0x06:
					display("You pressed 6.");
					break;
					
				case 0x08:
					display("You pressed 7.");
					break;
					
				case 0x09:
					display("You pressed 8.");
					break;
					
				case 0x0A:
					display("You pressed 9.");
					break;
				
				case 0x0C:
					display("You pressed *.");
					break;
					
				case 0x0D:
					display("You pressed 0.");
					break;

				case 0x0E:
					display("You pressed #.");
					break;
					
				default: break;
			}
			while(RD4);
		}
    }
		
}
