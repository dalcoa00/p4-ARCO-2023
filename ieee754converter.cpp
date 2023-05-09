#include <stdio.h>
#include <unistd.h>
#include "ieee754converter.h"

/*
 *
 *  Este código es una modificación del código dado en el ágora
 *
 *
 */

unsigned int IEEE754Converter::floattoIEENumex(float num)
{
    union Code a;
    a.numero = num;
    return a.numerox;
}

unsigned int IEEE754Converter::floattoIEESign(float num)
{
    union Code a;
    a.numero = num;
    return a.bitfield.sign;
}

unsigned int IEEE754Converter::floattoIEEExp(float num)
{
    union Code a;
    a.numero = num;
    return a.bitfield.expo;
}

unsigned int IEEE754Converter::floattoIEEMantisa(float num)
{
    union Code a;
    a.numero = num;
    return a.bitfield.partFrac;
}

unsigned int IEEE754Converter::floattoIEE2 (){

    union Code a;
    a.numero=2.1;
    return a.bitfield.partFrac;

}

float IEEE754Converter::IEEtofloat (int signo, int exponente, int mantisa){

    union Code a;

    a.bitfield.sign = signo;
    a.bitfield.expo = exponente;
    a.bitfield.partFrac = mantisa;

    return a.numero;
}

float IEEE754Converter::IEEtofloat2 (){

    union Code a;

    a.bitfield.partFrac = 4718592;
    a.bitfield.expo = 131;
    a.bitfield.sign = 0;
    return a.numero;

}
