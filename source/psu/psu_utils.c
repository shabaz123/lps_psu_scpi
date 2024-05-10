//
// psu_utils.c
// rev 1 - shabaz - May 2024
//

#include "psu_utils.h"
#include <stdio.h>

// ************ Definitions ************
#define PSU_UART_ID uart1
#define PSU_BAUD_RATE 2400
#define PSU_UART_DATA_BITS 8
#define PSU_UART_STOP_BITS 1
#define PSU_UART_PARITY    UART_PARITY_NONE
#define PSU_UART_TX_PIN 8
#define PSU_UART_RX_PIN 9

#define DEBUG_LED_PIN 25
#define DEBUG_LED_ON gpio_put(DEBUG_LED_PIN, 1)
#define DEBUG_LED_OFF gpio_put(DEBUG_LED_PIN, 0)

// expect max 240 bytes per second at 2400 baud
// lets say 10ms per byte (100 bytes per second)
#define USEC_INTER_BYTE 10000
// expect to wait 300msec for a response
#define USEC_RESPONSE_WAIT 300000

// ********* UART functions **********
void
flush_uart_rx(void) {
    uint8_t dummy[1];
    while(1) {
        sleep_ms(10);
        if (uart_is_readable(PSU_UART_ID)) {
            uart_read_blocking(PSU_UART_ID, dummy, 1);
        } else {
            break;
        }
    }
}

uint8_t
read_uart_string(uint8_t* buf) {
    uint32_t i = 0;
    buf[0] = '\0';
    // read the first byte from the UART
    if (uart_is_readable_within_us(PSU_UART_ID, USEC_RESPONSE_WAIT)) {
        buf[i] = uart_getc(PSU_UART_ID);
        i++;
    } else {
        return(0);
    }
    // read the rest of the bytes
    while(1) {
        if (uart_is_readable_within_us(PSU_UART_ID, USEC_INTER_BYTE)) {
            buf[i] = uart_getc(PSU_UART_ID);
            if (i == 30) { // max 32 bytes
                buf[i+1] = '\0';
                break;
            }
            if ((buf[i] == '\n') || (buf[i] == '\r') || (buf[i] == '\0')){
                buf[i] = '\0';
                break;
            }
            i++;
        } else {
            buf[i] = '\0';
            break;
        }
    }
    return(i);
}

// ************ PSU UART functions ************
void do_vset(uint32_t index, float value) {
    char buf[32];
    int len;
    if ((index==1) || (index==2)) {
        sprintf(buf, "VSET%d %.3f\n\r", index, value);
    } else if (index==3) { // channel 3 is a fixed voltage channel
        return;
    } else { // no other channels are supported
        return;
    }
    len = strlen(buf);
    uart_write_blocking(PSU_UART_ID, (const uint8_t*)buf, len);
    return;
}

float do_vget(uint32_t index) {
    char buf[32];
    int len;
    // flush the UART RX buffer in case there's any old data
    flush_uart_rx();
    sprintf(buf, "VOUT%d\n\r", index);
    len = strlen(buf);
    uart_write_blocking(PSU_UART_ID, (const uint8_t*)buf, len);
    uint8_t rx_buf[32];
    int rx_len = 0;
    rx_len = read_uart_string(rx_buf);
    // parse the response
    float v;
    if (rx_len > 1) {
        sscanf((const char*)rx_buf, "%f", &v);
    } else {
        v = -999.9;
    }
    return(v);
}

void do_iset(uint32_t index, float value) {
    char buf[32];
    int len;
    sprintf(buf, "ISET%d %.3f\n\r", index, value);
    len = strlen(buf);
    uart_write_blocking(PSU_UART_ID, (const uint8_t*)buf, len);
    return;
}

float do_iget(uint32_t index) {
    char buf[32];
    int len;
    // flush the UART RX buffer in case there's any old data
    flush_uart_rx();
    sprintf(buf, "IOUT%d\n\r", index);
    len = strlen(buf);
    uart_write_blocking(PSU_UART_ID, (const uint8_t*)buf, len);
    uint8_t rx_buf[32];
    int rx_len = 0;
    rx_len = read_uart_string(rx_buf);
    // parse the response
    float v;
    if (rx_len > 1) {
        sscanf((const char*)rx_buf, "%f", &v);
    } else {
        v = -999.9;
    }
    return(v);
}

void do_outstate(uint32_t index, int32_t value) {
    char buf[32];
    int len;
    // channels 1 and 2 can only be enabled/disabled together
    if ((index==1) || (index==2)) {
        sprintf(buf, "OUT%d\n\r", value);
    } else if (index==3) { // channel 3 is a fixed voltage channel
        if (value == 0) {
            sprintf(buf, "VDD0\n\r");
        } else {
            sprintf(buf, "VDD5\n\r");
        }
    } else { // no other channels are supported
        return;
    }
    len = strlen(buf);
    uart_write_blocking(PSU_UART_ID, (const uint8_t*)buf, len);
    return;
}

// ************ Initialize the Pico board to support the PSU ************
void initPsuUtils() {
    // initialize UART1 for 2400 baud
    uart_init(PSU_UART_ID, PSU_BAUD_RATE);
    gpio_set_function(PSU_UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(PSU_UART_RX_PIN, GPIO_FUNC_UART);
    gpio_set_outover(PSU_UART_TX_PIN, GPIO_OVERRIDE_INVERT); // invert UART to look like RS232
    gpio_set_inover(PSU_UART_RX_PIN, GPIO_OVERRIDE_INVERT);  // invert UART to look like RS232
    uart_set_baudrate(PSU_UART_ID, PSU_BAUD_RATE);
    uart_set_format(PSU_UART_ID, PSU_UART_DATA_BITS, PSU_UART_STOP_BITS, PSU_UART_PARITY);
    // set up the debug LED
    gpio_init(DEBUG_LED_PIN);
    gpio_set_dir(DEBUG_LED_PIN, GPIO_OUT);
    DEBUG_LED_OFF;
    return;
}

#ifdef JUNK
uint32_t psuPinCount() {
    return(3);
}

void initPsuPins() {
    return;
}

void setPsuVoltage(uint32_t index, float value) {
    return;
}

float getPsuVoltage(uint32_t index) {
    return(0.0);
}
#endif

// ************* SCPI related functions *************
// retrieve context number from SCPI commands (this will be the PSU channel number)
int32_t get_context_number(scpi_t * context) {
    int32_t numbers[1];
    SCPI_CommandNumbers(context, numbers, 1, 0);
    // for the PSU, the context number is the PSU channel index (1-3)
    if ((numbers[0] < 1) || (numbers[0] > 3)) {
        return(-1);
    }
    return(numbers[0]);
}

// Voltage-related SCPI commands
scpi_result_t SCPI_PsuVoltage(scpi_t * context) {
    float param1;
    int32_t chan;
    // retrieve the PSU channel index (1-3) from the SCPI command
    chan = get_context_number(context);
    if (chan < 0) {
        SCPI_ErrorPush(context, SCPI_ERROR_INVALID_SUFFIX);
        return SCPI_RES_ERR;
    }
    // read first parameter if present
    if (!SCPI_ParamFloat(context, &param1, TRUE)) {
        return SCPI_RES_ERR;
    }
    // set the voltage by issuing a command to the PSU
    do_vset(chan, param1);
    DEBUG_LED_ON;
    return SCPI_RES_OK;
}

scpi_result_t SCPI_PsuVoltageQ(scpi_t * context) {
    int32_t chan;
    // retrieve the PSU channel index (1-3) from the SCPI query
    chan = get_context_number(context);
    if (chan < 0) {
        SCPI_ErrorPush(context, SCPI_ERROR_INVALID_SUFFIX);
        return SCPI_RES_ERR;
    }
    // get the voltage by issuing a command to the PSU
    float return_val = do_vget(chan);
    // return the value to the SCPI query
    SCPI_ResultFloat(context, return_val);
    return SCPI_RES_OK;
}

// Current-related SCPI commands
scpi_result_t SCPI_PsuCurrent(scpi_t * context) {
    float param1;
    int32_t chan;
    // retrieve the PSU channel index (1-3) from the SCPI command
    chan = get_context_number(context);
    if (chan < 0) {
        SCPI_ErrorPush(context, SCPI_ERROR_INVALID_SUFFIX);
        return SCPI_RES_ERR;
    }
    // read first parameter if present
    if (!SCPI_ParamFloat(context, &param1, TRUE)) {
        return SCPI_RES_ERR;
    }
    // set the current by issuing a command to the PSU
    do_iset(chan, param1);
    DEBUG_LED_ON;
    return SCPI_RES_OK;
}

scpi_result_t SCPI_PsuCurrentQ(scpi_t * context) {
    int32_t chan;
    // retrieve the PSU channel index (1-3) from the SCPI query
    chan = get_context_number(context);
    if (chan < 0) {
        SCPI_ErrorPush(context, SCPI_ERROR_INVALID_SUFFIX);
        return SCPI_RES_ERR;
    }
    // get the current by issuing a command to the PSU
    float return_val = do_iget(chan);
    // return the value to the SCPI query
    SCPI_ResultFloat(context, return_val);
    return SCPI_RES_OK;
}

// PSU output state SCPI command, used to turn channels on/off
scpi_result_t SCPI_PsuOutState(scpi_t * context) {
    int32_t chan;
    // retrieve the PSU channel index (1-3) from the SCPI command
    chan = get_context_number(context);
    if (chan < 0) {
        SCPI_ErrorPush(context, SCPI_ERROR_INVALID_SUFFIX);
        return SCPI_RES_ERR;
    }
    // read first parameter if present
    int32_t param1;
    if (!SCPI_ParamInt32(context, &param1, TRUE)) {
        return SCPI_RES_ERR;
    }
    // set the output state by issuing a command to the PSU
    do_outstate(chan, param1);
    return SCPI_RES_OK;
}


