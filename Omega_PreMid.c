#include <xc.h> 

#pragma config FOSC = XT
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config BOREN = ON
#pragma config LVP = OFF
#pragma config CPD = OFF
#pragma config WRT = OFF 	
#pragma config CP = OFF

unsigned char tens = 2; 
unsigned char ones = 4; 
unsigned char VAL;
double currentDelay = 122; 
int count = 1; // Start with counting down
bit count_flag = 0;
bit paused = 0;

void delay(int num)
{
    int flags = 0;
    while (flags < num)
    {
        if (count_flag)
        {
            count_flag = 0;
            flags++;
        }
    }
}

void instCtrl(unsigned char INST)
{
    PORTC = INST;
    RB5 = 0; // RS = 0 to accept command
    RB6 = 0;
    RB7 = 1; // E = 1 (enable)
    delay(1);
    RB7 = 0; // E = 0 (strobe)
}

void dataCtrl(unsigned char DATA)
{
    PORTC = DATA;
    RB5 = 1; // RS = 1 for data mode
    RB6 = 0;
    RB7 = 1; // E = 1 (enable)
    delay(1);
    RB7 = 0; // E = 0 (strobe)
}

void initLCD()
{
    delay(1);
    instCtrl(0x38); // Function set: 8-bit; dual-line
    instCtrl(0x08); // Display off
    instCtrl(0x01); // Clear display
    instCtrl(0x06); // Entry mode: increment; shift off
    instCtrl(0x0C); // Display on; cursor off; blink off
}

void displayNumber()
{
    instCtrl(0xC0); 
    dataCtrl((tens % 10) + '0'); 
    dataCtrl((ones % 10) + '0'); 
}

void interrupt ISR()
{
    GIE = 0; // Disable global interrupts

    if (INTF) 
    {
        INTF = 0; // Clear external interrupt flag
    } 
    else if (T0IF) 
    {
        T0IF = 0; // Clear Timer0 interrupt flag

        if (RB0 == 1) 
        {
            VAL = PORTD & 0x0F;  
        }

        count_flag = 1;
    }

    GIE = 1; // Enable global interrupts
}

// Function to count from 24 down to 0
void countDown()
{
    if (tens == 0 && ones == 0) // If reaching 00 go back to 24
    {
        tens = 2;
        ones = 4;
    }
    else if (ones == 0) // If ones is 0, decrement tens
    {
        ones = 9;
        tens--;
    }
    else
    {
        ones--; // Decrease ones
    }

    displayNumber();
    delay(currentDelay = (122*0.25));
}

void displayString(const char *str){
    while (*str) {
        dataCtrl(*str);
        str++;
    }
}

void main()
{
    TRISB = 0x01; // RB0 as input, others output
    TRISC = 0x00; // PORTC as output
    TRISD = 0x0F; // Lower nibble as input

    OPTION_REG = 0x04; // Set Timer0 prescaler
    T0IE = 1;  // Enable Timer0 interrupt
    T0IF = 0;  // Clear Timer0 interrupt flag
    GIE = 1;   // Enable global interrupts

    initLCD();
	instCtrl(0x80);
	displayString("TIMER:");
    displayNumber(); 

    while (1)
    {
	if (VAL == 0x0D){
		paused = !paused;
	if (!paused){
		countDown();
	} 
	
}	
}
}
    