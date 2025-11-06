// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include "main.h"
#include "ADE9000.h" 
#include <string>
#include <math.h>

// *****************************************************************************
// *****************************************************************************
// Section: Variables
// *****************************************************************************
// *****************************************************************************

#define ENUM_BASE 1
#define ENUM_STEP 4

#define X(name, index) name = (ENUM_BASE + (index * ENUM_STEP))

enum MyEnum
{
    X(RMS_Voltage_A, 0),
    X(RMS_Voltage_B, 1),
    X(RMS_Voltage_C, 2),
    X(Line_Voltage_A, 3),
    X(Line_Voltage_B, 4),
    X(Line_Voltage_C, 5),
    X(RMS_Current_A, 6),
    X(RMS_Current_B, 7),
    X(RMS_Current_C, 8),
    X(Freq_A, 9),
    X(Freq_B, 10),
    X(Freq_C, 11),
    X(Active_Power_A, 12),
    X(Active_Power_B, 13),
    X(Active_Power_C, 14),
    X(Reactive_Power_A, 15),
    X(Reactive_Power_B, 16),
    X(Reactive_Power_C, 17),
    X(Apparent_Power_A, 18),
    X(Apparent_Power_B, 19),
    X(Apparent_Power_C, 20),
    X(Power_Factor_A, 21),
    X(Power_Factor_B, 22),
    X(Power_Factor_C, 23),
    X(Pos_Active_Energy_A, 24),
    X(Pos_Active_Energy_B, 25),
    X(Pos_Active_Energy_C, 26),
    X(Neg_Active_Energy_A, 27),
    X(Neg_Active_Energy_B, 28),
    X(Neg_Active_Energy_C, 29),
    X(Voltage_THD_A, 30),
    X(Voltage_THD_B, 31),
    X(Voltage_THD_C, 32),
    X(Current_THD_A, 33),
    X(Current_THD_B, 34),
    X(Current_THD_C, 35),
    X(Temperature, 36),
    X(Angle_A, 37),
    X(Angle_B, 38),
    X(Angle_C, 39),
};

typedef struct
{
    uint8_t channelID;
    float phase_a;
    float phase_b;
    float phase_c;
} CurrentCalibration;

ADE9000UnitMeasurements unitMeasurements[TOTAL_ADE9000_UNITS];
ChannelConfig channelConfig[TOTAL_ADE9000_UNITS];
CurrentCalibration currentCalibration;

uint8_t ADE9000_hotplug[TOTAL_ADE9000_UNITS];
uint8_t ADE9000_status[TOTAL_ADE9000_UNITS];
uint8_t ADE9000_currentGain[TOTAL_ADE9000_UNITS];
uint8_t ADE9000_voltageGain[TOTAL_ADE9000_UNITS];
uint8_t ADE9000_arrayBuffer[TOTAL_ARRAYSIZE];

uint32_t reg_values[TOTAL_ADE9000_UNITS][TOTAL_PHASES];

char rxBuffer[UART_RX_BUFFER_SIZE];
char txBuffer[UART_TX_BUFFER_SIZE];
size_t rxSize = 0;
volatile bool rxComplete = false;

volatile uint32_t msCounter = 0;            // ticks count from tc0

// *****************************************************************************
// *****************************************************************************
// Section: Functions
// *****************************************************************************
// *****************************************************************************
static inline float cleanFloat(float x)
{
    return (isnan(x) || isinf(x)) ? 0.0f : x;
}

void timer_callback(TC_TIMER_STATUS status, uintptr_t context)
{
    msCounter++;
}

void delay_ms(uint32_t milliseconds)
{
    uint32_t start = msCounter;
    while ((msCounter - start) < milliseconds);
}

void storeIEEE754InBuffer(float floatValue, uint8_t *buffer, uint16_t index)
{
    if (index < 0 || index >= TOTAL_ARRAYSIZE - 4)
    {
        return;
    }
    union
    {
        float floatValue;
        uint8_t bytes[4];
    } converter;
    converter.floatValue = floatValue;
    buffer[index] = converter.bytes[3];
    buffer[index + 1] = converter.bytes[2];
    buffer[index + 2] = converter.bytes[1];
    buffer[index + 3] = converter.bytes[0];
}

uint8_t calculateCRC8(const uint8_t *data, size_t len)
{
    uint8_t crc = 0xFF; // Initial value
    for (size_t i = 0; i < len; ++i)
    {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x80)
            {
                crc = (crc << 1) ^ 0x31; // Polynomial 0x31 (Dallas/Maxim)
            }
            else
            {
                crc <<= 1;
            }
        }
    }
    return crc;
}

// *****************************************************************************
// *****************************************************************************
// Section: ADE9000 Functions
// *****************************************************************************
// *****************************************************************************

void sendChannelJsonData(uint8_t channel)
{
    if (channel == 0 || channel > 12)
        return;
    ADE9000UnitMeasurements &m = unitMeasurements[channel - 1];
    char jsonBuffer[2048]; // Increase if needed
    char *ptr = jsonBuffer;
    size_t remaining = sizeof(jsonBuffer);
    int written = snprintf(ptr, remaining,
                           "{"
                           "\"ID\":%d,"
                           "\"status\":%d,"
                           "\"temperature\":%.2f,"
                           "\"data\":{",
                           channel,
                           ADE9000_status[channel - 1],
                           cleanFloat(m.temperature.Temperature));
    ptr += written;
    remaining -= written;
    const char *phases[3] = {"phase_a", "phase_b", "phase_c"};
    float rmsVoltage[3] = {m.voltageRMS.VoltageRMSValue_A, m.voltageRMS.VoltageRMSValue_B, m.voltageRMS.VoltageRMSValue_C};
    float lineVoltage[3] = {m.lineVoltage.LineVoltage_AB, m.lineVoltage.LineVoltage_BC, m.lineVoltage.LineVoltage_CA};
    float frequency[3] = {m.periodFrequency.FrequencyValue_A, m.periodFrequency.FrequencyValue_B, m.periodFrequency.FrequencyValue_C};
    float current[3] = {m.currentRMS.CurrentRMSValue_A, m.currentRMS.CurrentRMSValue_B, m.currentRMS.CurrentRMSValue_C};
    float powerFactor[3] = {m.powerFactor.PowerFactorValue_A, m.powerFactor.PowerFactorValue_B, m.powerFactor.PowerFactorValue_C};
    float activePower[3] = {m.activePower.ActivePowerValue_A, m.activePower.ActivePowerValue_B, m.activePower.ActivePowerValue_C};
    float reactivePower[3] = {m.reactivePower.ReactivePowerValue_A, m.reactivePower.ReactivePowerValue_B, m.reactivePower.ReactivePowerValue_C};
    float apparentPower[3] = {m.apparentPower.ApparentPowerValue_A, m.apparentPower.ApparentPowerValue_B, m.apparentPower.ApparentPowerValue_C};
    float posEnergy[3] = {m.activeEnergy.PActiveEnergyValue_kWh_A, m.activeEnergy.PActiveEnergyValue_kWh_B, m.activeEnergy.PActiveEnergyValue_kWh_C};
    float negEnergy[3] = {m.activeEnergy.NActiveEnergyValue_kWh_A, m.activeEnergy.NActiveEnergyValue_kWh_B, m.activeEnergy.NActiveEnergyValue_kWh_C};

    float reactiveposEnergy[3];
    float reactivenegEnergy[3];
    if (m.activeEnergy.ReactiveEnergyValue_kWh_A > 0)
    {
        reactiveposEnergy[0] = m.activeEnergy.ReactiveEnergyValue_kWh_A;
        reactivenegEnergy[0] = 0;
    }
    else
    {
        reactivenegEnergy[0] = m.activeEnergy.ReactiveEnergyValue_kWh_A;
        reactiveposEnergy[0] = 0;
    }

    if (m.activeEnergy.ReactiveEnergyValue_kWh_B > 0)
    {
        reactiveposEnergy[1] = m.activeEnergy.ReactiveEnergyValue_kWh_B;
        reactivenegEnergy[1] = 0;
    }
    else
    {
        reactivenegEnergy[1] = m.activeEnergy.ReactiveEnergyValue_kWh_B;
        reactiveposEnergy[1] = 0;
    }

    if (m.activeEnergy.ReactiveEnergyValue_kWh_C > 0)
    {
        reactiveposEnergy[2] = m.activeEnergy.ReactiveEnergyValue_kWh_C;
        reactivenegEnergy[2] = 0;
    }
    else
    {
        reactivenegEnergy[2] = m.activeEnergy.ReactiveEnergyValue_kWh_C;
        reactiveposEnergy[2] = 0;
    }

    //    float reactiveEnergy[3] = { m.activeEnergy.ReactiveEnergyValue_kWh_A, m.activeEnergy.ReactiveEnergyValue_kWh_B, m.activeEnergy.ReactiveEnergyValue_kWh_C };

    float currentTHD[3] = {m.currentTHD.CurrentTHDValue_A, m.currentTHD.CurrentTHDValue_B, m.currentTHD.CurrentTHDValue_C};
    float voltageTHD[3] = {m.voltageTHD.VoltageTHDValue_A, m.voltageTHD.VoltageTHDValue_B, m.voltageTHD.VoltageTHDValue_C};
    float angle[3] = {m.angleDegree.AngleValue_VA_IA, m.angleDegree.AngleValue_VB_IB, m.angleDegree.AngleValue_VC_IC};
    for (int i = 0; i < 3; i++)
    {
        written = snprintf(ptr, remaining,
                           "\"%s\":{"
                           "\"general\":{"
                           "\"line_voltage\":%.2f,"
                           "\"rms_voltage\":%.2f,"
                           "\"frequency\":%.2f,"
                           "\"current\":%.2f"
                           "},"
                           "\"power\":{"
                           "\"factor\":%.2f,"
                           "\"active\":%.2f,"
                           "\"reactive\":%.2f,"
                           "\"apparent\":%.2f"
                           "},"
                           "\"energy\":{"
                           "\"active\":{"
                           "\"positive\":%.2f,"
                           "\"negative\":%.2f"
                           "},"
                           "\"reactive\":{"
                           "\"positive\":%.2f,"
                           "\"negative\":%.2f"
                           "}"
                           "},"
                           "\"thd\":{"
                           "\"current\":%.2f,"
                           "\"voltage\":%.2f,"
                           "\"angle\":%.2f"
                           "}"
                           "}%s",
                           phases[i],
                           cleanFloat(lineVoltage[i]), cleanFloat(rmsVoltage[i]), cleanFloat(frequency[i]), cleanFloat(current[i]),
                           cleanFloat(powerFactor[i]), cleanFloat(activePower[i]), cleanFloat(reactivePower[i]), cleanFloat(apparentPower[i]),
                           cleanFloat(posEnergy[i]), cleanFloat(negEnergy[i]), cleanFloat(reactiveposEnergy[i]), cleanFloat(reactivenegEnergy[i]),
                           cleanFloat(currentTHD[i]), cleanFloat(voltageTHD[i]), cleanFloat(angle[i]),
                           (i < 2) ? "," : "");
        ptr += written;
        remaining -= written;
    }
    // Close outer JSON objects
    written = snprintf(ptr, remaining, "}}\n");
    ptr += written;
    // Send over UART (SERCOM2)
    SERCOM2_USART_Write((uint8_t *)jsonBuffer, ptr - jsonBuffer);
}

void ADE9000MeasurementsForUnit(SPI_CSS_Pin cssPin, uint8_t unitIndex)
{
    uint16_t baseIndex = (unitIndex - 1) * ARRAYSIZE_PER_UNIT;
    ADE9000_arrayBuffer[baseIndex] = 0xAA + (unitIndex - 1);

    ReadVoltageRMSRegs(&unitMeasurements[unitIndex - 1].voltageRMS, cssPin);
    voltage_offset_calibration(&unitMeasurements[unitIndex - 1].voltageRMS, cssPin);
    //    ReadVoltageRMSOSRegs(&voltageRMSOS[unitIndex - 1], cssPin);

    ReadLineVoltages(&unitMeasurements[unitIndex - 1].voltageRMS, &unitMeasurements[cssPin - 1].lineVoltage);
    ReadCurrentRMSRegs(&unitMeasurements[unitIndex - 1].currentRMS, cssPin);
    current_offset_calibration(&unitMeasurements[unitIndex - 1].currentRMS, cssPin);
    ReadPeriodRegsnValues(&unitMeasurements[unitIndex - 1].periodFrequency, &unitMeasurements[unitIndex - 1].voltageRMS, cssPin);

    ReadActivePowerRegs(&unitMeasurements[unitIndex - 1].activePower, &unitMeasurements[unitIndex - 1].voltageRMS, &unitMeasurements[unitIndex - 1].currentRMS, cssPin);
    ReadReactivePowerRegs(&unitMeasurements[unitIndex - 1].reactivePower, &unitMeasurements[unitIndex - 1].voltageRMS, &unitMeasurements[unitIndex - 1].currentRMS, cssPin);
    ReadApparentPowerRegs(&unitMeasurements[unitIndex - 1].apparentPower, &unitMeasurements[unitIndex - 1].voltageRMS, &unitMeasurements[unitIndex - 1].currentRMS, cssPin);
    ReadPowerFactorRegsnValues(&unitMeasurements[unitIndex - 1].powerFactor, &unitMeasurements[unitIndex - 1].voltageRMS, &unitMeasurements[unitIndex - 1].currentRMS, cssPin);

    ReadEnergyRegs(&unitMeasurements[unitIndex - 1].activeEnergy, &unitMeasurements[unitIndex - 1].activePower, &unitMeasurements[unitIndex - 1].currentRMS, cssPin);

    ReadVoltageTHDRegsnValues(&unitMeasurements[unitIndex - 1].voltageTHD, cssPin);
    ReadCurrentTHDRegsnValues(&unitMeasurements[unitIndex - 1].currentTHD, cssPin);
    ReadAngleRegsnValues(&unitMeasurements[unitIndex - 1].angleDegree, cssPin);

    ReadTempRegnValue(&unitMeasurements[unitIndex - 1].temperature, cssPin);

    uint16_t crcIndex = ((unitIndex - 1) * ARRAYSIZE_PER_UNIT) + ARRAYSIZE_PER_UNIT - 1;
    uint8_t crc = calculateCRC8(&ADE9000_arrayBuffer[(unitIndex - 1) * ARRAYSIZE_PER_UNIT], ARRAYSIZE_PER_UNIT - 1);
    ADE9000_arrayBuffer[crcIndex] = crc;
}

void ADE9000MeasurementsForUnit_NON(SPI_CSS_Pin cssPin, uint8_t unitIndex)
{
    uint16_t baseIndex = (unitIndex - 1) * ARRAYSIZE_PER_UNIT;
    ADE9000_arrayBuffer[baseIndex] = 0xAA + (unitIndex - 1);

    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 61);
    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 65);
    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 69);

    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 73);
    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 77);
    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 81);

    // Settle
    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 85);
    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 89);
    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 93);

    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 121);
    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 125);
    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 129);

    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 25);
    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 29);
    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 33);

    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 1);
    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 5);
    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 9);

    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 13);
    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 17);
    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 21);

    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 37);
    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 41);
    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 45);

    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 49);
    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 53);
    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 57);

    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 97);
    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 101);
    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 105);

    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 109);
    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 113);
    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 117);

    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 133);
    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 137);
    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 141);

    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 145);

    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 149);
    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 153);
    storeIEEE754InBuffer(0, ADE9000_arrayBuffer, baseIndex + 157);

    uint16_t crcIndex = ((unitIndex - 1) * ARRAYSIZE_PER_UNIT) + ARRAYSIZE_PER_UNIT - 1;
    uint8_t crc = calculateCRC8(&ADE9000_arrayBuffer[(unitIndex - 1) * ARRAYSIZE_PER_UNIT], ARRAYSIZE_PER_UNIT - 1);
    ADE9000_arrayBuffer[crcIndex] = crc;
}

void printADE9000ArrayBuffer(uint8_t *buffer, size_t bufferSize)
{
    printf("ADE9000 Array Buffer:\n");
    for (size_t i = 0; i < bufferSize; i++)
    {
        printf("0x%02X ", buffer[i]);
        if ((i + 1) % 16 == 0)
        { // New line after every 16 bytes for better readability
            printf("\n");
        }
    }
    printf("\n"); // New line at the end for cleanliness
}

void LED_Clear(int ledIndex)
{
    switch (ledIndex)
    {
    case 1:
        LED_STATUS1_Set();
        break;
    case 2:
        LED_STATUS2_Set();
        break;
    case 3:
        LED_STATUS3_Set();
        break;
    case 4:
        LED_STATUS4_Set();
        break;
    }
}

void LED_Set(int ledIndex)
{
    switch (ledIndex)
    {
    case 1:
        LED_STATUS1_Clear();
        break;
    case 2:
        LED_STATUS2_Clear();
        break;
    case 3:
        LED_STATUS3_Clear();
        break;
    case 4:
        LED_STATUS4_Clear();
        break;
    }
}

void setLEDStatus(int ledIndex, int statusIndex)
{
    if (ADE9000_status[statusIndex] == 1 && ADE9000_status[statusIndex + 1] == 1 && ADE9000_status[statusIndex + 2] == 1)
    {
        LED_Set(ledIndex);
    }
    else
    {
        LED_Clear(ledIndex);
    }
}

void ADE9000Status_Check()
{
    for (int i = 0; i < 12; i += 3)
    {
        setLEDStatus(i / 3 + 1, i);
    }
}

void ADE9000SetupForUnit(SPI_CSS_Pin cssPin)
{
    if (IsADE9000(cssPin))
    {
        // printf("ADE9000 Setup Successful\n");
        ADE9000_status[cssPin - 1] = 1;
        ADE9000_hotplug[cssPin - 1] = 1;
        ADE9000_Initialize(cssPin);
        channelConfig[cssPin - 1].ctRatio = 100;
        ct_ratio_calibration(cssPin - 1);
    }
    else
    {
        // printf("ADE9000 Setup Failed\n");
        ADE9000_status[cssPin - 1] = 0;
        ADE9000_hotplug[cssPin - 1] = 0;
    }
}

void ADE9000ForUnitCheck(SPI_CSS_Pin cssPin)
{
    if (IsADE9000(cssPin))
    {
        // printf("ADE9000 Setup Successful\n");
        ADE9000_status[cssPin - 1] = 1;
    }
    else
    {
        // printf("ADE9000 Setup Failed\n");
        ADE9000_status[cssPin - 1] = 0;
    }
}

void ADE9000_Checking()
{
    for(int i = SPI_CSS1; i <= SPI_CSS12; i++)
    {
        ADE9000ForUnitCheck((SPI_CSS_Pin)i);
    }
    ADE9000Status_Check();
}

void ADE9000Setup()
{
    for(int i = SPI_CSS1; i <= SPI_CSS12; i++)
    {
        ADE9000SetupForUnit((SPI_CSS_Pin)i);
    }
    ADE9000_Checking();

    memset(ADE9000_arrayBuffer, 0, TOTAL_ARRAYSIZE);
    memset(unitMeasurements, 0, sizeof(unitMeasurements));
}

// *****************************************************************************
// *****************************************************************************
// Section: UART Functions
// *****************************************************************************
// *****************************************************************************

void SendDataAsASCII(const uint8_t *data, size_t size, uint8_t index)
{
    char hexBuffer[3]; // Temporary buffer for hex conversion

    for (size_t i = (size * index); i < (size * (index + 1)); i++)
    {
        sprintf(hexBuffer, "%02X", data[i]);          // Convert byte to two hex characters
        SERCOM2_USART_Write((uint8_t *)hexBuffer, 2); // Send the two hex characters
    }
}

void send_response_ct_ratio()
{
    char buffer_2[600]; // Buffer for sending response of command ct_ratio
    for (int i = 0; i < 600; i++)
    {
        buffer_2[i] = '\0';
    }
    int count = snprintf(buffer_2, sizeof(buffer_2),
                         "{"
                         "\"status\":true,"
                         "\"message\":\"Response\""
                         "}\n");

    if (count > 0 && count < sizeof(buffer_2))
    {
        SERCOM2_USART_Write((uint16_t *)buffer_2, count);
    }
    //}
    //    SERCOM2_USART_Write((uint8_t *)buffer, count);
}

void ConvertDataToHexAndSend(const uint8_t *data, size_t size)
{
    char *ptr = txBuffer;
    for (size_t i = 0; i < size; i++)
    {
        sprintf(ptr, "%02X", data[i]);
        ptr += 2;
    }
    *ptr = '\0';
    SERCOM2_USART_Write((uint8_t *)txBuffer, ptr - txBuffer);
}

void ConvertDataToHexAndSend(const uint8_t *data, size_t size, uint8_t index)
{
    char *ptr = txBuffer;
    for (size_t i = (size * (index - 1)); i < (size * index); i++)
    {
        sprintf(ptr, "%02X", data[i]);
        ptr += 2;
    }
    *ptr++ = '\n';
    *ptr = '\0';
    SERCOM2_USART_Write((uint8_t *)txBuffer, ptr - txBuffer);
}

// ??????ADE9000_status?????12????????
void ConvertStatusToHexAndSend(void)
{
    uint16_t statusBits = 0;
    for (int i = 0; i < TOTAL_ADE9000_UNITS; i++)
    {
        statusBits |= (ADE9000_status[i] << (TOTAL_ADE9000_UNITS - 1 - i));
    }
    sprintf(txBuffer, "A5%03X\n", statusBits);
    SERCOM2_USART_Write((uint8_t *)txBuffer, strlen(txBuffer));
}

void USART_ReadCallback(uintptr_t context)
{
    // Check for newline character
    if (rxBuffer[rxSize] == '\n')
    {
        rxComplete = true;
    }
    else
    {
        rxSize++;
        if (rxSize < UART_RX_BUFFER_SIZE)
        {
            SERCOM2_USART_Read((void *)&rxBuffer[rxSize], 1);
        }
    }
}

void resetEnergyValues(void)
{
    // change this to also reset energy values in ade9000 registers?
    // energy accumulation ot be maintained on v3s, so no need to reset here?
    for (int i = 0; i < TOTAL_ADE9000_UNITS; i++)
    {
        unitMeasurements[i].activeEnergy.ACC_NActiveEnergyValue_kWh_A = 0;
        unitMeasurements[i].activeEnergy.ACC_NActiveEnergyValue_kWh_B = 0;
        unitMeasurements[i].activeEnergy.ACC_NActiveEnergyValue_kWh_C = 0;
        unitMeasurements[i].activeEnergy.ACC_PActiveEnergyValue_kWh_A = 0;
        unitMeasurements[i].activeEnergy.ACC_PActiveEnergyValue_kWh_B = 0;
        unitMeasurements[i].activeEnergy.ACC_PActiveEnergyValue_kWh_C = 0;
    }
}
//{
//  "command": "set-ct-ratio",
//  "ct_ratio": {
//    "channel-1": 200,
//    "channel-2": 200,
//    "channel-3": 200,
//    "channel-4": 70,
//    "channel-5": 200,
//    "channel-6": 80,
//    "channel-7": 200,
//    "channel-8": 200,
//    "channel-9": 500,
//    "channel-10": 200,
//    "channel-11": 200,
//    "channel-12": 300
//  }
//}

float extract_channel_values(const char *channel)
{
    char *ch;
    if (strstr(rxBuffer, channel) != NULL)
    {
        ch = strstr(rxBuffer, channel);
        ch = strchr(ch, ':');
        if (ch)
        {
            ch++;
            while (*ch == ' ' || *ch == '\"')
                ch++;
            char value[16];
            int i = 0;
            while (*ch && *ch != '\"' && *ch != ',' && *ch != ' ')
            {
                value[i++] = *ch++;
            }
            value[i] = '\0';
            /* For Debugging*/
            //            strcat(txBuffer, "\n");
            //            strcat(txBuffer, channel);
            //            strcat(txBuffer, " Value received: ");
            //            strcat(txBuffer, value);
            return atof(value); // atoi(ASCII TO Int) function is used to convert value from character to integer
        }
    }
    return 0.0f;
}

float extract_phase_value(const char *key)
{
    char *ptr = strstr(rxBuffer, key);
    if (ptr)
    {
        ptr = strchr(ptr, ':');
        if (ptr)
        {
            float val = 0.0f;
            sscanf(ptr + 1, "%f", &val);
            return val;
        }
    }
    return 0.0f;
}

int extract_int_value(const char *key)
{
    char *ptr = strstr(rxBuffer, key);
    if (ptr)
    {
        ptr = strchr(ptr, ':');
        if (ptr)
        {
            int val = 0;
            // Skip colon and whitespace
            while (*(++ptr) == ' ' || *ptr == '\t');
            sscanf(ptr, "%d", &val);
            return val;
        }
    }
    return 0;
}

void parse_ct_ratio_json()
{
    char *ch;
    ch = strstr(rxBuffer, "ct-ratio");
    if (ch != NULL)
    {
        char key[20];
        for (uint8_t j = 0; j < TOTAL_ADE9000_UNITS; j++)
        {
            sprintf(key, "channel-%d", j+1);
            channelConfig[j].ctRatio = extract_channel_values(key);
            ct_ratio_calibration(j);
        }
        strcpy(txBuffer, "\nCT Ratio updated successfully!");
    }
}

void parse_v_offset_json()
{
    char *ch;
    ch = strstr(rxBuffer, "v_offset");
    if (ch != NULL)
    {
        char key[20];
        for (uint8_t j = 1; j < 13; j++)
        {
            sprintf(key, "channel-%d", j);
            channelConfig[j].setVOffset = extract_channel_values(key);

            // Write offset values to ADE9000 registers
            voltage_offset_write(j, channelConfig[j].setVOffset);
        }
        strcpy(txBuffer, "\nVoltage Offset updated successfully!");
    }
}

void parse_i_offset_json()
{
    char *ch;
    ch = strstr(rxBuffer, "i_offset");
    if (ch != NULL)
    {
        char key[20];
        for (uint8_t j = 1; j < 13; j++)
        {
            sprintf(key, "channel-%d", j);
            channelConfig[j - 1].setIOffset = extract_channel_values(key);
            //  Write offset Values to ADE9000 registers
            //            current_offset_write(j, channelConfig[j].setIOffset);
        }
        strcpy(txBuffer, "\nCurrent Offset updated successfully!");
    }
}

// for voltage gain register calibration
void parse_v_calibration_json()
{
    char *ch;
    ch = strstr(rxBuffer, "voltage");
    if (ch != NULL)
    {
        char key[10];
        memset(reg_values, 0, sizeof(reg_values));

        voltage_gain_calibration(extract_phase_value("phase_a"), PHASE_A);
        voltage_gain_calibration(extract_phase_value("phase_b"), PHASE_B);
        voltage_gain_calibration(extract_phase_value("phase_c"), PHASE_C);
        
        // Start building JSON
        // memset(txBuffer, 0, sizeof(txBuffer));
        strcpy(txBuffer, "{\n  \"command\": \"v-calibration\",\n  \"voltage\": {\n    \"channels\": [\n");

        for (int i = 0; i < TOTAL_ADE9000_UNITS; i++)
        {
            char temp[128];
            sprintf(temp,
                    "      {\n"
                    "        \"ID\": %d,\n"
                    "        \"phase_a\": \"0x%08X\",\n"
                    "        \"phase_b\": \"0x%08X\",\n"
                    "        \"phase_c\": \"0x%08X\"\n"
                    "      }%s\n",
                    i,
                    reg_values[i][PHASE_A], reg_values[i][PHASE_B], reg_values[i][PHASE_C],
                    (i < TOTAL_ADE9000_UNITS) ? "," : "");

            strcat(txBuffer, temp);
        }
        strcat(txBuffer, "    ]\n  }\n}");
    }
}

// for current gain register calibration
void parse_i_calibration_json()
{
    char *ch = strstr(rxBuffer, "\"current\"");
    if (ch != NULL)
    {
        currentCalibration.channelID = extract_int_value("\"ID\"");
        currentCalibration.phase_a   = extract_phase_value("phase_a");
        currentCalibration.phase_b   = extract_phase_value("phase_b");
        currentCalibration.phase_c   = extract_phase_value("phase_c");

        uint8_t channel_id_absolute = currentCalibration.channelID - 1;

        memset(txBuffer, 0, sizeof(txBuffer));

        // Start JSON
        sprintf(txBuffer,
                "{\n  \"command\": \"i-calibration\",\n"
                "  \"current\": {\n"
                "    \"ID\": %d,\n",
                currentCalibration.channelID);

        char temp[128];
        bool first = true; // to manage commas properly

        // Process and append only non-zero phases
        if (currentCalibration.phase_a != 0) {
            current_gain_calibration(currentCalibration.phase_a, channel_id_absolute, PHASE_A);
            sprintf(temp, "    \"phase_a\": \"0x%08X\"", reg_values[currentCalibration.channelID][PHASE_A]);
            strcat(txBuffer, temp);
            first = false;
        }
        if (currentCalibration.phase_b != 0) {
            current_gain_calibration(currentCalibration.phase_b, channel_id_absolute, PHASE_B);
            sprintf(temp, "%s\n    \"phase_b\": \"0x%08X\"", first ? "" : ",", reg_values[currentCalibration.channelID][PHASE_B]);
            strcat(txBuffer, temp);
            first = false;
        }
        if (currentCalibration.phase_c != 0) {
            current_gain_calibration(currentCalibration.phase_c, channel_id_absolute, PHASE_C);
            sprintf(temp, "%s\n    \"phase_c\": \"0x%08X\"", first ? "" : ",", reg_values[currentCalibration.channelID][PHASE_C]);
            strcat(txBuffer, temp);
        }

        // Close JSON
        strcat(txBuffer, "\n  }\n}");
    }
}

void ProcessCommand(void)
{
    rxBuffer[rxSize] = '\0';
    char *ch;

    if (rxBuffer[0] == '{' && rxBuffer[rxSize - 1] == '}')
    {
        // check for JSON of calibration
        ch = strstr(rxBuffer, "command");
        if (ch != NULL)
        {
            ch = strchr(ch, ':');
            if (ch)
            {
                ch++;
                while (*ch == ' ' || *ch == '\"')
                    ch++;
                char value[32];
                int i = 0;
                while (*ch && *ch != '\"')
                {
                    value[i++] = *ch++;
                }
                value[i] = '\0';
                if (strcmp(value, "set-ct-ratio") == 0)
                {
                    parse_ct_ratio_json();
                }
                else if (strcmp(value, "set-v-offset") == 0)
                {
                    parse_v_offset_json();
                }
                else if (strcmp(value, "set-i-offset") == 0)
                {
                    parse_i_offset_json();
                }
                else if (strcmp(value, "v-calibration") == 0)
                {
                    parse_v_calibration_json();
                }
                else if (strcmp(value, "i-calibration") == 0)
                {
                    parse_i_calibration_json();
                }
                else
                {
                    strcpy(txBuffer, "None Of The Three Calibration Parameter Values Found In JSON!");
                }
            }
        }
        else
        {
            strcpy(txBuffer, "JSON does not contain command key!");
        }
    }
    else if (strcmp(rxBuffer, "channel") == 0)
    {
        for (size_t i = 1; i <= 12; i++)
        {
            sendChannelJsonData(i);
        }
    }
    else if (strcmp(rxBuffer, "channel-1") == 0)
    {
        sendChannelJsonData(1);
    }
    else if (strcmp(rxBuffer, "channel-2") == 0)
    {
        sendChannelJsonData(2);
    }
    else if (strcmp(rxBuffer, "channel-3") == 0)
    {
        sendChannelJsonData(3);
    }
    else if (strcmp(rxBuffer, "channel-4") == 0)
    {
        sendChannelJsonData(4);
    }
    else if (strcmp(rxBuffer, "channel-5") == 0)
    {
        sendChannelJsonData(5);
    }
    else if (strcmp(rxBuffer, "channel-6") == 0)
    {
        sendChannelJsonData(6);
    }
    else if (strcmp(rxBuffer, "channel-7") == 0)
    {
        sendChannelJsonData(7);
    }
    else if (strcmp(rxBuffer, "channel-8") == 0)
    {
        sendChannelJsonData(8);
    }
    else if (strcmp(rxBuffer, "channel-9") == 0)
    {
        sendChannelJsonData(9);
    }
    else if (strcmp(rxBuffer, "channel-10") == 0)
    {
        sendChannelJsonData(10);
    }
    else if (strcmp(rxBuffer, "channel-11") == 0)
    {
        sendChannelJsonData(11);
    }
    else if (strcmp(rxBuffer, "channel-12") == 0)
    {
        sendChannelJsonData(12);
    }
    else if (strcmp(rxBuffer, "readstatus") == 0)
    {
        ConvertStatusToHexAndSend();
    }
    /* These calibration have been received in first if condition */

    //    else if (strcmp(rxBuffer, "ct-ratio") == 0)
    //    {
    ////        strcpy(txBuffer, "Calibration 1: Step 1 settings");
    //        strcpy(txBuffer, "CT Ratio updated successfully!");
    //
    ////        ct_ratio_calibration(100);
    ////        send_response_ct_ratio();
    //    }
    //    else if (strcmp(rxBuffer, "cal-2") == 0)
    //    {
    //        strcpy(txBuffer, "Calibration 2: Step 2 settings");
    //    }
    //    else if (strcmp(rxBuffer, "cal-3") == 0)
    //    {d
    //        strcpy(txBuffer, "Calibration 3: Step 3 settings");
    //    }
    else if (strcmp(rxBuffer, "cal-4") == 0)
    {
        strcpy(txBuffer, "Calibration 4: Step 4 settings");
    }
    else
    {
        strcpy(txBuffer, "again");
    }

    strcat(txBuffer, "\n");

    SERCOM2_USART_Write(txBuffer, strlen(txBuffer));

    memset(rxBuffer, 0, UART_RX_BUFFER_SIZE);
    rxSize = 0;
    rxComplete = false;

    SERCOM2_USART_Read((void *)rxBuffer, 1);
}

void SetupUSART(void)
{
    SERCOM2_USART_ReadCallbackRegister(USART_ReadCallback, 0);
    SERCOM2_USART_Read((void *)rxBuffer, 1);
}

void flash_check()
{
    for (uint8_t j = 0; j < 3; j++)
    {
        for(uint8_t i=1; i<=4; i++)
        {
            LED_Set(i);
            delay_ms(100);
        }
        for(uint8_t i=4; i>0; i--)
        {
            LED_Clear(i);
            delay_ms(100);
        }
    }
    
}

// *****************************************************************************
// *****************************************************************************
// Section: Main Entry Point
// *****************************************************************************
int main(void)
{
    SYS_Initialize(NULL);
    TC0_TimerStart();
    TC0_TimerCallbackRegister(timer_callback, NULL);
    flash_check();
    ADE9000Setup();
    SetupUSART();

    while (true)
    {
        ADE9000_Checking();

        if (ADE9000_status[SPI_CSS1 - 1] == 1)
        {
            if (ADE9000_hotplug[SPI_CSS1 - 1] == 0)
            {
                ADE9000SetupForUnit(SPI_CSS1);
                // ADE9000_hotplug[SPI_CSS1 - 1] = 1;
            }
            ADE9000MeasurementsForUnit(SPI_CSS1, 1);
        }
        else
        {
            ADE9000_hotplug[SPI_CSS1 - 1] = 0;
            ADE9000MeasurementsForUnit_NON(SPI_CSS1, 1);
        }
        if (ADE9000_status[SPI_CSS2 - 1] == 1)
        {
            if (ADE9000_hotplug[SPI_CSS2 - 1] == 0)
            {
                ADE9000SetupForUnit(SPI_CSS2);
                // ADE9000_hotplug[SPI_CSS2 - 1] = 1;
            }
            ADE9000MeasurementsForUnit(SPI_CSS2, 2);
        }
        else
        {
            ADE9000_hotplug[SPI_CSS2 - 1] = 0;
            ADE9000MeasurementsForUnit_NON(SPI_CSS2, 2);
        }
        if (ADE9000_status[SPI_CSS3 - 1] == 1)
        {
            if (ADE9000_hotplug[SPI_CSS3 - 1] == 0)
            {
                ADE9000SetupForUnit(SPI_CSS3);
                // ADE9000_hotplug[SPI_CSS3 - 1] = 1;
            }
            ADE9000MeasurementsForUnit(SPI_CSS3, 3);
        }
        else
        {
            ADE9000_hotplug[SPI_CSS3 - 1] = 0;
            ADE9000MeasurementsForUnit_NON(SPI_CSS3, 3);
        }
        if (ADE9000_status[SPI_CSS4 - 1] == 1)
        {
            if (ADE9000_hotplug[SPI_CSS4 - 1] == 0)
            {
                ADE9000SetupForUnit(SPI_CSS4);
                // ADE9000_hotplug[SPI_CSS4 - 1] = 1;
            }
            ADE9000MeasurementsForUnit(SPI_CSS4, 4);
        }
        else
        {
            ADE9000_hotplug[SPI_CSS4 - 1] = 0;
            ADE9000MeasurementsForUnit_NON(SPI_CSS4, 4);
        }
        if (ADE9000_status[SPI_CSS5 - 1] == 1)
        {
            if (ADE9000_hotplug[SPI_CSS5 - 1] == 0)
            {
                ADE9000SetupForUnit(SPI_CSS5);
                // ADE9000_hotplug[SPI_CSS5 - 1] = 1;
            }
            ADE9000MeasurementsForUnit(SPI_CSS5, 5);
        }
        else
        {
            ADE9000_hotplug[SPI_CSS5 - 1] = 0;
            ADE9000MeasurementsForUnit_NON(SPI_CSS5, 5);
        }
        if (ADE9000_status[SPI_CSS6 - 1] == 1)
        {
            if (ADE9000_hotplug[SPI_CSS6 - 1] == 0)
            {
                ADE9000SetupForUnit(SPI_CSS6);
                // ADE9000_hotplug[SPI_CSS6 - 1] = 1;
            }
            ADE9000MeasurementsForUnit(SPI_CSS6, 6);
        }
        else
        {
            ADE9000_hotplug[SPI_CSS6 - 1] = 0;
            ADE9000MeasurementsForUnit_NON(SPI_CSS6, 6);
        }
         if (ADE9000_status[SPI_CSS7 - 1] == 1)
         {
             if (ADE9000_hotplug[SPI_CSS7 - 1] == 0)
             {
                 ADE9000SetupForUnit(SPI_CSS7);
                //  ADE9000_hotplug[SPI_CSS7 - 1] = 1;
             }
             ADE9000MeasurementsForUnit(SPI_CSS7, 7);
         }
         else
         {
             ADE9000_hotplug[SPI_CSS7 - 1] = 0;
             ADE9000MeasurementsForUnit_NON(SPI_CSS7, 7);
         }
        if (ADE9000_status[SPI_CSS8 - 1] == 1)
        {
            if (ADE9000_hotplug[SPI_CSS8 - 1] == 0)
            {
                ADE9000SetupForUnit(SPI_CSS8);
                // ADE9000_hotplug[SPI_CSS8 - 1] = 1;
            }
            ADE9000MeasurementsForUnit(SPI_CSS8, 8);
        }
        else
        {
            ADE9000_hotplug[SPI_CSS8 - 1] = 0;
            ADE9000MeasurementsForUnit_NON(SPI_CSS8, 8);
        }
        if (ADE9000_status[SPI_CSS9 - 1] == 1)
        {
            if (ADE9000_hotplug[SPI_CSS9 - 1] == 0)
            {
                ADE9000SetupForUnit(SPI_CSS9);
                // ADE9000_hotplug[SPI_CSS9 - 1] = 1;
            }
            ADE9000MeasurementsForUnit(SPI_CSS9, 9);
        }
        else
        {
            ADE9000_hotplug[SPI_CSS9 - 1] = 0;
            ADE9000MeasurementsForUnit_NON(SPI_CSS9, 9);
        }
       if (ADE9000_status[SPI_CSS10 - 1] == 1)
       {
           if (ADE9000_hotplug[SPI_CSS10 - 1] == 0)
           {
               ADE9000SetupForUnit(SPI_CSS10);
            //    ADE9000_hotplug[SPI_CSS10 - 1] = 1;
           }
           ADE9000MeasurementsForUnit(SPI_CSS10, 10);
       }
       else
       {
           ADE9000_hotplug[SPI_CSS10 - 1] = 0;
           ADE9000MeasurementsForUnit_NON(SPI_CSS10, 10);
       }
        if (ADE9000_status[SPI_CSS11 - 1] == 1)
        {
            if (ADE9000_hotplug[SPI_CSS11 - 1] == 0)
            {
                ADE9000SetupForUnit(SPI_CSS11);
                // ADE9000_hotplug[SPI_CSS11 - 1] = 1;
            }
            ADE9000MeasurementsForUnit(SPI_CSS11, 11);
        }
        else
        {
            ADE9000_hotplug[SPI_CSS11 - 1] = 0;
            ADE9000MeasurementsForUnit_NON(SPI_CSS11, 11);
        }
        if (ADE9000_status[SPI_CSS12 - 1] == 1)
        {
            if (ADE9000_hotplug[SPI_CSS12 - 1] == 0)
            {
                ADE9000SetupForUnit(SPI_CSS12);
                // ADE9000_hotplug[SPI_CSS12 - 1] = 1;
            }
            ADE9000MeasurementsForUnit(SPI_CSS12, 12);
        }
        else
        {
            ADE9000_hotplug[SPI_CSS12 - 1] = 0;
            ADE9000MeasurementsForUnit_NON(SPI_CSS12, 12);
        }

        // printADE9000ArrayBuffer(ADE9000_arrayBuffer, TOTAL_ARRAYSIZE);

        if (rxComplete)
        {
            ProcessCommand();
        }
    }
    return EXIT_FAILURE;
}