// Visual Micro is in vMicro>General>Tutorial Mode
// 
/*
    Name:       dmxconvert.ino
    Created:	08.04.2019 0:23:31
    Author:     Den-ПК\Den Ivanov D.M.
*/

// Define User Types below here or use a .h file
//
#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define F_OSC 16000000 // Clock frequency
#define BAUD_RATE 250000

#define MOSFET 1
#define RELAY 2
/*
Here you have to select the output mode accordingly to the receiver type you are using.
 Choose MOSFET or RELAY for OUTPUT_MODE
 */
#define OUTPUT_MODE RELAY
#define OUTPUT_MODE MOSFET
#define THR 20 //threshold for analogRead

#if OUTPUT_MODE == RELAY

#define PWMpin4 6 // PWM OUT4
#define PWMpin1 10 // PWM OUT1
#define PWMpin2 9 // PWM OUT2
#define PWMpin3 5 // PWM OUT3

#else

#define PWMpin1 6 // PWM OUT1
#define PWMpin4 10 // PWM OUT4
#define PWMpin3 9 // PWM OUT3
#define PWMpin2 5 // PWM OUT2

#endif


// Define Function Prototypes that use User Types below here or use a .h file
//


// Define Functions below here or use other .ino or cpp files
//
#define DE 2
#define LED 7
#define SerialTxControl 3   //RS485 управляющий контакт на arduino pin 10
#define RS485Transmit    HIGH
#define RS485Receive     LOW 
enum {
	BREAK, STARTB, STARTADD, DATA
};

volatile unsigned int dmxStatus;
volatile unsigned int dmxStartAddress;
volatile unsigned int dmxCount = 0;
volatile unsigned int ch1, ch2, ch3, ch4;
volatile unsigned int MASTER;

/*Initialization of USART*/
void init_USART()
{
	UBRR1L = (uint8_t)(F_CPU / (BAUD_RATE * 16L) - 1); //Set Baud rate at 250 kbit/s
	UBRR1H = (F_CPU / (BAUD_RATE * 16L) - 1) >> 8;     // 
	UDR1 = 0;
	UCSR1A = 0;	                        // clear error flags, disable U2X and MPCM
	UCSR1B = (1 << RXCIE1) | (1 << RXEN1);	// Enable receiver
	UCSR1C = (1 << USBS1) | (3 << UCSZ10);	// 8bit 2 stop
}

// The setup() function runs once each time the micro-controller starts
void setup()
{
	//control LED setup
	pinMode(DE, OUTPUT); // enable Tx Rx
	pinMode(LED, OUTPUT); // led DMX
	pinMode(SerialTxControl, OUTPUT);
	digitalWrite(SerialTxControl, RS485Receive);
	//start DMX receive
	init_USART();

}

// Add the main program code into the continuous loop() function
void loop()
{

	volatile unsigned int address1, address2, address3, address4, address5, address6, address7, address8, address9;
	
	cli(); //disable interrupt
	
	address1 = 1;
	address2 = 0;
	address3 = 0;
	address4 = 0;
	address5 = 0;
	address6 = 0;
	address7 = 0;
	address8 = 0;
	address9 = 0;
	//start to get DMX flow begin from 1 
	/*Calculation of dmxStartAddress*/
	dmxStartAddress = (address1 * 1) + (address2 * 2) + (address3 * 4) + (address4 * 8) + (address5 * 16) + (address6 * 32) + (address7 * 64) + (address8 * 128) + (address9 * 256);

	if (dmxStartAddress == 0) { //If all dipswitches are 0 
		demo();                //call the demo function
	}

	else if (dmxStartAddress >= 509)  //The receiver manages 4 channels, so you can't set a start address above 509;
		dmxStartAddress = 509;

	digitalWrite(DE, LOW);


	sei(); //enable global interrupt

	for (;;) {}


}

//UART processing function
SIGNAL(USART1_RX_vect)
{
	int temp = UCSR1A;
	int dmxByte = UDR1;

	digitalWrite(LED, HIGH);
	if (temp&(1 << DOR1))	// Data Overrun?
	{
		dmxStatus = BREAK;	// wait for reset (BREAK)
		UCSR1A &= ~(1 << DOR1);
		goto tail;
	}

	if (temp&(1 << FE1))	//BREAK or FramingError?
	{

		dmxCount = 0;	// reset byte counter
		dmxStatus = STARTB;	// let's think it's a BREAK ;-) ->wait for start byte
		UCSR1A &= ~(1 << FE1);
		goto tail;
	}


	switch (dmxStatus)
	{
	case STARTB:

		if (dmxByte == 0)  //This is our star byte
		{

			if (dmxStartAddress == 1) dmxStatus = DATA; // the FE WAS a BREAK -> the next byte is our first channel
			else dmxStatus = STARTADD;	// the FE WAS a BREAK -> wait for the right channel
			dmxCount = 1;


		}
		else
		{
			dmxStatus = BREAK;	// wait for reset (BREAK) it was a framing error
		}
		goto tail;
		break;

	case STARTADD:

		if (dmxCount == dmxStartAddress - 1) //Is the next byte channel one?
		{
			dmxStatus = DATA; //Yes, so let's wait the data
		}
		dmxCount++;

		break;


	case DATA:	// HERE YOU SHOULD PROCESS THE CHOSEN DMX CHANNELS!!!
		if (dmxCount == dmxStartAddress)
		{
			ch1 = dmxByte;
			dmxCount++;
		}
		else if (dmxCount == (dmxStartAddress + 1))
		{
			ch2 = dmxByte;
			dmxCount++;
		}
		else if (dmxCount == (dmxStartAddress + 2))
		{

			ch3 = dmxByte;
			dmxCount++;


		}
		else  if (dmxCount == (dmxStartAddress + 3))
		{
			ch4 = dmxByte;
			dmxCount = 1;
			dmxStatus = BREAK;	// ALL CHANNELS RECEIVED

			if (OUTPUT_MODE == MOSFET)        //Mosfet or Relay receiver?
			{
				analogWrite(PWMpin1, ch1);	// update mosfet outputs
				analogWrite(PWMpin2, ch2);
				analogWrite(PWMpin3, ch3);
				analogWrite(PWMpin4, ch4);
			}
			else if (OUTPUT_MODE == RELAY)
			{
				if (ch1 > 0)
				{
					analogWrite(PWMpin1, 255);	// update relay outputs
				}
				else
				{
					analogWrite(PWMpin1, 0);
				}

				if (ch2 > 0)
				{
					analogWrite(PWMpin2, 255);
				}
				else
				{
					analogWrite(PWMpin2, 0);
				}

				if (ch3 > 0)
				{
					analogWrite(PWMpin3, 255);
				}
				else
				{
					analogWrite(PWMpin3, 0);
				}

				if (ch4 > 0)
				{
					analogWrite(PWMpin4, 255);
				}
				else
				{
					analogWrite(PWMpin4, 0);
				}

			}

		}

	}

tail:
	asm("nop");
}


//test demo funtion
void demo()
{
	int bright;
	if (OUTPUT_MODE == MOSFET)
	{
		for (;;)
		{

			for (bright = 0; bright < 255; bright++)	// infinite loop
			{
				analogWrite(PWMpin1, bright);
				analogWrite(PWMpin2, bright);
				analogWrite(PWMpin3, bright);
				analogWrite(PWMpin4, bright);
				delay(10);
			}

			for (bright = 255; bright >= 0; bright--)	// infinite loop
			{
				analogWrite(PWMpin1, bright);
				analogWrite(PWMpin2, bright);
				analogWrite(PWMpin3, bright);
				analogWrite(PWMpin4, bright);
				delay(10);
			}

			analogWrite(PWMpin1, 255);
			analogWrite(PWMpin3, 255);
			delay(10);
			analogWrite(PWMpin1, 0);
			analogWrite(PWMpin3, 0);
			analogWrite(PWMpin2, 255);
			analogWrite(PWMpin4, 255);
			delay(10);
		}
	}
	else if (OUTPUT_MODE == RELAY)
	{
		for (;;)
		{
			analogWrite(PWMpin1, 255);
			analogWrite(PWMpin2, 0);
			analogWrite(PWMpin3, 0);
			analogWrite(PWMpin4, 0);
			delay(1000);
			analogWrite(PWMpin1, 0);
			analogWrite(PWMpin2, 255);
			analogWrite(PWMpin3, 0);
			analogWrite(PWMpin4, 0);

			delay(1000);
			analogWrite(PWMpin1, 0);
			analogWrite(PWMpin2, 0);
			analogWrite(PWMpin3, 255);
			analogWrite(PWMpin4, 0);

			delay(1000);
			analogWrite(PWMpin1, 0);
			analogWrite(PWMpin2, 0);
			analogWrite(PWMpin3, 0);
			analogWrite(PWMpin4, 255);
			delay(1000);
		}
	}
}

