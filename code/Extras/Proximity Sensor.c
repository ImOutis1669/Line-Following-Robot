/*
 * Proximity Sensor.c
 *
 * Created: 17/03/2026 19:46:51
 * Author : Outis
 */ 

#define F_CPU 20E6
#include <avr/io.h>
#include <util/delay.h>

#define TRIG_PIN PB3						// Trigger Pin on PORT B, Pin 3
#define ECHO_PIN PB4						// Echo Pin on PORT B, Pin 4
#define BUZZER_PIN PB1						// Buzzer Pin on PORT B, Pin 1

#define TIME_PER_CM 50						// Converts time into distance for the UMR8, Every 50 time units = 1 cm
#define TIMEOUT 50000						// At 50 milliseconds the program stops if the sensor is taking too long to recieve a pulse back
#define MAX_DISTANCE 15						// Maximum range set for the Ultrasonic Sensor, anything further than 15 cm would not be read

unsigned long measurePulse()				// Function measures how long echo takes to receive input
{
	unsigned long time = 0;

	// Sends out a short trigger pulses
	PORTB &= ~(1 << TRIG_PIN);	
	_delay_us(2);
	PORTB |= (1 << TRIG_PIN);

	while (PINB & (1 << ECHO_PIN))
	{
		_delay_us(1);
		time++;
		 if (time >= TIMEOUT)
		{
			return TIMEOUT;					// if time reaches TIMEOUT (50 millisecs), it stops
		}
	}
	time = 0;
	while (!(PINB & (1 << ECHO_PIN)))		
	{										
		_delay_us(1);
		time++;
		if (time >= TIMEOUT)
		{
			return TIMEOUT;					// if time reaches TIMEOUT (50 millisecs), it stops
		}
	}
	return time;
}
	


unsigned int measureDistance()				// This function uses how long Echo takes to get High (measurePulse) to measure distance
{
	unsigned long pulseTime = measurePulse();

	if (pulseTime >= TIMEOUT)
	{
		return MAX_DISTANCE;				// If Echo doesn't turn high in 50 millisecs, assume Max Distance has been reached
	}

	return pulseTime / TIME_PER_CM;			// otherwise covert time into distance
}

unsigned int getBeepDelay(unsigned int distance)
{
	unsigned int fast = 60;
	unsigned int slow = 700;
	unsigned long gap;
	unsigned long delayTime;				// Determines how quickly buzzer should beep, close obj -> small delay -> fast beeping
											// far obj -> big delay -> slow beeping
	if (distance >= MAX_DISTANCE)
	{
		return 0;							// If the obj is at max distance or more, don't beep
	}

	if (distance < 1)						// Ensures distance never becomes 0
	{
		distance = 1;
	}

	gap = slow - fast;						// Scales the beep delay based on distance
	delayTime = fast + (gap * (distance - 1)) / (MAX_DISTANCE - 1);

	return delayTime;
}

void waitMs(unsigned int ms)				// Custom millisecond wait function which allows for if waitMs(100), it does 100x 1 millisecond delays
{
	while (ms > 0)
	{
		_delay_ms(1);
		ms--;
	}
}

void beep(unsigned int delayTime)			// Triggers on and off the buzzer based on half the delay so if delayTime = 200, on for 100ms, off for 100ms
{
	PORTB |= (1 << BUZZER_PIN);
	waitMs(delayTime / 2);

	PORTB &= ~(1 << BUZZER_PIN);
	waitMs(delayTime / 2);
}

int main()
{
	unsigned int distance;					// measured distance
	unsigned int delayTime;					// Time b/n beeps

	DDRB |= (1 << TRIG_PIN) | (1 << BUZZER_PIN);
	DDRB &= ~( 1<< ECHO_PIN);
	PORTB |= (1 << TRIG_PIN);
	PORTB &= ~(1 << BUZZER_PIN);

	while (1)
	{
		distance = measureDistance();
		delayTime = getBeepDelay(distance);

		if (delayTime == 0)
		{
			_delay_ms(100);
		}
		else
		{
			beep(delayTime);
		}
	}
}