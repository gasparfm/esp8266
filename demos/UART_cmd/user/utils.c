#include "espmissingincludes.h"
#include "math.h"
#include "ets_sys.h"
#include "osapi.h"
#include <stdarg.h>

char * dtostrf(double number, signed char width, unsigned char prec, char *s) {
    bool negative = false;

    if (isnan(number)) {
        strcpy(s, "nan");
        return s;
    }
    if (isinf(number)) {
        strcpy(s, "inf");
        return s;
    }

    char* out = s;

    int fillme = width; // how many cells to fill for the integer part
    if (prec > 0) {
        fillme -= (prec+1);
    }
   // Handle negative numbers
    if (number < 0.0) {
        negative = true;
        fillme--;
        number = -number;
    }

    // Round correctly so that print(1.999, 2) prints as "2.00"
    // I optimized out most of the divisions
    double rounding = 2.0;
    for (uint8_t i = 0; i < prec; ++i)
        rounding *= 10.0;
    rounding = 1.0 / rounding;

    number += rounding;

    // Figure out how big our number really is
    double tenpow = 1.0;
    int digitcount = 1;
    while (number >= 10.0 * tenpow) {
        tenpow *= 10.0;
        digitcount++;
    }
    number /= tenpow;
    fillme -= digitcount;

    // Pad unused cells with spaces
    while (fillme-- > 0) {
        *out++ = ' ';
    }

    // Handle negative sign
    if (negative) *out++ = '-';

    // Print the digits, and if necessary, the decimal point
    digitcount += prec;
    int8_t digit = 0;
    while (digitcount-- > 0) {
        digit = (int8_t)number;
        if (digit > 9) digit = 9; // insurance
        *out++ = (char)('0' | digit);
        if ((digitcount == prec) && (prec > 0)) {
            *out++ = '.';
        }
        number -= digit;
        number *= 10.0;
    }

    // make sure the string is terminated
    *out = 0;
    return s;
}

int os_snprintf(char* buffer, size_t size, const char* format, ...) {
    int ret;
    va_list arglist;
    va_start(arglist, format);
    ret = ets_vsnprintf(buffer, size, format, arglist);
    va_end(arglist);
    return ret;
}

char* timeInterval(char* buffer, size_t bufferSize, unsigned long seconds)
{
    uint16_t hours = seconds / 3600, 
			rem = seconds % 3600, 
			minutes = rem / 60, 
			secs = rem % 60;
    
    os_snprintf (buffer, bufferSize, "%d:%d:%d", hours, minutes, secs);
    
    return buffer;
}
