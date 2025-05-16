#include <xc.h>

// CONFIG
#pragma config FOSC = XT, WDTE = OFF, PWRTE = ON, BOREN = ON, LVP = OFF, CPD = OFF, WRT = OFF, CP = OFF

#define _XTAL_FREQ 4000000     // 4MHz crystal

#define MIN_PULSE_WIDTH 51     // ~1ms (0°)
#define MAX_PULSE_WIDTH 102    // ~2ms (180°)

void initialize_ports(void);
void initialize_pwm(void);
void set_servo_position(unsigned char position);

void main(void) {
    initialize_ports();
    initialize_pwm();
    __delay_ms(500);  // startup delay

    while (1) {
        // Check Button on RB0 → 0°
        if (PORTBbits.RB0 == 0) {
            set_servo_position(0);
            __delay_ms(500);  // debounce
        }

        // Check Button on RB1 → 90°
        if (PORTBbits.RB1 == 0) {
            set_servo_position(90);
            __delay_ms(500);
        }

        // Check Button on RB2 → 180°
        if (PORTBbits.RB2 == 0) {
            set_servo_position(180);
            __delay_ms(500);
        }
    }
}

void initialize_ports(void) {
    // Set RC2 (PWM) and RC1 (LED) as output
    TRISC2 = 0;
    TRISCbits.TRISC1 = 0;
    PORTC = 0x00;

    // Set RB0, RB1, RB2 as input for buttons
    TRISBbits.TRISB0 = 1;
    TRISBbits.TRISB1 = 1;
    TRISBbits.TRISB2 = 1;

    // Enable PORTB internal pull-ups (only works when INTCON2.RBPU = 0 on newer PICs)
    OPTION_REGbits.nRBPU = 0;
    PORTB = 0xFF;  // pull-ups on for all PORTB inputs
}

void initialize_pwm(void) {
    PR2 = 124;             // PWM period = 20ms
    CCP1CON = 0x0C;        // PWM mode
    T2CON = 0x07;          // Timer2 on, Prescaler 1:16
    set_servo_position(90); // start at center
}

void set_servo_position(unsigned char position) {
    unsigned int duty_cycle;

    if (position > 180) position = 180;

    duty_cycle = MIN_PULSE_WIDTH + ((MAX_PULSE_WIDTH - MIN_PULSE_WIDTH) * position / 180);
    
    CCPR1L = duty_cycle >> 2;
    CCP1CON = (CCP1CON & 0xCF) | ((duty_cycle & 0x03) << 4);
}
