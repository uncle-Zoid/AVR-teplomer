/*
 * MrLCD_library.h
 *
 * Created: 28.8.2014 10:23:28
 *  Author: John.Zoidberg
 *
 *	Knihovna pro LCD display HD44780, pro 4bitovy mod, ovladaci i datove vodice musi byt zapojeny na jednom portu!!!
 *
 */ 


#ifndef MRLCD_LIBRARY_H_
#define MRLCD_LIBRARY_H_

/* ======================== POUZIVANE KNIHOVNY ========================= */
#define F_CPU 16000000UL	// definice frekvence krystalu pro knihovnu delay.h
#include <util/delay.h>
#include <avr/io.h>
#include <stdlib.h>

/* ======================== DEFINICE ==================================== */

/* POUZIVANY PORT */
#define LCD_PORT PORTC
#define LCD_PIN  PINC
#define LCD_DDR  DDRC
/* PRIRAZENI PINU */
#define LCD_PIN_DB4 0
#define LCD_PIN_DB5 1
#define LCD_PIN_DB6 2
#define LCD_PIN_DB7 3
#define LCD_PIN_RW  4
#define LCD_PIN_RS  5
#define LCD_PIN_E   6
/* ===================================================================== */

#define LCD_COMMAND 0
#define LCD_DATA 1

#define LCD_COMMAND_SHIFT_DISPLAY_LEFT  0X18
#define LCD_COMMAND_SHIFT_DISPLAY_RIGHT 0X1C
#define LCD_COMMAND_DISPLAY_OFF			0X08
#define LCD_COMMAND_DISPLAY_ON		    0X0C
#define LCD_COMMAND_CURSOR_ON			0X0A
#define LCD_COMMAND_CURSOR_OFF			0X08
#define LCD_COMMAND_CURSOR_DECREASE		0X04
#define LCD_COMMAND_CUROSR_INCREASE		0X06
#define LCD_COMMAND_SCROLL_DISPLAY_ON	0X05
#define LCD_COMMAND_SCROLL_DISPLAY_OFF  0X04
typedef enum 
{
	DISPLAY_OFF = 0b00001000,
	DISPLAY_ON  = 0b00001100,
	DISPLAY_ON_WITH_CURSOR = 0b00001110,
	DISPLAY_ON_WITH_CURSOR_BLINK_1 = 0b00001101,
	DISPLAY_ON_WITH_CURSOR_BLINK_2 = 0b00001111
}LCD_INIT_DISPLAY;

typedef enum
{
	RAM = 0, PROGAM_MEMORY = 1
}LCD_VLASTNI_ZNAK_ODKUD;
/* ===================================================================== */

/************************************************************************/
/*      Inicializuje display, nastavi 4.bitovou komunikaci,				*/
/*		dva radky a velikost fontu 5x8 pixelu.							*/
/*		Mod zapnuti lze volit z LCD_INIT_DISPLAY.						*/
/*																		*/
/*		NUTNE INICIALIZOVAT PORT LCD V PROGRAMU (pr. DDRx = 0b01111111) */
/************************************************************************/
void LCD_init(LCD_INIT_DISPLAY mod);

void LCD_check_bussy();
/************************************************************************/
/* uint8_t data ... znak/prikaz											*/
/* uint8_t typ  ... 0 = prikaz / 1 = znak								*/
/************************************************************************/
void LCD_send(uint8_t data, uint8_t typ);

void LCD_clear();
void LCD_home();
void LCD_printChar(char ch);
void LCD_printString(char *str);
void LCD_printString_PROGMEM(char* pgmstr);
/************************************************************************/
/* Pro zobrazeni cisla na display se pouziva funkde itoa()              */
/* uint8_t soustava ... desitkova = 10, sestnactkova = 16, dvojkova = 2 */
/* pocetCislic = 2  ... cislo=10  => zobrazi "10"                       */
/************************************************************************/
void LCD_printInt(int cislo, uint8_t pocetCislic, uint8_t soustava);
void LCD_gotoXY(uint8_t x, uint8_t y);
/************************************************************************/
/* pixels ... pole s daty, max velikost pole je 8 (1znak = 8radku)      */
/* pozice ... adresa, na kterou bude znak ulozen (0 - 7)                */
/* ulozenoV ... 0 = znak v RAM, 1 = znak v PROGMEM                      */
/************************************************************************/
void LCD_vlastniZnak(char *pixels, uint8_t pozice, LCD_VLASTNI_ZNAK_ODKUD ulozenoV);
#endif /* MRLCD_LIBRARY_H_ */