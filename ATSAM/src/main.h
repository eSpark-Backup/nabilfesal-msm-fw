#ifndef MAIN_H    
#define MAIN_H

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */

#include <stdint.h>
#include <stddef.h>                     
#include <stdbool.h>                    
#include <stdlib.h>  
#include <cstring>
#include <math.h>
#include "definitions.h" 
#include "ADE9000.h" 

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Definition                                                        */
/* ************************************************************************** */
/* ************************************************************************** */
#define ARRAYSIZE_PER_UNIT  162 //(40 * 4) + 2
#define TOTAL_ADE9000_UNITS 12
#define TOTAL_ARRAYSIZE (ARRAYSIZE_PER_UNIT * TOTAL_ADE9000_UNITS)
#define UART_RX_BUFFER_SIZE 512
#define UART_TX_BUFFER_SIZE (TOTAL_ARRAYSIZE * 2 + 1)

void delay_ms(uint32_t milliseconds);
uint16_t extract_channel_values(const char* channel);
void parse_ct_ratio_json();
void parse_i_offset_json();
void parse_v_offset_json();
#endif 
/* *********************************#define ARRAYSIZE_PER_UNIT 40
#define TOTAL_ADE9000_UNITS 12
#define TOTAL_ARRAYSIZE (ARRAYSIZE_PER_UNIT * TOTAL_ADE9000_UNITS)
****************************************** */

