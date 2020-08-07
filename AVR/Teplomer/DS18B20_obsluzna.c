/*
 * DS18B20_obsluzna.c
 *
 * Created: 6. 7. 2015 17:17:00
 *  Author: Zoidbee
 *
 * obsahuje funkce pro obsluhu teplotniho cidla
 */ 

#include "DS18B20_obsluzna.h"
#include "OneWireInterface.h"

BYTE DS18B20_scratchpad[9];
BYTE DS18B20_ROM[8];
BYTE DS18B20_stav;

/************************************************************************/
/* cteni konfiguracniho registru (TH, TL, rozliseni) do                 */ 
/* globalni promenne DS18B20_scratchpad									*/
/************************************************************************/
void DS18B20_ctiKonfiguracniRegistr()
{
	cli();
	OWI_reset(); // reset, zacne nova komunikace
	OWI_skipROM(); // pocita se pouze s jednom cidlem na sbernici
	
	OWI_writeByte(DS18B20_READ);
	
	OWI_readByte();OWI_readByte(); // vycte a preskoci BYTE 0, 1 ... obsahuji zmerenou teplotu
	DS18B20_scratchpad[DS18B20_SCRPD_ALARM_TH]   = OWI_readByte(); // ctu Th
	DS18B20_scratchpad[DS18B20_SCRPD_ALARM_TL]   = OWI_readByte(); // cte Tl
	DS18B20_scratchpad[DS18B20_SCRPD_RESOLUTION] = OWI_readByte(); // cte rozliseni
	OWI_reset(); // ukoncim cteni
	sei();	
}

/************************************************************************/
/* zapis do konfiguracniho registru cidla								*/
/* BYTE *config ... pole o velikosti 4 BYTE (TH, TL, rozliseni,         */
/*                                       prenesDoEEPROM [0-ne, 1-ano] ) */
/************************************************************************/ 
void DS18B20_nastavKonfiguracniRegistr(BYTE *config)
{
	cli();
	OWI_reset(); // reset, zacne nova komunikace
	OWI_skipROM(); // pocita se pouze s jednom cidlem na sbernici
	
	OWI_writeByte(DS18B20_WRITE); // chci zapsat do cidla
	
	for(register BYTE i = 0; i < 3; i++) // zapis th, tl, rozliseni
		OWI_writeByte(config[i]);
	
	if(config[3] > 0)	// preneseni hodnot do EEPROM, pokud pozadovano
		OWI_writeByte(DS18B20_COPY_SCRATCHPAD);
	
	sei();
}

/************************************************************************/
/* cte informace o cidle, ROM, napajeni, konfiguracni registr           */
/************************************************************************/
void DS18B20_ctiInfo()
{
	cli();
	if(OWI_reset() == 1)
	{
		// cteni ROM cidla, jen kvuli jeho zjisteni ... dale v programu se pro komunikaci pouzije prikaz "OSLOV_VSECHNY" 
		OWI_readROM(DS18B20_ROM);
				
		// zjisteni napajeni cidla 
		OWI_reset(); // reset, zacne nova komunikace
		OWI_skipROM(); // pocita se pouze s jednom cidlem na sbernici
		OWI_writeByte(DS18B20_POWER_SUPLY);
		
		if(OWI_readBit() == 0) //napajeni parazitni kapacitou
			DS18B20_stav = DS18B20_STATE__PARAZITNI_NAPAJENI;
		else
			DS18B20_stav = DS18B20_STATE__EXTERNI_NAPAJENI;
		
		/* cteni konfig registru */
		DS18B20_ctiKonfiguracniRegistr();
	}
	else
	{
		DS18B20_stav = DS18B20_STATE__CIDLO_NENALEZENO;
	}
	sei();
}

/************************************************************************/
/* precte cely registr cidla (vsech 9 BYTE) do					        */
/* globalni promenne DS18B20_scratchpad									*/
/************************************************************************/
void DS18B20_ctiVse()
{
	cli();
	OWI_reset(); // reset, zacne nova komunikace
	OWI_skipROM(); // pocita se pouze s jednom cidlem na sbernici
	
	OWI_writeByte(DS18B20_READ);
	
	DS18B20_scratchpad[DS18B20_SCRPD_TMP_LSB]    = OWI_readByte(); // ctu Th
	DS18B20_scratchpad[DS18B20_SCRPD_TMP_MSB]    = OWI_readByte(); // ctu Th
	DS18B20_scratchpad[DS18B20_SCRPD_ALARM_TH]   = OWI_readByte(); // ctu Th
	DS18B20_scratchpad[DS18B20_SCRPD_ALARM_TL]   = OWI_readByte(); // cte Tl
	DS18B20_scratchpad[DS18B20_SCRPD_RESOLUTION] = OWI_readByte(); // cte rozliseni
	DS18B20_scratchpad[DS18B20_SCRPD_RESERVED_1] = OWI_readByte(); // cte rezervovany 1
	DS18B20_scratchpad[DS18B20_SCRPD_RESERVED_2] = OWI_readByte(); // cte rezervovany 2
	DS18B20_scratchpad[DS18B20_SCRPD_RESERVED_3] = OWI_readByte(); // cte rezervovany 3
	DS18B20_scratchpad[DS18B20_SCRPD_CRC]		 = OWI_readByte(); // cte CRC ... CRC je v cidle pocitano ze vsech osmi predchazejicich BYTE !!!
	sei();
}

/************************************************************************/
/* odesle cidlu prikaz - zahaj konverzi teploty					        */
/* pri konverzi teploty je nutne cekat alespon 750 ms pro 12.b			*/
/************************************************************************/
void DS18B20_zahajMereniTeploty()
{
	OWI_reset(); // reset, zacne nova komunikace
	OWI_skipROM(); // OWI_matchROM(DS18B20_ROM); // kdyz bych mel vic cidel
	OWI_writeByte(DS18B20_CONVERT); // spusti konverzi teploty, nutne cekat alespon 750 ms pro 12.b
}

/************************************************************************/
/*  Vrati teplotu ve stupnich Celsia                                   */
/************************************************************************/
int DS18B20_ctiTeplotu()
{
	int teplota_vysl = (DS18B20_scratchpad[DS18B20_SCRPD_TMP_MSB]<<8) + DS18B20_scratchpad[DS18B20_SCRPD_TMP_LSB]; // posklada oba BYTE teploty do jedne promenne
	BYTE znamenko = 1; // teplota kladna
	// pokud je teplota zaporna, nejvyssi bit je kladny, udelej dvojkovy doplnek
	if(teplota_vysl & 0x8000)
	{
		znamenko = -1; // teplota je zaporna
		teplota_vysl = (teplota_vysl^0xFFFF) + 1; // dvojkovy doplnek
	}
	// teplota je ulozena v typu int s presnosti dve desetinna mista 0b 0000 0001 1001 0001 = 401 => 401 * 0,0625 = 25,0625
	//																								 401 * 6,25   = 2506 ~~ 25,061 °C
	switch(DS18B20_scratchpad[DS18B20_SCRPD_RESOLUTION])
	{
		case DS18B20_RESOLUTION_12B: // vsechny bity platne ... vysledna teplota se ziska nasobenim nejmensim desetinnym prirustkem = teplota * 0,0625
		// nasobim 6,25 ... tj nasob 6x a vydel 4 (1/4=0,25) ... 6x = 4x + 2x = posun2x + posun1x
		teplota_vysl = ((teplota_vysl<<2)+(teplota_vysl<<1)) + (teplota_vysl>>2); // 6*teplota_vysl + teplota_vysl/4;
		break;
		case DS18B20_RESOLUTION_11B:
		teplota_vysl = teplota_vysl >> 1; // bit 0 neni platny ... rozliseni je 0,125 ... teplota bude 11b cislo
		// nasobim 12,5 ... teplota*12 + teplota/2 (1/2=0,5) ... 12x = 8x + 4x = posun3x + posun2x
		teplota_vysl = ((teplota_vysl<<3)+(teplota_vysl<<2))  + (teplota_vysl>>1);
		break;
		case DS18B20_RESOLUTION_10B:
		teplota_vysl = teplota_vysl >> 2; // bity 0 a 1 nejsou platne ... rozliseni 0,25
		// nasobim 25 ... teplota*25 ... 25x = 16x + 8x + 1x
		teplota_vysl = (teplota_vysl<<4) + (teplota_vysl<<3) + teplota_vysl; //25*teplota_vysl;
		break;
		case DS18B20_RESOLUTION_9B: // bity 0, 1 a 2 nejsou platne
		teplota_vysl = teplota_vysl >> 3; // rozliseni 0,5 ... nasobim 50x = 32x + 16x + 2x
		teplota_vysl = (teplota_vysl<<5)+(teplota_vysl<<4)+(teplota_vysl<<1);//50*teplota_vysl;
		break;
	}
	
	return znamenko * teplota_vysl;
}
