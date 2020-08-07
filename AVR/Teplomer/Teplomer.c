/*
 * Teplomer.c
 *
 * Created: 26.8.2014 22:11:47
 *  Author: John.Zoidberg
 *
 *	Program pro mereni teploty pomoci cidla DS18B20. Perioda mereni zvolena 10s, kterou lze pripadne menit nastavenim
 *	casovace 1. Zmerena teplota se zobrazi na LCD display-i a zaroven posle po USART. Z USART lze prijimat nektere prikazy.
 *
 *  PREDPOKLADA SE PRIPOJENI POUZE JEDNOHO TEPLOTNIHO CIDLA NA SBERNICI OWI - CIDLO JE PRIPOJENO K PORTC PC1!!!
 *
 *  POUZIVAT MISTO SPRINTF() FUNKCI ATOI() --> sprintf() ~~ 4KB, atoi ~~ 400B !!!
 *
 */ 
#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "uart.h"
#include "DS18B20_obsluzna.h"

// === DEFINICE ===
#define BYTE unsigned char
#define NULL 0

// === DEFINICE TYPU PACKETU PRO KOMUNIKACI ===
#define UART_RX_PACKET_HEAD		 ';' // zacatek prichoziho datoveho packetu
#define UART_RX_PACKET_GET_INFO	 '?' // pozadavek na odelsani info o cidle
#define UART_PCK_ROM		0x61	// (97d) = 'a' ... odesila ROM
#define UART_PCK_SUPLY		0x62	// (98d) = 'b' ... odesila stav napajeni
#define UART_PCK_FULL_SCRPD 0x63	// (99d) = 'c' ... odesila cely scratchpad (kvuli CRC a overeni v PC alikaci)
#define UART_PCK_CONFIG		0x64    // (100d) = 'd' ... odesila nastaveni konfiguracnich registru cidla
#define UART_PCK_ERROR		0x65    // (101d) = 'e' ... chyba kominikace, preteceni prichoziho bufferu

#define UART_PCK_RECIEVE_SIZE 4		// velikost prichoziho packetu, z pc jsou posilany vzdy 4  hodnoty (th,tl,res.,ulozDoEEPROM)
// === GLOBALNI PROMENNE ===
volatile BYTE zmena = 0;

// promenne pro multiplex 7seg
unsigned char mux7seg_CISLICE[11]= {0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90, 0xFF}; // vyjadreni cislic pro 7-seg
unsigned char mux7seg_zobrazCislo[4] = {0xFF, 0xFF, 0xFF, 0xFF}; // cisla, ktera se zobrazi na 7-seg
unsigned char mux7seg_cisla_buff[4];  // buffer, sem se zkopiruje obsah mux7seg_zobrazCislo atoto se zobrazuje na 7seg (bez toho mi ta 7seg blika) 
unsigned char mux7seg_counter = 3;    // pocitadlo, ktera cislice se prave zobrazuje

// === DEKLARACE FUNKCI === 
void bin2BCD2SevenSeg(unsigned int cislo, unsigned char *mux7seg_cisla, unsigned char *mux7seg_cislice);
void uart_sendPacket(BYTE typ, BYTE *data, BYTE dataLen);
void ctiAOdesliInfo();

// === MAIN === 
int main(void)
{
	// inicializace I/O
	DDRC = 0xFF;  // cely port slouzi jako data pro 7seg (a-h)
	PORTC = 0xFF; // zhasnu vse
	
	DDRA |= 0xF0;  // horni nible slouzi pro prepinani aktivni 7seg	... na pinu PA1 je teplotni cidlo
	PORTA |= 0xF0; // vypnu je
	
	// inicializace seriove linky
	uart_init(UART_BAUD_SELECT(9600, F_CPU));	
	
	// inicializace casovace 0, pro cteni teploty za cca 1 s, ten se pri preruseni VZDY sam vypne (bude zpousten jednorazove casovacem T1)
	//TCCR0 = 0; // zakazu casovac 
	
	// inicializace casovace 1 s periodou 10 s, bude spoustet konverzi teploty
	TCCR1B |= (1 <<CS10) | (1<<CS12) /*| (1<<WGM12) | (1<<WGM13)*/; // preddelicka 1024		
	TCNT1 = 40355; // nastav hodnotu pro preteceni za cca 1,6 s, pak 2x preteceni na plnym rozsahu (~4,2s) a dostaneme 10 s
	
	// inicializace casovace pro 7seg MUX ... CTC mod
	TCCR2 |= (1<<CS20) | (1<<CS22) | (1<<WGM21);
	OCR2 = 61; // frekvence preruseni 250 Hz
	
	// povoleni preruseni (pri preteceni casovacu T0, T1 a pri komparaci T2)
	TIMSK |= (1<<TOIE1) | (1<<TOIE0) | (1<<OCIE2); 		
	sei();	
	
	ctiAOdesliInfo(); // precte info o senzoru a posle na seriovou linku
	
	DS18B20_zahajMereniTeploty();	
	TCCR0 |= (1<<CS00) | (1<<CS02); // povoli casovac pro cteni teploty za cca 1 s
	
	// promenne pro main
	BYTE pckInd = 0;   // index prijateho datoveho byte
	BYTE pckStart = 0; // 1 = ahaa, od ted budou chodit data
	BYTE oneByteOfPacket = 0;
	unsigned int uartPrijato;
	BYTE dataPrijatoPacket[UART_PCK_RECIEVE_SIZE];
    while(1)
    {					
		// naslouchani serive lince
		if(((uartPrijato = uart_getc()) & UART_NO_DATA) == 0) // vyctu jeden BYTE ze zasobniku seriove linky
		{
			oneByteOfPacket = uartPrijato; // data obsahuje jen dolni BYTE
			
			if(pckInd >= UART_PCK_RECIEVE_SIZE) // preteceni bufferu, budu cekat na novy zacatek packetu
			{	
				pckInd = 0;		
				uart_sendPacket(UART_PCK_ERROR, NULL, 0);
			}
			
			if(oneByteOfPacket == UART_RX_PACKET_HEAD && pckInd == 0 ) // znaci zacatek datoveho packetu
			{
				pckStart = 1; // od ted cti dany pocet BYTE datoveho packetu
			}
			else if(oneByteOfPacket == UART_RX_PACKET_GET_INFO && pckStart == 0) // vykonej prikaz, pokud nebude zahajeno cteni
			{
					ctiAOdesliInfo();		// datoveho packetu, (data totiz mohou nabyvat libovolneho znaku, proto indikace pomoci pckStart)		
			}
			else if(pckStart == 1) // kdyz byla obdrzena hlavicka packetu, tak muzu zacit prijimat data
			{			
				dataPrijatoPacket[pckInd++] = oneByteOfPacket;
			
				if(pckInd == UART_PCK_RECIEVE_SIZE)	// kdyz prijmu vsechny 4 datove BYTE, poslu je ke zpracovani a zacnu cekat na novou hlavicku dalsiho packetu
				{
					DS18B20_nastavKonfiguracniRegistr(dataPrijatoPacket);
					ctiAOdesliInfo();
					pckStart = 0; // uz byly prijaty vsechna data
					pckInd = 0;   // ndexator zpatky na 0
				}
			}
		}
		
		// aktualizace zobrazene teploty a odeslani dat po seriove lince
		if(zmena == 0x01)
		{
			zmena = 0x00;
			// odesle oba BYTE po uart
			uart_sendPacket(UART_PCK_FULL_SCRPD, DS18B20_scratchpad, DS18B20_SIZE_SCRATCHPAD);			
			//teplota_vysl = DS18B20_ctiTeplotu(); // vyjadrena teplota je 100x vyssi nez skutecna, dve posledni cislice jsou desetinna mista			
			bin2BCD2SevenSeg((unsigned int)DS18B20_ctiTeplotu()/10, mux7seg_zobrazCislo, mux7seg_CISLICE); // prevod teploty pro zobrazeni na 7seg			
		}		 
    } // end while(1)
} // end main

// === OBSLUHA PRERUSENI ===

// 7seg MUX
ISR(TIMER2_COMP_vect)
{
	// double buffer ... kdyz ho neudelam, tak 7seg pri zmene teploty preblikava ... nechapu proc, kdyz sem to nikdy nedelal a nikdy se to nedelo ???
	// a taky, kdyz sem dal double buffer, tak se zacala zobrazovat tecka, bez nej se tecka nezobrazovala ... tohle vzkutku nechapu ???
	if (++mux7seg_counter > 3)
	{
		mux7seg_counter = 0;
		mux7seg_cisla_buff[0]=mux7seg_zobrazCislo[0];
		mux7seg_cisla_buff[1]=mux7seg_zobrazCislo[1];
		mux7seg_cisla_buff[2]=mux7seg_zobrazCislo[2];
		mux7seg_cisla_buff[3]=mux7seg_zobrazCislo[3];	
	}
	PORTA |= 0xF0; // znasnu horni 4 piny portuA
	PORTA &= ~(1<<(4+mux7seg_counter));
	
	PORTC = 0xFF;
	// zobrazeni desetinne tecky
	PORTC = mux7seg_cisla_buff[mux7seg_counter];
	if(mux7seg_counter == 2)
	{
		PORTC &= 0x7F;
	}
}

// preteceni jednou za 10 s, spousti konverzi teploty
ISR(TIMER1_OVF_vect)
{
	cli(); // vypnu zpracovani preruseni, aby nebyla komunikace prerusena od preruseni multiplexu 7seg
	static BYTE rounds = 0;
	if(++rounds > 2)
	{
		DS18B20_zahajMereniTeploty();
		
		TCCR0 |= (1<<CS00) | (1<<CS02); // povoli casovac pro cteni teploty za cca 1 s

		rounds = 0;			// reset citace pruchodu prerusenimi
		TCNT1 = 40355;		// zde je moznost korekce, kod vise muze trvat az 3 ms, bylo by to 40402
	}
	sei(); // a opet ho zapnu
}

// casovac pro cteni namerene teploty po cca 1s od spusteni konverze
ISR(TIMER0_OVF_vect)
{
	cli(); // vypnu zpracovani preruseni, aby nebyla komunikace prerusena od preruseni multiplexu 7seg
	static BYTE rounds = 0;
	// casovac ma preteceni za cca 16 ms ... do 1 s potrebujeme priblizne 61 preteceni
	if(++rounds > 61)
	{
		TCCR0 = 0x00; // vypne casovac
		rounds = 0;
		
		zmena = 0x01; // nova data k dispozici, zpracuj v main	
		DS18B20_ctiVse(); // preste cely zapisnik cidla, obsahuje cli() a sei(); (komunikace muze trvat ~~ 15 ms)		
	}
	sei(); // a opet ho zapnu
}

// === DEFINICE FUNKCI ===
/************************************************************************/
/* Odesle predana data po seriove lince									*/		
/* HlavickaPacketu -> delka datove casti -> typ -> data					*/
/* delka dat maximalne 255 znaku										*/
/************************************************************************/
void uart_sendPacket(BYTE typ, BYTE *data, BYTE dataLen)
{
	uart_putc(UART_RX_PACKET_HEAD); // zajajuji vysilani, synchronizuj!	
	uart_putc(dataLen);		// delka dat, (hlavicka, dataLen, typ se do teto delky nezapocitavaji !!!)
	uart_putc(typ);

	if(data != NULL)	
	{
		for(BYTE *udata=data; udata!=data+dataLen; udata++)
		{
			uart_putc(*udata);
		}
	}
		
	//for(register uint8_t i = 0; i < dataLen_max255; i++)
	//	uart_putc(data[i]);
}

// prevod binarniho vyjadreni cisla do BCD kodu (rozklad na jedn. cislice, MAX 4 CISLICE)
// pote prevede cislice do tvaru pro zobrazeni na 7seg
// unsigned int cislo  ... kladne cislo, max 4 mista
// unsigned char *mux7seg_cisla ... pole, kam se ulozi cislice pro dalsi zobrazeni (VELIKOST POLE MUSI BYT 4 !!!)
// unsigned char *mux7seg_cislice ... pole, kde jsou ulozeny vyjadreni jedn. cislic pro zobrazeni na 7seg (VELIKOST POLE MUSI BYT 11 !!! [0 - 9 + ZHASNUTO])
void bin2BCD2SevenSeg(unsigned int cislo, unsigned char *mux7seg_cisla, unsigned char *mux7seg_cislice)
{
	unsigned int pom, cisloBack = cislo;
	
	if(mux7seg_cisla == NULL || mux7seg_cislice == NULL)
	return;
	
	// prvni krok, posun cisla o tri doprava, to co se mi z cisla vysune, vlozim do pomocne
	pom = (cislo & 0xE000) >> 13;
	cislo = (cislo << 3);
	
	// cyklicky se testuji podminky, a posouva cislo o jeden bit vpravo
	// princip vysvetlen ve videu https://www.youtube.com/watch?v=kusZDF3IfBA
	for(unsigned char i = 0; i < 13; i++)
	{
		if((pom & 0xF00) >= 0x500)
		pom += 0x300;

		if((pom & 0xF0) >= 0x50)
		pom += 0x30;

		if((pom & 0x0F) >= 0x05)
		pom += 0x3;

		pom = (pom << 1) | ((cislo & 0x8000) >> 15);
		cislo <<= 1;
	}
	// v pom jsou ulozeny jednotlive cislice, ktere je nutne rozdelit
	//rozlozeno[0] = (pom&0xF000)>>12; // tisice
	//rozlozeno[1] = (pom&0xF00)>>8;   // stovky
	//rozlozeno[2] = (pom&0xF0)>>4;	   // desitky
	//rozlozeno[3] = pom&0xF;		   // jednotky
	
	mux7seg_cisla[2] = mux7seg_cislice[10]; // desitky - nejprve se nezobrazi nic
	mux7seg_cisla[1] = mux7seg_cislice[10]; // stovky - nejprve se nezobrazi nic
	mux7seg_cisla[0] = mux7seg_cislice[10]; // tisice - nejprve se nezobrazi nic
	
	mux7seg_cisla[3] = mux7seg_cislice[pom & 0xF];	// jednotky (budou zobrazeny vzdy!)
	if(cisloBack>9)
	{
		mux7seg_cisla[2] = mux7seg_cislice[(pom & 0xF0) >> 4]; // desitky
	}
	if(cisloBack>99)
	{
		mux7seg_cisla[1] = mux7seg_cislice[(pom & 0xF00) >> 8]; // stovky
	}
	if(cisloBack>999)
	{
		mux7seg_cisla[0] = mux7seg_cislice[(pom & 0xF000) >> 12]; // tisice
	}
}

/************************************************************************/
/* Precte ifo o senzoru a odesle na seriovou linku					    */
/* posila se ROM, zpusob napajeni, obsah konfig registru (th,tl, res.)  */
/************************************************************************/
void ctiAOdesliInfo()
{
		// precte nastaveni cisla a odesle na uart
		DS18B20_ctiInfo();

		uart_sendPacket(UART_PCK_ROM, DS18B20_ROM, DS18B20_SIZE_ROM);
		uart_sendPacket(UART_PCK_SUPLY, &DS18B20_stav, 1);
		uart_sendPacket(UART_PCK_CONFIG, DS18B20_scratchpad+DS18B20_SCRPD_ALARM_TH, 3);
}