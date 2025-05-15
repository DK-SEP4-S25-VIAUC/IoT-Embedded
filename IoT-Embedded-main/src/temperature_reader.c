#include <stddef.h>
#include "temperature_reader.h"
#include "dht11.h"

TEMP_ERROR_t temperature_reader_get(uint8_t* temperature)
{
    uint8_t temp_int = 0, temp_dec = 0;

    if (dht11_get(NULL, NULL, &temp_int, &temp_dec) == DHT11_OK)
    {
        if (temperature != NULL)
            *temperature = temp_int;
        return TEMP_OK;
    }

    return TEMP_FAIL;
}