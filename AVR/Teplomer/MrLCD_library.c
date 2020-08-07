/*
 * MrLCD_library.c
 *
 * Created: 28.8.2014 10:23:38
 *  Author: John.Zoidberg
 */ 
#include "MrLCD_library.h"
#include <avr/pgmspace.h>

#define LCD_SECOND_ROW 64

/* ================ DEFINICE MAKER ===================================== */
/* zavrti pinem E pro LCD aby si precetl prichozi data */
#define SEND	LCD_PORT |= (1 << LCD_PIN_E); asm volatile("nop"); LCD_PORT &= ~(1 << LCD_PIN_E);

#define setBit(PORT, PIN) PORT |= (1<<PIN);
#define resBit(PORT, PIN) PORT &= ~(1<<PIN);
#define tstBit(PORT, PIN) PORT & (1<<PIN)


void LCD_init(LCD_INIT_DISPLAY mod)
{
	LCD_DDR = (1<<LCD_PIN_DB4) | (1<<LCD_PIN_DB5) | (1<<LCD_PIN_DB6) | (1<<LCD_PIN_DB7) | (1<<LCD_PIN_E) | (1<<LCD_PIN_RS) | (1<<LCD_PIN_RW);
	
	/* pocatecni zresetovani LCD, zde neni mozne cist BUSSY FLAG, nutne pouzit fce delay() */
	LCD_PORT = (1 << LCD_PIN_DB4) | (1 << LCD_PIN_DB5);
	_delay_ms(16); // Wait for more than 15 ms after VCC rises to 4.5 V
	SEND
	_delay_ms(5); // Wait for more than 4.1 ms
	SEND
	_delay_us(200); // Wait for more than 100 µs
	SEND
	
	/* inicializace LCD */
	LCD_PORT = (1 << LCD_PIN_DB5); // Set interface to be 4 bits long
	SEND
	//_delay_ms(2);
	LCD_send(0b00101000, 0); // commmand: 4 bit mode, 2 lines, 5*8 font
	//_delay_ms(2);
	LCD_send(0b00000001, 0); // Display off
	//_delay_ms(2);
	LCD_clear();			 // Display clear
	//_delay_ms(2);
	LCD_send(0b00000110,0);  // Entry mode set: increase cursor position, scroll display OFF
	//_delay_ms(2);
	LCD_send(mod,0);		// zapne dislay a cursor podle volby
	
	_delay_ms(2);
}

void LCD_check_bussy()
{
	LCD_PORT = 0x00;
	/* When RS = 0 and R/W = 1, the busy flag is output to DB7 */
	LCD_DDR &= ~(1<<LCD_PIN_DB7);	// set BD7 as input (for read)
	LCD_PORT &= ~(1<<LCD_PIN_RS);	
	LCD_PORT |= (1<<LCD_PIN_RW);
	SEND;	
	
	//uint8_t volatile pom = LCD_PIN & (1<<LCD_PIN_DB7);
	/* okoukano uz nevim odkum, misto while cyklu pouziju for -- program se nezacykly pri nepripojenym nebo vadnym display */
	// na 16 MHz ... 62,5 ns, max overovaci zpozdeni cca 2ms => 32 000 cyklu
	for(register int i = 0; i < 32000; i++)
	{
		SEND;
		if((LCD_PIN & (1<<LCD_PIN_DB7) || LCD_PORT & (1<<LCD_PIN_DB7)) == 0)
			break;
	}
	/*
	while(LCD_PIN & (1<<LCD_PIN_DB7) || LCD_PORT & (1<<LCD_PIN_DB7)) // checking BF
	{
		SEND;
	}
	*/
	LCD_DDR |= (1<<LCD_PIN_DB7); // set back to output
}

void LCD_send(uint8_t data, uint8_t typ)
{
	LCD_check_bussy();
	
	//LCD_PORT = 0x00; // vynulovani vsech bitu
	if(typ == 0)
	{
		//resBit(LCD_PORT, LCD_PIN_RS); resBit(LCD_PORT, LCD_PIN_RW);
		
		/* posle horni nibble */
		/*if(data & 0b10000000) LCD_PORT |= 0b10000000;//setBit(LCD_PORT, LCD_PIN_DB7);
		else
			LCD_PORT =0; 
		if(data & 0b01000000) LCD_PORT |= 0b01000000;//setBit(LCD_PORT, LCD_PIN_DB6);
		else
			LCD_PORT =0; 
		if(data & 0b00100000) LCD_PORT |= 0b00100000;//setBit(LCD_PORT, LCD_PIN_DB5);
		else
			LCD_PORT =0; 
		if(data & 0b00010000) LCD_PORT |= 0b00010000;//setBit(LCD_PORT, LCD_PIN_DB4);
		else
			LCD_PORT =0; 
		SEND
		*/
		/*
		if(data & 0x80)
			LCD_PORT |=1;
		else asm("nop");
		if(data & 0x40)
			LCD_PORT |=1;
		else asm("nop");
		if(data & 0x20>0)
			LCD_PORT |=1;
		else asm("nop");
		if(data & 0x10)
			LCD_PORT |=1;
		else asm("nop");
		*/
		/*
		LCD_PORT |= (data & 0x80) ? (1<<LCD_PIN_DB7) : 0x00;
		LCD_PORT |= (data & 0x40) ? (1<<LCD_PIN_DB6) : 0x00;
		LCD_PORT |= (data & 0x20) ? (1<<LCD_PIN_DB5) : 0x00;
		LCD_PORT |= (data & 0x10) ? (1<<LCD_PIN_DB4) : 0x00;	
		SEND
		
		/* posle spodni nibble *
		LCD_PORT = 0x00; // vynuluj dateove bity, pro posilani prikazu maji byt i RS a RW v nule
		/*
		if(tstBit(data, 0x08)) setBit(LCD_PORT, LCD_PIN_DB7);
		if(tstBit(data, 0x04)) setBit(LCD_PORT, LCD_PIN_DB6);
		if(tstBit(data, 0x02)) setBit(LCD_PORT, LCD_PIN_DB5);
		if(tstBit(data, 0x01)) setBit(LCD_PORT, LCD_PIN_DB4);
		*
		LCD_PORT |= (data & 0x08) ? (1<<LCD_PIN_DB7) : 0x00;
		LCD_PORT |= (data & 0x04) ? (1<<LCD_PIN_DB6) : 0x00;
		LCD_PORT |= (data & 0x02) ? (1<<LCD_PIN_DB5) : 0x00;
		LCD_PORT |= (data & 0x01) ? (1<<LCD_PIN_DB4) : 0x00;
		SEND
		*/
		LCD_PORT &= ~(1<<LCD_PIN_RS);
	}
	else
	{		
		LCD_PORT |= (1<<LCD_PIN_RS);
	}
	LCD_PORT &= ~(1<<LCD_PIN_RW);
	
	LCD_PORT &= ~((1<<LCD_PIN_DB4)|(1<<LCD_PIN_DB5)|(1<<LCD_PIN_DB6)|(1<<LCD_PIN_DB7));
	
	LCD_PORT |= (data & 0x80) ? (1<<LCD_PIN_DB7) : 0x00;
	LCD_PORT |= (data & 0x40) ? (1<<LCD_PIN_DB6) : 0x00;
	LCD_PORT |= (data & 0x20) ? (1<<LCD_PIN_DB5) : 0x00;
	LCD_PORT |= (data & 0x10) ? (1<<LCD_PIN_DB4) : 0x00;	
	SEND
		
	/* posle spodni nibble */
	// vynuluj dateove bity, pro posilani prikazu maji byt i RS a RW v nule
	LCD_PORT &= ~((1<<LCD_PIN_DB4)|(1<<LCD_PIN_DB5)|(1<<LCD_PIN_DB6)|(1<<LCD_PIN_DB7));

	LCD_PORT |= (data & 0x08) ? (1<<LCD_PIN_DB7) : 0x00;
	LCD_PORT |= (data & 0x04) ? (1<<LCD_PIN_DB6) : 0x00;
	LCD_PORT |= (data & 0x02) ? (1<<LCD_PIN_DB5) : 0x00;
	LCD_PORT |= (data & 0x01) ? (1<<LCD_PIN_DB4) : 0x00;
	SEND
}

void LCD_clear()
{
	LCD_send(0x01,0);
}

void LCD_home()
{
	LCD_send(0x02, 0);
}

void LCD_printChar(char ch)
{
	LCD_send(ch, 1);
}

void LCD_printString(char *str)
{
	while(*str > 0)
		LCD_printChar(*str++);
}

void LCD_printString_PROGMEM(char* pgmstr)
{
	char pom;
	while((pom=pgm_read_byte(pgmstr++)) > 0)
	{
		LCD_printChar(pom);
	}
}

void LCD_printInt(int cislo, uint8_t pocetCislic, uint8_t soustava)
{
	char pom[pocetCislic];
	itoa(cislo, pom, soustava);
	LCD_printString(pom);
}

void LCD_gotoXY(uint8_t x, uint8_t y)
{
	// data bits: D7 D6 D5 D4 D3 D2 D1 D0
	//			  1  -- ADRESA v DDRAM --
	LCD_send(0x80 | y*LCD_SECOND_ROW | x,0);
}

void LCD_vlastniZnak(char *pixels, uint8_t pozice, LCD_VLASTNI_ZNAK_ODKUD ulozenoV)
{
	// posli prikaz k zapisu do CG-RAM na danou pozici
	LCD_send(0x40 | pozice*8, 0);
	
	if(ulozenoV == RAM)
	{			
		for(uint8_t i = 0; i < 8; i++)		
			LCD_send(*pixels++, 1);				
	}
	
	else if(ulozenoV == PROGAM_MEMORY)
	{
		for(uint8_t i = 0; i < 8; i++)		
			LCD_send(pgm_read_byte(pixels++), 1);		
	}
	LCD_home();
}

