#include <xc.h>
#include <stdint.h>

// Configuration bits
#pragma config FOSC = XT        // External crystal oscillator
#pragma config WDTE = OFF       // Watchdog timer disabled
#pragma config PWRTE = ON       // Power-up timer enabled
#pragma config BOREN = ON       // Brown-out reset enabled
#pragma config LVP = OFF        // Low-voltage programming disabled
#pragma config CPD = OFF        // Data EEPROM memory code protection disabled
#pragma config WRT = OFF        // Flash program memory write disabled
#pragma config CP = OFF         // Flash program memory code protection disabled

#define _XTAL_FREQ 4000000      // 4MHz crystal frequency

// Button pins on PORTD (ACTIVE HIGH)
#define BTN_0_DEG RD0           // First button - 0° position
#define BTN_90_DEG RD1          // Second button - 90° position
#define BTN_180_DEG RD2         // Third button - 180° position

// LED pins on PORTC
#define LED_GREEN RC4           // Green LED - 0° position
#define LED_YELLOW RC5          // Yellow LED - 90° position
#define LED_RED RC6             // Red LED - 180° position

// Servo parameters
#define SERVO_PIN RC2           // Servo on RC2/CCP1
#define SERVO_0DEG 500          // Pulse width for 0 degrees
#define SERVO_90DEG 1500        // Pulse width for 90 degrees
#define SERVO_180DEG 2500       // Pulse width for 180 degrees
#define SERVO_STEP 15           // Step size for smooth movement
#define SERVO_DELAY 20          // Delay between steps in milliseconds

// Function prototypes
void initialize(void);
void setServoPosition(uint16_t position);
void moveToPosition(uint16_t position);
void updateLEDs(uint8_t position);

// Global variables
uint16_t currentPosition = SERVO_0DEG;
uint8_t buttonPressed = 0;
uint8_t previousButtons = 0;

void main(void) {
    initialize();
    
    // Initial position - 0 degrees
    moveToPosition(SERVO_0DEG);
    updateLEDs(0);
    
    // Main loop
    while(1) {
        // Read current button states (active high)
        uint8_t currentButtons = 0;
        if(PORTDbits.RD0 == 1) currentButtons |= 0x01;
        if(PORTDbits.RD1 == 1) currentButtons |= 0x02;
        if(PORTDbits.RD2 == 1) currentButtons |= 0x04;
        
        // Detect button presses (transition from not pressed to pressed)
        buttonPressed = currentButtons & ~previousButtons;
        previousButtons = currentButtons;
        
        // Process button presses
        if(buttonPressed & 0x01) {  // First button (RD0) pressed
            updateLEDs(0);  // Update LED immediately for visual feedback
            moveToPosition(SERVO_0DEG);
        }
        else if(buttonPressed & 0x02) {  // Second button (RD1) pressed
            updateLEDs(1);  // Update LED immediately
            moveToPosition(SERVO_90DEG);
        }
        else if(buttonPressed & 0x04) {  // Third button (RD2) pressed
            updateLEDs(2);  // Update LED immediately
            moveToPosition(SERVO_180DEG);
        }
        
        // Short delay for button debounce
        __delay_ms(20);
    }
}

void initialize(void) {
    // Configure I/O pins
    TRISC2 = 0;     // RC2/CCP1 as output for servo
    
    // Configure LED pins as outputs
    TRISCbits.TRISC4 = 0;  // Green LED
    TRISCbits.TRISC5 = 0;  // Yellow LED
    TRISCbits.TRISC6 = 0;  // Red LED
    
    // Configure button pins as inputs
    TRISDbits.TRISD0 = 1;  // First button
    TRISDbits.TRISD1 = 1;  // Second button
    TRISDbits.TRISD2 = 1;  // Third button
    
    // Initialize all LEDs as off
    PORTCbits.RC4 = 0;
    PORTCbits.RC5 = 0;
    PORTCbits.RC6 = 0;
    
    // Quick LED test - flash all LEDs
    PORTCbits.RC4 = 1;
    PORTCbits.RC5 = 1;
    PORTCbits.RC6 = 1;
    __delay_ms(300);
    PORTCbits.RC4 = 0;
    PORTCbits.RC5 = 0;
    PORTCbits.RC6 = 0;
    __delay_ms(300);
    
    // Set up Timer2 for PWM
    PR2 = 249;      // Set period register for 50Hz PWM frequency
                    // (4MHz / (4 * 50Hz * 16)) - 1 = 249
    
    // Configure CCP1 for PWM mode
    CCP1CON = 0x0C; // PWM mode, all bits active
    
    // Set up Timer2 with 1:16 prescaler
    T2CON = 0x06;   // Timer2 ON, prescaler 1:16
    
    // Allow system to stabilize
    __delay_ms(500);
}

void setServoPosition(uint16_t position) {
    // Ensure position is within valid range
    if(position < 400) position = 400;
    if(position > 2600) position = 2600;
    
    // Set PWM duty cycle for the servo
    uint16_t duty = position;
    
    // Set the PWM duty cycle
    CCPR1L = duty >> 2;                 // 8 most significant bits
    CCP1CON = (CCP1CON & 0xCF) | ((duty & 0x03) << 4); // 2 least significant bits
    
    // Update the current position
    currentPosition = position;
}

void moveToPosition(uint16_t position) {
    // Move in small steps for smoother motion
    if(position > currentPosition) {
        // Move forward
        for(uint16_t pos = currentPosition; pos < position; pos += SERVO_STEP) {
            setServoPosition(pos);
            __delay_ms(SERVO_DELAY);
        }
    } else if(position < currentPosition) {
        // Move backward
        for(uint16_t pos = currentPosition; pos > position; pos -= SERVO_STEP) {
            setServoPosition(pos);
            __delay_ms(SERVO_DELAY);
        }
    }
    
    // Ensure we reach exactly the target position
    setServoPosition(position);
    __delay_ms(300);  // Allow servo to settle
}

void updateLEDs(uint8_t position) {
    // Turn off all LEDs
    PORTCbits.RC4 = 0;  // Green off
    PORTCbits.RC5 = 0;  // Yellow off
    PORTCbits.RC6 = 0;  // Red off
    
    // Turn on the appropriate LED based on position
    switch(position) {
        case 0:  // 0 degrees
            PORTCbits.RC4 = 1;  // Green on
            break;
        case 1:  // 90 degrees
            PORTCbits.RC5 = 1;  // Yellow on
            break;
        case 2:  // 180 degrees
            PORTCbits.RC6 = 1;  // Red on
            break;
    }
}
