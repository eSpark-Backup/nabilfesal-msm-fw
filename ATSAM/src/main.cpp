// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include "main.h"
#include "ADE9000.h" 
#include <string>
#include <math.h>
// #include <queue>

// *****************************************************************************
// *****************************************************************************
// Section: Variables
// *****************************************************************************
// *****************************************************************************

// enum MyEnum
// {
//     X(RMS_Voltage_A, 0),
//     X(RMS_Voltage_B, 1),
//     X(RMS_Voltage_C, 2),
//     X(Line_Voltage_A, 3),
//     X(Line_Voltage_B, 4),
//     X(Line_Voltage_C, 5),
//     X(RMS_Current_A, 6),
//     X(RMS_Current_B, 7),
//     X(RMS_Current_C, 8),
//     X(Freq_A, 9),
//     X(Freq_B, 10),
//     X(Freq_C, 11),
//     X(Active_Power_A, 12),
//     X(Active_Power_B, 13),
//     X(Active_Power_C, 14),
//     X(Reactive_Power_A, 15),
//     X(Reactive_Power_B, 16),
//     X(Reactive_Power_C, 17),
//     X(Apparent_Power_A, 18),
//     X(Apparent_Power_B, 19),
//     X(Apparent_Power_C, 20),
//     X(Power_Factor_A, 21),
//     X(Power_Factor_B, 22),
//     X(Power_Factor_C, 23),
//     X(Pos_Active_Energy_A, 24),
//     X(Pos_Active_Energy_B, 25),
//     X(Pos_Active_Energy_C, 26),
//     X(Neg_Active_Energy_A, 27),
//     X(Neg_Active_Energy_B, 28),
//     X(Neg_Active_Energy_C, 29),
//     X(Voltage_THD_A, 30),
//     X(Voltage_THD_B, 31),
//     X(Voltage_THD_C, 32),
//     X(Current_THD_A, 33),
//     X(Current_THD_B, 34),
//     X(Current_THD_C, 35),
//     X(Temperature, 36),
//     X(Angle_A, 37),
//     X(Angle_B, 38),
//     X(Angle_C, 39),
// };

typedef struct {
    const char *key;
    ADE9000_parameter_type type;
} ADE9000_param_map_t;

ADE9000UnitMeasurements unitMeasurements[TOTAL_ADE9000_UNITS];
ChannelConfig channelConfig[TOTAL_ADE9000_UNITS];
CurrentCalibration currentCalibration;

uint8_t ADE9000_hotplug[TOTAL_ADE9000_UNITS];
uint8_t ADE9000_status[TOTAL_ADE9000_UNITS];
uint8_t ADE9000_currentGain[TOTAL_ADE9000_UNITS];
uint8_t ADE9000_voltageGain[TOTAL_ADE9000_UNITS];
uint8_t ADE9000_arrayBuffer[TOTAL_ARRAYSIZE];

uint32_t reg_values[TOTAL_ADE9000_UNITS][TOTAL_PHASES];



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

uint32_t get_msCounter(void)
{
    return msCounter;
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
    float* lineVoltage = (float*)&m.lineVoltage;
    float* rmsVoltage = (float*)&m.voltageRMS;
    float* frequency = (float*)&m.periodFrequency;
    float* current = (float*)&m.currentRMS;
    float* powerFactor = (float*)&m.powerFactor;
    float* activePower = (float*)&m.activePower;
    float* reactivePower = (float*)&m.reactivePower;
    float* apparentPower = (float*)&m.apparentPower;
    float* posEnergy = (float*)&m.activeEnergy;
    float* negEnergy = (float*)&m.activeEnergy;
    float* reactiveposEnergy = (float*)&m.activeEnergy;
    float* reactivenegEnergy = (float*)&m.activeEnergy;


    float* currentTHD = (float*)&m.currentTHD;
    float* voltageTHD = (float*)&m.voltageTHD;
    float* angle = (float*)&m.angleDegree;
    for (int i = PHASE_A; i < TOTAL_PHASES; i++)
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
                           "\"positive\":%.5f,"
                           "\"negative\":%.5f"
                           "},"
                           "\"reactive\":{"
                           "\"positive\":%.5f,"
                           "\"negative\":%.5f"
                           "}"
                           "},"
                           "\"thd\":{"
                           "\"current\":%.2f,"
                           "\"voltage\":%.2f,"
                           "\"angle\":%.2f"
                           "}"
                           "}%s",
                           ((i == PHASE_A) ? PHASE_A_STRING : ((i == PHASE_B) ? PHASE_B_STRING : PHASE_C_STRING)) ,
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

    // reset active energy values
    m.activeEnergy.PActiveEnergyValue_kWh_A = 0;
    m.activeEnergy.PActiveEnergyValue_kWh_B = 0;
    m.activeEnergy.PActiveEnergyValue_kWh_C = 0;
    m.activeEnergy.NActiveEnergyValue_kWh_A = 0;
    m.activeEnergy.NActiveEnergyValue_kWh_B = 0;
    m.activeEnergy.NActiveEnergyValue_kWh_C = 0;
    // reset reactive energy values
    m.activeEnergy.PReactiveEnergyValue_kWh_A = 0;
    m.activeEnergy.PReactiveEnergyValue_kWh_B = 0;
    m.activeEnergy.PReactiveEnergyValue_kWh_C = 0;
    m.activeEnergy.NReactiveEnergyValue_kWh_A = 0;
    m.activeEnergy.NReactiveEnergyValue_kWh_B = 0;
    m.activeEnergy.NReactiveEnergyValue_kWh_C = 0;
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

// void ConvertDataToHexAndSend(const uint8_t *data, size_t size)
// {
//     char buffer[(size * 2) + 1];
//     char *ptr = buffer;
//     for (size_t i = 0; i < size; i++)
//     {
//         sprintf(ptr, "%02X", data[i]);
//         ptr += 2;
//     }
//     *ptr = '\0';
//     SERCOM2_USART_Write((uint8_t *)buffer, ptr - buffer);
// }

// void ConvertDataToHexAndSend(const uint8_t *data, size_t size, uint8_t index)
// {
//     char buffer[(size * 2) + 1];
//     char *ptr = buffer;
//     for (size_t i = (size * (index - 1)); i < (size * index); i++)
//     {
//         sprintf(ptr, "%02X", data[i]);
//         ptr += 2;
//     }
//     *ptr++ = '\n';
//     *ptr = '\0';
//     SERCOM2_USART_Write((uint8_t *)buffer, ptr - buffer);
// }

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

int extract_int_value_from_pos(const char *key, const char *pos)
{
    if (!key || !pos)
        return -1;  // invalid input

    // Find the key in the string
    const char *key_pos = strstr(pos, key);
    if (!key_pos)
        return -1;  // key not found

    // Move pointer to the character after the key
    key_pos += strlen(key);

    // Skip whitespace, colon, or quotes if present
    while (*key_pos && (*key_pos == ' ' || *key_pos == ':' || *key_pos == '\"'))
        key_pos++;

    // Read integer value
    int value = 0;
    if (sscanf(key_pos, "%d", &value) != 1)
    {
        return -1;  // failed to parse integer
    }

    return value;
}

uint32_t extract_hex_value_from_pos(const char *key, char *start)
{
    char *ptr = strstr(start, key);
    if (!ptr) return 0;

    ptr = strchr(ptr, ':');
    if (!ptr) return 0;
    ptr++; // skip ':'

    // Skip whitespace and quotes
    while (*ptr == ' ' || *ptr == '"' || *ptr == '\t')
        ptr++;

    char value_str[32];
    int i = 0;
    while (*ptr && *ptr != '"' && *ptr != ',' && *ptr != ' ' && *ptr != '}')
    {
        value_str[i++] = *ptr++;
        if (i >= (int)(sizeof(value_str) - 1)) break;
    }
    value_str[i] = '\0';

    // Convert hex or decimal string to uint32_t
    return (uint32_t)strtoul(value_str, NULL, 0);
}

char *find_next_object(char *start)
{
    // Find the next '{' after the current one
    char *ptr = strchr(start, '{');
    return ptr;
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
        strcpy(txBuffer, "{\"command\": \"ct-ratio\", \"message\": \"CT Ratio updated successfully!\"}");
    }
}

void parse_reboot_calibration_json()
{
    // Detect type (voltage or current)
    char* ch = rxBuffer;
    ADE9000_parameter_type data_type = INVALID_PARAMETER;

    static const ADE9000_param_map_t param_map[] = {
        {"voltage", VOLTAGE_PARAMETER},
        {"current", CURRENT_PARAMETER},
        {"power",   POWER_PARAMETER},
        {"phase",   PERIOD_PARAMETER}, // or PHASE_PARAMETER if defined
    };

    for (int i = 0; i < sizeof(param_map) / sizeof(param_map[0]); i++)
    {
        char searchPattern[32];
        snprintf(searchPattern, sizeof(searchPattern), "\"%s\"", param_map[i].key);
        if (strstr(rxBuffer, searchPattern))
        {
            data_type = param_map[i].type;
        }
    }

    // Move to first object after the array name
    char *pos = strchr(ch, '[');
    if (!pos)
    {
        // printf("Error: invalid JSON array.\n");
        return;
    }

    // Parse up to 12 objects
    int channel_index = 0;
    while (channel_index < TOTAL_ADE9000_UNITS)
    {
        pos = find_next_object(pos);
        if (!pos)
            break;

        // Extract the ID for this object
        int id = extract_int_value_from_pos("ID", pos);
        if (id < 1 || id > TOTAL_ADE9000_UNITS)
        {
            // Invalid ID, skip this object
            pos++;
            continue;
        }

        // Extract each phase from this object
        reg_values[id-1][PHASE_A] = extract_hex_value_from_pos(PHASE_A_STRING, pos);
        reg_values[id-1][PHASE_B] = extract_hex_value_from_pos(PHASE_B_STRING, pos);
        reg_values[id-1][PHASE_C] = extract_hex_value_from_pos(PHASE_C_STRING, pos);

        channel_index++;

        pos++;      // Move ahead to avoid reprocessing the same object
    }

    // handle ADE9000 register writes based on parsed data
    for(int i = 0; i < TOTAL_ADE9000_UNITS; i++)
    {
        if(ADE9000_status[i])
        {
            ADE9000_Set_Calibration(data_type, i);
        }
    }

    sprintf(txBuffer,
        "{\"command\": \"reboot-calibration\", \"message\": \"%s calibration updated successfully!\"}",
         param_map[data_type].key);      // add other responses as required
}

void parse_v_offset_json()
{
    char *ch;
    ch = strstr(rxBuffer, "v_offset");
    if (ch != NULL)
    {
        char key[20];
        for (uint8_t j = 0; j < SPI_CSS12; j++)
        {
            sprintf(key, "channel-%d", j + 1);
            channelConfig[j].setVOffset = extract_channel_values(key);

            // Write offset values to ADE9000 registers
            voltage_offset_write(j, channelConfig[j].setVOffset);
        }
        strcpy(txBuffer, "{\"command\": \"v_offset\", \"message\": \"Voltage Offset updated successfully!\"}");
    }
}

void parse_i_offset_json()
{
    char *ch;
    ch = strstr(rxBuffer, "i_offset");
    if (ch != NULL)
    {
        char key[20];
        for (uint8_t j = 0; j < SPI_CSS12; j++)
        {
            sprintf(key, "channel-%d", j + 1);
            channelConfig[j].setIOffset = extract_channel_values(key) / 1000.0;     // value received in milliamps
            //  Write offset Values to ADE9000 registers
            current_offset_write(j, channelConfig[j].setIOffset);
        }
        strcpy(txBuffer, "{\"command\": \"i_offset\", \"message\": \"Current Offset updated successfully!\"}");
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

        voltage_gain_calibration_start(extract_phase_value(PHASE_A_STRING), PHASE_A);
        voltage_gain_calibration_start(extract_phase_value(PHASE_B_STRING), PHASE_B);
        voltage_gain_calibration_start(extract_phase_value(PHASE_C_STRING), PHASE_C);

        delay_ms(5000); // wait for calibration to settle

        voltage_gain_calibration_end(extract_phase_value(PHASE_A_STRING), PHASE_A);
        voltage_gain_calibration_end(extract_phase_value(PHASE_B_STRING), PHASE_B);
        voltage_gain_calibration_end(extract_phase_value(PHASE_C_STRING), PHASE_C);
        
        // Start building JSON
        // memset(txBuffer, 0, sizeof(txBuffer));
        strcpy(txBuffer, "{\"command\":\"v-calibration\",\"voltage\":{\"channels\":[");

        for (int i = 0; i < TOTAL_ADE9000_UNITS; i++)
        {
            char temp[128];
            sprintf(temp,
                    "{"
                    "\"ID\": %d,"
                    "\"phase_a\": \"0x%08X\","
                    "\"phase_b\": \"0x%08X\","
                    "\"phase_c\": \"0x%08X\""
                    "}%s",
                    i+1,
                    reg_values[i][PHASE_A], reg_values[i][PHASE_B], reg_values[i][PHASE_C],
                    (i < (TOTAL_ADE9000_UNITS-1)) ? "," : "");

            strcat(txBuffer, temp);
        }
        strcat(txBuffer, "]}}");
    }
}

// for current gain register calibration
void parse_i_calibration_json()
{
    char *ch = strstr(rxBuffer, "\"current\"");
    if (ch != NULL)
    {
        currentCalibration.channelID = extract_int_value("\"ID\"");
        currentCalibration.phase_a   = extract_phase_value(PHASE_A_STRING);
        currentCalibration.phase_b   = extract_phase_value(PHASE_B_STRING);
        currentCalibration.phase_c   = extract_phase_value(PHASE_C_STRING);

        uint8_t channel_id_absolute = currentCalibration.channelID - 1;

        memset(txBuffer, 0, sizeof(txBuffer));

        // Start JSON
        sprintf(txBuffer,
                "{\"command\": \"i-calibration\","
                "\"current\": {"
                "\"ID\": %d,",
                currentCalibration.channelID);

        // if(!ADE9000_status[channel_id_absolute])
        // {
        //     strcat(txBuffer, "\"message\": \"ADE9000 not detected on this channel!\"}}");
        //     return;
        // }

        char temp[128];
        bool first = true; // to manage commas properly

        // Process and append only non-zero phases
        if (currentCalibration.phase_a != 0) {
            current_gain_calibration_start(currentCalibration.phase_a, channel_id_absolute, PHASE_A);
        }
        if (currentCalibration.phase_b != 0) {
            current_gain_calibration_start(currentCalibration.phase_b, channel_id_absolute, PHASE_B);
        }
        if (currentCalibration.phase_c != 0) {
            current_gain_calibration_start(currentCalibration.phase_c, channel_id_absolute, PHASE_C);
        }

        delay_ms(5000); // wait for calibration to settle

        if (currentCalibration.phase_a != 0) {
            current_gain_calibration_end(currentCalibration.phase_a, channel_id_absolute, PHASE_A);
            sprintf(temp, "\"phase_a\":\"0x%08X\"", reg_values[channel_id_absolute][PHASE_A]);
            strcat(txBuffer, temp);
            first = false;
        }
        if (currentCalibration.phase_b != 0) {
            current_gain_calibration_end(currentCalibration.phase_b, channel_id_absolute, PHASE_B);
            sprintf(temp, "%s\"phase_b\":\"0x%08X\"", first ? "" : ",", reg_values[channel_id_absolute][PHASE_B]);
            strcat(txBuffer, temp);
            first = false;
        }
        if (currentCalibration.phase_c != 0) {
            current_gain_calibration_end(currentCalibration.phase_c, channel_id_absolute, PHASE_C);
            sprintf(temp, "%s\"phase_c\":\"0x%08X\"", first ? "" : ",", reg_values[channel_id_absolute][PHASE_C]);
            strcat(txBuffer, temp);
        }

        // Close JSON
        strcat(txBuffer, "}}");
    }
}

void ADE9000UnitDataRead(uint8_t unit)
{
    if (ADE9000_status[unit - 1] == 1)
    {
        if (ADE9000_hotplug[unit - 1] == 0)
        {
            ADE9000SetupForUnit((SPI_CSS_Pin)unit);
        }
        ADE9000MeasurementsForUnit((SPI_CSS_Pin)unit, unit);
    }
    else
    {
        ADE9000_hotplug[unit - 1] = 0;
        ADE9000MeasurementsForUnit_NON((SPI_CSS_Pin)unit, unit);
    }
}


// startup indication using onboard LEDs
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
    TC0_TimerCallbackRegister(timer_callback, NULL);
    TC0_TimerStart();
    flash_check();
    ADE9000Setup();
    SetupUSART();

    uint8_t i = SPI_CSS1;

    while (true)
    {
        ADE9000_Checking();             // check ADE9000 availability

        // for(int i = SPI_CSS1; i <= SPI_CSS12; i++)      // loop through all 12 units
        // {
            ADE9000UnitDataRead(i);
        // }
        i = (i % SPI_CSS12) + 1;

        // printADE9000ArrayBuffer(ADE9000_arrayBuffer, TOTAL_ARRAYSIZE);

        if (rxComplete)
        {
            ProcessCommand();
        }
    }

    return EXIT_FAILURE;
}