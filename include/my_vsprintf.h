#ifndef MY_VSPRINTF_H
#define MY_VSPRINTF_H

#include <stdarg.h>

int __vsnprintf(char *buffer, int n, const char *format, va_list va);
int __vsprintf(char *buffer, const char *format, va_list va);
int __snprintf(char *buffer, int n, const char *format, ...);
int __sprintf(char *buffer, const char *format, ...);

#endif /* MY_VSPRINTF_H */
