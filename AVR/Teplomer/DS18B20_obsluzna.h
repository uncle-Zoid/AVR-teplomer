/*
 * DS18B20_obsluzna.h
 *
 * Created: 6. 7. 2015 17:16:35
 *  Author: Zoidbee
 */ 


#ifndef DS18B20_OBSLUZNA_H_
#define DS18B20_OBSLUZNA_H_

#define BYTE unsigned char

// === DEFINICE PRIKAZU PRO CIDLO ===
#define DS18B20_CONVERT 0x44 // spusti konverzi teploty. Nutne cekat danou dobu, nebo kontrolovat sbernici: 0=konverze, 1=prevedeno
#define DS18B20_WRITE   0X4E // ZAPIS DO PAMETI CIDLA, PORADI: Th (alerm), Tl(alarm), CONFIG(rozliseni)
#define DS18B20_READ    0XBE // CTENI DAT Z CIDLA, CELKEM AZ 9 BYTE TEMP_LSB, TEMP_MSB, Th, Tl, CONFIG, 3X RESERVED, CRC
#define DS18B20_COPY_SCRATCHPAD 0X48 // ZKOPIRUJE HODNOTY Th, Tl A CONFIG REGISTRU DO EEPROM CIDLA
#define DS18B20_RECALL  0XB8 // ZKOPIRUJE DATA Z EEPROM DO PRACOVNI PAMETI CIDLA
#define DS18B20_POWER_SUPLY 0XB4 // PRIKAZ NASLEDUJE CTENI JEDNOHO SLOTU (BITU), KDY CIDLO KTERE POUZIVA PARAZITNI NAPAJENI STAHNE SBERNICI DO 0, EXTERNE NAPAJENE CIDLO PONECHA SBERNICI V 1

// === DEFINICE ROZLISENI CIDLA ===
#define DS18B20_RESOLUTION_9B  0b00011111 // convert time: 93,75 ms ; increment: 0,5 °C
#define DS18B20_RESOLUTION_10B 0b00111111 // convert time: 187,5 ms ; increment: 0,25 °C
#define DS18B20_RESOLUTION_11B 0b01011111 // convert time: 375 ms   ; increment: 0,125 °C
#define DS18B20_RESOLUTION_12B 0b01111111 // convert time: 750 ms   ; increment: 0,0625 °C

// === DEFINICE POZIC DAT V PROMENNE SCRATCHPAD ===
// konfiguracni registr configReg[3] pozice
#define DS18B20_SCRPD_TMP_LSB    0 // SPODNI BYTE TEPLOTY
#define DS18B20_SCRPD_TMP_MSB    1 // HORNI BYTE TEPLOTY
#define DS18B20_SCRPD_ALARM_TH   2 // HORNI HODNOTA ALARMU
#define DS18B20_SCRPD_ALARM_TL   3 // DOLNI HODNOTA ALARMU
#define DS18B20_SCRPD_RESOLUTION 4 // ROZLISENI SENZORU
#define DS18B20_SCRPD_RESERVED_1 5 // REZERVOVANO (obsahuje hodnotu 0XFF)
#define DS18B20_SCRPD_RESERVED_2 6 // REZERVOVANO
#define DS18B20_SCRPD_RESERVED_3 7 // REZERVOVANO (obsahuje hodnotu 0X10)
#define DS18B20_SCRPD_CRC		 8 // CRC

// === DEFINICE STAVU CIDLA ===
#define DS18B20_STATE__CIDLO_NENALEZENO    0
#define DS18B20_STATE__PARAZITNI_NAPAJENI  1
#define DS18B20_STATE__EXTERNI_NAPAJENI    2

// === DEFINICE VELIKOSTI POLI ===
#define DS18B20_SIZE_ROM        8
#define DS18B20_SIZE_SCRATCHPAD 9

// === GLOBALNI PROMENNE ===
extern BYTE DS18B20_scratchpad[DS18B20_SIZE_SCRATCHPAD]; // obsah registru teplotniho cidla
extern BYTE DS18B20_ROM[DS18B20_SIZE_ROM]; // ROM cidla DS18B20 na EVB-kitu:  28 fc 14 f0 02 00 00 4c
extern BYTE DS18B20_stav; // STAV CIDLA (NENALEZENO, PARAZITNI/EXTERNI NAPAJENI
// === DEFINICE FUNKCI ===

/************************************************************************/
/* cteni konfiguracniho registru (TH, TL, rozliseni) do                 */
/* globalni promenne DS18B20_scratchpad									*/
/************************************************************************/
void DS18B20_ctiKonfiguracniRegistr();

/************************************************************************/
/* zapis do konfiguracniho registru cidla								*/
/* BYTE *config ... pole o velikosti 4 BYTE (TH, TL, rozliseni,         */
/*                                       prenesDoEEPROM [0-ne, 1-ano] ) */
/************************************************************************/
void DS18B20_nastavKonfiguracniRegistr(BYTE *config);

/************************************************************************/
/* cte informace o cidle - ROM, napajeni, konfiguracni registr          */
/************************************************************************/
void DS18B20_ctiInfo();

/************************************************************************/
/* precte cely registr cidla (vsech 9 BYTE) do					        */
/* globalni promenne DS18B20_scratchpad									*/
/************************************************************************/
void DS18B20_ctiVse();

/************************************************************************/
/* odesle cidlu prikaz - zahaj konverzi teploty					        */
/* pri konverzi teploty je nutne cekat alespon 750 ms pro 12.b			*/
/************************************************************************/
void DS18B20_zahajMereniTeploty();

/************************************************************************/
/*  Vrati teplotu ve stupnich Celsia                                   */
/************************************************************************/
int DS18B20_ctiTeplotu();


#endif /* DS18B20_OBSLUZNA_H_ */