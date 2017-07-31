#ifndef UTILS_H
#define UTILS_H

#include <c_types.h>

char * dtostrf(double number, signed char width, unsigned char prec, char *s);
int os_snprintf(char* buffer, size_t size, const char* format, ...);
char* timeInterval(char* buffer, size_t bufferSize, unsigned long seconds);

#endif
