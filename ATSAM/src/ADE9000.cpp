/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */

#include "ADE9000.h"

bool DEBUG = false;
bool SECONDHEX = false;

Calibration_regs calibration_data;
VoltageRMSOSRegs voltage_RMS_OS;
CurrentRMSOSRegs current_RMS_OS;




// *****************************************************************************
// *****************************************************************************
// Section: ADE9000 SPI Functions
// *****************************************************************************
// *****************************************************************************

void SPI_CSS_Control(SPI_CSS_Pin pin, bool set)
{
    switch (pin)
    {
    case SPI_CSS1:
        set ? SPI_CSS1_Set() : SPI_CSS1_Clear();
        break;
    case SPI_CSS2:
        set ? SPI_CSS2_Set() : SPI_CSS2_Clear();
        break;
    case SPI_CSS3:
        set ? SPI_CSS3_Set() : SPI_CSS3_Clear();
        break;
    case SPI_CSS4:
        set ? SPI_CSS4_Set() : SPI_CSS4_Clear();
        break;
    case SPI_CSS5:
        set ? SPI_CSS5_Set() : SPI_CSS5_Clear();
        break;
    case SPI_CSS6:
        set ? SPI_CSS6_Set() : SPI_CSS6_Clear();
        break;
    case SPI_CSS7:
        set ? SPI_CSS7_Set() : SPI_CSS7_Clear();
        break;
    case SPI_CSS8:
        set ? SPI_CSS8_Set() : SPI_CSS8_Clear();
        break;
    case SPI_CSS9:
        set ? SPI_CSS9_Set() : SPI_CSS9_Clear();
        break;
    case SPI_CSS10:
        set ? SPI_CSS10_Set() : SPI_CSS10_Clear();
        break;
    case SPI_CSS11:
        set ? SPI_CSS11_Set() : SPI_CSS11_Clear();
        break;
    case SPI_CSS12:
        set ? SPI_CSS12_Set() : SPI_CSS12_Clear();
        break;
    default:
        break;
    }
}

void SPI_Write(uint16_t address, uint32_t data, bool is32Bit, SPI_CSS_Pin css_pin)
{
    char debugMsg[128]; // Buffer for debug messages
    uint16_t cmd_hdr = (address << 4) & 0xFFF0;
    uint8_t addressHigh = (cmd_hdr >> 8) & 0xFF;
    uint8_t addressLow = cmd_hdr & 0xFF;

    if (DEBUG)
    {
        sprintf(debugMsg, "SPI_Write: Start, Address=0x%04X, Data=0x%08X, is32Bit=%d", cmd_hdr, data, is32Bit);
        printf("%s\n", debugMsg);
    }

    SPI_CSS_Control(css_pin, false);

    if (DEBUG)
    {
        sprintf(debugMsg, "SPI_Write: CSS Pin=%d set to LOW", css_pin);
        printf("%s\n", debugMsg);
    }

    SERCOM0_SPI_Write(&addressHigh, 1);
    SERCOM0_SPI_Write(&addressLow, 1);
    if (DEBUG)
    {
        sprintf(debugMsg, "SPI_Write: Address sent (High=0x%02X, Low=0x%02X)", addressHigh, addressLow);
        printf("%s\n", debugMsg);
    }

    if (is32Bit)
    {
        for (int i = 3; i >= 0; i--)
        {
            uint8_t dataChunk = (data >> (i * 8)) & 0xFF;
            SERCOM0_SPI_Write(&dataChunk, 1);
            if (DEBUG)
            {
                sprintf(debugMsg, "SPI_Write: Sent 32-bit Data Chunk[%d]=0x%02X", 3 - i, dataChunk);
                printf("%s\n", debugMsg);
            }
        }
    }
    else
    {
        uint8_t dataHigh = (data >> 8) & 0xFF;
        uint8_t dataLow = data & 0xFF;
        SERCOM0_SPI_Write(&dataHigh, 1);
        SERCOM0_SPI_Write(&dataLow, 1);
        if (DEBUG)
        {
            sprintf(debugMsg, "SPI_Write: Sent 16-bit Data (High=0x%02X, Low=0x%02X)", dataHigh, dataLow);
            printf("%s\n", debugMsg);
        }
    }
    SPI_CSS_Control(css_pin, true);
    if (DEBUG)
    {
        sprintf(debugMsg, "SPI_Write: CSS Pin=%d set to HIGH", css_pin);
        printf("%s\n", debugMsg);
    }
}

uint32_t SPI_Read(uint32_t address, bool is32Bit, SPI_CSS_Pin css_pin)
{
    uint32_t data = 0;
    uint8_t txBuf[2];
    uint8_t rxBuf[4] = {0};
    char debugMsg[128];

    // ADE9000 SPI read: 0x01 + 24-bit address
    txBuf[0] = (address >> 4) & 0xFF;
    txBuf[1] = (address << 4) | 0x8; // read byte 0x8 added for write it should be 0x0
    // txBuf[2] = (address >> 8) & 0xFF;
    // txBuf[3] = address & 0xFF;

    if (DEBUG)
    {
        sprintf(debugMsg, "SPI_Read: Addr=0x%06lX (is32Bit=%d)", (unsigned long)address, is32Bit);
        printf("%s\n", debugMsg);
    }

    // Pull CS low
    SPI_CSS_Control(css_pin, false);

    // Send the 4-byte command
    SERCOM0_SPI_Write(txBuf, 2);

    // Now receive 2 or 4 bytes
    if (is32Bit)
    {
        SERCOM0_SPI_Read(rxBuf, 4);
        data = ((uint32_t)rxBuf[0] << 24) |
               ((uint32_t)rxBuf[1] << 16) |
               ((uint32_t)rxBuf[2] << 8) |
               (uint32_t)rxBuf[3];
    }
    else
    {
        SERCOM0_SPI_Read(rxBuf, 2);
        data = ((uint32_t)rxBuf[0] << 8) | rxBuf[1];
    }

    // Pull CS high
    SPI_CSS_Control(css_pin, true);

    if (DEBUG)
    {
        sprintf(debugMsg, "SPI_Read: Data=0x%08lX", (unsigned long)data);
        printf("%s\n", debugMsg);
    }

    return data;
}

// *****************************************************************************
// *****************************************************************************
// Section: ADE9000 Initialize Functions
// *****************************************************************************
// *****************************************************************************

bool IsADE9000(SPI_CSS_Pin css_pin)
{
    uint32_t part_id = SPI_Read(ADDR_PART_ID, true, css_pin); // Assume SPI_Read reads 32-bit value
    printf("PART_ID Register: 0x%08lX\n", part_id);

    if (part_id == 0xffffffff)
        return 0;
    // Check if bit 20 (ADE9000_ID) is set
    return (part_id & (1 << 20)) != 0;
}

void ADE9000_Initialize(SPI_CSS_Pin css_pin)
{
    SPI_Write(ADDR_PGA_GAIN, ADE9000_PGA_GAIN, false, css_pin);
    SPI_Write(ADDR_CONFIG0, ADE9000_CONFIG0, true, css_pin);
    SPI_Write(ADDR_CONFIG1, ADE9000_CONFIG1, false, css_pin);
    SPI_Write(ADDR_CONFIG2, ADE9000_CONFIG2, false, css_pin);
    SPI_Write(ADDR_CONFIG3, ADE9000_CONFIG3, false, css_pin);
    SPI_Write(ADDR_ACCMODE, ADE9000_ACCMODE, false, css_pin);
    SPI_Write(ADDR_ZX_LP_SEL, ADE9000_ZX_LP_SEL, false, css_pin);
    SPI_Write(ADDR_MASK0, ADE9000_MASK0, true, css_pin);
    SPI_Write(ADDR_MASK1, ADE9000_MASK1, true, css_pin);
    SPI_Write(ADDR_EVENT_MASK, ADE9000_EVENT_MASK, true, css_pin);
    SPI_Write(ADDR_WFB_CFG, ADE9000_WFB_CFG, false, css_pin);
    SPI_Write(ADDR_VLEVEL, ADE9000_VLEVEL, true, css_pin);
    SPI_Write(ADDR_DICOEFF, ADE9000_DICOEFF, true, css_pin);
    SPI_Write(ADDR_EGY_TIME, ADE9000_EGY_TIME, false, css_pin);
    SPI_Write(ADDR_AVGAIN, 0x0131B28D, true, css_pin);
    SPI_Write(ADDR_BVGAIN, 0x0131CB92, true, css_pin);
    SPI_Write(ADDR_CVGAIN, 0x01321952, true, css_pin);
    
    SPI_Write(ADDR_AIGAIN, 0xFF200000, true, css_pin); // unit2   Current is 9.38 A through Clamp meter and 9.35 A through ADE9000
//    SPI_Write(ADDR_BIGAIN, 0xFF200000, true, css_pin); // unit2
//    SPI_Write(ADDR_CIGAIN, 0xFF200000, true, css_pin); // unit2
//    SPI_Write(ADDR_AIGAIN, 0xFE146727, true, css_pin);
//    SPI_Write(ADDR_BIGAIN, 0xFDFDC6ED, true, css_pin);
//    SPI_Write(ADDR_CIGAIN, 0xFDF8B72C, true, css_pin);
//    SPI_Write(ADDR_AIGAIN, 0, true, css_pin);
//    SPI_Write(ADDR_BIGAIN, 0, true, css_pin);
//    SPI_Write(ADDR_CIGAIN, 0, true, css_pin);
    
    SPI_Write(ADDR_APGAIN, 0, true, css_pin);
    SPI_Write(ADDR_BPGAIN, 0, true, css_pin);
    SPI_Write(ADDR_CPGAIN, 0, true, css_pin);
    SPI_Write(ADDR_EP_CFG, ADE9000_EP_CFG, false, css_pin);
    SPI_Write(ADDR_RUN, ADE9000_RUN_ON, false, css_pin);
    SPI_Write(ADDR_TEMP_CFG, ADE9000_TEMP_CFG, false, css_pin);
}

// *****************************************************************************
// *****************************************************************************
// Section: ADE9000 Measurement Functions
// *****************************************************************************
// *****************************************************************************

/***************************GENERAL VALUES*****************************/
void ReadVoltageRMSRegs(VoltageRMSRegs *Data, SPI_CSS_Pin css_pin)
{
    uint32_t rawValue;
    Data->VoltageRMSReg_A = int32_t(SPI_Read(ADDR_AVRMS, true, css_pin));
    Data->VoltageRMSReg_B = int32_t(SPI_Read(ADDR_BVRMS, true, css_pin));
    Data->VoltageRMSReg_C = int32_t(SPI_Read(ADDR_CVRMS, true, css_pin));

    if (css_pin)
    {
        Data->VoltageRMSValue_A = (float)Data->VoltageRMSReg_A / (IC_CAL_VRMS_CC);
        Data->VoltageRMSValue_B = (float)Data->VoltageRMSReg_B / (IC_CAL_VRMS_CC);
        Data->VoltageRMSValue_C = (float)Data->VoltageRMSReg_C / (IC_CAL_VRMS_CC);
        if (Data->VoltageRMSValue_A < NOMINAL_INPUT_VOLTAGE)
        {
            Data->VoltageRMSValue_A = 0.00f;
        }
        if (Data->VoltageRMSValue_B < NOMINAL_INPUT_VOLTAGE)
        {
            Data->VoltageRMSValue_B = 0.00f;
        }
        if (Data->VoltageRMSValue_C < NOMINAL_INPUT_VOLTAGE)
        {
            Data->VoltageRMSValue_C = 0.00f;
        }
    }

    if (DEBUG)
    {
        printf("Reading Voltage RMS Registers...\n");
        printf("VoltageRMSReg_A: %d\n", Data->VoltageRMSReg_A);
        printf("VoltageRMSReg_B: %d\n", Data->VoltageRMSReg_B);
        printf("VoltageRMSReg_C: %d\n", Data->VoltageRMSReg_C);
        printf("VoltageRMSValue_A: %.6f V, VoltageRMSValue_B: %.6f V, VoltageRMSValue_C: %.6f V\n",
               Data->VoltageRMSValue_A, Data->VoltageRMSValue_B, Data->VoltageRMSValue_C);
    }
    
    
}

void ReadVoltageRMSOSRegs(VoltageRMSOSRegs* VRMS_OS, SPI_CSS_Pin css_pin)
{
    int32_t VoltageRMSOSReg_A = int32_t(SPI_Read(ADDR_AVRMSOS, true, css_pin));
    int32_t VoltageRMSOSReg_B = int32_t(SPI_Read(ADDR_BVRMSOS, true, css_pin));
    int32_t VoltageRMSOSReg_C = int32_t(SPI_Read(ADDR_CVRMSOS, true, css_pin));

    if (css_pin)
    {
         VRMS_OS->VoltageRMSOSValue_A = (float)VoltageRMSOSReg_A/1000;
         VRMS_OS->VoltageRMSOSValue_B = (float)VoltageRMSOSReg_B/1000;
         VRMS_OS->VoltageRMSOSValue_C = (float)VoltageRMSOSReg_C/1000;
    }

}

void ReadCurrentRMSOSRegs(CurrentRMSOSRegs* IRMS_OS, SPI_CSS_Pin css_pin)
{
    int32_t CurrentRMSOSReg_A = int32_t(SPI_Read(ADDR_AIRMSOS, true, css_pin));
    int32_t CurrentRMSOSReg_B = int32_t(SPI_Read(ADDR_BIRMSOS, true, css_pin));
    int32_t CurrentRMSOSReg_C = int32_t(SPI_Read(ADDR_CIRMSOS, true, css_pin));

    if (css_pin)
    {
         IRMS_OS->CurrentRMSOSValue_A = (float)CurrentRMSOSReg_A/1000;
         IRMS_OS->CurrentRMSOSValue_B = (float)CurrentRMSOSReg_B/1000;
         IRMS_OS->CurrentRMSOSValue_C = (float)CurrentRMSOSReg_C/1000;
    }

}

void ReadLineVoltages(VoltageRMSRegs *rmsData, LineVoltageRegs *lineData)
{
    const float sqrt3 = sqrtf(3.0f);

    lineData->VoltageRMS_A = rmsData->VoltageRMSValue_A;
    lineData->VoltageRMS_B = rmsData->VoltageRMSValue_B;
    lineData->VoltageRMS_C = rmsData->VoltageRMSValue_C;

    lineData->LineVoltage_AB = lineData->VoltageRMS_A * sqrt3;
    lineData->LineVoltage_BC = lineData->VoltageRMS_B * sqrt3;
    lineData->LineVoltage_CA = lineData->VoltageRMS_C * sqrt3;

    if (DEBUG)
    {
        printf("Calculating Line Voltages...\n");
        printf("LineVoltage_AB: %.6f V, LineVoltage_BC: %.6f V, LineVoltage_CA: %.6f V\n",
               lineData->LineVoltage_AB, lineData->LineVoltage_BC, lineData->LineVoltage_CA);
    }
}

void ReadCurrentRMSRegs(CurrentRMSRegs *Data, SPI_CSS_Pin css_pin)
{
    uint32_t rawValue;
    Data->CurrentRMSReg_A = uint32_t(SPI_Read(ADDR_AIRMS, true, css_pin));
    Data->CurrentRMSReg_B = int32_t(SPI_Read(ADDR_BIRMS, true, css_pin));
    Data->CurrentRMSReg_C = int32_t(SPI_Read(ADDR_CIRMS, true, css_pin));

    if (css_pin)
    {
//        Data->CurrentRMSValue_A = (float)Data->CurrentRMSReg_A * calibration_data.cal_irms_cc;
//        \frac{0.04}{100}\cdot10
//        \frac{\operatorname{ans}}{0.707}
//        \frac{5\cdot10^{6}}{\operatorname{ans}\cdot52702092}
//        Data->CurrentRMSValue_A = (float)Data->CurrentRMSReg_A * 16.76878406e-6;
//        Data->CurrentRMSValue_A = (float)Data->CurrentRMSReg_A * 1.67687841e-4;
        
//    float current_transfer_func  = (float) (R_BURDEN/(100)) *10;  //
////    current_transfer_functionn = current_transfer_func;
//    
//    float temp_var;
//    temp_var = (current_transfer_func / 0.707); // I_FSP
//    // sqrt_2 * 0.04 / 100 * 10 * 52702092
//    calibration_data.cal_irms_cc = (NOMINAL_INPUT_CURRENT)/(temp_var * ADE9000_RMS_FULL_SCALE_CODES); // Current Conversion Constant
    //----------------------For testing-------------------
//    current_conversion_constant = calibration_data.cal_irms_cc;
    
    
    Data->CurrentRMSValue_A = (float)((float)(Data->CurrentRMSReg_A) / (float)calibration_data.cal_irms_cc);
//    Data->CurrentRMSValue_A = (float)((float)(Data->CurrentRMSReg_A) / (float)(2981.2805308733837434676435559465));

//        Data->CurrentRMSValue_A = (float)Data->CurrentRMSReg_A * calibration_data.cal_irms_cc;
    Data->CurrentRMSValue_B = (float)((float)(Data->CurrentRMSReg_B) / (float)calibration_data.cal_irms_cc);

    Data->CurrentRMSValue_C = (float)((float)(Data->CurrentRMSReg_C) / (float)calibration_data.cal_irms_cc);
    }

    if (DEBUG)
    {
        printf("Reading Current RMS Registers...\n");
        printf("CurrentRMSReg_A: %d\n", Data->CurrentRMSReg_A);
        printf("CurrentRMSReg_B: %d\n", Data->CurrentRMSReg_B);
        printf("CurrentRMSReg_C: %d\n", Data->CurrentRMSReg_C);
        printf("CurrentRMSValue_A: %.6f A, CurrentRMSValue_B: %.6f A, CurrentRMSValue_C: %.6f A\n",
               Data->CurrentRMSValue_A, Data->CurrentRMSValue_B, Data->CurrentRMSValue_C);
    }
}

void ReadPeriodRegsnValues(PeriodRegs *Data, VoltageRMSRegs *RMSData, SPI_CSS_Pin css_pin)
{
    if (DEBUG)
    {
        printf("Reading Period Registers and Calculating Frequencies...\n");
    }
    uint32_t tempReg;

    if (RMSData->VoltageRMSValue_A)
    {
        tempReg = SPI_Read(ADDR_APERIOD, true, css_pin);
        Data->PeriodReg_A = tempReg;
        Data->FrequencyValue_A = (float)(8000 * 65536) / (tempReg + 1);
        if (DEBUG)
        {
            printf("PeriodReg_A: %u, FrequencyValue_A: %.2f Hz\n", tempReg, Data->FrequencyValue_A);
        }
    }

    if (RMSData->VoltageRMSValue_B)
    {
        tempReg = SPI_Read(ADDR_BPERIOD, true, css_pin);
        Data->PeriodReg_B = tempReg;
        Data->FrequencyValue_B = (float)(8000 * 65536) / (tempReg + 1);
        if (DEBUG)
        {
            printf("PeriodReg_B: %u, FrequencyValue_B: %.2f Hz\n", tempReg, Data->FrequencyValue_B);
        }
    }

    if (RMSData->VoltageRMSValue_C)
    {
        tempReg = SPI_Read(ADDR_CPERIOD, true, css_pin);
        Data->PeriodReg_C = tempReg;
        Data->FrequencyValue_C = (float)(8000 * 65536) / (tempReg + 1);
        if (DEBUG)
        {
            printf("PeriodReg_C: %u, FrequencyValue_C: %.2f Hz\n", tempReg, Data->FrequencyValue_C);
        }
    }
}

/***************************POWER VALUES*****************************/
void ReadActivePowerRegs(ActivePowerRegs *Data, SPI_CSS_Pin css_pin)
{
    uint32_t rawValue;
    Data->ActivePowerReg_A = int32_t(SPI_Read(ADDR_AWATT, true, css_pin));
    Data->ActivePowerReg_B = int32_t(SPI_Read(ADDR_BWATT, true, css_pin));
    Data->ActivePowerReg_C = int32_t(SPI_Read(ADDR_CWATT, true, css_pin));
    if (css_pin)
    {
        Data->ActivePowerValue_A = (float)Data->ActivePowerReg_A / (calibration_data.cal_power_cc * 1000.0f);
        Data->ActivePowerValue_B = (float)Data->ActivePowerReg_B / (calibration_data.cal_power_cc * 1000.0f);
        Data->ActivePowerValue_C = (float)Data->ActivePowerReg_C / (calibration_data.cal_power_cc * 1000.0f);
    }

    if (DEBUG)
    {
        printf("Reading Active Power Registers...\n");
        printf("ActivePowerReg_A: 0x%X, ", Data->ActivePowerReg_A);
        printf("ActivePowerReg_B: 0x%X, ", Data->ActivePowerReg_B);
        printf("ActivePowerReg_C: 0x%X\n", Data->ActivePowerReg_C);
        printf("ActivePowerValue_A: %.3f kW, ActivePowerValue_B: %.3f kW, ActivePowerValue_C: %.3f kW\n",
               Data->ActivePowerValue_A, Data->ActivePowerValue_B, Data->ActivePowerValue_C);
    }
}

void ReadReactivePowerRegs(ReactivePowerRegs *Data1, ApparentPowerRegs *Data2, PowerFactorRegs *Data3, SPI_CSS_Pin css_pin)
{
    Data1->ReactivePowerReg_A = int32_t(SPI_Read(ADDR_AVAR, true, css_pin));
    Data1->ReactivePowerReg_B = int32_t(SPI_Read(ADDR_BVAR, true, css_pin));
    Data1->ReactivePowerReg_C = int32_t(SPI_Read(ADDR_CVAR, true, css_pin));

    float RPV1_sign = (float)Data1->ReactivePowerReg_A / calibration_data.cal_power_cc;
    float RPV2_sign = (float)Data1->ReactivePowerReg_B / calibration_data.cal_power_cc;
    float RPV3_sign = (float)Data1->ReactivePowerReg_C / calibration_data.cal_power_cc;

    float phaseAngle1 = acosf(Data3->PowerFactorValue_A);
    float phaseAngle2 = acosf(Data3->PowerFactorValue_B);
    float phaseAngle3 = acosf(Data3->PowerFactorValue_C);

    Data1->ReactivePowerValue_A = (Data2->ApparentPowerValue_A * sin(phaseAngle1)) * (RPV1_sign < 0 ? -1 : 1);
    Data1->ReactivePowerValue_B = (Data2->ApparentPowerValue_B * sin(phaseAngle2)) * (RPV2_sign < 0 ? -1 : 1);
    Data1->ReactivePowerValue_C = (Data2->ApparentPowerValue_C * sin(phaseAngle3)) * (RPV3_sign < 0 ? -1 : 1);

    printf("Reactive Power Values: A: %f, B: %f, C: %f\n", Data1->ReactivePowerValue_A, Data1->ReactivePowerValue_B, Data1->ReactivePowerValue_C);
}

void ReadApparentPowerRegs(ApparentPowerRegs *Data, SPI_CSS_Pin css_pin)
{
    uint32_t rawValue;
    Data->ApparentPowerReg_A = int32_t(SPI_Read(ADDR_AVA, true, css_pin));
    Data->ApparentPowerReg_B = int32_t(SPI_Read(ADDR_BVA, true, css_pin));
    Data->ApparentPowerReg_C = int32_t(SPI_Read(ADDR_CVA, true, css_pin));
    if (css_pin)
    {
        Data->ApparentPowerValue_A = (float)Data->ApparentPowerReg_A / (calibration_data.cal_power_cc * 1000.0f);
        Data->ApparentPowerValue_B = (float)Data->ApparentPowerReg_B / (calibration_data.cal_power_cc * 1000.0f);
        Data->ApparentPowerValue_C = (float)Data->ApparentPowerReg_C / (calibration_data.cal_power_cc * 1000.0f);
    }

    if (DEBUG)
    {
        printf("Reading Apparent Power Registers...\n");
        printf("ApparentPowerReg_A: 0x%X, ", Data->ApparentPowerReg_A);
        printf("ApparentPowerReg_B: 0x%X, ", Data->ApparentPowerReg_B);
        printf("ApparentPowerReg_C: 0x%X\n", Data->ApparentPowerReg_C);
        printf("ApparentPowerValue_A: %.3f kVA, ApparentPowerValue_B: %.3f kVA, ApparentPowerValue_C: %.3f kVA\n",
               Data->ApparentPowerValue_A, Data->ApparentPowerValue_B, Data->ApparentPowerValue_C);
    }
}

void ReadPowerFactorRegsnValues(PowerFactorRegs *Data, VoltageRMSRegs *rmsData, CurrentRMSRegs *currentData, SPI_CSS_Pin css_pin)
{
    if (DEBUG)
    {
        printf("Reading Power Factor Registers and Values...\n");
    }

    uint32_t rawValue;
    int32_t tempReg;

    if (rmsData->VoltageRMSValue_A > NOMINAL_INPUT_VOLTAGE && currentData->CurrentRMSValue_A > 0.1)
    {
        rawValue = SPI_Read(ADDR_APF, true, css_pin);
        tempReg = *(int32_t *)&rawValue;
        Data->PowerFactorReg_A = tempReg;
        Data->PowerFactorValue_A = (float)tempReg / 134217728.0f;
        if (DEBUG)
        {
            printf("PowerFactorReg_A: %u, PowerFactorValue_A: %.2f\n", tempReg, Data->PowerFactorValue_A);
        }
    }
    else
    {
        Data->PowerFactorValue_A = 1.0f;
    }

    if (rmsData->VoltageRMSValue_B > NOMINAL_INPUT_VOLTAGE && currentData->CurrentRMSValue_B > 0.1)
    {
        rawValue = SPI_Read(ADDR_BPF, true, css_pin);
        tempReg = *(int32_t *)&rawValue;
        Data->PowerFactorReg_B = tempReg;
        Data->PowerFactorValue_B = (float)tempReg / 134217728.0f;
        if (DEBUG)
        {
            printf("PowerFactorReg_B: %u, PowerFactorValue_B: %.2f\n", tempReg, Data->PowerFactorValue_B);
        }
    }
    else
    {
        Data->PowerFactorValue_B = 1.0f;
    }

    if (rmsData->VoltageRMSValue_C > NOMINAL_INPUT_VOLTAGE && currentData->CurrentRMSValue_C > 0.1)
    {
        rawValue = SPI_Read(ADDR_CPF, true, css_pin);
        tempReg = *(int32_t *)&rawValue;
        Data->PowerFactorReg_C = tempReg;
        Data->PowerFactorValue_C = (float)tempReg / 134217728.0f;
        if (DEBUG)
        {
            printf("PowerFactorReg_C: %u, PowerFactorValue_C: %.2f\n", tempReg, Data->PowerFactorValue_C);
        }
    }
    else
    {
        Data->PowerFactorValue_C = 1.0f;
    }
}

/***************************ENERGY VALUES*****************************/
void ReadEnergyRegs(EnergyRegs *Data1, ActivePowerRegs *Data2, SPI_CSS_Pin css_pin)
{
    static uint8_t discardInitialReadingsCount = 0;
    const uint8_t INITIAL_READINGS_TO_DISCARD = 10;
    uint32_t status0;
    uint32_t rawValue;
    status0 = SPI_Read(ADDR_STATUS0, true, css_pin);
    SECONDHEX = true;
    // -----------------------------For Testing --------------------------------
    status0 = 0x01;
    // -------------------------------------------------------------------------

    if (status0 & 0x01)
    {
        if (discardInitialReadingsCount < INITIAL_READINGS_TO_DISCARD)
        {
            discardInitialReadingsCount++;
            return;
        }
        if (SECONDHEX)
        {
            rawValue = SPI_Read(ADDR_AFWATTHR_HI, true, css_pin);
            Data1->ActiveEnergyReg_A = (rawValue & 0x80000000) ? -((int32_t)(~rawValue + 1)) : (int32_t)rawValue;
            rawValue = SPI_Read(ADDR_BFWATTHR_HI, true, css_pin);
            Data1->ActiveEnergyReg_B = (rawValue & 0x80000000) ? -((int32_t)(~rawValue + 1)) : (int32_t)rawValue;
            rawValue = SPI_Read(ADDR_CFWATTHR_HI, true, css_pin);
            Data1->ActiveEnergyReg_C = (rawValue & 0x80000000) ? -((int32_t)(~rawValue + 1)) : (int32_t)rawValue;
            rawValue = SPI_Read(ADDR_AFVARHR_HI, true, css_pin);
            Data1->ReactiveEnergyReg_A = (rawValue & 0x80000000) ? -((int32_t)(~rawValue + 1)) : (int32_t)rawValue;
            rawValue = SPI_Read(ADDR_BFVARHR_HI, true, css_pin);
            Data1->ReactiveEnergyReg_B = (rawValue & 0x80000000) ? -((int32_t)(~rawValue + 1)) : (int32_t)rawValue;
            rawValue = SPI_Read(ADDR_CFVARHR_HI, true, css_pin);
            Data1->ReactiveEnergyReg_C = (rawValue & 0x80000000) ? -((int32_t)(~rawValue + 1)) : (int32_t)rawValue;
        }
        else
        {
            Data1->ActiveEnergyReg_A = SPI_Read(ADDR_AFWATTHR_HI, true, css_pin);
            Data1->ActiveEnergyReg_B = SPI_Read(ADDR_BFWATTHR_HI, true, css_pin);
            Data1->ActiveEnergyReg_C = SPI_Read(ADDR_CFWATTHR_HI, true, css_pin);
            Data1->ReactiveEnergyReg_A = SPI_Read(ADDR_AFVARHR_HI, true, css_pin);
            Data1->ReactiveEnergyReg_B = SPI_Read(ADDR_BFVARHR_HI, true, css_pin);
            Data1->ReactiveEnergyReg_C = SPI_Read(ADDR_CFVARHR_HI, true, css_pin);
        }

        // Data1->ActiveEnergyValue_kWh_A = Data1->ActiveEnergyReg_A * (float) (CAL_ENERGY_CC * 8000 * 3600) / 1000000 / 1000;
        // Data1->ActiveEnergyValue_kWh_B = Data1->ActiveEnergyReg_B * (float) (CAL_ENERGY_CC * 8000 * 3600) / 1000000 / 1000;
        // Data1->ActiveEnergyValue_kWh_C = Data1->ActiveEnergyReg_C * (float) (CAL_ENERGY_CC * 8000 * 3600) / 1000000 / 1000;
//         Data1->ReactiveEnergyValue_kWh_A = Data1->ReactiveEnergyReg_A * (float) (calibration_data.cal_energy_cc * 8000 * 3600) / 1000000 / 1000;
//         Data1->ReactiveEnergyValue_kWh_B = Data1->ReactiveEnergyReg_B * (float) (calibration_data.cal_energy_cc * 8000 * 3600) / 1000000 / 1000;
//         Data1->ReactiveEnergyValue_kWh_C = Data1->ReactiveEnergyReg_C * (float) (calibration_data.cal_energy_cc * 8000 * 3600) / 1000000 / 1000;

        const float timeFactor = 1.0 / 3600.0;
        if (Data2->ActivePowerValue_A < 0)
        {
            Data1->NActiveEnergyValue_kWh_A = Data2->ActivePowerValue_A * timeFactor;
            Data1->PActiveEnergyValue_kWh_A = 0;
        }
        else
        {
            Data1->PActiveEnergyValue_kWh_A = Data2->ActivePowerValue_A * timeFactor;
            Data1->NActiveEnergyValue_kWh_A = 0;
        }
        if (Data2->ActivePowerValue_B < 0)
        {
            Data1->NActiveEnergyValue_kWh_B = Data2->ActivePowerValue_B * timeFactor;
            Data1->PActiveEnergyValue_kWh_B = 0;
        }
        else
        {
            Data1->PActiveEnergyValue_kWh_B = Data2->ActivePowerValue_B * timeFactor;
            Data1->NActiveEnergyValue_kWh_B = 0;
        }
        if (Data2->ActivePowerValue_C < 0)
        {
            Data1->NActiveEnergyValue_kWh_C = Data2->ActivePowerValue_C * timeFactor;
            Data1->PActiveEnergyValue_kWh_C = 0;
        }
        else
        {
            Data1->PActiveEnergyValue_kWh_C = Data2->ActivePowerValue_C * timeFactor;
            Data1->NActiveEnergyValue_kWh_C = 0;
        }

        SPI_Write(ADDR_STATUS0, (status0 | 0x01), true, css_pin);
    }
    else
    {
        Data1->ActiveEnergyReg_A = 0;
        Data1->ActiveEnergyReg_B = 0;
        Data1->ActiveEnergyReg_C = 0;
        Data1->PActiveEnergyValue_kWh_A = 0;
        Data1->PActiveEnergyValue_kWh_B = 0;
        Data1->PActiveEnergyValue_kWh_C = 0;
        Data1->NActiveEnergyValue_kWh_A = 0;
        Data1->NActiveEnergyValue_kWh_B = 0;
        Data1->NActiveEnergyValue_kWh_C = 0;

        Data1->ReactiveEnergyReg_A = 0;
        Data1->ReactiveEnergyReg_B = 0;
        Data1->ReactiveEnergyReg_C = 0;
        Data1->ReactiveEnergyValue_kWh_A = 0;
        Data1->ReactiveEnergyValue_kWh_B = 0;
        Data1->ReactiveEnergyValue_kWh_C = 0;
    }
}

/***************************THRESHOLD VALUES*****************************/
void ReadVoltageTHDRegsnValues(VoltageTHDRegs *Data, SPI_CSS_Pin css_pin)
{
    if (DEBUG)
    {
        printf("Reading Voltage THD Registers and Values...\n");
    }
    uint32_t rawValue;
    int32_t tempReg;

    rawValue = SPI_Read(ADDR_AVTHD, true, css_pin);
    tempReg = *(int32_t *)&rawValue;
    Data->VoltageTHDReg_A = tempReg;
    Data->VoltageTHDValue_A = (float)tempReg * 100 / 134217728.0f;
    if (DEBUG)
    {
        printf("VoltageTHDReg_A: %u, VoltageTHDValue_A: %.2f%%\n", tempReg, Data->VoltageTHDValue_A);
    }

    rawValue = SPI_Read(ADDR_BVTHD, true, css_pin);
    tempReg = *(int32_t *)&rawValue;
    Data->VoltageTHDReg_B = tempReg;
    Data->VoltageTHDValue_B = (float)tempReg * 100 / 134217728.0f;
    if (DEBUG)
    {
        printf("VoltageTHDReg_B: %u, VoltageTHDValue_B: %.2f%%\n", tempReg, Data->VoltageTHDValue_B);
    }

    rawValue = SPI_Read(ADDR_CVTHD, true, css_pin);
    tempReg = *(int32_t *)&rawValue;
    Data->VoltageTHDReg_C = tempReg;
    Data->VoltageTHDValue_C = (float)tempReg * 100 / 134217728.0f;
    if (DEBUG)
    {
        printf("VoltageTHDReg_C: %u, VoltageTHDValue_C: %.2f%%\n", tempReg, Data->VoltageTHDValue_C);
    }
}

void ReadCurrentTHDRegsnValues(CurrentTHDRegs *Data, SPI_CSS_Pin css_pin)
{
    if (DEBUG)
    {
        printf("Reading Current THD Registers and Values...\n");
    }

    uint32_t rawValue;
    int32_t tempReg;

    rawValue = SPI_Read(ADDR_AITHD, true, css_pin);
    tempReg = *(int32_t *)&rawValue;
    Data->CurrentTHDReg_A = tempReg;
    Data->CurrentTHDValue_A = (float)tempReg * 100 / 134217728.0f;
    if (DEBUG)
    {
        printf("CurrentTHDReg_A: %u, CurrentTHDValue_A: %.2f%%\n", tempReg, Data->CurrentTHDValue_A);
    }

    rawValue = SPI_Read(ADDR_BITHD, true, css_pin);
    tempReg = *(int32_t *)&rawValue;
    Data->CurrentTHDReg_B = tempReg;
    Data->CurrentTHDValue_B = (float)tempReg * 100 / 134217728.0f;
    if (DEBUG)
    {
        printf("CurrentTHDReg_B: %u, CurrentTHDValue_B: %.2f%%\n", tempReg, Data->CurrentTHDValue_B);
    }

    rawValue = SPI_Read(ADDR_CITHD, true, css_pin);
    tempReg = *(int32_t *)&rawValue;
    Data->CurrentTHDReg_C = tempReg;
    Data->CurrentTHDValue_C = (float)tempReg * 100 / 134217728.0f;
    if (DEBUG)
    {
        printf("CurrentTHDReg_C: %u, CurrentTHDValue_C: %.2f%%\n", tempReg, Data->CurrentTHDValue_C);
    }
}

void ReadAngleRegsnValues(AngleRegs *Data, SPI_CSS_Pin css_pin)
{
    printf("Reading Angle Registers and Values...\n");

    uint32_t tempReg;
    uint16_t temp;
    float mulConstant;
    float tempValue;

    temp = SPI_Read(ADDR_ACCMODE, false, css_pin);
    printf("ACC MODE Register: %u\n", temp);

    if (temp & 0x0100)
    {
        mulConstant = 0.02109375; // Multiplier constant for 60Hz system
        printf("System Frequency: 60Hz, Multiplier Constant: %f\n", mulConstant);
    }
    else
    {
        mulConstant = 0.017578125; // Multiplier constant for 50Hz system
        printf("System Frequency: 50Hz, Multiplier Constant: %f\n", mulConstant);
    }

    tempReg = SPI_Read(ADDR_ANGL_VA_VB, true, css_pin);
    Data->AngleReg_VA_VB = tempReg;
    tempValue = tempReg * 0.017578125; // mulConstant
    Data->AngleValue_VA_VB = tempValue;
    printf("AngleReg_VA_VB: %u, AngleValue_VA_VB: %f\n", tempReg, tempValue);

    tempReg = SPI_Read(ADDR_ANGL_VB_VC, true, css_pin);
    Data->AngleReg_VB_VC = tempReg;
    tempValue = tempReg * 0.017578125; // mulConstant
    Data->AngleValue_VB_VC = tempValue;
    printf("AngleReg_VB_VC: %u, AngleValue_VB_VC: %f\n", tempReg, tempValue);

    tempReg = SPI_Read(ADDR_ANGL_VA_VC, true, css_pin);
    Data->AngleReg_VA_VC = tempReg;
    tempValue = tempReg * 0.017578125; // mulConstant
    Data->AngleValue_VA_VC = tempValue;
    printf("AngleReg_VA_VC: %u, AngleValue_VA_VC: %f\n", tempReg, tempValue);

    tempReg = int32_t(SPI_Read(ADDR_ANGL_VA_IA, true, css_pin));
    Data->AngleReg_VA_IA = tempReg;
    tempValue = tempReg * 0.017578125; // mulConstant
    Data->AngleValue_VA_IA = tempValue;
    printf("AngleValue_VA_IA: %u, AngleValue_VA_IA: %f\n", tempReg, tempValue);

    tempReg = int32_t(SPI_Read(ADDR_ANGL_VB_IB, true, css_pin));
    Data->AngleReg_VB_IB = tempReg;
    tempValue = tempReg * 0.017578125; // mulConstant
    Data->AngleValue_VB_IB = tempValue;
    printf("AngleValue_VB_IB: %u, AngleValue_VB_IB: %f\n", tempReg, tempValue);

    tempReg = int32_t(SPI_Read(ADDR_ANGL_VC_IC, true, css_pin));
    Data->AngleReg_VC_IC = tempReg;
    tempValue = tempReg * 0.017578125; // mulConstant
    Data->AngleValue_VC_IC = tempValue;
    printf("AngleValue_VC_IC: %u, AngleValue_VC_IC: %f\n", tempReg, tempValue);

    tempReg = int32_t(SPI_Read(ADDR_ANGL_IA_IB, true, css_pin));
    Data->AngleReg_IA_IB = tempReg;
    tempValue = tempReg * 0.017578125; // mulConstant
    Data->AngleValue_IA_IB = tempValue;
    printf("AngleValue_IA_IB: %u, AngleValue_IA_IB: %f\n", tempReg, tempValue);

    tempReg = int32_t(SPI_Read(ADDR_ANGL_IB_IC, true, css_pin));
    Data->AngleReg_IB_IC = tempReg;
    tempValue = tempReg * 0.017578125; // mulConstant
    Data->AngleValue_IB_IC = tempValue;
    printf("AngleValue_IB_IC: %u, AngleValue_IB_IC: %f\n", tempReg, tempValue);

    tempReg = int32_t(SPI_Read(ADDR_ANGL_IA_IC, true, css_pin));
    Data->AngleReg_IA_IC = tempReg;
    tempValue = tempReg * 0.017578125; // mulConstant
    Data->AngleValue_IA_IC = tempValue;
    printf("AngleValue_IA_IC: %u, AngleValue_IA_IC: %f\n", tempReg, tempValue);
}

/***************************TEMPERATURE VALUES*****************************/
void ReadTempRegnValue(TemperatureRegs *Data, SPI_CSS_Pin css_pin)
{
    uint32_t TEMP_CFG;
    uint32_t trim;
    uint16_t gain;
    uint16_t offset;
    uint16_t tempReg;
    float tempValue;

    trim = SPI_Read(ADDR_TEMP_TRIM, true, css_pin);
    gain = (trim & 0xFFFF);
    offset = ((trim >> 16) & 0xFFFF);
    //    printf("Read trim value: 0x%X, Gain: %d, Offset: %d\n", trim, gain, offset);

    tempReg = SPI_Read(ADDR_TEMP_RSLT, false, css_pin);
    //    printf("Read raw temperature register value: %d\n", tempReg);

    tempValue = (float)(offset >> 5) - ((float)tempReg * (float)gain / 65536.0f);
    //    printf("Calculated temperature value: %.2f\n", tempValue);

    Data->Temperature_Reg = tempReg;
    Data->Temperature = tempValue;
}

// *****************************************************************************
// *****************************************************************************
// Section: ADE9000 Calibration Functions
// *****************************************************************************
// *****************************************************************************

void getPGA_gain(SPI_CSS_Pin css_pin, uint8_t *currentGain, uint8_t *voltageGain)
{
    uint16_t pgaGainRegister;

    pgaGainRegister = SPI_Read(ADDR_PGA_GAIN, false, css_pin);
    uint8_t temp = pgaGainRegister & 0x0003;
    *currentGain = (temp == 0) ? 1 : (temp == 1) ? 2
                                                 : 4;
    temp = (pgaGainRegister >> 8) & 0x0003;
    *voltageGain = (temp == 0) ? 1 : (temp == 1) ? 2
                                                 : 4;
}

void ct_ratio_calibration(uint16_t ratio, uint8_t channel, CurrentRMSRegs Data)
{
    /****************************************************************************************************/
    /* Calculation of xIGAIN */
    /****************************************************************************************************/
    
    float current_transfer_func  = (float) (R_BURDEN/(ratio));  //

//    Current Conversion Constant = (I nominal * 10^6)/(Ifsp*full_scale_rms_codes);

    calibration_data.cal_irms_cc = (SQRT_2 * current_transfer_func * ADE9000_RMS_FULL_SCALE_CODES); // I_FSP
    float I_FSP;
    
    I_FSP = (current_transfer_func / 0.707); // I_FSP
    
//    calibration_data.cal_irms_cc = (NOMINAL_INPUT_CURRENT)/(I_FSP * ADE9000_RMS_FULL_SCALE_CODES); // Current Conversion Constant
    calibration_data.cal_power_cc = (2 * current_transfer_func * VOLTAGE_TRANSFER_FUNCTION * ADE9000_WATT_FULL_SCALE_CODES);
    calibration_data.cal_energy_cc = (2 * current_transfer_func * VOLTAGE_TRANSFER_FUNCTION * ADE9000_WATT_FULL_SCALE_CODES);
//    calibration_data.cal_energy_cc = (5 * 220e6)/(I_FSP*VOLTAGE_TRANSFER_FUNCTION*ADE9000_WATT_FULL_SCALE_CODES*3600*8000*pow(2,-13));
    
//    float AIGAIN_value;
//    AIGAIN_value = (float)((AIGAIN_value - 1) * pow(2,27));
    
    // For testing the value is hardcoded below
//    float I_FSP = ((0.004 / 100) * 10) / 0.707;
//    float AIRMS_exp = (float)(I_FSP * ADE9000_RMS_FULL_SCALE_CODES);    
//    float GAIN = (float)((float)(AIRMS_exp) / (float)(Data.CurrentRMSReg_A));
//    AIGAIN_value = (float)((GAIN - 1) * pow(2,27));
//    AIGAIN_value = (float)((4 - 1) * pow(2,27));
//    AIGAIN_value = 164774527.7;
//  
//  AIGAIN = (GAIN - 1) * 2^27;
    
//    float BIGAIN_value = (calibration_data.cal_irms_cc) / Data.CurrentRMSValue_B;
//    BIGAIN_value = (BIGAIN_value - 1) * pow(2,27);
//    
//    float CIGAIN_value = (calibration_data.cal_irms_cc) / Data.CurrentRMSValue_C;
//    CIGAIN_value = (CIGAIN_value - 1) * pow(2,27);
    
//    SPI_Write(ADDR_AIGAIN, AIGAIN_value, true, get_css_pin(channel));
//    SPI_Write(ADDR_AIGAIN, 0xEE6B2800, true, get_css_pin(channel));
    
//    SPI_Write(ADDR_BIGAIN, BIGAIN_value, true, get_css_pin(channel));
//    SPI_Write(ADDR_CIGAIN, CIGAIN_value, true, get_css_pin(channel));
    
    /****************************************************************************************************/
    /* Calculation of xPGAIN */
    /****************************************************************************************************/
    
    uint32_t A_Active_Energy = SPI_Read(ADDR_AWATTHR_HI, true, get_css_pin(channel));
    uint32_t B_Active_Energy = SPI_Read(ADDR_BWATTHR_HI, true, get_css_pin(channel));
    uint32_t C_Active_Energy = SPI_Read(ADDR_CWATTHR_HI, true, get_css_pin(channel));
//    
//    float A_Active_Energy_Expected = (2 * current_transfer_func * 10 * NOMINAL_VOLTAGE * ACCUMULATION_TIME * 8000 * pow(2,-13));
//    float B_Active_Energy_Expected = (2 * current_transfer_func * 10 * NOMINAL_VOLTAGE * ACCUMULATION_TIME * 8000 * pow(2,-13));
//    float C_Active_Energy_Expected = (2 * current_transfer_func * 10 * NOMINAL_VOLTAGE * ACCUMULATION_TIME * 8000 * pow(2,-13));
//            
//    float AP_GAIN = (float)((A_Active_Energy_Expected - A_Active_Energy)/ (A_Active_Energy)) * pow(2,27);
//    A_Active_Energy = 4294967295;
    float A_Active_Energy_Expected = 355587.79;
    float AP_GAIN = (float)(((A_Active_Energy_Expected - A_Active_Energy)/ (A_Active_Energy)) * pow(2,27));
//    float BP_GAIN = (float)((B_Active_Energy_Expected - B_Active_Energy)/ (B_Active_Energy)) * pow(2,27);
//    float CP_GAIN = (float)((C_Active_Energy_Expected - C_Active_Energy)/ (C_Active_Energy)) * pow(2,27);
    
//    SPI_Write(ADDR_APGAIN, AP_GAIN, true, get_css_pin(channel));
//    SPI_Write(ADDR_APGAIN, (uint32_t)AP_GAIN, true, get_css_pin(channel));
    
//    SPI_Write(ADDR_BPGAIN, BP_GAIN, true, get_css_pin(channel));
//    SPI_Write(ADDR_CPGAIN, CP_GAIN, true, get_css_pin(channel));    
    
    /****************************************************************************************************/
           
}

void current_offset_calibration(CurrentRMSRegs *Data, SPI_CSS_Pin css_pin){
    /*
     * Formula for Calibration of IRMS
     * xRMS = sqrt(xRMS0^2 + 2^15 × xRMOSOS)
     */
    
    ReadCurrentRMSOSRegs(&current_RMS_OS,css_pin);
    
    float tmp;
    tmp = pow(Data->CurrentRMSValue_A,2) + (pow(2,15)*(current_RMS_OS.CurrentRMSOSValue_A));
//    Data->CurrentRMSValue_A = tmp > 0 ? sqrt(tmp) : 0;
    
    tmp = pow(Data->CurrentRMSValue_B,2) + pow(2,15)*(current_RMS_OS.CurrentRMSOSValue_B);
    Data->CurrentRMSValue_B = tmp > 0 ? sqrt(tmp) : 0;
    
    tmp = pow(Data->CurrentRMSValue_C,2) + pow(2,15)*(current_RMS_OS.CurrentRMSOSValue_C);
    Data->CurrentRMSValue_C = tmp > 0 ? sqrt(tmp) : 0;
}

void current_offset_write(int8_t channel, uint32_t offset){
    
    SPI_Write(ADDR_AIRMSOS, offset, true, get_css_pin(channel));
    SPI_Write(ADDR_BIRMSOS, offset, true, get_css_pin(channel));
    SPI_Write(ADDR_CIRMSOS, offset, true, get_css_pin(channel));
    
}

void voltage_offset_calibration(VoltageRMSRegs *Data, SPI_CSS_Pin css_pin){
    /*
     * Formula for Calibration of VRMS
     * xRMS = sqrt(xRMS0^2 + 2^15 × xRMOSOS)
     */
    
    ReadVoltageRMSOSRegs(&voltage_RMS_OS,css_pin);
    
    float tmp;
    tmp = pow(Data->VoltageRMSValue_A,2) + (pow(2,15)*(voltage_RMS_OS.VoltageRMSOSValue_A));
    Data->VoltageRMSValue_A = tmp > 0 ? sqrt(tmp) : 0;
    
    tmp = pow(Data->VoltageRMSValue_B,2) + pow(2,15)*(voltage_RMS_OS.VoltageRMSOSValue_B);
    Data->VoltageRMSValue_B = tmp > 0 ? sqrt(tmp) : 0;
    
    tmp = pow(Data->VoltageRMSValue_C,2) + pow(2,15)*(voltage_RMS_OS.VoltageRMSOSValue_C);
    Data->VoltageRMSValue_C = tmp > 0 ? sqrt(tmp) : 0;
}
void voltage_offset_write(int8_t channel, uint32_t offset){
    
    SPI_Write(ADDR_AVRMSOS, offset, true, get_css_pin(channel));
    SPI_Write(ADDR_BVRMSOS, offset, true, get_css_pin(channel));
    SPI_Write(ADDR_CVRMSOS, offset, true, get_css_pin(channel));
    
}



// float ct_ratio_calibration(uint16_t ratio)
// {
//     float current_transfer_func  = (float) R_BURDEN/ratio;
// // CAL_IRMS_CC in uA/code = 10^6/(CURRENT_TRANSFER_FUNCTION*ADE9000_RMS_FULL_SCALE_CODES*SQRT(2)) 
//     float cal_irms_cc = FULL_SCALE_VOLTAGE / (SQRT_2 * current_transfer_func * ADE9000_RMS_FULL_SCALE_CODES);
//     return cal_irms_cc;
// }

// float ct_ratio_calibration(uint16_t ratio)
// {
//     float current_transfer_func  = (float) R_BURDEN/ratio;
// // CAL_IRMS_CC in uA/code = 10^6/(CURRENT_TRANSFER_FUNCTION*ADE9000_RMS_FULL_SCALE_CODES*SQRT(2)) 
//     float cal_irms_cc = FULL_SCALE_VOLTAGE / (SQRT_2 * current_transfer_func * ADE9000_RMS_FULL_SCALE_CODES);
//     return cal_irms_cc;
// }

SPI_CSS_Pin get_css_pin(uint8_t channel){
    switch(channel){
            case 1: return SPI_CSS1;
            case 2: return SPI_CSS2;
            case 3: return SPI_CSS3;
            case 4: return SPI_CSS4;
            case 5: return SPI_CSS5;
            case 6: return SPI_CSS6;
            case 7: return SPI_CSS7;
            case 8: return SPI_CSS8;
            case 9: return SPI_CSS9;
            case 10: return SPI_CSS10;
            case 11: return SPI_CSS11;
            case 12: return SPI_CSS12;
            default: return SPI_CSS1;
    }
}