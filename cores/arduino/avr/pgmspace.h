#ifndef AVR_PGMSPACE_H
#define AVR_PGMSPACE_H

#include <inttypes.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#define PROGMEM
#define PGM_P const char*
#define PGM_VOID_P const void*

#define pgm_read_byte(addr)     (*(const unsigned char *)(addr))
#define pgm_read_word(addr)     (*(const unsigned short *)(addr))
#define pgm_read_dword(addr)    (*(const unsigned long *)(addr))
#define pgm_read_float(addr)    (*(const float *)(addr))
#define pgm_read_ptr(addr)      (*(const void * const *)(addr))

#define pgm_read_byte_near(addr)  pgm_read_byte(addr)
#define pgm_read_word_near(addr)  pgm_read_word(addr)
#define pgm_read_dword_near(addr) pgm_read_dword(addr)
#define pgm_read_float_near(addr) pgm_read_float(addr)

#define pgm_read_byte_far(addr)   pgm_read_byte(addr)
#define pgm_read_word_far(addr)   pgm_read_word(addr)
#define pgm_read_dword_far(addr)  pgm_read_dword(addr)
#define pgm_read_float_far(addr)  pgm_read_float(addr)

#define memcpy_P(dest, src, num)  memcpy((dest), (src), (num))
#define strcpy_P(dest, src)       strcpy((dest), (src))
#define strcat_P(dest, src)       strcat((dest), (src))
#define strcmp_P(dest, src)       strcmp((dest), (src))
#define strlen_P(s)               strlen(s)

#define _FPSTR(s) (s)

inline char* dtostrf(double val, signed char width, unsigned char prec, char *s) {
    (void)width;
    sprintf(s, "%.*f", prec, val);
    return s;
}

inline char* ltoa(long val, char *s, int radix) {
    sprintf(s, "%ld", val);
    (void)radix;
    return s;
}

inline char* ultoa(unsigned long val, char *s, int radix) {
    sprintf(s, "%lu", val);
    (void)radix;
    return s;
}

#endif
