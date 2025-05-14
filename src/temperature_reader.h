#pragma once
#include <stdint.h>

typedef enum {
    TEMP_OK,
    TEMP_FAIL
} TEMP_ERROR_t;

TEMP_ERROR_t temperature_and_humidity_get(uint8_t* temperature, uint8_t* humidity);