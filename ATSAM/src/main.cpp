// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include "main.h"

// *****************************************************************************
// *****************************************************************************
// Section: Variables
// *****************************************************************************
// *****************************************************************************

#define ENUM_BASE 1
#define ENUM_STEP 4

#define X(name, index) name = (ENUM_BASE + (index * ENUM_STEP))
 
enum MyEnum {
    X(RMS_Voltage_A,             0),
    X(RMS_Voltage_B,             1),
    X(RMS_Voltage_C,             2),
    X(Line_Voltage_A,            3),
    X(Line_Voltage_B,            4),
    X(Line_Voltage_C,            5),
    X(RMS_Current_A,             6),
    X(RMS_Current_B,             7),
    X(RMS_Current_C,             8),
    X(Freq_A,                    9),
    X(Freq_B,                   10),
    X(Freq_C,                   11), 
    X(Active_Power_A,           12),
    X(Active_Power_B,           13),
    X(Active_Power_C,           14),
    X(Reactive_Power_A,         15),
    X(Reactive_Power_B,         16),
    X(Reactive_Power_C,         17),
    X(Apparent_Power_A,         18),
    X(Apparent_Power_B,         19),
    X(Apparent_Power_C,         20),
    X(Power_Factor_A,           21),
    X(Power_Factor_B,           22),
    X(Power_Factor_C,           23), 
    X(Pos_Active_Energy_A,      24),
    X(Pos_Active_Energy_B,      25),
    X(Pos_Active_Energy_C,      26),
    X(Neg_Active_Energy_A,      27),
    X(Neg_Active_Energy_B,      28),
    X(Neg_Active_Energy_C,      29),
    X(Voltage_THD_A,            30),
    X(Voltage_THD_B,            31),
    X(Voltage_THD_C,            32),
    X(Current_THD_A,            33),
    X(Current_THD_B,            34),
    X(Current_THD_C,            35),
    X(Temperature,              36),
    X(Angle_A,                  37),
    X(Angle_B,                  38),
    X(Angle_C,                  39),
};

ADE9000UnitMeasurements unitMeasurements[TOTAL_ADE9000_UNITS];
uint8_t ADE9000_hotplug[TOTAL_ADE9000_UNITS];
uint8_t ADE9000_status[TOTAL_ADE9000_UNITS];
uint8_t ADE9000_currentGain[TOTAL_ADE9000_UNITS];
uint8_t ADE9000_voltageGain[TOTAL_ADE9000_UNITS];
uint8_t ADE9000_arrayBuffer[TOTAL_ARRAYSIZE];

char rxBuffer[UART_RX_BUFFER_SIZE];
char txBuffer[UART_TX_BUFFER_SIZE];
size_t rxSize = 0;
volatile bool rxComplete = false;

// *****************************************************************************
// *****************************************************************************
// Section: Functions
// *****************************************************************************
// *****************************************************************************

void delay_ms(uint32_t milliseconds)
{
    TC0_TimerStart();
    for (uint32_t ms = 0; ms < milliseconds; ms++)
    {
        TC0_Timer16bitCounterSet(0);
        while (TC0_Timer16bitCounterGet() < 2000)
        {
        }
    }
    TC0_TimerStop();
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

void ADE9000MeasurementsForUnit(SPI_CSS_Pin cssPin, uint8_t unitIndex)
{
    uint16_t baseIndex = (unitIndex - 1) * ARRAYSIZE_PER_UNIT;
    ADE9000_arrayBuffer[baseIndex] = 0xAA + (unitIndex - 1);

    // Settle
    ReadVoltageRMSRegs(&unitMeasurements[unitIndex - 1].voltageRMS, cssPin);
    storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].voltageRMS.VoltageRMSValue_A, ADE9000_arrayBuffer, baseIndex + RMS_Voltage_A);
    storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].voltageRMS.VoltageRMSValue_B, ADE9000_arrayBuffer, baseIndex + RMS_Voltage_B);
    storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].voltageRMS.VoltageRMSValue_C, ADE9000_arrayBuffer, baseIndex + RMS_Voltage_C);

    //Settle
    ReadLineVoltages(&unitMeasurements[unitIndex - 1].voltageRMS, &unitMeasurements[cssPin - 1].lineVoltage);
    storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].lineVoltage.LineVoltage_AB, ADE9000_arrayBuffer, baseIndex + Line_Voltage_A);
    storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].lineVoltage.LineVoltage_BC, ADE9000_arrayBuffer, baseIndex + Line_Voltage_B);
    storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].lineVoltage.LineVoltage_CA, ADE9000_arrayBuffer, baseIndex + Line_Voltage_C);
    
    //Settle
    // ReadCurrentRMSRegs(&unitMeasurements[unitIndex - 1].currentRMS, cssPin);
    // storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].currentRMS.CurrentRMSValue_A, ADE9000_arrayBuffer, baseIndex + RMS_Current_A);
    // storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].currentRMS.CurrentRMSValue_B, ADE9000_arrayBuffer, baseIndex + RMS_Current_B);
    // storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].currentRMS.CurrentRMSValue_C, ADE9000_arrayBuffer, baseIndex + RMS_Current_C);

    //Settle
    ReadPeriodRegsnValues(&unitMeasurements[unitIndex - 1].periodFrequency, cssPin);
    storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].periodFrequency.FrequencyValue_A, ADE9000_arrayBuffer, baseIndex + Freq_A);
    storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].periodFrequency.FrequencyValue_B, ADE9000_arrayBuffer, baseIndex + Freq_B);
    storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].periodFrequency.FrequencyValue_C, ADE9000_arrayBuffer, baseIndex + Freq_C);

    //Settle
    // ReadActivePowerRegs(&unitMeasurements[unitIndex - 1].activePower, cssPin);
    // storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].activePower.ActivePowerValue_A, ADE9000_arrayBuffer, baseIndex + Active_Power_A);
    // storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].activePower.ActivePowerValue_B, ADE9000_arrayBuffer, baseIndex + Active_Power_B);
    // storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].activePower.ActivePowerValue_C, ADE9000_arrayBuffer, baseIndex + Active_Power_C);

    //Settle (Not yet Test)
    // ReadReactivePowerRegs(&unitMeasurements[unitIndex - 1].reactivePower,&unitMeasurements[unitIndex - 1].apparentPower,&unitMeasurements[unitIndex - 1].powerFactor, cssPin);
    // storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].reactivePower.ReactivePowerValue_A, ADE9000_arrayBuffer, baseIndex + Reactive_Power_A);
    // storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].reactivePower.ReactivePowerValue_B, ADE9000_arrayBuffer, baseIndex + Reactive_Power_B);
    // storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].reactivePower.ReactivePowerValue_C, ADE9000_arrayBuffer, baseIndex + Reactive_Power_C);
    
    //Settle
    // ReadApparentPowerRegs(&unitMeasurements[unitIndex - 1].apparentPower, cssPin);
    // storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].apparentPower.ApparentPowerValue_A, ADE9000_arrayBuffer, baseIndex + Apparent_Power_A);
    // storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].apparentPower.ApparentPowerValue_B, ADE9000_arrayBuffer, baseIndex + Apparent_Power_B);
    // storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].apparentPower.ApparentPowerValue_C, ADE9000_arrayBuffer, baseIndex + Apparent_Power_C);
    
    //Settle
    // ReadPowerFactorRegsnValues(&unitMeasurements[unitIndex - 1].powerFactor, cssPin);
    // storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].powerFactor.PowerFactorValue_A, ADE9000_arrayBuffer, baseIndex + Power_Factor_A);
    // storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].powerFactor.PowerFactorValue_B, ADE9000_arrayBuffer, baseIndex + Power_Factor_B);
    // storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].powerFactor.PowerFactorValue_C, ADE9000_arrayBuffer, baseIndex + Power_Factor_C);

    //Settle (Not yet Test)
    // ReadEnergyRegs(&unitMeasurements[unitIndex - 1].activeEnergy,&unitMeasurements[unitIndex - 1].activePower, cssPin);
    // unitMeasurements[unitIndex - 1].activeEnergy.ACC_PActiveEnergyValue_kWh_A += unitMeasurements[unitIndex - 1].activeEnergy.PActiveEnergyValue_kWh_A;
    // unitMeasurements[unitIndex - 1].activeEnergy.ACC_PActiveEnergyValue_kWh_B += unitMeasurements[unitIndex - 1].activeEnergy.PActiveEnergyValue_kWh_B;
    // unitMeasurements[unitIndex - 1].activeEnergy.ACC_PActiveEnergyValue_kWh_C += unitMeasurements[unitIndex - 1].activeEnergy.PActiveEnergyValue_kWh_C;
    // storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].activeEnergy.ACC_PActiveEnergyValue_kWh_A, ADE9000_arrayBuffer, baseIndex + Pos_Active_Energy_A);
    // storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].activeEnergy.ACC_PActiveEnergyValue_kWh_B, ADE9000_arrayBuffer, baseIndex + Pos_Active_Energy_B);
    // storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].activeEnergy.ACC_PActiveEnergyValue_kWh_C, ADE9000_arrayBuffer, baseIndex + Pos_Active_Energy_C);

    //Settle (Not yet Test)
    // unitMeasurements[unitIndex - 1].activeEnergy.ACC_NActiveEnergyValue_kWh_A += unitMeasurements[unitIndex - 1].activeEnergy.NActiveEnergyValue_kWh_A;
    // unitMeasurements[unitIndex - 1].activeEnergy.ACC_NActiveEnergyValue_kWh_B += unitMeasurements[unitIndex - 1].activeEnergy.NActiveEnergyValue_kWh_B;
    // unitMeasurements[unitIndex - 1].activeEnergy.ACC_NActiveEnergyValue_kWh_C += unitMeasurements[unitIndex - 1].activeEnergy.NActiveEnergyValue_kWh_C;
    // storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].activeEnergy.ACC_NActiveEnergyValue_kWh_A, ADE9000_arrayBuffer, baseIndex + Neg_Active_Energy_A);
    // storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].activeEnergy.ACC_NActiveEnergyValue_kWh_B, ADE9000_arrayBuffer, baseIndex + Neg_Active_Energy_B);
    // storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].activeEnergy.ACC_NActiveEnergyValue_kWh_C, ADE9000_arrayBuffer, baseIndex + Neg_Active_Energy_C);

    /*
    // ReadEnergyRegs(&unitMeasurements[unitIndex - 1].activeEnergy,&unitMeasurements[unitIndex - 1].activePower, cssPin);
    // storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].activeEnergy.PActiveEnergyValue_kWh_A, ADE9000_arrayBuffer, baseIndex + Pos_Active_Energy_A);
    // storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].activeEnergy.PActiveEnergyValue_kWh_B, ADE9000_arrayBuffer, baseIndex + Pos_Active_Energy_B);
    // storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].activeEnergy.PActiveEnergyValue_kWh_C, ADE9000_arrayBuffer, baseIndex + Pos_Active_Energy_C);

    //Settle (Not yet Test)
    storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].activeEnergy.NActiveEnergyValue_kWh_A, ADE9000_arrayBuffer, baseIndex + Neg_Active_Energy_A);
    storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].activeEnergy.NActiveEnergyValue_kWh_B, ADE9000_arrayBuffer, baseIndex + Neg_Active_Energy_B);
    storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].activeEnergy.NActiveEnergyValue_kWh_C, ADE9000_arrayBuffer, baseIndex + Neg_Active_Energy_C);
    */

   // Settle
    ReadTempRegnValue(&unitMeasurements[unitIndex - 1].temperature, cssPin);
    storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].temperature.Temperature, ADE9000_arrayBuffer, baseIndex + Temperature);
   
    // //Settle
    ReadVoltageTHDRegsnValues(&unitMeasurements[unitIndex - 1].voltageTHD, cssPin);
    storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].voltageTHD.VoltageTHDValue_A, ADE9000_arrayBuffer, baseIndex + Voltage_THD_A);
    storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].voltageTHD.VoltageTHDValue_B, ADE9000_arrayBuffer, baseIndex + Voltage_THD_B);
    storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].voltageTHD.VoltageTHDValue_C, ADE9000_arrayBuffer, baseIndex + Voltage_THD_C);

    // //Settle (Verifying Value)
    ReadCurrentTHDRegsnValues(&unitMeasurements[unitIndex - 1].currentTHD, cssPin);
    storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].currentTHD.CurrentTHDValue_A, ADE9000_arrayBuffer, baseIndex + Current_THD_A);
    storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].currentTHD.CurrentTHDValue_B, ADE9000_arrayBuffer, baseIndex + Current_THD_B);
    storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].currentTHD.CurrentTHDValue_C, ADE9000_arrayBuffer, baseIndex + Current_THD_C);

    // Settle (Unused Data)
    //  ReadAngleRegsnValues(&unitMeasurements[unitIndex - 1].angleDegree, cssPin);
    //  storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].angleDegree.AngleValue_VA_IA, ADE9000_arrayBuffer, baseIndex + Angle_A);
    //  storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].angleDegree.AngleValue_VB_IB, ADE9000_arrayBuffer, baseIndex + Angle_B);
    //  storeIEEE754InBuffer(unitMeasurements[unitIndex - 1].angleDegree.AngleValue_VC_IC, ADE9000_arrayBuffer, baseIndex + Angle_C);

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
    }
    else
    {
        // printf("ADE9000 Setup Failed\n");
        ADE9000_status[cssPin - 1] = 0;
        ADE9000_hotplug[cssPin - 1] = 0;
    }
}

void ADE9000Setup()
{
    ADE9000SetupForUnit(SPI_CSS1);
    ADE9000SetupForUnit(SPI_CSS2);
    ADE9000SetupForUnit(SPI_CSS3);
    ADE9000SetupForUnit(SPI_CSS4);
    ADE9000SetupForUnit(SPI_CSS5);
    ADE9000SetupForUnit(SPI_CSS6);
    ADE9000SetupForUnit(SPI_CSS7);
    ADE9000SetupForUnit(SPI_CSS8);
    ADE9000SetupForUnit(SPI_CSS9);
    ADE9000SetupForUnit(SPI_CSS10);
    ADE9000SetupForUnit(SPI_CSS11);
    ADE9000SetupForUnit(SPI_CSS12);
    ADE9000Status_Check();
    memset(ADE9000_arrayBuffer, 0, TOTAL_ARRAYSIZE);
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
    ADE9000ForUnitCheck(SPI_CSS1);
    ADE9000ForUnitCheck(SPI_CSS2);
    ADE9000ForUnitCheck(SPI_CSS3);
    ADE9000ForUnitCheck(SPI_CSS4);
    ADE9000ForUnitCheck(SPI_CSS5);
    ADE9000ForUnitCheck(SPI_CSS6);
    ADE9000ForUnitCheck(SPI_CSS7);
    ADE9000ForUnitCheck(SPI_CSS8);
    ADE9000ForUnitCheck(SPI_CSS9);
    ADE9000ForUnitCheck(SPI_CSS10);
    ADE9000ForUnitCheck(SPI_CSS11);
    ADE9000ForUnitCheck(SPI_CSS12);
    ADE9000Status_Check();
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

void ProcessCommand(void)
{
    rxBuffer[rxSize] = '\0';

    if (strcmp(rxBuffer, "channel") == 0)
    {
        ConvertDataToHexAndSend(ADE9000_arrayBuffer, TOTAL_ARRAYSIZE);
        resetEnergyValues();
    }
    else if (strcmp(rxBuffer, "channel-1") == 0)
    {
        ConvertDataToHexAndSend(ADE9000_arrayBuffer, ARRAYSIZE_PER_UNIT, 1);
    }
    else if (strcmp(rxBuffer, "channel-2") == 0)
    {
        ConvertDataToHexAndSend(ADE9000_arrayBuffer, ARRAYSIZE_PER_UNIT, 2);
    }
    else if (strcmp(rxBuffer, "channel-3") == 0)
    {
        ConvertDataToHexAndSend(ADE9000_arrayBuffer, ARRAYSIZE_PER_UNIT, 3);
    }
    else if (strcmp(rxBuffer, "channel-4") == 0)
    {
        ConvertDataToHexAndSend(ADE9000_arrayBuffer, ARRAYSIZE_PER_UNIT, 4);
    }
    else if (strcmp(rxBuffer, "channel-5") == 0)
    {
        ConvertDataToHexAndSend(ADE9000_arrayBuffer, ARRAYSIZE_PER_UNIT, 5);
    }
    else if (strcmp(rxBuffer, "channel-6") == 0)
    {
        ConvertDataToHexAndSend(ADE9000_arrayBuffer, ARRAYSIZE_PER_UNIT, 6);
    }
    else if (strcmp(rxBuffer, "channel-7") == 0)
    {
        ConvertDataToHexAndSend(ADE9000_arrayBuffer, ARRAYSIZE_PER_UNIT, 7);
    }
    else if (strcmp(rxBuffer, "channel-8") == 0)
    {
        ConvertDataToHexAndSend(ADE9000_arrayBuffer, ARRAYSIZE_PER_UNIT, 8);
    }
    else if (strcmp(rxBuffer, "channel-9") == 0)
    {
        ConvertDataToHexAndSend(ADE9000_arrayBuffer, ARRAYSIZE_PER_UNIT, 9);
    }
    else if (strcmp(rxBuffer, "channel-10") == 0)
    {
        ConvertDataToHexAndSend(ADE9000_arrayBuffer, ARRAYSIZE_PER_UNIT, 10);
    }
    else if (strcmp(rxBuffer, "channel-11") == 0)
    {
        ConvertDataToHexAndSend(ADE9000_arrayBuffer, ARRAYSIZE_PER_UNIT, 11);
    }
    else if (strcmp(rxBuffer, "channel-12") == 0)
    {
        ConvertDataToHexAndSend(ADE9000_arrayBuffer, ARRAYSIZE_PER_UNIT, 12);
    }
    else if (strcmp(rxBuffer, "readstatus") == 0)
    {
        ConvertStatusToHexAndSend();
    }
    else if (strcmp(rxBuffer, "cal-1") == 0)
    {
        strcpy(txBuffer, "Calibration 1: Step 1 settings");
    }
    else if (strcmp(rxBuffer, "cal-2") == 0)
    {
        strcpy(txBuffer, "Calibration 2: Step 2 settings");
    }
    else if (strcmp(rxBuffer, "cal-3") == 0)
    {
        strcpy(txBuffer, "Calibration 3: Step 3 settings");
    }
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

// *****************************************************************************
// *****************************************************************************
// Section: Main Entry Point
// *****************************************************************************
// *****************************************************************************

int main(void)
{
    SYS_Initialize(NULL);
    TC0_TimerStart();
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
                ADE9000_hotplug[SPI_CSS1 - 1] = 1;
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
                ADE9000_hotplug[SPI_CSS2 - 1] = 1;
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
                ADE9000_hotplug[SPI_CSS3 - 1] = 1;
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
                ADE9000_hotplug[SPI_CSS4 - 1] = 1;
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
                ADE9000_hotplug[SPI_CSS5 - 1] = 1;
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
                ADE9000_hotplug[SPI_CSS6 - 1] = 1;
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
                ADE9000_hotplug[SPI_CSS7 - 1] = 1;
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
                ADE9000_hotplug[SPI_CSS8 - 1] = 1;
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
                ADE9000_hotplug[SPI_CSS9 - 1] = 1;
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
                ADE9000_hotplug[SPI_CSS10 - 1] = 1;
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
                ADE9000_hotplug[SPI_CSS11 - 1] = 1;
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
                ADE9000_hotplug[SPI_CSS12 - 1] = 1;
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