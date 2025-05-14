#ifndef WATERPUMP_H
#define WATERPUMP_H

#include <stdint.h>

// Funktioner til at styre vandpumpen
void waterpump_init(void); //initaliserer vandpumpen
void waterpump_on(void);      // Tænd vandpumpen
void waterpump_off(void);     // Sluk vandpumpen
void waterpump_run(uint8_t seconds); // Kør pumpen i et bestemt antal sekunder

#endif
