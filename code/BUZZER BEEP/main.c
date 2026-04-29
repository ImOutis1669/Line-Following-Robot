/*
 * Bluetooth reception.c
 *
 * Created: 27/02/2026 17:54:39
 * Author : ZEBED
 *
 * Modes:
 *  - Joystick mode: controlled by Bluetooth commands
 *  - Line follow mode: controlled by IR sensors
 *
 * Extra features:
 *  - URM37 ultrasonic sensor for obstacle distance
 *  - buzzer beeps faster when object is closer
 *  - USART RX interrupt for Bluetooth reception
 *  - INT0 external interrupt used only to raise a flag
 */

#define F_CPU 20000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

/* ---------------- UART settings ---------------- */
#define Baud_Rate 9600UL
#define Baud_Register_Value ((F_CPU / (16UL * Baud_Rate)) - 1)

/* ---------------- LEDs ---------------- */
#define Red_LED     PA0
#define Green_LED   PA1
#define Blue_LED    PA2
#define Yellow_LED  PA3

/* ---------------- Motor pins ---------------- */
#define Motor_1_PWM PD5      // OC1A
#define Motor_2_PWM PD4      // OC1B
#define DIR_1       PC0
#define DIR_2       PC1

/* ---------------- IR sensors ---------------- */
#define IR_Left     PD2
#define IR_Right    PD3

/* ---------------- URM37 ultrasonic sensor ---------------- */
#define TRIG_PIN    PB3
#define ECHO_PIN    PB4
#define BUZZER_PIN  PB1

#define TIME_PER_CM 50       // URM37 PWM mode: about 50 us per cm
#define TIMEOUT     50000UL
#define MAX_DISTANCE 15      // stop/beep range limit in cm

/* ---------------- Modes ---------------- */
#define Joystick_Mode    0
#define Line_Follow_Mode 1

/* ---------------- Global variables ---------------- */
volatile char Rx_Char = 'S';                 // latest received Bluetooth command
volatile uint8_t mode_switch_request = 0;    // set when 'Z' is received
volatile uint8_t int0_request = 0;           // set when INT0 occurs
volatile unsigned int distance = 0;          // latest measured distance

int current_mode = Line_Follow_Mode;

int Width = 255;      // joystick mode motor speed
int lf_Width = 200;   // line-follow mode motor speed
int stop = 0;

/* =========================================================
   URM37 ULTRASONIC FUNCTIONS
   ========================================================= */

/*
 * measurePulse()
 * Sends a trigger pulse to the URM37 and measures how long
 * the ECHO pin stays LOW.
 *
 * Returns:
 *   pulse time in microseconds (approx)
 *   or TIMEOUT if no valid pulse is received in time
 */
unsigned long measurePulse(void)
{
    unsigned long time = 0;

    /* URM37 trigger:
       TRIG stays high normally,
       pull low briefly,
       then high again */
    PORTB |= (1 << TRIG_PIN);
    PORTB &= ~(1 << TRIG_PIN);
    _delay_us(10);
    PORTB |= (1 << TRIG_PIN);

    /* Wait for ECHO to go low if it is currently high */
    while (PINB & (1 << ECHO_PIN))
    {
        /* do nothing */
    }

    /* Measure how long ECHO stays low */
    while (!(PINB & (1 << ECHO_PIN)))
    {
        _delay_us(1);
        time++;

        if (time >= TIMEOUT)
        {
            return TIMEOUT;
        }
    }

    return time;
}

/*
 * measureDistance()
 * Converts pulse time into distance in cm.
 */
unsigned int measureDistance(void)
{
    unsigned long pulseTime = measurePulse();

    if (pulseTime >= TIMEOUT)
    {
        return MAX_DISTANCE;
    }

    return (unsigned int)(pulseTime / TIME_PER_CM);
}

/*
 * getBeepDelay()
 * Converts distance into buzzer delay.
 *
 * close object  -> small delay -> fast beeping
 * far object    -> large delay -> slow beeping
 * too far       -> no beep
 */
unsigned int getBeepDelay(unsigned int dist)
{
    unsigned int fast = 60;
    unsigned int slow = 700;
    unsigned long gap;
    unsigned long delayTime;

    if (dist >= MAX_DISTANCE)
    {
        return 0;
    }

    if (dist < 1)
    {
        dist = 1;
    }

    gap = slow - fast;
    delayTime = fast + (gap * (dist - 1)) / (MAX_DISTANCE - 1);

    return (unsigned int)delayTime;
}

/*
 * waitMs()
 * Custom millisecond delay.
 */
void waitMs(unsigned int ms)
{
    while (ms > 0)
    {
        _delay_ms(1);
        ms--;
    }
}

/*
 * beep()
 * Turns buzzer on and off once.
 */
void beep(unsigned int delayTime)
{
    PORTB |= (1 << BUZZER_PIN);
    waitMs(delayTime / 2);

    PORTB &= ~(1 << BUZZER_PIN);
    waitMs(delayTime / 2);
}

/* =========================================================
   MOTOR DIRECTION FUNCTIONS
   ========================================================= */

void forward(void)
{
    PORTC |= (1 << DIR_1);
    PORTC &= ~(1 << DIR_2);
}

void backward(void)
{
    PORTC |= (1 << DIR_2);
    PORTC &= ~(1 << DIR_1);
}

void left(void)
{
    PORTC |= (1 << DIR_2);
    PORTC |= (1 << DIR_1);
}

void right(void)
{
    PORTC &= ~(1 << DIR_2);
    PORTC &= ~(1 << DIR_1);
}

/*
 * stopMotors()
 * Sets both motor PWM outputs to 0.
 */
void stopMotors(void)
{
    OCR1A = 0;
    OCR1B = 0;
}

/*
 * clearLEDs()
 * Turns all LEDs off.
 */
void clearLEDs(void)
{
    PORTA &= ~(1 << Red_LED);
    PORTA &= ~(1 << Green_LED);
    PORTA &= ~(1 << Blue_LED);
    PORTA &= ~(1 << Yellow_LED);
}

/* =========================================================
   INTERRUPTS
   ========================================================= */

/*
 * USART receive interrupt
 * Runs whenever a Bluetooth character is received.
 *
 * 'Z' = request mode switch
 * anything else = store as latest command
 */
ISR(USART0_RX_vect)
{
    char received = UDR0;

    if (received == 'Z')
    {
        mode_switch_request = 1;
    }
    else
    {
        Rx_Char = received;
    }
}

/*
 * External interrupt INT0
 * Keep this ISR short.
 * Only raise a flag here.
 */
ISR(INT0_vect)
{
    int0_request = 1;
}

/* =========================================================
   MAIN
   ========================================================= */

int main(void)
{
    unsigned int delayTime;

    /* ---------- Ultrasonic pins ---------- */
    DDRB |= (1 << TRIG_PIN) | (1 << BUZZER_PIN);   // outputs
    DDRB &= ~(1 << ECHO_PIN);                      // input

    PORTB |= (1 << TRIG_PIN);                      // TRIG idle high
    PORTB &= ~(1 << BUZZER_PIN);                   // buzzer off

    /* ---------- Motor pins ---------- */
    DDRD |= (1 << Motor_1_PWM) | (1 << Motor_2_PWM);
    DDRC |= (1 << DIR_1) | (1 << DIR_2);

    /* ---------- IR sensor pins ---------- */
    DDRD &= ~((1 << IR_Left) | (1 << IR_Right));   // inputs

    /* ---------- LEDs ---------- */
    DDRA |= (1 << Green_LED) | (1 << Red_LED) | (1 << Blue_LED) | (1 << Yellow_LED);
    clearLEDs();

    /* ---------- PWM setup (Timer1, 8-bit fast PWM) ---------- */
    OCR1A = Width;
    OCR1B = Width;

    TCCR1A = (1 << WGM10) | (1 << COM1A1) | (1 << COM1B1);
    TCCR1B = (1 << WGM12) | (1 << CS10);

    /* ---------- External interrupt INT0 ---------- */
    EICRA |= (1 << ISC01);     // falling edge on INT0
    EICRA &= ~(1 << ISC00);
    EIMSK |= (1 << INT0);      // enable INT0

    /* ---------- UART setup ---------- */
    UBRR0 = Baud_Register_Value;
    UCSR0B = (1 << RXEN0) | (1 << RXCIE0);         // RX enable + RX interrupt
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);        // 8-bit data

    /* ---------- Global interrupts ---------- */
    sei();

    while (1)
    {
        /* Measure distance first, then calculate beep delay */
        distance = measureDistance();
        delayTime = getBeepDelay(distance);

        /* Buzzer behavior */
        if (delayTime == 0)
        {
            _delay_ms(100);
        }
        else
        {
            beep(delayTime);
        }

        /* If obstacle is very close, stop motors */
        if (distance <= 5)
        {
            stopMotors();
        }

        /* Handle external interrupt request */
        if (int0_request)
        {
            int0_request = 0;

            /* You can place extra event-based behavior here if needed.
               For now we just stop if obstacle is too close. */
            if (distance <= 5)
            {
                stopMotors();
            }
        }

        /* Handle mode switching request from Bluetooth */
        if (mode_switch_request)
        {
            mode_switch_request = 0;

            if (current_mode == Joystick_Mode)
            {
                current_mode = Line_Follow_Mode;
            }
            else
            {
                current_mode = Joystick_Mode;
            }
        }

        /* ---------------- Joystick mode ---------------- */
        if (current_mode == Joystick_Mode)
        {
            /* only allow movement if obstacle is not too close */
            if (distance <= 5)
            {
                clearLEDs();
                stopMotors();
            }
            else
            {
                OCR1A = Width;
                OCR1B = Width;

                if (Rx_Char == 'F')
                {
                    clearLEDs();
                    PORTA |= (1 << Red_LED);
                    forward();
                }
                else if (Rx_Char == 'B')
                {
                    clearLEDs();
                    PORTA |= (1 << Green_LED);
                    backward();
                }
                else if (Rx_Char == 'L')
                {
                    clearLEDs();
                    PORTA |= (1 << Blue_LED);
                    left();
                }
                else if (Rx_Char == 'R')
                {
                    clearLEDs();
                    PORTA |= (1 << Yellow_LED);
                    right();
                }
                else
                {
                    clearLEDs();
                    stopMotors();
                }
            }
        }

        /* ---------------- Line follow mode ---------------- */
        else if (current_mode == Line_Follow_Mode)
        {
            if (distance <= 5)
            {
                stopMotors();
            }
            else
            {
                OCR1A = lf_Width;
                OCR1B = lf_Width;

                /* Both sensors see black -> forward */
                if (!(PIND & ((1 << IR_Left) | (1 << IR_Right))))
                {
                    forward();
                }
                /* Left sees black -> turn left */
                else if (!(PIND & (1 << IR_Left)))
                {
                    left();
                }
                /* Right sees black -> turn right */
                else if (!(PIND & (1 << IR_Right)))
                {
                    right();
                }
                /* Both see white -> stop */
                else if (PIND & ((1 << IR_Left) | (1 << IR_Right)))
                {
                    stopMotors();
                }
            }
        }
    }
}