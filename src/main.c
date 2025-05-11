#include "wifi.h"
#include "uart.h"
#include "soil.h"
#include "temperature_reader.h"
#include "waterpump.h"
#include <util/delay.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <avr/interrupt.h>


static uint8_t _buff[100];
static uint8_t _index = 0;
volatile static bool _done = false;
static uint8_t pump_time = 0;
static char sensor_payload[100];

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

// Funktion til at modtage TCP-besked og handle på vandpumpen
/*void handle_tcp_waterpump_command(char* message) {
    if (strstr(message, "water")) {
        char* time_str = strstr(message, "\"sec\":");
        if (time_str) {
            pump_time = atoi(time_str + 6);  // Konverter tiden fra string til integer
            if (pump_time > 0) {
                waterpump_run(pump_time);  // Kører pumpen i den ønskede tid
            }
        }
    }
    //Besked eksempel: {"cmd":"water", "sec":10}
}
*/
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

    waterpump_init();
    uart_send_string_blocking(USART_0,"waterpump init OK\r\n");

    

    sei();

    uart_send_string_blocking(USART_0, "Connecting to WiFi...\r\n");
    wifi_command_join_AP("TASKALE70", "cen7936219can");
    uart_send_string_blocking(USART_0, "WiFi connected!\r\n");

    /*  OPRET TCP‑FORBINDELSE */

    uart_send_string_blocking(USART_0, "Connecting to TCP server...\r\n");
    wifi_command_create_TCP_connection("4.209.30.127 ", 5000, NULL, NULL);
    uart_send_string_blocking(USART_0, "TCP connection established!\r\n");

    uart_send_string_blocking(USART_0, "Welcome from SEP4 IoT hardware!\n");

    while (1)
{
     if (_done)
     {
         wifi_command_TCP_transmit(_buff, strlen((char*)_buff));
         uart_send_string_blocking(USART_0, "Sent user input via TCP\r\n");
         _done = false;
     }
 
     /*  læs jordfugtighed og temperatur */
     uint8_t humidity      = soil_sensor_read();
     uint8_t temperature   = 0;
     bool    temp_ok       = (temperature_reader_get(&temperature) == TEMP_OK);
 
     /*  én samlet JSON‑streng */
     if (temp_ok)
     {
         sprintf(sensor_payload,"{\"soil_humidity\":%d,\"air_temperature\":%d}\r\n",humidity, temperature);
 

         wifi_command_TCP_transmit((uint8_t*)sensor_payload,
                                   strlen(sensor_payload));
         uart_send_string_blocking(USART_0, "Sent: ");
         uart_send_string_blocking(USART_0, sensor_payload);
     }
     else
     {
         uart_send_string_blocking(USART_0,
                                   "Failed to read temperature\r\n");
     }
 
     /* håndter evt. kommando til vandpumpe (hvis/når den virker) */
     /*char message[100];
     if (wifi_receive_message(message, sizeof(message)))
     {
         handle_tcp_waterpump_command(message);
     }*/

     /* LUK TCP‑FORBINDELSEN 
     wifi_command_close_TCP_connection();
     uart_send_string_blocking(USART_0,"TCP closed\r\n");

     */
     /* 6) – vent en time før næste måling */
     _delay_ms(3600000);
 }

    return 0;
}
