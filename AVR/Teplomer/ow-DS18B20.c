/*
 * File:   DS18B20.c
 * Author: Zoidbee
 *
 * Created on 19. ??jen 2011, 14:34
 */

#define F_CPU 16000000L//definuje frekvenci cpu

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

//definice funkci
#define setbit(port,bit) (port |= (1<<bit));
#define resbit(port,bit) (port &= ~(1<<bit));
#define testbit(port, bit) (port & (1<<bit))

#define OW_VYSTUPNI_PORT  PORTA
#define OW_VSTUPNI_PORT   PINA
#define OWDDR             DDRA
#define OWPIN             1

#define CTI   resbit(OWDDR,OWPIN)  
#define VYSLI setbit(OWDDR,OWPIN)  

//DS18B20 p??kazy
/*
1. Reset sb?rnice.
2. P??kaz 0CCh, p??kaz 044h - zah?j? m??en?.
3. Prodleva 0,75 sekund nutn? pro p?evod nap?t? na data.
4. P??kaz 0CCh, p??kaz 0BEh - obvod je p?ipraven vyslat obsah sv? pam?ti na sb?rnici.
5. ?ten? bajtu ze sb?rnice, bajt obsahuje spodn?ch osm bit? nam??en? teploty.
6. ?ten? bajtu ze sb?rnice, bajt obsahuje horn? ?ty?i bity nam??en? teploty.
 * 
 * Z?kladn? rozli?en? teplom?ru je 0, 0625?C, p?itom nejvy??? z dvan?cti bit? je znam?nko.
 */

/**Procesor m?e pou??t tento p??kaz t?sn? po resetu k tomu, aby dal??m p??kazem oslovil v?echna 
 * za??zen? na sb?rnici, bez nutnosti pou??vat n?jak? dal?? k?dovac? informace. 
 * Nap??klad je mo?n? p?ik?zat v?em obvod?m, aby provedly teplotn? konverzi pomoc? sekvence p??kaz? 0CCh, 044h.
 * P??kaz ?et?? ?as p?i komunikaci, ale nesm? b?t pou?it v souvislosti s p??kazy pro ?ten?, kdy? je na sb?rnici v?c obvod?.
 */
#define OSLOV 0xCC
/**Tento p??kaz spust? jeden p?evod teploty. 
 * Po p?eveden? je v?sledn? teplota ulo?ena do dvou bajt? pam?ti RAM a obvod se vr?t? do n?zkoodb?rov?ho re?imu. 
 */
#define SPUST_KONVERZI 0x44 	
/**Umo?n? ?ten? pam?ti obvodu. P?esun dat za??n? nejni???m bitem bajtu 0 a pokra?uje p?es celou pam? 
 * a? k bajtu 9, kde je ulo?en CRC k?d. ?ten? lze ukon?it sekvenc? reset, kdy? dal?? data nejsou pot?ebn?. 
 * Mnohdy sta?? p?e??st pouze prvn? dv? hodnoty, kde je ulo?ena nam??en? teplota.
 */
#define CTI_TEPLOTU 0xBE
/**Tento p??kaz lze pou??t tam, kde je pouze jeden obvod na sb?rnici. 
 * Dovoluje ??d?c?mu obvodu ??st 64 - bitov? k?d obvodu. Kdy? tento p??kaz pou?ijeme na sb?rnici kde je v?c obvod?, 
 * dojde ke kolizi dat, proto?e se v?echny obvody pokus? odpov?d?t najednou.
 */
#define CTI_KOD_OBVODU 0x33
/***********************************************************************
 * Tento p??kaz, n?sledovan? 64 - bitov?m k?dem umo?n? procesoru zvolit jedno z n?kolika za??zen? p?ipojen?ch na 
 * sb?rnici. Bude komunikovat pouze ten obvod, kter?mu odpov?d? 64 - bitov? k?d. Ostatn? obvody budou ?ekat 
 * na resetovac? impuls vyslan? procesorem.
 */
#define ZVOL_OBVOD 0x55

/*******************************************************************
 * Generuje reset, vraci 1 pokud teplomer je nalezen
 * jenak vraci 0
 */
unsigned char owReset() {
    _delay_us(1);
    VYSLI;
    resbit(OW_VYSTUPNI_PORT, OWPIN);
    _delay_us(480);
    CTI;
    _delay_us(70);
    return (testbit(OW_VSTUPNI_PORT, OWPIN) ? 1 : 0);
}

/*******************************************************************
 * posle na sbernici log. 1 nebo log. 0
 */
void owZapisBit(unsigned char bit) {
    _delay_us(70);
    if (bit) {
        resbit(OW_VYSTUPNI_PORT, OWPIN);
        VYSLI;
        _delay_us(6);
        CTI;
        _delay_us(64);
    } else {
        resbit(OW_VYSTUPNI_PORT, OWPIN);
        VYSLI;
        _delay_us(60);
        CTI;
        _delay_us(10);
    }
}

/*******************************************************************
 * precte bit ze sbernici
 */
unsigned char owCtiBit() {
    _delay_us(70);

    resbit(OW_VYSTUPNI_PORT, OWPIN);
    VYSLI;
    _delay_us(6);
    CTI;
    _delay_us(9);
    return (testbit(OW_VSTUPNI_PORT, OWPIN) ? 1 : 0);
}

/*******************************************************************
 * vysle jeden byte. Prvni LSB
 */

void owVysliByte(unsigned char byte) {
    int i;
    for (i = 0; i < 8; i++) {
        owZapisBit(byte & 0x1);
        byte >>= 1;
    }
}

/*******************************************************************
 * prijme jeden byte. Prvni LSB
 */

unsigned char owCtiByte(void) {
    int i;
    unsigned char vysledek = 0;
    for (i = 0; i < 8; i++) {
        vysledek >>= 1;
        if (owCtiBit()) vysledek |= 0x80;
    }
    return vysledek;
}

/*
1. Reset sb?rnice.
2. P??kaz 0CCh, p??kaz 044h - zah?j? m??en?.
3. Prodleva 0,75 sekund nutn? pro p?evod nap?t? na data.
4. P??kaz 0CCh, p??kaz 0BEh - obvod je p?ipraven vyslat obsah sv? pam?ti na sb?rnici.
5. ?ten? bajtu ze sb?rnice, bajt obsahuje spodn?ch osm bit? nam??en? teploty.
6. ?ten? bajtu ze sb?rnice, bajt obsahuje horn? ?ty?i bity nam??en? teploty.*/
unsigned int zmer() {
    unsigned char data_lo, data_hi, desetiny;
    signed char teplota;
    unsigned int tmp = 0;
    cli();
    owReset();
    owVysliByte(OSLOV);
    owVysliByte(SPUST_KONVERZI);
    _delay_ms(800);
    owReset();
    owVysliByte(OSLOV);
    owVysliByte(CTI_TEPLOTU);
    data_lo = owCtiByte(); 
    data_hi = owCtiByte(); 
    teplota = (data_lo & 0xF0) >> 4 | (data_hi & 0x0F) << 4; 
    
    desetiny = (data_lo & 0x0F) * 0.625;
    tmp = 10 * teplota + 1000;
    
    if (tmp < 1000) tmp -= desetiny;
    else tmp += desetiny;
    //if ((tmp < 700) || (tmp > 2200)) tmp = 0;
    sei();
    return tmp;
}
