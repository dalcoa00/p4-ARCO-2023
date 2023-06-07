#include <stdio.h>
#include <unistd.h>
#include "ieee754converter.h"

unsigned long IEEE754Converter::floattoIEESign(float num)
{
    union Code a;
    a.numero = num;
    return a.bitfield.sign;
}

unsigned long IEEE754Converter::floattoIEENumex(float num)
{
    union Code a;
    a.numero = num;
    return a.numerox;
}

unsigned long IEEE754Converter::floattoIEE2 (){

    union Code a;
    a.numero=2.1;
    return a.bitfield.partFrac;

}

float IEEE754Converter::IEEtofloat2 (){

    union Code a;

    a.bitfield.partFrac = 4718592;
    a.bitfield.expo = 131;
    a.bitfield.sign = 0;
    return a.numero;
}

unsigned long IEEE754Converter::floattoIEEExp(float num)
{
    union Code a;
    a.numero = num;
    return a.bitfield.expo;
}

unsigned long IEEE754Converter::floattoIEEMantisa(float num)
{
    union Code a;
    a.numero = num;
    return a.bitfield.partFrac;
}


float IEEE754Converter::IEEtofloat (int signo, int exponente, int mantisa){

    union Code a;

    a.bitfield.sign = signo;
    a.bitfield.expo = exponente;
    a.bitfield.partFrac = mantisa;

    return a.numero;
}
