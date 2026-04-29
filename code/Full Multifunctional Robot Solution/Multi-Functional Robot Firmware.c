/*
 * Multi-functional Robot Firmware.c
 *
 * Created: 27/02/2026 17:54:39
 * Author : ZEBED & Jeremy
 */ 

#define F_CPU 20E6
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>
#include <util/delay.h>	
#define Baud_Rate 9600UL
#define Baud_Register_Value ((F_CPU / (16 * Baud_Rate))-1)
#define Red_LED PA0									// Red LED is on Port A, Pin 0
#define Green_LED PA1								// Green LED is on Port A, Pin 1
#define Blue_LED PA2								// Blue LED is on Port A, Pin 2
#define Yellow_LED PA3								// Yellow LED is on Port A, Pin 3

#define Motor_1_PWM PD5			// Motor 1 PWM output on OC1A (PD5) - Speed control
#define Motor_2_PWM PD4			// Motor 2 PWM output on OC1B (PD4) - Speed control
#define DIR_1 PC0				// DIR1 (Motor 1 direction control) on PC0
#define DIR_2 PC1				// DIR1 (Motor 2 direction control) on PC1

#define IR_Left PD2							// IR sensor 1 is on PD2
#define IR_Right PD3						// IR sensor 2 is on PD3

#define Joystick_Mode 0
#define Line_Follow_Mode 1

volatile char Rx_Char = 'S';
volatile int mode_switch_request = 0; // Mode Switch Flag
volatile unsigned int distance;					// measured distance
int current_mode = Line_Follow_Mode;
int Width = 255;				// Sets PWM width - controlling motor speed
int lf_Width = 235;				// Sets PWM width - controlling motor speed
int stop = 0;

// ------ Motor Direction  ------ //
void forward()
{
	PORTC |= 1<<DIR_1;
	PORTC &= ~(1<<DIR_2);
}

void backward()
{
	PORTC |= 1<<DIR_2;
	PORTC &= ~(1<<DIR_1);
}

void left()
{
	PORTC |= 1<<DIR_2;
	PORTC |= 1<<DIR_1;
}

void right()
{
	PORTC &= ~(1<<DIR_2);
	PORTC &= ~(1<<DIR_1);
}

int main (void)
{
	// Make control pins outputs -- Motor Function
	DDRD = 1<<Motor_1_PWM | 1<<Motor_2_PWM;
	DDRC = 1<<DIR_1 | 1<<DIR_2;
	
	// Load the width
	OCR1A = Width;
	OCR1B = Width;
	
	// Settings for 8-bit fast PWM, no prescale and set Blue LED to non-inverting PWM
	TCCR1A = 1<<WGM10 | 1<<COM1A1 | 1<<COM1B1; // sets COM1A1 to non inverting
	TCCR1B = 1<<WGM12 | 1<<CS10;
	
	UBRR0 = Baud_Register_Value;			// Set UART's baud to 9600 with F_CPU = 20 MHz]
	UCSR0B = 1<<RXEN0 | 1<<RXCIE0;			// Receive enable, receive interrupt enable
	UCSR0C = 1<<UCSZ01 | 1<<UCSZ00;		// 8-bit data format
	
	DDRA = 1<<Green_LED | 1<<Red_LED | 1<<Blue_LED | 1<<Yellow_LED;		// Pin with green and red LED an output

	sei();                                   // Enable global interrupts
	
	while (1)
	{		
		if (mode_switch_request)
		{
			mode_switch_request = 0; // Clears the flag

			if (current_mode == Joystick_Mode)
			{
				current_mode = Line_Follow_Mode;
			}
			else
			{
				current_mode = Joystick_Mode;
			}
		}

		// Normal Operation
		if (current_mode == Joystick_Mode)
		{
			OCR1A = Width;
			OCR1B = Width;

			if (Rx_Char == 'F')
			{
				PORTA |= 1<<Red_LED;
				forward();
			}
			else if (Rx_Char == 'B')
			{
				PORTA |= 1<<Green_LED;
				backward();
			}
			else if (Rx_Char == 'L')
			{
				PORTA |= 1<<Blue_LED;
				left();
			}
			else if (Rx_Char == 'R')
			{
				PORTA |= 1<<Yellow_LED;
				right();
			}
			else
			{
				PORTA &= ~(1<<Red_LED);
				PORTA &= ~(1<<Green_LED);
				PORTA &= ~(1<<Blue_LED);
				PORTA &= ~(1<<Yellow_LED);
				OCR1A = 0;
				OCR1B = 0;
			}
		}
		else if (current_mode == Line_Follow_Mode)
		{
			// Load the width
			OCR1A = lf_Width;
			OCR1B = lf_Width;

			if (!(PIND & (1<<IR_Left | 1<<IR_Right)))
			{
				// Both IR sees black -> go forwards
				forward();
			}
			else if (!(PIND & (1<<IR_Left)))
			{
				// Left = Black, Right = White, go left
				left();
			}
			else if (!(PIND & (1<<IR_Right)))
			{
				// Left = White, Right = Black, go right
				right();
			}

			if ((PIND & (1<<IR_Left | 1<<IR_Right)))
			{
				OCR1A = stop;
				OCR1B = stop;
			}
		}
	
	}
}
ISR(USART0_RX_vect)
{
	 char recieved = UDR0;
	 if (recieved == 'Z')
	{
		mode_switch_request = 1; // flag raised
	}
	else 
	{
		Rx_Char = recieved;
	}
}
