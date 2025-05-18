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
#pragma config CP = OFF         // Flash memory code protection disabled

#define _XTAL_FREQ 4000000      // 4MHz crystal frequency

// Button pins on PORTD (ACTIVE HIGH)
#define BTN_0_DEG RD0           // First button - minimum position
#define BTN_90_DEG RD1          // Second button - middle position
#define BTN_180_DEG RD2         // Third button - maximum position

// LED pins on PORTC
#define LED_GREEN RC4           // Green LED - minimum position
#define LED_YELLOW RC5          // Yellow LED - middle position
#define LED_RED RC6             // Red LED - maximum position

// Default values for SG90 servo
#define SERVO_PIN RC2           // Servo on RC2/CCP1
#define SERVO_MIN_POS_DEFAULT 50  // Minimum position (~0.5ms pulse)
#define SERVO_MID_POS_DEFAULT 150 // Middle position (~1.5ms pulse)
#define SERVO_MAX_POS_DEFAULT 240 // Maximum position (~2.4ms pulse)

// Movement parameters optimized for SG90
#define SERVO_STEP 3            // Smaller step size for smoother movement
#define SERVO_DELAY 20          // Reduced delay between steps (SG90 is lighter and faster)
#define SERVO_SETTLE_TIME 500   // Shorter settle time (SG90 stabilizes faster)

// NOTE: Ensure power supply is between 3.0-5.0V for SG90 servo
// Higher voltages may damage the servo

// Function prototypes
void initialize(void);
void setServoPosition(uint8_t position);
void moveToPosition(uint8_t position);
void updateLEDs(uint8_t position);
void calibrateServo(void);
void sweepFullRange(void);
void blinkLED(uint8_t led, uint8_t times);
uint8_t awaitButtonPress(void);
void allLEDsOff(void);

// Global variables
uint8_t currentPosition = 150;
uint8_t SERVO_MIN_POS = SERVO_MIN_POS_DEFAULT;
uint8_t SERVO_MID_POS = SERVO_MID_POS_DEFAULT;
uint8_t SERVO_MAX_POS = SERVO_MAX_POS_DEFAULT;
uint8_t buttonPressed = 0;
uint8_t previousButtons = 0;

void main(void) {
    initialize();
    
    // Demonstrate the full range of motion
    sweepFullRange();
    
    // Run calibration routine to set exact positions
    calibrateServo();
    
    // After calibration, move to minimum position
    moveToPosition(SERVO_MIN_POS);
    updateLEDs(0);
    
    // Main loop - normal operation
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
            moveToPosition(SERVO_MIN_POS);
        }
        else if(buttonPressed & 0x02) {  // Second button (RD1) pressed
            updateLEDs(1);  // Update LED immediately
            moveToPosition(SERVO_MID_POS);
        }
        else if(buttonPressed & 0x04) {  // Third button (RD2) pressed
            updateLEDs(2);  // Update LED immediately
            moveToPosition(SERVO_MAX_POS);
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
    
    // Set up Timer2 for PWM (increased frequency for SG90)
    PR2 = 199;      // PWM Period = 3.2ms (312.5Hz)
                    // Formula: PWM Period = (PR2+1) * 4 * Tosc * Prescaler
                    // 3.2ms = (199+1) * 4 * 0.25μs * 16
    
    // Configure CCP1 for PWM mode
    CCP1CON = 0x0C; // PWM mode, all bits active
    
    // Set up Timer2 with 1:16 prescaler
    T2CON = 0x06;   // Timer2 ON, prescaler 1:16
    
    // Allow system to stabilize and set initial position
    __delay_ms(500);
    setServoPosition(SERVO_MID_POS_DEFAULT);
}

void setServoPosition(uint8_t position) {
    // Limit to safe range for SG90
    if(position < 50) position = 50;       // Ensure minimum is 0.5ms
    if(position > 240) position = 240;     // Ensure maximum is 2.4ms
    
    // Scale the position value to the actual PWM duty cycle
    uint16_t scaledPosition = (uint16_t)position * 10;
    
    // Set the PWM duty cycle
    uint16_t duty = scaledPosition;
    CCPR1L = duty >> 2;                 // 8 most significant bits
    CCP1CON = (CCP1CON & 0xCF) | ((duty & 0x03) << 4); // 2 least significant bits
    
    // Update the current position
    currentPosition = position;
}

void moveToPosition(uint8_t position) {
    // Only move if the position has changed significantly
    if (position > currentPosition + 2 || position < currentPosition - 2) {
        // Move in small steps for smoother motion
        if(position > currentPosition) {
            // Move forward
            for(uint8_t pos = currentPosition; pos < position; pos += SERVO_STEP) {
                setServoPosition(pos);
                __delay_ms(SERVO_DELAY);
            }
        } else if(position < currentPosition) {
            // Move backward
            for(uint8_t pos = currentPosition; pos > position; pos -= SERVO_STEP) {
                setServoPosition(pos);
                __delay_ms(SERVO_DELAY);
            }
        }
        
        // Ensure we reach exactly the target position
        setServoPosition(position);
        
        // Allow servo to fully stabilize with shorter delay for SG90
        __delay_ms(SERVO_SETTLE_TIME);
    }
}

void updateLEDs(uint8_t position) {
    // Turn off all LEDs
    allLEDsOff();
    
    // Turn on the appropriate LED based on position index
    if(position == 0) {          // Minimum position
        PORTCbits.RC4 = 1;       // Green on
    } else if(position == 1) {   // Middle position
        PORTCbits.RC5 = 1;       // Yellow on
    } else if(position == 2) {   // Maximum position
        PORTCbits.RC6 = 1;       // Red on
    }
}

void allLEDsOff(void) {
    PORTCbits.RC4 = 0;  // Green off
    PORTCbits.RC5 = 0;  // Yellow off
    PORTCbits.RC6 = 0;  // Red off
}

void blinkLED(uint8_t led, uint8_t times) {
    allLEDsOff();
    
    for(uint8_t i = 0; i < times; i++) {
        // Turn on specified LED
        if(led == 0) {
            PORTCbits.RC4 = 1;  // Green
        } else if(led == 1) {
            PORTCbits.RC5 = 1;  // Yellow
        } else if(led == 2) {
            PORTCbits.RC6 = 1;  // Red
        }
        __delay_ms(200);
        
        // Turn off specified LED
        if(led == 0) {
            PORTCbits.RC4 = 0;  // Green
        } else if(led == 1) {
            PORTCbits.RC5 = 0;  // Yellow
        } else if(led == 2) {
            PORTCbits.RC6 = 0;  // Red
        }
        __delay_ms(200);
    }
}

uint8_t awaitButtonPress(void) {
    uint8_t button = 0;
    uint8_t lastButtons = 0;
    
    // Wait until a button is pressed
    while(!button) {
        // Read current button states
        uint8_t currentButtons = 0;
        if(PORTDbits.RD0 == 1) currentButtons |= 0x01;
        if(PORTDbits.RD1 == 1) currentButtons |= 0x02;
        if(PORTDbits.RD2 == 1) currentButtons |= 0x04;
        
        // Detect new button presses
        button = currentButtons & ~lastButtons;
        lastButtons = currentButtons;
        
        __delay_ms(20);  // Debounce delay
    }
    
    // Return which button was pressed (1 = RD0, 2 = RD1, 4 = RD2)
    return button;
}

void calibrateServo(void) {
    // Start calibration mode - signal with LED pattern
    blinkLED(0, 3);  // Blink green LED 3 times
    blinkLED(1, 3);  // Blink yellow LED 3 times
    blinkLED(2, 3);  // Blink red LED 3 times
    
    // Set servo to middle position initially
    uint8_t calibPosition = SERVO_MID_POS_DEFAULT;
    setServoPosition(calibPosition);
    __delay_ms(1000);
    
    // Instructions for minimum position (green LED)
    blinkLED(0, 5);  // Blink green LED 5 times
    
    // Allow user to adjust position with buttons
    uint8_t done = 0;
    while(!done) {
        // Turn on green LED to indicate calibrating minimum position
        allLEDsOff();
        PORTCbits.RC4 = 1;
        
        uint8_t button = awaitButtonPress();
        
        if(button & 0x01) {      // RD0 - decrease position
            if(calibPosition > 50)  // Don't go below 0.5ms
                calibPosition -= 3;  // Smaller increment for finer control
            setServoPosition(calibPosition);
        }
        else if(button & 0x02) { // RD1 - increase position
            if(calibPosition < 240)  // Don't exceed 2.4ms
                calibPosition += 3;  // Smaller increment for finer control
            setServoPosition(calibPosition);
        }
        else if(button & 0x04) { // RD2 - confirm position
            SERVO_MIN_POS = calibPosition;
            done = 1;
        }
        
        __delay_ms(200);  // Delay to allow servo to move
    }
    
    // Signal minimum position saved
    blinkLED(0, 2);
    
    // Instructions for middle position (yellow LED)
    blinkLED(1, 5);  // Blink yellow LED 5 times
    
    // Allow user to adjust position with buttons
    done = 0;
    while(!done) {
        // Turn on yellow LED to indicate calibrating middle position
        allLEDsOff();
        PORTCbits.RC5 = 1;
        
        uint8_t button = awaitButtonPress();
        
        if(button & 0x01) {      // RD0 - decrease position
            if(calibPosition > 50)  // Don't go below 0.5ms
                calibPosition -= 3;  // Smaller increment for finer control
            setServoPosition(calibPosition);
        }
        else if(button & 0x02) { // RD1 - increase position
            if(calibPosition < 240)  // Don't exceed 2.4ms
                calibPosition += 3;  // Smaller increment for finer control
            setServoPosition(calibPosition);
        }
        else if(button & 0x04) { // RD2 - confirm position
            SERVO_MID_POS = calibPosition;
            done = 1;
        }
        
        __delay_ms(200);  // Delay to allow servo to move
    }
    
    // Signal middle position saved
    blinkLED(1, 2);
    
    // Instructions for maximum position (red LED)
    blinkLED(2, 5);  // Blink red LED 5 times
    
    // Allow user to adjust position with buttons
    done = 0;
    while(!done) {
        // Turn on red LED to indicate calibrating maximum position
        allLEDsOff();
        PORTCbits.RC6 = 1;
        
        uint8_t button = awaitButtonPress();
        
        if(button & 0x01) {      // RD0 - decrease position
            if(calibPosition > 50)  // Don't go below 0.5ms
                calibPosition -= 3;  // Smaller increment for finer control
            setServoPosition(calibPosition);
        }
        else if(button & 0x02) { // RD1 - increase position
            if(calibPosition < 240)  // Don't exceed 2.4ms
                calibPosition += 3;  // Smaller increment for finer control
            setServoPosition(calibPosition);
        }
        else if(button & 0x04) { // RD2 - confirm position
            SERVO_MAX_POS = calibPosition;
            done = 1;
        }
        
        __delay_ms(200);  // Delay to allow servo to move
    }
    
    // Signal maximum position saved
    blinkLED(2, 2);
    
    // Signal calibration complete - all LEDs on then off
    PORTCbits.RC4 = 1;
    PORTCbits.RC5 = 1;
    PORTCbits.RC6 = 1;
    __delay_ms(1000);
    PORTCbits.RC4 = 0;
    PORTCbits.RC5 = 0;
    PORTCbits.RC6 = 0;
    __delay_ms(500);
}

void sweepFullRange(void) {
    // Start at middle position
    setServoPosition(SERVO_MID_POS_DEFAULT);
    __delay_ms(1000);
    
    // Sweep to minimum position
    for(uint8_t pos = SERVO_MID_POS_DEFAULT; pos > SERVO_MIN_POS_DEFAULT; pos -= SERVO_STEP) {
        setServoPosition(pos);
        __delay_ms(SERVO_DELAY);
    }
    
    // Hold at minimum
    setServoPosition(SERVO_MIN_POS_DEFAULT);
    PORTCbits.RC4 = 1;  // Green LED on
    __delay_ms(1000);
    PORTCbits.RC4 = 0;
    
    // Sweep to maximum position
    for(uint8_t pos = SERVO_MIN_POS_DEFAULT; pos < SERVO_MAX_POS_DEFAULT; pos += SERVO_STEP) {
        setServoPosition(pos);
        __delay_ms(SERVO_DELAY);
    }
    
    // Hold at maximum
    setServoPosition(SERVO_MAX_POS_DEFAULT);
    PORTCbits.RC6 = 1;  // Red LED on
    __delay_ms(1000);
    PORTCbits.RC6 = 0;
    
    // Return to middle position
    for(uint8_t pos = SERVO_MAX_POS_DEFAULT; pos > SERVO_MID_POS_DEFAULT; pos -= SERVO_STEP) {
        setServoPosition(pos);
        __delay_ms(SERVO_DELAY);
    }
    
    // Hold at middle
    setServoPosition(SERVO_MID_POS_DEFAULT);
    PORTCbits.RC5 = 1;  // Yellow LED on
    __delay_ms(1000);
    PORTCbits.RC5 = 0;
}
