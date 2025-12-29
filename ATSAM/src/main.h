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

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Definition                                                        */
/* ************************************************************************** */
/* ************************************************************************** */
#define ENUM_BASE 1
#define ENUM_STEP 4
#define X(name, index) name = (ENUM_BASE + (index * ENUM_STEP))

#define ARRAYSIZE_PER_UNIT  162 //(40 * 4) + 2
#define TOTAL_ADE9000_UNITS 12
#define TOTAL_ARRAYSIZE (ARRAYSIZE_PER_UNIT * TOTAL_ADE9000_UNITS)
#define UART_RX_BUFFER_SIZE 2048
#define UART_TX_BUFFER_SIZE (TOTAL_ARRAYSIZE * 2 + 1)

#define UART_TIMEOUT_CHECK_INTERVAL      10
#define UART_READ_TIMEOUT                500

#define PHASE_A_STRING   "phase_a"
#define PHASE_B_STRING   "phase_b"
#define PHASE_C_STRING   "phase_c"

typedef struct
{
    uint8_t channelID;
    float phase_a;
    float phase_b;
    float phase_c;
} CurrentCalibration;


extern char rxBuffer[UART_RX_BUFFER_SIZE];
extern char txBuffer[UART_TX_BUFFER_SIZE];
extern size_t rxSize;
extern volatile bool rxComplete;        // not required after queue implementation?
extern volatile bool rxTimedout;
extern volatile bool rxStarted;         // flag to determine if uart reception have started or not, to avoid timeout occuring at startup
extern volatile uint64_t rxStartTimestamp;

#ifdef __cplusplus // Provide C++ Compatibility
extern "C" {
#endif

void timer_callback(TC_TIMER_STATUS status, uintptr_t context);
void delay_ms(uint64_t milliseconds);
uint64_t get_msCounter(void);
float extract_channel_values(const char* channel);
void parse_ct_ratio_json();
void parse_i_offset_json();
void parse_v_offset_json();

void ProcessCommand(void);
void USART_ReadCallback(uintptr_t context);
void CheckUSART_Timeout(void);
void StartUSART_Reading(void);
void SetupUSART(void);

void parse_reboot_calibration_json(void);           // calibration parameters received on system power-up/reboot
void parse_v_calibration_json(void);
void parse_i_calibration_json(void);
void sendChannelJsonData(uint8_t channel);
void ConvertStatusToHexAndSend(void);


#ifdef __cplusplus  // Provide C++ Compatibility
}
#endif

#endif
/* *********************************#define ARRAYSIZE_PER_UNIT 40
#define TOTAL_ADE9000_UNITS 12
#define TOTAL_ARRAYSIZE (ARRAYSIZE_PER_UNIT * TOTAL_ADE9000_UNITS)
****************************************** */

