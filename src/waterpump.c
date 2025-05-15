#include "waterpump.h"
#include <avr/io.h>
#include <util/delay.h>


#define PUMP_PORT   PORTC
#define PUMP_DDR    DDRC
#define PUMP_PIN    PC7          // svarer til digital pin 30
#define PUMP_ACTIVE_HIGH  1      // tændes med HIGH

static inline void pump_set_off(void)
{
#if PUMP_ACTIVE_HIGH
    PUMP_PORT &= ~_BV(PUMP_PIN);   // OFF = LOW
#else
    PUMP_PORT |=  _BV(PUMP_PIN);   // OFF = HIGH
#endif
}

static inline void pump_set_on(void)
{
#if PUMP_ACTIVE_HIGH
    PUMP_PORT |=  _BV(PUMP_PIN);   // ON = HIGH
#else
    PUMP_PORT &= ~_BV(PUMP_PIN);   // ON = LOW
#endif
}

void waterpump_init(void)
{
    pump_set_off();               // off‑niveau FØR pin bliver output
    PUMP_DDR |= _BV(PUMP_PIN);    //  PC7 output
}

void waterpump_on(void)  { pump_set_on();  }
void waterpump_off(void) { pump_set_off(); }

void waterpump_run(uint8_t sec)
{
    waterpump_on();
    while (sec--)
        _delay_ms(1000);
    waterpump_off();
}

