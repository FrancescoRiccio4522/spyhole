#ifndef FONTS_H
#define FONTS_H

#include <stdint.h>

/* Struttura font */
typedef struct {
    const uint8_t Width;      // larghezza in pixel
    const uint8_t Height;     // altezza in pixel
    const uint16_t *data;     // dati bitmap
} FontDef;

/* Font 7x10 */
extern FontDef Font_7x10;

#endif /* FONTS_H */
