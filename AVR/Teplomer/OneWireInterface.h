/*
 * OneWireInterface.h
 *
 * Created: 2.9.2014 13:45:09
 *  Author: John.Zoidberg
 */ 


#ifndef ONEWIREINTERFACE_H_
#define ONEWIREINTERFACE_H_

#ifndef F_CPU
	#define F_CPU 16000000UL
#endif

/* ======================== POUZIVANE KNIHOVNY ========================= */
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

/* ======================== DEFINICE ==================================== */

#define BYTE unsigned char

#define OWI_PORT_OUT PORTA
#define OWI_PORT_IN  PINA  
#define OWI_DDR		 DDRA
#define OWI_PIN		 1


/* ======================== ROM commands ================================= */

/* Pouzitelne pouze pri jednom zarizeni na sbernici, zarizeni posle jako odpoved svoji ROM - 64 bitova */
#define OWI_READ_ROM 0x33		
/* Komunikace se vsemi zarizenimi na sbernici, pri jednom zarizeni se bude komunikovat prave s timto, nemusi se oslovovat primo */
#define OWI_SKIP_ROM 0xCC
/* Po tomto prikazu zarizeni na sbernici ocekavaji ROM, nasledna komunikace bude probihat s odpovidajicim zarizenim */
#define OWI_MATCH_ROM 0x55


uint8_t OWI_reset();
void OWI_writeBit(unsigned char bit);
void OWI_writeByte(unsigned char byte);
BYTE OWI_readByte();
BYTE OWI_readBit();

/************************************************************************/
/*   Zajisti reset OWI a nasledne vysle prikaz SKIP ROM, vsechna		*/
/*   zarizeni na sbernici budou komunikovat!							*/
/*   Pouziti pri jednom zarizeni na sbernici							*/
/************************************************************************/
void OWI_skipROM();
/************************************************************************/
/* Precte ROM zarizeni, musi byt jedno na sbernici!!!                   */
/* Nutne predat unsegned char buffer[8]  === BYTE buffer[8]				*/
/************************************************************************/
void OWI_readROM(BYTE *buffer_64bit);

void OWI_matchROM(BYTE *ROM_64bit);

//unsigned int zmer() ;
#endif /* ONEWIREINTERFACE_H_ */