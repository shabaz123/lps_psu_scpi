//
// Created by shabaz on 08/05/2024.
//

#ifndef PSU_SCPI_ADAPTER_PSU_UTILS_H
#define PSU_SCPI_ADAPTER_PSU_UTILS_H

/********** includes **********/
#include "pico/stdlib.h"
#include "scpi/scpi.h"

#define INSTRUMENT_SOURCE_COMMANDS \
    {.pattern = "SOURce#:VOLTage:LEVel:IMMediate:AMPlitude", .callback = SCPI_PsuVoltage,}, \
    {.pattern = "SOURce#:VOLTage:LEVel:IMMediate:AMPlitude?", .callback = SCPI_PsuVoltageQ,}, \
    {.pattern = "SOURce#:CURRent:LEVel:IMMediate:AMPlitude", .callback = SCPI_PsuCurrent,}, \
    {.pattern = "SOURce#:CURRent:LEVel:IMMediate:AMPlitude?", .callback = SCPI_PsuCurrentQ,}, \
    {.pattern = "SOURce#:OUTPut:STATe", .callback = SCPI_PsuOutState,},

/********* functions *********/
void initPsuUtils(); // configures the PSU feature

scpi_result_t SCPI_PsuVoltage(scpi_t * context);
scpi_result_t SCPI_PsuVoltageQ(scpi_t * context);
scpi_result_t SCPI_PsuCurrent(scpi_t * context);
scpi_result_t SCPI_PsuCurrentQ(scpi_t * context);
scpi_result_t SCPI_PsuOutState(scpi_t * context);

#endif //PSU_SCPI_ADAPTER_PSU_UTILS_H
