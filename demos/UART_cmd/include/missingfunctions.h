#ifndef MISSINGFUNCTIONS_H
#define MISSINGFUNCTIONS_H

#include <stdarg.h>
#include <c_types.h>
#include "espmissingincludes.h"

int os_snprintf(char* buffer, size_t size, const char* format, ...) {
    int ret;
    va_list arglist;
    va_start(arglist, format);
    ret = ets_vsnprintf(buffer, size, format, arglist);
    va_end(arglist);
    return ret;
}

#endif
