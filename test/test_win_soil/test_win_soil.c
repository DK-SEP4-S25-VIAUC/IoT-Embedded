#include "../fff.h"
#include "unity.h"

#include "soil.h"

DEFINE_FFF_GLOBALS
/* ---- Fakes ---- */
FAKE_VALUE_FUNC(uint16_t, read_adc_value)

/* ---- Globale register-fakes (samme navne som i soil.c) ---- */
uint8_t ADMUX;
uint8_t ADCSRA;
uint8_t ADCSRB;
uint16_t ADC;   // brugt af soil.c’s statiske helper

void setUp(void)
{
    RESET_FAKE(read_adc_value);
}

/* Init-test: tjekker at ADMUX/ADCSRA sættes til det rigtige */
void test_soil_init_sets_registers_correctly(void)
{
    soil_sensor_init();
    /* 5 V ref (REFS0) + kanal 5 (0b0101) = 0x40 | 0x05 = 0x45 = 69 dec */
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(0x45, ADMUX, "ADMUX forkert");
    /* Enable + prescaler ÷64 => ADEN|ADPS2|ADPS1 = 10000110b = 0x86 */
    TEST_ASSERT_EQUAL_HEX8(0x86, ADCSRA);
}

/* Read-test: tjekker mapping fra rå ADC til procent */
void test_soil_read_returns_100_percent_when_wet(void)
{
    read_adc_value_fake.return_val = 188;   // konstanten 'wet' :contentReference[oaicite:8]{index=8}:contentReference[oaicite:9]{index=9}
    uint8_t hum = soil_sensor_read();
    TEST_ASSERT_EQUAL_UINT8(100, hum);
}

void test_soil_read_returns_0_percent_when_dry(void)
{
    read_adc_value_fake.return_val = 450;   // konstanten 'dry' :contentReference[oaicite:10]{index=10}:contentReference[oaicite:11]{index=11}
    uint8_t hum = soil_sensor_read();
    TEST_ASSERT_EQUAL_UINT8(0, hum);
}

void test_soil_read_clamps_negative_values_to_0(void)
{
    read_adc_value_fake.return_val = 900;   // urealistisk højt => negativ %
    uint8_t hum = soil_sensor_read();
    TEST_ASSERT_EQUAL_UINT8(0, hum);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_soil_init_sets_registers_correctly);
    RUN_TEST(test_soil_read_returns_100_percent_when_wet);
    RUN_TEST(test_soil_read_returns_0_percent_when_dry);
    RUN_TEST(test_soil_read_clamps_negative_values_to_0);
    return UNITY_END();
}
