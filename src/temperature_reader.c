#include <stddef.h>
#include "temperature_reader.h"
#include "dht11.h"

TEMP_ERROR_t temperature_and_humidity_get(uint8_t* temperature, uint8_t* humidity)
{
    uint8_t temp_int = 0, temp_dec = 0;
    uint8_t hum_int = 0, hum_dec = 0;

    if (dht11_get(&hum_int, &hum_dec, &temp_int, &temp_dec) == DHT11_OK)
    {
        if (temperature != NULL)
            *temperature = temp_int;
        if (humidity != NULL)
            *humidity = hum_int;
        return TEMP_OK;
    }

    return TEMP_FAIL;
}
