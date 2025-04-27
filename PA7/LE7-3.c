#include <xc.h>
#include <pic.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

// Configuration bits
#pragma config FOSC = HS       
#pragma config WDTE = OFF      
#pragma config PWRTE = ON       
#pragma config BOREN = ON       
#pragma config LVP = OFF        
#pragma config CPD = OFF        
#pragma config WRT = OFF        
#pragma config CP = OFF         

#define _XTAL_FREQ 4000000 // 4 MHz
#define DHT11_Data_Pin PORTDbits.RD5 //input
#define DHT11_Data_Pin_Direction TRISDbits.TRISD5

// global variables
int RH;
float T;

// delay
void delay_ms(int cnt);

// LCD Functions
void initLCD();
void instCtrl(unsigned char INST);
void dataCtrl(unsigned char DATA);

// Display Functions

void printRH();
void printT();

// I2C_master_mode
void I2C_Wait(void);
void I2C_Start(void);
void I2C_Stop(void);
void init_I2C_Master(void);
void I2C_RepeatedStart(void);
void I2C_Send(unsigned char data);
unsigned char I2C_Receive(unsigned char ack);

void main(void) {
	int res = (int)(pow(2,14));
	unsigned int RH_temp = 0x0000, T_temp = 0x0000;
	unsigned char temp = 0x00;
	TRISB = 0x00;	// set all ports of PORTB to output
	TRISD = 0x00;	// set all ports of PORTD to output
	PORTB = 0x00;
	PORTD = 0x00;
	init_I2C_Master(); // initialize I2C as master
	
	initLCD();		// initialize LCD

	while(1)
	{
		// for reading relative humidity
		I2C_Start(); 				// init start condition
		I2C_Send(0x80); 			// sending slave address + write
		I2C_Send(0xE5); 			// 11100101 -> command for reading humidity
		I2C_RepeatedStart();		// read data from another address without stop
		I2C_Send(0x81); 			// send the slave address + read
		RH_temp = I2C_Receive(1); 	// read MSB and acknowledge 
		temp = I2C_Receive(0); 		// read LSB and not acknowledge 
		I2C_Stop(); 				// initiate stop condition
		delay_ms(200); 				// delay
			
		RH_temp = RH_temp << 6; 	// shift left 6 times
		temp = temp >> 2; 			// shift right 2 times
		RH_temp = RH_temp | temp; 	// combine MSB and LSB
		
		RH = (int)(-6+(125*((float)RH_temp/(float)res)));
		
		// ACK POLLING
		I2C_Start(); 		// init start condition
		I2C_Send(0x80); 	// sending slave address + write
		I2C_Send(0xE3); 	// 11100101 -> command for reading humidity
		I2C_Stop();
		delay_ms(200);
		
		temp = 0x00;		// reset temp value
		
		// for reading temperature
		I2C_Start(); 				// initiate start condition
		I2C_Send(0x80); 			// sending slave address + write
		I2C_Send(0xE3); 			// 11100101 -> command for temperature
		I2C_RepeatedStart();		// read data from another address without stop
		I2C_Send(0x81); 			// sending slave address + read
		T_temp = I2C_Receive(1); 	// read MSB and acknowledge
		temp = I2C_Receive(0); 		// read LSB and not acknowledge
		I2C_Stop(); 				// init stop condition
		delay_ms(200); 				// delay
		
		T_temp = T_temp << 6; 		// shift left 6 times
		temp = temp >> 2; 			// shift right 2 times
		T_temp = T_temp | temp; 	// combine MSB and LSB

		T = (float)(-46.85+(175.72*((float)T_temp/(float)res)));

		// ACK POLLING
		I2C_Start(); 		// initiate start condition
		I2C_Send(0x80); 	// send the slave address + write
		I2C_Send(0xE5); 	// 11100101 -> command for reading humidity
		I2C_Stop();
		delay_ms(200);

		printRH();
		printT();
	}
}
void delay_ms(int cnt) {
	int i, j;
	for (i = cnt; i != 0; i--); 
	for (j = 0; j < 5000; j++); 
}

void instCtrl(unsigned char INST) {
	PORTB = INST;
	RD0 = 0;
	RD1 = 1; 
	delay_ms(1);
	RD1 = 0;
}

void displayString(const char *str){
	while (*str) {
		dataCtrl(*str);
		str++;
}
}

void initLCD() {
	delay_ms(1);
	instCtrl(0x38); // set function to 8 bits, 2 line display
	instCtrl(0x01); // Display clear
	instCtrl(0x06); // entry mode set: increase, display is not shifted
	instCtrl(0x0E); // display on, cursor off, blink off
	
	instCtrl(0x80); 
    displayString("Humidity: ");
	
	instCtrl(0xC0); 
    displayString("Temperature: ");
}

void dataCtrl(unsigned char DATA) {
 	PORTB = DATA;
	RD0 = 1;	
	RD1 = 1;    
	delay_ms(5); 
	RD1 = 0;   
}

void printRH() {
    instCtrl(0x8A); // set display address

    char val[10];
    if (RH < 10)
        sprintf(val, "  %d%%", RH);  // 1-digit: 2 spaces before
    else if (RH < 100)
        sprintf(val, " %d%%", RH);   // 2-digit: 1 space before
    else
        sprintf(val, "%d%%", RH);    // 3-digit: no space before
    
    int i = 0;
    while (val[i] != '\0') {
        dataCtrl(val[i]);
        i++;
    }
}

void printT() {
    instCtrl(0xCD); // set display address

    char val[10];
    if (T < 10.0)
        sprintf(val, "  %.1fC", T);  // 1-digit: 2 spaces before
    else if (T < 100.0)
        sprintf(val, " %.1fC", T);   // 2-digit: 1 space before
    else
        sprintf(val, "%.1fC", T);    // 3-digit: no space before

    int i = 0;
    while (val[i] != '\0') {
        dataCtrl(val[i]);
        i++;
    }
}

// function definitions for I2C_master_mode
void init_I2C_Master(void){
	TRISC3 = 1; 		// set RC3 (SCL) to input
	TRISC4 = 1; 		// set RC4 (SDA) to input
	SSPCON = 0x28; 		// SSP enabled, I2C master mode
	SSPCON2 = 0x00; 	// start condition idle, stop condition idle
	
	// receive idle
	SSPSTAT = 0x00; 	// slew rate enabled
	SSPADD = 0x09; 		// clock frequency at 100 KHz (FOSC = 4MHz)
}

void I2C_Wait(void)
{
/* wait until all I2C operation are finished*/
while((SSPCON2 & 0x1F) || (SSPSTAT & 0x04));
}

void I2C_Start(void){
	/* wait until all I2C operation are finished*/
	I2C_Wait();
	/* enable start condition */
	SEN = 1; // SSPCON2
}

void I2C_Stop(void){
	/* wait until all I2C operation are finished*/
	I2C_Wait();
	/* enable stop condition */
	PEN = 1; // SSPCON2
}

void I2C_RepeatedStart(void){
	/* wait until all I2C operation are finished*/
	I2C_Wait();
	/* enable repeated start */
	RSEN = 1; // SSPCON2
}

void I2C_Send(unsigned char data){
	/* wait until all I2C operation are finished*/
	I2C_Wait();
	/* write data to buffer and transmit */
	SSPBUF = data;
}

unsigned char I2C_Receive(unsigned char ack){
	unsigned char temp;
	I2C_Wait(); 		// wait until all I2C operation are finished
	RCEN = 1; 			// enable receive (SSPCON2 reg)
	I2C_Wait(); 		// wait until all I2C operation are finished
	temp = SSPBUF; 		// read SSP buffer
	I2C_Wait(); 		// wait until all I2C operation are finished
	ACKDT = (ack)?0:1; 	// set acknowledge (ACK) or not acknowledge (NACK)
	ACKEN = 1; 			// enable acknowledge sequence
	return temp;
}
