#ifndef SOIL_SENSOR_H
#define SOIL_SENSOR_H

#include <stdint.h>

//Initialiser ADC-kanalen til jordfugtighedssensoren
void soil_sensor_init(void);

//Returnér fugtighed i % (0-100)
uint8_t soil_sensor_read(void);

#endif /* SOIL_SENSOR_H */