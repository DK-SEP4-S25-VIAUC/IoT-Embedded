#include "waterpump.h"
#include <avr/io.h>
#include <util/delay.h>

#define RELAY_PORT  PORTC    
#define RELAY_DDR   DDRC 
#define RELAY_PIN   PC7       


// Initialiserer vandet pumpen
void waterpump_init(void) {
    RELAY_DDR  |= (1 << RELAY_PIN);  // Sætter pin som udgang
    RELAY_PORT |= (1 << RELAY_PIN);  // er off som standard
}

// Tænder vandpumpen
void waterpump_on(void) {
    RELAY_PORT &= ~(1 << RELAY_PIN);  
}

// Slukker vandpumpen
void waterpump_off(void) {
    RELAY_PORT |= (1 << RELAY_PIN); 
}

// Kører pumpen i et bestemt antal sekunder
void waterpump_run(uint32_t seconds) {
    waterpump_on();  // Tænd pumpen
    _delay_ms(seconds * 1000);  // Vent i sekunder
    waterpump_off();  // Sluk pumpen
}
