/*
 * BT Transmit ISR.c
 *
 * Created: 11/03/2026 10:38:58
 * Author : ZEBED & Jeremy
 */ 

#define F_CPU 20E6
#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include <stdio.h>
#include <avr/interrupt.h>

#define Compare 62499								// Compare value for 0.2s


#define Green_LED PB4								// Green LED is on PortB bit 4
#define Blue_LED PD4								// Blue LED is on PortD bit 4
#define Mode_Switch_Btn PD2							// Mode Switch Button is on PORTD bit 2
volatile int ADC_Result; // Declaring ADC Result as volatile
volatile int ADC_Result_X = 0;
volatile int ADC_Result_Y = 0;

char UART_Buffer[30];

//Define Baud rate, UL tells complier to treat value as long unisgned integer, 32 bits
// This is needed for calculation that follows so the compiler doesnt default to 16-bits
#define Baud_Rate 9600UL
#define Baud_Register_Value ((F_CPU / (16 * Baud_Rate))-1)

int ADC_Conversion()  // ADC conversion function
{
	ADCSRA |= 1<<ADSC;         // ADSC (starts conversion) bit set
	while(ADCSRA & 1<<ADSC);   // As long as ADSC high, stay in while loop
	return(ADC);
}

void Transmit_Character(char C)
{
	while((UCSR0A & 1<<UDRE0) == 0);							// Wait for Data Register to be empty
	UDR0 = C;													// Transmit the character passed to the function
}

void Transmit_String(char String[])
{
	uint8_t String_Length = strlen(String);						// Obtain string's length
	for (uint8_t Index = 0; Index < String_Length; Index++)		// Loop through the string
	{
		Transmit_Character(String[Index]);						// Transmit a character
	}
}

void ADC_Select_PA2()
{
	// Select PA2 (ADC2) as input
	ADMUX = (1 << REFS0) | (1 << MUX1); // MUX1 set for PA2 (ADC2)
}

void ADC_Select_PA3()
{
	// Select PA3 (ADC3) as input
	ADMUX = (1 << REFS0) | (1 << MUX1) | (1 << MUX0); // MUX1 + MUX2 set for PA3 (ADC3)
}

int main(void)
{
	UBRR0 = Baud_Register_Value;			// Set UART's baud to 9600 with F_CPU = 20 MHz
	UCSR0B = 1<<TXEN0;						// Transmitter enable
	
	// ADC initialisation
	ADMUX = 1<<REFS0 | 1<<MUX1;              // Sets Reference voltage and input to the ADC
	ADCSRA = 1<<ADEN | 1<<ADPS0 | 1<<ADPS1 | 1<<ADPS2;  // Enables ADC and sets prescale
	DIDR0 = 0xFF;                            // Sets register DIDR off
	
	DDRD |= 1<<Blue_LED; 							// Pin with blue LED an output
	DDRD &= ~(1 << Mode_Switch_Btn);   // PD2 as input
	PORTD |=  (1 << Mode_Switch_Btn);  // enable pull-up 

	OCR1A = Compare;								// Load compare value
	TCCR1B	= 1<<WGM12 | 1<<CS11 | 1<<CS10;			// CTC mode, prescale c/64
	TIMSK1 = 1<<OCIE1A;								// Enable output Compare Interrupts A
	
	sei();											// Enable global interrupts
	
	while (1)
	{
		
	}
}

ISR(TIMER1_COMPA_vect)
{
	PORTD ^= 1<<Blue_LED;							// Toggle Blue LED
	// Read joystick x values
	ADC_Select_PA2();					// change ADC to read x values
	ADC_Conversion();					// throwaway conversion
	ADC_Result_X = ADC_Conversion();	// store x value

	// Read joystick y values
	ADC_Select_PA3();					// change ADC to read y values
	ADC_Conversion();					// throwaway conversion
	ADC_Result_Y = ADC_Conversion();	// store y value
	
	
	if (ADC_Result_X > 700)				// Joystick pushed forwards
	{
		Transmit_Character('F');
	}
	else if (ADC_Result_X < 300)		// Joystick pushed backwards
	{
		Transmit_Character('B');
	}
	else if (ADC_Result_Y > 700)		// Joystick pushed left
	{
		Transmit_Character('L');
	}
	else if (ADC_Result_Y < 300)		// Joystick pushed right
	{
		Transmit_Character('R');
	}
	else if (!(PIND & ( 1<< Mode_Switch_Btn)) )
	{
		Transmit_Character('Z');
	}
	else
	{
		Transmit_Character('s');
	}
}

