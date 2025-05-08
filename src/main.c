#include "wifi.h"
#include "uart.h"
#include "soil.h"
#include "temperature_reader.h"
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <avr/interrupt.h>


static uint8_t _buff[100];
static uint8_t _index = 0;
volatile static bool _done = false;

void console_rx(uint8_t _rx)
{
    uart_send_blocking(USART_0, _rx);
    if (('\r' != _rx) && ('\n' != _rx))
    {
        if (_index < 100 - 1)
        {
            _buff[_index++] = _rx;
        }
    }
    else
    {
        _buff[_index] = '\0';
        _index = 0;
        _done = true;
        uart_send_blocking(USART_0, '\n');
    }
}

int main()
{
    char soil_text[50];
    char temp_text[50];
    uint8_t temperature = 0;

    uart_init(USART_0, 9600, console_rx);
    uart_send_string_blocking(USART_0, "Booting...\r\n");

    wifi_init();
    uart_send_string_blocking(USART_0, "WiFi init OK\r\n");

    soil_sensor_init();
    uart_send_string_blocking(USART_0, "Soil sensor init OK\r\n");

    sei();

    uart_send_string_blocking(USART_0, "Connecting to WiFi...\r\n");
    wifi_command_join_AP("TASKALE70", "cen7936219can");
    uart_send_string_blocking(USART_0, "WiFi connected!\r\n");

    uart_send_string_blocking(USART_0, "Connecting to TCP server...\r\n");
    wifi_command_create_TCP_connection("20.13.198.93", 5000, NULL, NULL);
    uart_send_string_blocking(USART_0, "TCP connection established!\r\n");

    wifi_command_TCP_transmit((uint8_t*)"Welcome from SEP4 IoT hardware!\n", 32);

    while (1)
{
    if (_done)
    {
        wifi_command_TCP_transmit(_buff, strlen((char*)_buff));
        uart_send_string_blocking(USART_0, "Sent user input via TCP\r\n");
        _done = false;
    }

    /* jordfugtighed */

    uint8_t humidity = soil_sensor_read();
    sprintf(soil_text, "{\"soil_humidity_value\":%d}\r\n", humidity);

    wifi_command_TCP_transmit((uint8_t*)soil_text, strlen(soil_text));
    uart_send_string_blocking(USART_0, "Sent: ");
    uart_send_string_blocking(USART_0, soil_text);

    _delay_ms(60000);  //1min

    /*  temperatur  */
    
    if (temperature_reader_get(&temperature) == TEMP_OK)
    {
        sprintf(temp_text, "{\"temperature_value\":%d}\r\n", temperature);

        wifi_command_TCP_transmit((uint8_t*)temp_text, strlen(temp_text));
        uart_send_string_blocking(USART_0, "Sent: ");
        uart_send_string_blocking(USART_0, temp_text);
    }
    else
    {
        uart_send_string_blocking(USART_0, "Failed to read temperature\r\n");
    }

    _delay_ms(5000);        // måling hvert 5 sekund
}

    return 0;
}
