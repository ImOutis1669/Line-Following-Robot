/*
 * DC Motor control.c
 *
 * Created: 09/12/2025 16:09:52
 * Author : ZEBED
 */ 

/* This code controls a DC motor using an L293D on ATMega 644PA. */



#define F_CPU 20E6
#include <avr/io.h>
#include <util/delay.h>

#define Motor_PWM PD5			// PWM output on OC1A (PD5)
#define Motor_direction1 PC0	// Motor direction control on PC0
#define Motor_direction2 PC1	// Motor direction control on PC1

int Width = 255;				// Sets PWM width - controlling motor speed


int main(void)
{
	// Make L293D pins outputs
	DDRD = 1<<Motor_PWM;
	DDRC = 1<<Motor_direction1 | 1<<Motor_direction2;
	
	// Load the width
	OCR1A = Width;


	// Settings for 8-bit fast PWM, no prescale and set Blue LED to non-invertingPWM
	TCCR1A = 1<<WGM10 | 1<<COM1A1; // sets COM1A1 to non inverting
	TCCR1B = 1<<WGM12 | 1<<CS10;



	while (1)
	{
		// Spin clockwise for 500ms 
		PORTC |= 1<<Motor_direction1;
		PORTC &= ~(1<<Motor_direction2);
		_delay_ms(500);
		// Spin counter-clockwise for 500ms
		PORTC |= 1<<Motor_direction2;
		PORTC &= ~(1<<Motor_direction1);
		_delay_ms(500);
	}
}

