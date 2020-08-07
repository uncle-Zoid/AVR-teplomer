/*
 * OneWireInterface.c
 *
 * Created: 2.9.2014 13:44:50
 *  Author: John.Zoidberg
 */ 
#include "OneWireInterface.h"

#define setbit(port,bit) (port |= (1<<bit));
#define resbit(port,bit) (port &= ~(1<<bit));
#define testbit(port, bit) (port & (1<<bit))



#define OW_VYSTUPNI_PORT  PORTA
#define OW_VSTUPNI_PORT   PINA
#define OWDDR             DDRA
#define OWPIN             1

#define CTI   resbit(OWI_DDR,OWI_PIN);
#define VYSLI setbit(OWI_DDR,OWI_PIN);

uint8_t OWI_reset()
{	
	uint8_t presence = 0x00;
	
	cli();
	OWI_DDR |= (1 << OWI_PIN); // nastav pin jako vystupni
	OWI_PORT_OUT |= (1 << OWI_PIN); // zapis do nej 1 (5V) ... nakoby po zapnuti zarizeni byla na sbernici 1
	_delay_us(5);
	
	/* drive bus low */
	OWI_PORT_OUT &= ~(1 << OWI_PIN); // zapis do nej 0 (0V)
		
	_delay_us(480); // cekat 480 us
	
	/* release bus */
	OWI_PORT_OUT |= (1 << OWI_PIN); // pin stale jako vystupni, zapis na nej 1 (+5V)
		
	_delay_us(70); // cekat 70 us
	
	/* read bus state */
	OWI_DDR &= ~(1 << OWI_PIN); // nastvav pin jako vstupni
	
	// s pripojenym pull-up rezistorem -- uz je od prikazu OWI_PORT_OUT |= (1 << OWI_PIN);
	presence = !(OWI_PORT_IN & (1 << OWI_PIN)); // precti hodnotu na pinu
	
	_delay_us(410); // cekat 410 us
		
	sei();
	
	// prectena hodnota -- 0=zadne zarizeni, 1=zarizeni na sbernici
	return presence;
}

void OWI_writeBit(unsigned char bit)
{
	
	cli();
	
	OWI_DDR |= (1 << OWI_PIN); // nastav pin jako vystupni
	OWI_PORT_OUT &= ~(1 << OWI_PIN); // zapis 0 (0V)	
	
	if(bit == 0x01) // posli na OWI 1
	{
		_delay_us(6); // cekat 6us
		OWI_PORT_OUT |= (1 << OWI_PIN);  // zapis 1 (5V)
		_delay_us(64); // cekat 64us
	}
	else // posli na OWI 0
	{
		_delay_us(60); // cekat 60us
		OWI_PORT_OUT |= (1 << OWI_PIN); // zapis 1 (5V)
		_delay_us(10); // cekat 10us
	}
	
	sei();
}

BYTE OWI_readBit()
{
	
	uint8_t read = 0x00;
	
	cli();
	
	OWI_DDR |= (1 << OWI_PIN); // nastav pin jako vystupni
	OWI_PORT_OUT &= ~(1 << OWI_PIN); // zapis 0 (0V)
	
	_delay_us(6); // cekat 6us
	
	OWI_DDR &= ~(1 << OWI_PIN); // nastvav pin jako vstupni s pripojenym pull-up rezistorem -- uz je od prikazu OWI_PORT_OUT |= (1 << OWI_PIN);
	OWI_PORT_OUT |= (1 << OWI_PIN); 
	
	_delay_us(9); // cekat 9us
	
	read = OWI_PORT_IN & (1 << OWI_PIN); // precti hodnotu na pinu
	
	_delay_us(55);
	
	sei();
	return read;
}

void OWI_writeByte(unsigned char byte)
{
	// LS-bit first	
	for(register uint8_t i = 0; i < 8; i++)
	{
		OWI_writeBit(byte & 0x01);
		byte = byte >> 1;
	}
}

BYTE OWI_readByte()
{
	
	uint8_t result = 0x00;
	
	//	LS-bit first
	for(register uint8_t i = 0; i < 8; i++)
	{
		result = result >> 1;
		if (OWI_readBit()) result |= 0x80;	
	}
	
	return result;
}

void OWI_readROM(BYTE *buffer_64bit)
{
	OWI_reset();
	OWI_writeByte(OWI_READ_ROM);
	
	for(register BYTE i = 0; i < 8; i++)
	{
		buffer_64bit[i] = OWI_readByte();
	}
}

void OWI_skipROM()
{
	OWI_reset();
	OWI_writeByte(OWI_SKIP_ROM);	
}

void OWI_matchROM(BYTE *ROM_64bit)
{
	OWI_reset();
	OWI_writeByte(OWI_MATCH_ROM);
	for(register uint8_t i = 0; i < 8; i++) 
		OWI_writeByte(ROM_64bit[i]);	
}

/*
unsigned int zmer() {
	unsigned char data_lo, data_hi;

	cli();
	OWI_reset();
	OWI_writeByte(OSLOV);
	OWI_writeByte(SPUST_KONVERZI);
	_delay_ms(2000);
	OWI_reset();
	OWI_writeByte(OSLOV);
	OWI_writeByte(CTI_TEPLOTU);
	data_lo = OWI_readByte();
	data_hi = OWI_readByte();
	//OWI_reset();
	
	int16_t teplota = 10 * ((data_hi & 0x0F) << 4 | (data_lo & 0xF0)>>4); //(data_lo & 0xF0) >> 4 | (data_hi & 0x0F) << 4;
	
	uint8_t desetiny = (data_lo & 0x0F) * 0.625; // rozliseni cidla je 0,0625 °C, toto mi dava 10x vetsi hodnoty. misto 0,5°C => 5
	if(data_hi & 0x80)
		teplota += desetiny;
	else
		teplota -= desetiny;
	
	//teplota = (data_lo & 0xF0) >> 4 | (data_hi & 0x0F) << 4;
	
	//desetiny = (data_lo & 0x0F) * 0.625;
	//tmp = 10 * teplota + 1000;
	
	//if (tmp < 1000) tmp -= desetiny;
	//else tmp += desetiny;
	//if ((tmp < 700) || (tmp > 2200)) tmp = 0;
	
	sei();
	return teplota;
}
*/