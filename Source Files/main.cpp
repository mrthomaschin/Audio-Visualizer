
/*
 * 120b_custom_proj.cpp
 *
 * Created: 3/5/2019 12:28:38 PM
 * Author : User
 */ 

#include <avr/io.h>
#include <math.h>
#include "io.c"

//Custom Characters
unsigned char basicBar[8] = {0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111};
unsigned char smileyFace[8] = {0b00000, 0b00000, 0b01010, 0b00000, 0b10001, 0b01110, 0b00000, 0b00000};
unsigned char upsideDownSmiley[8] = {0b00000, 0b00000, 0b01110, 0b10001, 0b00000, 0b01010, 0b00000, 0b00000};
unsigned char musicNote[8] = {0b00100, 0b00110, 0b00101, 0b00101, 0b00100, 0b01100, 0b01100, 0b00000};
unsigned char empty[8] = {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000};
	
//Global Variables
unsigned short ADCshort;
unsigned char btn = 0x00;
volatile unsigned char TimerFlag = 0; // TimerISR()
int counter = 0;

// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks

enum States {start, barChar, smileyChar, upsideChar, musicChar, release} state;
	
void TimerOn() {
	// AVR timer/counter controller register TCCR1
	TCCR1B = 0x0B;// bit3 = 0: CTC mode (clear timer on compare)
	// bit2bit1bit0=011: pre-scaler /64
	// 00001011: 0x0B
	// SO, 8 MHz clock or 8,000,000 /64 = 125,000 ticks/s
	// Thus, TCNT1 register will count at 125,000 ticks/s

	// AVR output compare register OCR1A.
	OCR1A = 125;	// Timer interrupt will be generated when TCNT1==OCR1A
	// We want a 1 ms tick. 0.001 s * 125,000 ticks/s = 125
	// So when TCNT1 register equals 125,
	// 1 ms has passed. Thus, we compare to 125.
	// AVR timer interrupt mask register
	TIMSK1 = 0x02; // bit1: OCIE1A -- enables compare match interrupt

	//Initialize avr counter
	TCNT1=0;

	_avr_timer_cntcurr = _avr_timer_M;
	// TimerISR will be called every _avr_timer_cntcurr milliseconds

	//Enable global interrupts
	SREG |= 0x80; // 0x80: 1000000
}

void TimerOff() {
	TCCR1B = 0x00; // bit3bit1bit0=000: timer off
}

void TimerISR() {
	TimerFlag = 1;
}

ISR(TIMER1_COMPA_vect) {
	// CPU automatically calls when TCNT1 == OCR1 (every 1 ms per TimerOn settings)
	_avr_timer_cntcurr--; // Count down to 0 rather than up to TOP
	if (_avr_timer_cntcurr == 0) { // results in a more efficient compare
		TimerISR(); // Call the ISR that the user uses
		_avr_timer_cntcurr = _avr_timer_M;
	}
}

void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}
	
void LoadCustomChars (unsigned char location, unsigned char *customChar) //Credits to Aswinth Raj on CircuitDigest
{
	unsigned char i;
	if(location < 8)
	{
		LCD_WriteCommand (0x40 + (location * 8));	/* Command 0x40 and onwards forces the device to point CGRAM address */
		
		for(i = 0; i < 8; i++)	/* Write 8 byte for generation of 1 character */
			LCD_WriteData(customChar[i]);
	}
}

void ADC_init() {
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
	// ADEN: setting this bit enables analog-to-digital conversion.
	// ADSC: setting this bit starts the first conversion.
	// ADATE: setting this bit enables auto-triggering. Since we are
	//        in Free Running Mode, a new conversion will trigger whenever
	//        the previous conversion completes.
}

void DisplayLCD(unsigned value, unsigned customChar)
{
	for(unsigned x = 2; x <= 15; x++)
	{
		LCD_Cursor(x);
		if(x <= value)
			LCD_WriteData(customChar);
		else
			LCD_WriteData(7);
			
		LCD_Cursor(x + 16);
		if(x <= value)
			LCD_WriteData(customChar);
		else
			LCD_WriteData(7);
			
		LCD_Cursor(16);
	}
	
}

void CalculateLevel(unsigned customChar)
{
	ADCshort = ADC * 7; // PA0
	unsigned int increment = 1023 / 14;

	if (ADCshort < increment)
	{
		DisplayLCD(1, customChar);
	}
	else
	{
		for(int x = 1; x < 15; x++)
		{
			if(ADCshort >= x * increment && ADCshort < (x + 1) * increment)
			{
				DisplayLCD(x + 1, customChar);
			}
		}
	}
}

/*******STATE MACHINE**********/
	
void Tick()
{
	btn = ~PINB & 0x01;
	switch(state) //Transitions
	{
		case start:
			state = release;
			break;
				
		case barChar:
			counter = 1;
			if(btn)
			{
				state = barChar;
			}
			else
				if(!btn)
				{	
					state = release;
				}
			break;
				
		case smileyChar:
			counter = 2;
			if(btn)
			{
				state = smileyChar;
			}
			else
				if(!btn)
				{
					state = release;
				}
			break;
				
		case upsideChar:
			counter = 3;
			if(btn)
			{
				state = upsideChar;
			}
			else
				if(!btn)
					state = release;
			break;
			
		case musicChar:
			counter = 4;
			if(btn)
			{
				state = musicChar;
			}
			else
				if(!btn)
					state = release;
			break;
						
		case release:
			if(btn)
			{
				if(counter == 0)
				{
					state = barChar;
				}
				else
					if(counter == 1)
					{
						state = smileyChar;
					}
					else
						if(counter == 2)
						{
							state = upsideChar;
						}
						else
							if(counter == 3)
							{
								state = musicChar;
							}
							else
								if(counter == 4)
								{
									state = barChar;
								}
			}
			else
			{
				if(counter == 0)
					CalculateLevel(0);
				else
					if(counter == 1)
						CalculateLevel(1);
					else
						if(counter == 2)
							CalculateLevel(2);
						else
							if(counter == 3)
								CalculateLevel(3);
							else
								if(counter == 4)
									CalculateLevel(0);
								
				state = release;
			}
			break;
		default:
			break;

	}
	
	switch(state) //Actions
	{
		case start:
			LCD_Cursor(16);
			break;
			
		case barChar:
			LCD_Cursor(16);
			break;
			
		case smileyChar:
			LCD_Cursor(16);
			break;
			
		case upsideChar:
			LCD_Cursor(16);
			break;
			
		case musicChar:
			LCD_Cursor(16);
			break;
			
		case release:
			break;
	}
		
}
int main(void)
{
	DDRA = 0x00; PORTA = 0xFF;
	DDRB = 0x00; PORTB = 0xFF;
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	LCD_init();
	TimerSet(70);
	TimerOn();
	
	//Load custom characters to CGRAM
	LoadCustomChars(0, basicBar);
	LoadCustomChars(1, smileyFace);
	LoadCustomChars(2, upsideDownSmiley);
	LoadCustomChars(3, musicNote);
	LoadCustomChars(7, empty);
	
	LCD_ClearScreen();
	LCD_Cursor(1);
	
	LCD_Cursor(1);
	LCD_WriteData(1 + '0');
	
	LCD_Cursor(17);
	LCD_WriteData(2 + '0');
	
	LCD_Cursor(2);
	
	state = start;
	ADC_init();
	while(1) {
		Tick();
		while (!TimerFlag);
			TimerFlag = 0;
	}
}