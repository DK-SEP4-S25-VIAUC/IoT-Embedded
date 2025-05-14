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

#define INITIAL_WATERING_TIME_SEC 2
#define SENSOR_UPLOAD_INTERVAL_MS 600000UL   // 10 min
#define KEEPALIVE_INTERVAL_MS      120000UL   // 2 min (skal være under 3 min ellers timeout i server)



static uint8_t   console_buff[100];
static uint8_t   console_idx  = 0;
volatile bool    console_done = false;

static char      tcp_rx_buf[128];
volatile uint8_t pump_seconds_requested = 0;   // sættes i TCP‑callback


//  millis() – simpel stub (evt. Timer0 overflow‑interrupt på AVR)
static volatile uint32_t _millis = 0;
ISR(TIMER0_OVF_vect) {                        // 1 ms pr. overflow (tilpas prescaler)
    ++_millis;
}
static inline uint32_t millis(void) {
    uint32_t m;
    uint8_t sreg = SREG; cli(); m = _millis; SREG = sreg;
    return m;
}


//  UART‑modtagehandler
static void console_rx(uint8_t rx)
{
    uart_send_blocking(USART_0, rx);          // ekko til terminal

    if (rx != '\r' && rx != '\n') {
        if (console_idx < sizeof console_buff - 1) {
            console_buff[console_idx++] = rx;
        }
    } else {
        console_buff[console_idx] = '\0';
        console_idx  = 0;
        console_done = true;
        uart_send_blocking(USART_0, '\n');
    }
}


static void tcp_message_callback(void)
{
    /* NUL‑termineret buffer antages */
    uart_send_string_blocking(USART_0, "Received: ");
    uart_send_string_blocking(USART_0, tcp_rx_buf);
    uart_send_string_blocking(USART_0, "");

    /* find "cmd" */
    char *cmd_ptr = strcasestr(tcp_rx_buf, "\"cmd\"");
    if (!cmd_ptr) return;  // ingen cmd‑felt

    /* find næste kolon og næste citationstegn */
    cmd_ptr = strchr(cmd_ptr, ':');
    if (!cmd_ptr) return;
    cmd_ptr = strchr(cmd_ptr, '"');
    if (!cmd_ptr) return;
    ++cmd_ptr;  // peg på selve værdien

    /* sammenligner værdien (indtil næste ") */
    if (strncasecmp(cmd_ptr, "water", 5) != 0) return;  // ikke water‑cmd

    /* find "ml" feltet */
    char *ml_ptr = strcasestr(tcp_rx_buf, "\"ml\"");
    if (!ml_ptr) return;
    ml_ptr = strchr(ml_ptr, ':');
    if (!ml_ptr) return;
    uint16_t ml_val = (uint16_t)atoi(ml_ptr + 1);  // kolon + evt. mellemrum
    if (ml_val == 0 || ml_val > 4000) return;  // hvis volumen er 0 eller for høj

    // Beregn tiden (i sekunder) pumpen skal køre for at pumpe den ønskede mængde ml
    // 45.45 ml pr. sekund
    uint16_t pump_time_sec = (uint16_t)((ml_val / 45.45));  // 45.45 ml per sekund

    pump_seconds_requested = pump_time_sec;  // opdater det ønskede antal sekunder
    uart_send_string_blocking(USART_0, "ml = ");
    char num[6];
    sprintf(num, "%u", ml_val);
    uart_send_string_blocking(USART_0, num);
    uart_send_string_blocking(USART_0, "");
}//  Eksempel: {"cmd":"water","ml":10}

static bool ping_or_reconnect(void)
{
    // prøver at sende PING til serveren
    if (wifi_command_TCP_transmit((uint8_t*)"PING\r\n", 6) == WIFI_OK) {
        uart_send_string_blocking(USART_0, "PING sent\r\n");
        return true;
    }

    // socket lukkes
    uart_send_string_blocking(USART_0, "PING failed, reconnecting…\r\n");

    // Lukker session ned
    wifi_command_close_TCP_connection();

    // Ny forbindelse
    if (wifi_command_create_TCP_connection("4.207.72.20",5000,tcp_message_callback,tcp_rx_buf) == WIFI_OK)
    {
        uart_send_string_blocking(USART_0, "TCP reconnected!\r\n");
        return true; 
    }

    uart_send_string_blocking(USART_0, "TCP reconnect failed\r\n");
    return false;
}

int main(void)
{
    char sensor_payload[64];

    //inits
    uart_init(USART_0, 9600, console_rx);
    uart_send_string_blocking(USART_0, "Booting...\r\n");

    soil_sensor_init();
    uart_send_string_blocking(USART_0, "soil censor OK!\r\n");
    waterpump_init();

    // start med at vande lidt
    waterpump_run(INITIAL_WATERING_TIME_SEC);
    uart_send_string_blocking(USART_0, "Initial watering done\r\n");

    // Start millis‑timer  (Timer0, prescaler 64, 1 ms interrupt @ 16 MHz)
    TCCR0A = 0;                  // normal mode
    TCCR0B = (1 << CS01) | (1 << CS00); // prescaler 64
    TIMSK0 = (1 << TOIE0);       // enable overflow IRQ

    // Wi‑Fi + TCP
    wifi_init();
     uart_send_string_blocking(USART_0, "Connecting to WiFi...\r\n");
    wifi_command_join_AP("TASKALE70", "cen7936219can");
     uart_send_string_blocking(USART_0, "WiFi connected!\r\n");

    uart_send_string_blocking(USART_0, "Connecting to TCP server...\r\n");
    wifi_command_create_TCP_connection("4.207.72.20", 5000,tcp_message_callback, tcp_rx_buf);
    uart_send_string_blocking(USART_0, "TCP connection established!\r\n");

    sei();                 

    uint32_t next_upload_ms = millis();
    uint32_t next_keepalive_ms = millis() + KEEPALIVE_INTERVAL_MS;

    for (;;) {
        /* Håndter vandpumpekommando fra TCP hvis der er nogen */
        if (pump_seconds_requested) {
            uint8_t sec = pump_seconds_requested; pump_seconds_requested = 0;
            uart_send_string_blocking(USART_0, " Watering...\r\n");
            waterpump_run(sec);
            uart_send_string_blocking(USART_0, "Watering done\r\n");
        }

        uint32_t now = millis();

        /* Upload sensordata hvert 10. minut */
        if ((int32_t)(now - next_upload_ms) >= 0) {
            uint8_t hum  = soil_sensor_read();
            uint8_t temp = 0;
            if (temperature_reader_get(&temp) == TEMP_OK) {
                sprintf(sensor_payload, "{\"soil_humidity\":%u,\"air_temperature\":%u}\r\n", hum, temp);
                wifi_command_TCP_transmit((uint8_t*)sensor_payload, strlen(sensor_payload));
                uart_send_string_blocking(USART_0, "Sent ");
                uart_send_string_blocking(USART_0, sensor_payload);
            }
            next_upload_ms += SENSOR_UPLOAD_INTERVAL_MS;
        }

        /* Keep‑alive ping hver 2 min så serveren ikke interrupter */
        if ((int32_t)(now - next_keepalive_ms) >= 0) {
            ping_or_reconnect();
            next_keepalive_ms += KEEPALIVE_INTERVAL_MS;
        }

        
        if (console_done) {
            wifi_command_TCP_transmit(console_buff,
                                      strlen((char *)console_buff));
            uart_send_string_blocking(USART_0,
                                      "Sent console input via TCP\r\n");
            console_done = false;
        }
    }

    return 0; 
}