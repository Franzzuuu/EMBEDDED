#include <xc.h>  // XC8 compiler definitions

// CONFIGURATION BITS
#pragma config FOSC = XT       // Oscillator Selection bits
#pragma config WDTE = OFF      // Watchdog Timer Enable bit
#pragma config PWRTE = ON      // Power-up Timer Enable bit
#pragma config BOREN = ON      // Brown-out Reset Enable bit
#pragma config LVP = OFF       // Low-Voltage (Single-Supply) In-Circuit Serial Programming
#pragma config CPD = OFF       // Data EEPROM Memory Code Protection
#pragma config WRT = OFF       // Flash Program Memory Write Enable bits
#pragma config CP = OFF        // Flash Program Memory Code Protection

#define _XTAL_FREQ 4000000     // 4MHz external crystal

// Pulse width boundaries for SG90 at 20ms period
// SG90 typically needs 0.5ms (0°) to 2.5ms (180°) pulse
// We'll use slightly safer values of 1ms to 2ms for more reliable operation
#define SERVO_MIN_DUTY 62      // ~1ms (5% of 20ms)
#define SERVO_MAX_DUTY 125     // ~2ms (10% of 20ms)

// Function prototypes
void initialize_ports(void);
void initialize_pwm(void);
void move_servo_gradually(unsigned char from_angle, unsigned char to_angle);
void set_servo_position(unsigned char position);

// Position sequence: 90° ? 0° ? 90° ? 180° ? 90° ? 0°
const unsigned char positions[] = {90, 0, 90, 180, 90, 0};
#define NUM_POSITIONS 6  // Total number of positions in the sequence

void main(void) {
    unsigned char current_position = 0;
    unsigned char from_angle, to_angle;
    
    initialize_ports();
    initialize_pwm();
    
    // Initialize servo to a safe start position (middle)
    set_servo_position(90);
    __delay_ms(2000);  // Wait for servo to settle
    
    while(1) {
        // Get current and next position from sequence
        from_angle = positions[current_position];
        current_position = (current_position + 1) % NUM_POSITIONS;
        to_angle = positions[current_position];
        
        // Set LED based on position (ON for 0° and 180°, OFF for 90°)
        if (to_angle == 90) {
            PORTCbits.RC1 = 0;  // Turn LED off when moving to 90°
        } else {
            PORTCbits.RC1 = 1;  // Turn LED on when moving to 0° or 180°
        }
        
        // Move servo gradually to next position
        move_servo_gradually(from_angle, to_angle);
        
        // Wait at this position
        __delay_ms(1500);
    }
}

void initialize_ports(void) {
    TRISCbits.TRISC2 = 0;     // Set RC2/CCP1 as output (PWM)
    TRISCbits.TRISC1 = 0;     // Set RC1 as output for LED
    PORTC = 0x00;             // Clear PORTC
}

void initialize_pwm(void) {
    // For 20ms period at 4MHz with prescaler 16:
    // TMR2 period = (PR2+1) * 4 * (1/Fosc) * Prescaler
    // 0.02 = (PR2+1) * 4 * (1/4000000) * 16
    // PR2 = 62.5 - 1 = 61.5 ˜ 62
    PR2 = 124;              // Set period register (adjust if needed)
    
    T2CON = 0x06;           // Timer2 prescaler 1:16, timer off for now
    CCP1CON = 0x0C;         // PWM mode: P1A active-high
    
    CCPR1L = 0;             // Start with 0 duty cycle
    CCP1CON &= 0xCF;        // Clear lower bits
    
    TMR2 = 0;               // Clear Timer2
    T2CON |= 0x04;          // Turn on Timer2
    
    __delay_ms(50);         // Allow PWM to stabilize
}

// Set servo to a specific position immediately
void set_servo_position(unsigned char angle) {
    // Limit the angle between safe values
    if (angle > 180) angle = 180;
    
    // Map angle to duty cycle
    // Careful calculation to avoid overflow or truncation
    unsigned int duty = SERVO_MIN_DUTY + ((unsigned long)(SERVO_MAX_DUTY - SERVO_MIN_DUTY) * angle / 180);
    
    // Set the duty cycle
    CCPR1L = duty >> 2;                              // Upper 8 bits
    CCP1CON = (CCP1CON & 0xCF) | ((duty & 0x03) << 4); // Lower 2 bits in CCP1CON<5:4>
}

// Move servo gradually from one angle to another
void move_servo_gradually(unsigned char from_angle, unsigned char to_angle) {
    unsigned char current;
    char step = 1;
    
    // Determine direction
    if (from_angle > to_angle) {
        step = -1;  // Moving backward
    } else if (from_angle == to_angle) {
        return;     // No movement needed
    }
    
    // Move servo gradually, one degree at a time
    current = from_angle;
    while (1) {
        set_servo_position(current);
        __delay_ms(20);  // Fixed delay between steps - must be constant for XC8 Free
        
        // Move to next position
        current += step;
        
        // Check if we've reached the target angle
        if (step > 0) {
            if (current >= to_angle) break;
        } else {
            if (current <= to_angle) break;
        }
    }
    
    // Final position
    set_servo_position(to_angle);
    __delay_ms(20); 
}
