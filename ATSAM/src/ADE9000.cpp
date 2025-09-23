/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */

#include "ADE9000.h"

bool SECONDHEX = false;

Calibration_regs calibration_data[TOTAL_ADE9000_UNITS];
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
    uint16_t cmd_hdr = (address << 4) & 0xFFF0;
    uint8_t addressHigh = (cmd_hdr >> 8) & 0xFF;
    uint8_t addressLow = cmd_hdr & 0xFF;

    SPI_CSS_Control(css_pin, false);
    SERCOM0_SPI_Write(&addressHigh, 1);
    SERCOM0_SPI_Write(&addressLow, 1);

    if (is32Bit)
    {
        for (int i = 3; i >= 0; i--)
        {
            uint8_t dataChunk = (data >> (i * 8)) & 0xFF;
            SERCOM0_SPI_Write(&dataChunk, 1);
        }
    }
    else
    {
        uint8_t dataHigh = (data >> 8) & 0xFF;
        uint8_t dataLow = data & 0xFF;
        SERCOM0_SPI_Write(&dataHigh, 1);
        SERCOM0_SPI_Write(&dataLow, 1);
    }
    SPI_CSS_Control(css_pin, true);
}

uint32_t SPI_Read(uint32_t address, bool is32Bit, SPI_CSS_Pin css_pin)
{
    uint32_t data = 0;
    uint8_t txBuf[2];
    uint8_t rxBuf[4] = {0};

    // ADE9000 SPI read: 0x01 + 24-bit address
    txBuf[0] = (address >> 4) & 0xFF;
    txBuf[1] = (address << 4) | 0x8; // read byte 0x8 added for write it should be 0x0
    // txBuf[2] = (address >> 8) & 0xFF;
    // txBuf[3] = address & 0xFF;

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
    if (css_pin)
    {
        Data->VoltageRMSValue_A = (float)(int32_t(SPI_Read(ADDR_AVRMS, true, css_pin))) / (IC_CAL_VRMS_CC);
        Data->VoltageRMSValue_B = (float)(int32_t(SPI_Read(ADDR_BVRMS, true, css_pin))) / (IC_CAL_VRMS_CC);
        Data->VoltageRMSValue_C = (float)(int32_t(SPI_Read(ADDR_CVRMS, true, css_pin))) / (IC_CAL_VRMS_CC);
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
}

void ReadVoltageRMSOSRegs(VoltageRMSOSRegs *VRMS_OS, SPI_CSS_Pin css_pin)
{
    if (css_pin)
    {
        VRMS_OS->VoltageRMSOSValue_A = (float)(int32_t(SPI_Read(ADDR_AVRMSOS, true, css_pin))) / 1000;
        VRMS_OS->VoltageRMSOSValue_B = (float)(int32_t(SPI_Read(ADDR_BVRMSOS, true, css_pin))) / 1000;
        VRMS_OS->VoltageRMSOSValue_C = (float)(int32_t(SPI_Read(ADDR_CVRMSOS, true, css_pin))) / 1000;
    }
}

void ReadCurrentRMSOSRegs(CurrentRMSOSRegs *IRMS_OS, SPI_CSS_Pin css_pin)
{
    if (css_pin)
    {
        IRMS_OS->CurrentRMSOSValue_A = (float)(int32_t(SPI_Read(ADDR_AIRMSOS, true, css_pin))) / 1000;
        IRMS_OS->CurrentRMSOSValue_B = (float)(int32_t(SPI_Read(ADDR_BIRMSOS, true, css_pin))) / 1000;
        IRMS_OS->CurrentRMSOSValue_C = (float)(int32_t(SPI_Read(ADDR_CIRMSOS, true, css_pin))) / 1000;
    }
}

void ReadLineVoltages(VoltageRMSRegs *rmsData, LineVoltageRegs *lineData)
{
    lineData->LineVoltage_AB = rmsData->VoltageRMSValue_A * sqrtf(3.0f);
    lineData->LineVoltage_BC = rmsData->VoltageRMSValue_B * sqrtf(3.0f);
    lineData->LineVoltage_CA = rmsData->VoltageRMSValue_C * sqrtf(3.0f);
}

void ReadCurrentRMSRegs(CurrentRMSRegs *Data, SPI_CSS_Pin css_pin)
{
    if (css_pin)
    {
        Data->CurrentRMSValue_A = (float)((float)(int32_t(SPI_Read(ADDR_AIRMS, true, css_pin))) / (float)calibration_data[css_pin - 1].cal_irms_cc);
        Data->CurrentRMSValue_B = (float)((float)(int32_t(SPI_Read(ADDR_BIRMS, true, css_pin))) / (float)calibration_data[css_pin - 1].cal_irms_cc);
        Data->CurrentRMSValue_C = (float)((float)(int32_t(SPI_Read(ADDR_CIRMS, true, css_pin))) / (float)calibration_data[css_pin - 1].cal_irms_cc);
    }
}

void ReadPeriodRegsnValues(PeriodRegs *Data, VoltageRMSRegs *RMSData, SPI_CSS_Pin css_pin)
{
    if (RMSData->VoltageRMSValue_A)
    {
        Data->FrequencyValue_A = (float)(8000 * 65536) / (SPI_Read(ADDR_APERIOD, true, css_pin) + 1);
    }

    if (RMSData->VoltageRMSValue_B)
    {
        Data->FrequencyValue_B = (float)(8000 * 65536) / (SPI_Read(ADDR_BPERIOD, true, css_pin) + 1);
    }

    if (RMSData->VoltageRMSValue_C)
    {
        Data->FrequencyValue_C = (float)(8000 * 65536) / (SPI_Read(ADDR_CPERIOD, true, css_pin) + 1);
    }
}

/***************************POWER VALUES*****************************/
void ReadActivePowerRegs(ActivePowerRegs *Data, SPI_CSS_Pin css_pin)
{
    if (css_pin)
    {
        Data->ActivePowerValue_A = (float)(int32_t(SPI_Read(ADDR_AWATT, true, css_pin))) / (calibration_data[css_pin - 1].cal_power_cc * 1000.0f);
        Data->ActivePowerValue_B = (float)(int32_t(SPI_Read(ADDR_BWATT, true, css_pin))) / (calibration_data[css_pin - 1].cal_power_cc * 1000.0f);
        Data->ActivePowerValue_C = (float)(int32_t(SPI_Read(ADDR_CWATT, true, css_pin))) / (calibration_data[css_pin - 1].cal_power_cc * 1000.0f);
    }
}

void ReadReactivePowerRegs(ReactivePowerRegs *Data1, ApparentPowerRegs *Data2, PowerFactorRegs *Data3, SPI_CSS_Pin css_pin)
{
    Data1->ReactivePowerValue_A = (Data2->ApparentPowerValue_A * sin(acosf(Data3->PowerFactorValue_A))) * (((float)(int32_t(SPI_Read(ADDR_AVAR, true, css_pin))) / calibration_data[css_pin - 1].cal_power_cc) < 0 ? -1 : 1);
    Data1->ReactivePowerValue_B = (Data2->ApparentPowerValue_B * sin(acosf(Data3->PowerFactorValue_B))) * (((float)(int32_t(SPI_Read(ADDR_BVAR, true, css_pin))) / calibration_data[css_pin - 1].cal_power_cc) < 0 ? -1 : 1);
    Data1->ReactivePowerValue_C = (Data2->ApparentPowerValue_C * sin(acosf(Data3->PowerFactorValue_C))) * (((float)(int32_t(SPI_Read(ADDR_CVAR, true, css_pin))) / calibration_data[css_pin - 1].cal_power_cc) < 0 ? -1 : 1);
}

void ReadApparentPowerRegs(ApparentPowerRegs *Data, SPI_CSS_Pin css_pin)
{
    if (css_pin)
    {
        Data->ApparentPowerValue_A = (float)(int32_t(SPI_Read(ADDR_AVA, true, css_pin))) / (calibration_data[css_pin - 1].cal_power_cc * 1000.0f);
        Data->ApparentPowerValue_B = (float)(int32_t(SPI_Read(ADDR_BVA, true, css_pin))) / (calibration_data[css_pin - 1].cal_power_cc * 1000.0f);
        Data->ApparentPowerValue_C = (float)(int32_t(SPI_Read(ADDR_CVA, true, css_pin))) / (calibration_data[css_pin - 1].cal_power_cc * 1000.0f);
    }
}

void ReadPowerFactorRegsnValues(PowerFactorRegs *Data, VoltageRMSRegs *rmsData, CurrentRMSRegs *currentData, SPI_CSS_Pin css_pin)
{
    uint32_t rawValue;
    int32_t tempReg;

    if (rmsData->VoltageRMSValue_A > NOMINAL_INPUT_VOLTAGE && currentData->CurrentRMSValue_A > 0.1)
    {
        rawValue = SPI_Read(ADDR_APF, true, css_pin);
        tempReg = *(int32_t *)&(rawValue);
        Data->PowerFactorValue_A = (float)tempReg / 134217728.0f;
    }
    else
    {
        Data->PowerFactorValue_A = 1.0f;
    }

    if (rmsData->VoltageRMSValue_B > NOMINAL_INPUT_VOLTAGE && currentData->CurrentRMSValue_B > 0.1)
    {
        rawValue = SPI_Read(ADDR_BPF, true, css_pin);
        tempReg = *(int32_t *)&rawValue;
        Data->PowerFactorValue_B = (float)tempReg / 134217728.0f;
    }
    else
    {
        Data->PowerFactorValue_B = 1.0f;
    }

    if (rmsData->VoltageRMSValue_C > NOMINAL_INPUT_VOLTAGE && currentData->CurrentRMSValue_C > 0.1)
    {
        rawValue = SPI_Read(ADDR_CPF, true, css_pin);
        tempReg = *(int32_t *)&rawValue;
        Data->PowerFactorValue_C = (float)tempReg / 134217728.0f;
    }
    else
    {
        Data->PowerFactorValue_C = 1.0f;
    }
}

/***************************ENERGY VALUES*****************************/
const double k = (8192.0) / ((double)ADE9000_EGY_TIME + 1.0);
void ReadEnergyRegs(EnergyRegs *Data1, ActivePowerRegs *Data2, CurrentRMSRegs *CurrentData, SPI_CSS_Pin css_pin)
{
    static uint8_t discardInitialReadingsCount = 0;
    const uint8_t INITIAL_READINGS_TO_DISCARD = 10;
    uint32_t status0;
    uint32_t rawValue;
    status0 = SPI_Read(ADDR_STATUS0, true, css_pin);

    if (status0 & 0x01)
    {
        if (discardInitialReadingsCount < INITIAL_READINGS_TO_DISCARD)
        {
            discardInitialReadingsCount++;
            return;
        }
        if (1)
        {
            ////////////////////////////////////////////////////////////////
//            uint32_t reg_hi = SPI_Read(ADDR_AFWATTHR_HI, true, css_pin);
//            reg_hi = (reg_hi & 0x80000000) ? -((int32_t)(~reg_hi + 1)) : (int32_t)reg_hi;
//            uint32_t reg_lo = SPI_Read(ADDR_AFWATTHR_LO, true, css_pin);
//            
//            // Combine into 64 bits (only lower 45 bits are valid)
//            uint64_t raw45 = ((uint64_t)reg_hi << 32) | reg_lo;
//            raw45 &= ((1ULL << 45) - 1);  // mask to 45 bits
//
//            // Sign extend 45-bit to 64-bit signed integer
//            int64_t signed45;
//            if (raw45 & (1ULL << 44)) {       // if sign bit (bit 44) is set
//                signed45 = (int64_t)(raw45 | (~((1ULL << 45) - 1)));
//            } else {
//                signed45 = (int64_t)raw45;
//            }
            //////////////////////////////////////////////////////////////

//            Data1->ActiveEnergyReg_A = signed45;
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

//        if(Data2->ActivePowerValue_A == 0.0)
//        {
        
//        \frac{5\cdot220}{\frac{\left(\frac{0.004}{100}\right)}{0.707}\cdot0.311\cdot20694066\cdot2^{-13}\cdot3600\cdot8000}
//            Data1->PActiveEnergyValue_kWh_A = Data1->ActiveEnergyReg_A * (float)(2.06296 * 8000 * 3600) / 1000000 / 1000;
        if(css_pin == 4){
            
             
//            Data1->PActiveEnergyValue_kWh_A = (double)((Data1->ActiveEnergyReg_A * Data2 -> ActivePowerValue_A * k)/3600.0f);
    
//            \frac{\left(3427747\cdot0.55\cdot\left(\frac{8192}{488281+1}\right)\right)}{3600000}
//        Data1->PActiveEnergyValue_kWh_A = Data1->ActiveEnergyReg_A * (float)(2.56318184e-9);
//            \frac{\left(60\cdot\left(\frac{8192}{488281+1}\right)\right)}{3600000}
//            \frac{\left(60\cdot\left(\frac{8192}{488281+1}\right)\right)}{3600}
//            \frac{\left(\left(\frac{8192}{488281+1}\right)\right)}{3600}
            
//      Data1->PActiveEnergyValue_kWh_A = (Data1->ActiveEnergyReg_A * Data2->ActivePowerValue_A * (k))/(3600);
//        Data1->PActiveEnergyValue_kWh_A = (Data1->ActiveEnergyReg_A * 8.59/10000)/1000; // /1000 
        
//        \frac{5\cdot220}{\frac{\left(\frac{0.004}{100}\right)}{0.707}\cdot0.311\cdot20694066\cdot2^{-13}\cdot3600\cdot488281}
//        Data1->PActiveEnergyValue_kWh_A = (Data1->ActiveEnergyReg_A * 1.40787308)/100000;
//        Data1->PActiveEnergyValue_kWh_A = (Data1->ActiveEnergyReg_A * (float)(2.56318184 / 1000000000));
//            \frac{5\cdot220}{\frac{\left(\frac{0.004}{100}\right)}{0.707}\cdot0.311\cdot20694066\cdot2^{-13}\cdot3600\cdot8000}
        Data1->PActiveEnergyValue_kWh_A = (Data1->ActiveEnergyReg_A * (float)(1.40787308 / (100000 * 1000)));
        //        Data1->PActiveEnergyValue_kWh_A = Data1->ActiveEnergyReg_A * (float)(8.59e-4);    
        }    

//        3427747 | 2934.0476
//        3452007 | 2944.4348
//        3467017 | 2965.2742
//        3484477 | 2978.1677
//        3498584 | 2993.1658
//        }
        if (Data2->ActivePowerValue_B == 0.0)
        {
            Data1->PActiveEnergyValue_kWh_B = Data1->ActiveEnergyReg_B * (float)(2.06296 * 8000 * 3600) / 1000000 / 1000;
        }
        if (Data2->ActivePowerValue_C == 0.0)
        {
            Data1->PActiveEnergyValue_kWh_C = Data1->ActiveEnergyReg_C * (float)(2.06296 * 8000 * 3600) / 1000000 / 1000;
        }
        
        // Data1->ActiveEnergyValue_kWh_B = Data1->ActiveEnergyReg_B * (float)(CAL_ENERGY_CC * 8000 * 3600) / 1000000 / 1000;
        // Data1->ActiveEnergyValue_kWh_C = Data1->ActiveEnergyReg_C * (float)(CAL_ENERGY_CC * 8000 * 3600) / 1000000 / 1000;
        //         Data1->ReactiveEnergyValue_kWh_A = Data1->ReactiveEnergyReg_A * (float) (calibration_data.cal_energy_cc * 8000 * 3600) / 1000000 / 1000;
        //         Data1->ReactiveEnergyValue_kWh_B = Data1->ReactiveEnergyReg_B * (float) (calibration_data.cal_energy_cc * 8000 * 3600) / 1000000 / 1000;
        //         Data1->ReactiveEnergyValue_kWh_C = Data1->ReactiveEnergyReg_C * (float) (calibration_data.cal_energy_cc * 8000 * 3600) / 1000000 / 1000;

        // if (Data2->ActivePowerValue_A < 0)
        // {
        //     Data1->NActiveEnergyValue_kWh_A = Data2->ActivePowerValue_A * timeFactor;
        //     Data1->PActiveEnergyValue_kWh_A = 0;
        // }
        // else
        // {
        //     Data1->PActiveEnergyValue_kWh_A = Data2->ActivePowerValue_A * timeFactor;
        //     Data1->NActiveEnergyValue_kWh_A = 0;
        // }

        // const float timeFactor = 1.0 / 3600.0;
        // if (Data2->ActivePowerValue_A < 0)
        // {
        //     Data1->NActiveEnergyValue_kWh_A = Data2->ActivePowerValue_A * timeFactor;
        //     Data1->PActiveEnergyValue_kWh_A = 0;
        // }
        // else
        // {
        //     Data1->PActiveEnergyValue_kWh_A = Data2->ActivePowerValue_A * timeFactor;
        //     Data1->NActiveEnergyValue_kWh_A = 0;
        // }
        // if (Data2->ActivePowerValue_B < 0)
        // {
        //     Data1->NActiveEnergyValue_kWh_B = Data2->ActivePowerValue_B * timeFactor;
        //     Data1->PActiveEnergyValue_kWh_B = 0;
        // }
        // else
        // {
        //     Data1->PActiveEnergyValue_kWh_B = Data2->ActivePowerValue_B * timeFactor;
        //     Data1->NActiveEnergyValue_kWh_B = 0;
        // }
        // if (Data2->ActivePowerValue_C < 0)
        // {
        //     Data1->NActiveEnergyValue_kWh_C = Data2->ActivePowerValue_C * timeFactor;
        //     Data1->PActiveEnergyValue_kWh_C = 0;
        // }
        // else
        // {
        //     Data1->PActiveEnergyValue_kWh_C = Data2->ActivePowerValue_C * timeFactor;
        //     Data1->NActiveEnergyValue_kWh_C = 0;
        // }

        SPI_Write(ADDR_STATUS0, (status0 | 0x01), true, css_pin);
    }
    else
    {
        //        Data1->ActiveEnergyReg_A = 0;
        //        Data1->ActiveEnergyReg_B = 0;
        //        Data1->ActiveEnergyReg_C = 0;
        //        Data1->PActiveEnergyValue_kWh_A = 0;
        //        Data1->PActiveEnergyValue_kWh_B = 0;
        //        Data1->PActiveEnergyValue_kWh_C = 0;
        //        Data1->NActiveEnergyValue_kWh_A = 0;
        //        Data1->NActiveEnergyValue_kWh_B = 0;
        //        Data1->NActiveEnergyValue_kWh_C = 0;
        //
        //        Data1->ReactiveEnergyReg_A = 0;
        //        Data1->ReactiveEnergyReg_B = 0;
        //        Data1->ReactiveEnergyReg_C = 0;
        //        Data1->ReactiveEnergyValue_kWh_A = 0;
        //        Data1->ReactiveEnergyValue_kWh_B = 0;
        //        Data1->ReactiveEnergyValue_kWh_C = 0;
    }
}
//

///*Energy Accumulation Settings*/
// #define ADE9000_EP_CFG 0x0011         /*Enable energy accumulation, 8ksps, latch after EGYRDY*/
///* Set to ~10s accumulation window for testing */
// #define ADE9000_EGY_TIME 0x13880      /* 80,000 samples ? 10 seconds at 8kSPS */

// #define ADE9000_STATUS0_EGYRDY (1 << 0)

// void ReadEnergyRegs(EnergyRegs *Data1, ActivePowerRegs *Data2, SPI_CSS_Pin css_pin)
//{
//     static uint8_t discardInitialReadingsCount = 0;
//     const uint8_t INITIAL_READINGS_TO_DISCARD = 10;
//     uint32_t status0;
//     uint32_t rawValue;
//
//     // Read STATUS0 to check if energy accumulation is ready
//     status0 = SPI_Read(ADDR_STATUS0, true, css_pin);
//
//     if ((status0 & (ADE9000_STATUS0_EGYRDY)) != 0)
//     { // EGYRDY flag set
//         if (discardInitialReadingsCount < INITIAL_READINGS_TO_DISCARD)
//         {
//             discardInitialReadingsCount++;
//             return; // Skip initial unstable readings
//         }
//
//         if (SECONDHEX)
//         {
//             rawValue = SPI_Read(ADDR_AFWATTHR_HI, true, css_pin);
//             Data1->ActiveEnergyReg_A = (rawValue & 0x80000000) ? -((int32_t)(~rawValue + 1)) : (int32_t)rawValue;
//             rawValue = SPI_Read(ADDR_BFWATTHR_HI, true, css_pin);
//             Data1->ActiveEnergyReg_B = (rawValue & 0x80000000) ? -((int32_t)(~rawValue + 1)) : (int32_t)rawValue;
//             rawValue = SPI_Read(ADDR_CFWATTHR_HI, true, css_pin);
//             Data1->ActiveEnergyReg_C = (rawValue & 0x80000000) ? -((int32_t)(~rawValue + 1)) : (int32_t)rawValue;
//
//             rawValue = SPI_Read(ADDR_AFVARHR_HI, true, css_pin);
//             Data1->ReactiveEnergyReg_A = (rawValue & 0x80000000) ? -((int32_t)(~rawValue + 1)) : (int32_t)rawValue;
//             rawValue = SPI_Read(ADDR_BFVARHR_HI, true, css_pin);
//             Data1->ReactiveEnergyReg_B = (rawValue & 0x80000000) ? -((int32_t)(~rawValue + 1)) : (int32_t)rawValue;
//             rawValue = SPI_Read(ADDR_CFVARHR_HI, true, css_pin);
//             Data1->ReactiveEnergyReg_C = (rawValue & 0x80000000) ? -((int32_t)(~rawValue + 1)) : (int32_t)rawValue;
//         }
//         else
//         {
//             //            Data1->ActiveEnergyReg_A = SPI_Read(ADDR_AFWATTHR_HI, true, css_pin);
//
//             //            uint64_t energyA;
//             //
//             //            uint32_t hi = SPI_Read(ADDR_AFWATTHR_HI, true, css_pin);
//             //            uint16_t lo = SPI_Read(ADDR_AFWATTHR_LO, false, css_pin);
//             //
//             //            energyA = ((uint64_t)hi << 13) | (lo & 0x1FFF);
//             //
//             //            Data1->ActiveEnergyReg_A = energyA;
//
//             //            uint32_t hi = SPI_Read(ADDR_AFWATTHR_HI, true, css_pin);
//             //            uint16_t lo = SPI_Read(ADDR_AFWATTHR_LO, false, css_pin);
//             //
//             //            Data1->ActiveEnergyReg_A = ((uint64_t)hi << 13) | (lo & 0x1FFF);
//
//             uint32_t high = SPI_Read(ADDR_AFWATTHR_HI, true, css_pin);
//             uint32_t low = SPI_Read(ADDR_AFWATTHR_LO, true, css_pin);
//             //            return ((uint64_t)high << 13) | (low & 0x1FFF);
//
//             const double k = 8192.0 / ((double)ADE9000_EGY_TIME + 1.0);
//
//             uint64_t energyCodes;
//
//             // Phase A
//             energyCodes = ((uint64_t)high << 13) | (low & 0x1FFF);
//             Data1->PActiveEnergyValue_kWh_A = (float)(energyCodes * calibration_data.cal_power_cc * k) / 3600;
//             // clear flag
//             SPI_Write(ADDR_STATUS0, ADE9000_STATUS0_EGYRDY, true, css_pin);
//
//             Data1->ActiveEnergyReg_B = SPI_Read(ADDR_BFWATTHR_HI, true, css_pin);
//             Data1->ActiveEnergyReg_C = SPI_Read(ADDR_CFWATTHR_HI, true, css_pin);
//
//             Data1->ReactiveEnergyReg_A = SPI_Read(ADDR_AFVARHR_HI, true, css_pin);
//             Data1->ReactiveEnergyReg_B = SPI_Read(ADDR_BFVARHR_HI, true, css_pin);
//             Data1->ReactiveEnergyReg_C = SPI_Read(ADDR_CFVARHR_HI, true, css_pin);
//         }
//
//         // --- Apply scaling to convert to kWh ---
//         // ADE9000_EGY_TIME is the number of samples per accumulation window
//         // CAL_ENERGY_CC must be set from ADE9000 datasheet or calibration
//         //        Data1->PActiveEnergyValue_kWh_A =
//         //            Data1->ActiveEnergyReg_A * (float)(CAL_ENERGY_CC * ADE9000_EGY_TIME * 3600.0) / 1e9;
//         //        Data1->PActiveEnergyValue_kWh_B =
//         //            Data1->ActiveEnergyReg_B * (float)(CAL_ENERGY_CC * ADE9000_EGY_TIME * 3600.0) / 1e9;
//         //        Data1->PActiveEnergyValue_kWh_C =
//         //            Data1->ActiveEnergyReg_C * (float)(CAL_ENERGY_CC * ADE9000_EGY_TIME * 3600.0) / 1e9;
//         //
//         //        Data1->ReactiveEnergyValue_kWh_A =
//         //            Data1->ReactiveEnergyReg_A * (float)(CAL_ENERGY_CC * ADE9000_EGY_TIME * 3600.0) / 1e9;
//         //        Data1->ReactiveEnergyValue_kWh_B =
//         //            Data1->ReactiveEnergyReg_B * (float)(CAL_ENERGY_CC * ADE9000_EGY_TIME * 3600.0) / 1e9;
//         //        Data1->ReactiveEnergyValue_kWh_C =
//         //            Data1->ReactiveEnergyReg_C * (float)(CAL_ENERGY_CC * ADE9000_EGY_TIME * 3600.0) / 1e9;
//
//         // --- Positive/Negative energy split based on instantaneous power ---
//         const float timeFactor = 1.0 / 3600.0; // 1 second -> hour conversion
//         if (Data2->ActivePowerValue_A < 0)
//         {
//             Data1->NActiveEnergyValue_kWh_A += Data2->ActivePowerValue_A * timeFactor;
//         }
//         else
//         {
//             Data1->PActiveEnergyValue_kWh_A += Data2->ActivePowerValue_A * timeFactor;
//         }
//
//         if (Data2->ActivePowerValue_B < 0)
//         {
//             Data1->NActiveEnergyValue_kWh_B += Data2->ActivePowerValue_B * timeFactor;
//         }
//         else
//         {
//             Data1->PActiveEnergyValue_kWh_B += Data2->ActivePowerValue_B * timeFactor;
//         }
//
//         if (Data2->ActivePowerValue_C < 0)
//         {
//             Data1->NActiveEnergyValue_kWh_C += Data2->ActivePowerValue_C * timeFactor;
//         }
//         else
//         {
//             Data1->PActiveEnergyValue_kWh_C += Data2->ActivePowerValue_C * timeFactor;
//         }
//
//         // Clear EGYRDY flag
//         SPI_Write(ADDR_STATUS0, (status0 | 0x01), true, css_pin);
//     }
//     // IMPORTANT: else branch removed (no reset to zero anymore)
// }

/***************************THRESHOLD VALUES*****************************/
void ReadVoltageTHDRegsnValues(VoltageTHDRegs *Data, SPI_CSS_Pin css_pin)
{
    uint32_t rawValue;
    int32_t tempReg;

    rawValue = SPI_Read(ADDR_AVTHD, true, css_pin);
    tempReg = *(int32_t *)&rawValue;
    Data->VoltageTHDValue_A = (float)tempReg * 100 / 134217728.0f; // Multiplied by 2^-27
//    Data->VoltageTHDValue_A = (float)((float)tempReg * 7.4505806 / 1000000000)*100; // Multiply for 100 as it will be show in percentage

    rawValue = SPI_Read(ADDR_BVTHD, true, css_pin);
    tempReg = *(int32_t *)&rawValue;
    Data->VoltageTHDValue_B = (float)tempReg * 100 / 134217728.0f;

    rawValue = SPI_Read(ADDR_CVTHD, true, css_pin);
    tempReg = *(int32_t *)&rawValue;
    Data->VoltageTHDValue_C = (float)tempReg * 100 / 134217728.0f;
}

void ReadCurrentTHDRegsnValues(CurrentTHDRegs *Data, SPI_CSS_Pin css_pin)
{
    uint32_t rawValue;
    int32_t tempReg;

    rawValue = SPI_Read(ADDR_AITHD, true, css_pin);
    tempReg = *(int32_t *)&rawValue;
    Data->CurrentTHDValue_A = (float)tempReg * 100 / 134217728.0f;
    
//    Data->CurrentTHDValue_A = (float)((float)tempReg * 7.4505806 / 1000000000)*100; // Multiply for 100 as it will be show in percentage
    

    rawValue = SPI_Read(ADDR_BITHD, true, css_pin);
    tempReg = *(int32_t *)&rawValue;
    Data->CurrentTHDValue_B = (float)tempReg * 100 / 134217728.0f;

    rawValue = SPI_Read(ADDR_CITHD, true, css_pin);
    tempReg = *(int32_t *)&rawValue;
    Data->CurrentTHDValue_C = (float)tempReg * 100 / 134217728.0f;
}

void ReadAngleRegsnValues(AngleRegs *Data, SPI_CSS_Pin css_pin)
{
    // float mulConstant;

    // if ((SPI_Read(ADDR_ACCMODE, false, css_pin)) & 0x0100)
    // {
    //     mulConstant = 0.02109375; // Multiplier constant for 60Hz system
    // }
    // else
    // {
    //     mulConstant = 0.017578125; // Multiplier constant for 50Hz system
    // }

    // Data->AngleValue_VA_VB = (SPI_Read(ADDR_ANGL_VA_VB, true, css_pin)) * 0.017578125;          // mulConstant
    // Data->AngleValue_VB_VC = (SPI_Read(ADDR_ANGL_VB_VC, true, css_pin)) * 0.017578125;          // mulConstant
    // Data->AngleValue_VA_VC = (SPI_Read(ADDR_ANGL_VA_VC, true, css_pin)) * 0.017578125;          // mulConstant
    
//    int16_t angle_value_register = (int32_t(SPI_Read(ADDR_ANGL_VA_IA, false, css_pin)));
    Data->AngleValue_VA_IA = abs(((int32_t(SPI_Read(ADDR_ANGL_VA_IA, false, css_pin))) * 0.017578125) - 360); // in Degrees
//    Data->AngleValue_VA_IA = abs(((int32_t(SPI_Read(ADDR_ANGL_VA_IA, false, css_pin))) * 0.017578125) - 360) * (( 22/7 ) / 180); // in Radians

    Data->AngleValue_VB_IB = (int32_t(SPI_Read(ADDR_ANGL_VB_IB, true, css_pin))) * 0.017578125; // mulConstant
    Data->AngleValue_VC_IC = (int32_t(SPI_Read(ADDR_ANGL_VC_IC, true, css_pin))) * 0.017578125; // mulConstant

    // Data->AngleValue_IA_IB = (int32_t(SPI_Read(ADDR_ANGL_IA_IB, true, css_pin))) * 0.017578125; // mulConstant
    // Data->AngleValue_IB_IC = (int32_t(SPI_Read(ADDR_ANGL_IB_IC, true, css_pin))) * 0.017578125; // mulConstant
    // Data->AngleValue_IA_IC = (int32_t(SPI_Read(ADDR_ANGL_IA_IC, true, css_pin))) * 0.017578125; // mulConstant
}

/***************************TEMPERATURE VALUES*****************************/
void ReadTempRegnValue(TemperatureRegs *Data, SPI_CSS_Pin css_pin)
{
    uint32_t trim;
    uint16_t gain;
    uint16_t offset;

    trim = SPI_Read(ADDR_TEMP_TRIM, true, css_pin);
    gain = (trim & 0xFFFF);
    offset = ((trim >> 16) & 0xFFFF);
    Data->Temperature = (float)(offset >> 5) - ((float)(SPI_Read(ADDR_TEMP_RSLT, false, css_pin)) * (float)gain / 65536.0f);
}

// *****************************************************************************
// *****************************************************************************
// Section: ADE9000 Calibration Functions
// *****************************************************************************
// *****************************************************************************
void ct_ratio_calibration(uint16_t ratio, uint8_t channel, CurrentRMSRegs Data)
{
    /****************************************************************************************************/
    /* Calculation of xIGAIN */
    /****************************************************************************************************/

    float current_transfer_func = (float)(R_BURDEN / (ratio)); //

    //    Current Conversion Constant = (I nominal * 10^6)/(Ifsp*full_scale_rms_codes);

    calibration_data[channel].cal_irms_cc = (SQRT_2 * current_transfer_func * ADE9000_RMS_FULL_SCALE_CODES); // I_FSP
    float I_FSP;

    I_FSP = (current_transfer_func / 0.707); // I_FSP

    //    calibration_data.cal_irms_cc = (NOMINAL_INPUT_CURRENT)/(I_FSP * ADE9000_RMS_FULL_SCALE_CODES); // Current Conversion Constant
    calibration_data[channel].cal_power_cc = (2 * current_transfer_func * VOLTAGE_TRANSFER_FUNCTION * ADE9000_WATT_FULL_SCALE_CODES);
    calibration_data[channel].cal_energy_cc = (2 * current_transfer_func * VOLTAGE_TRANSFER_FUNCTION * ADE9000_WATT_FULL_SCALE_CODES);
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

//    uint32_t A_Active_Energy = SPI_Read(ADDR_AWATTHR_HI, true, get_css_pin(channel));
//    uint32_t B_Active_Energy = SPI_Read(ADDR_BWATTHR_HI, true, get_css_pin(channel));
//    uint32_t C_Active_Energy = SPI_Read(ADDR_CWATTHR_HI, true, get_css_pin(channel));
    //
    //    float A_Active_Energy_Expected = (2 * current_transfer_func * 10 * NOMINAL_VOLTAGE * ACCUMULATION_TIME * 8000 * pow(2,-13));
    //    float B_Active_Energy_Expected = (2 * current_transfer_func * 10 * NOMINAL_VOLTAGE * ACCUMULATION_TIME * 8000 * pow(2,-13));
    //    float C_Active_Energy_Expected = (2 * current_transfer_func * 10 * NOMINAL_VOLTAGE * ACCUMULATION_TIME * 8000 * pow(2,-13));
    //
    //    float AP_GAIN = (float)((A_Active_Energy_Expected - A_Active_Energy)/ (A_Active_Energy)) * pow(2,27);
    //    A_Active_Energy = 4294967295;
//    float A_Active_Energy_Expected = 355587.79;
//    float AP_GAIN = (float)(((A_Active_Energy_Expected - A_Active_Energy) / (A_Active_Energy)) * pow(2, 27));
    //    float BP_GAIN = (float)((B_Active_Energy_Expected - B_Active_Energy)/ (B_Active_Energy)) * pow(2,27);
    //    float CP_GAIN = (float)((C_Active_Energy_Expected - C_Active_Energy)/ (C_Active_Energy)) * pow(2,27);

    //    SPI_Write(ADDR_APGAIN, AP_GAIN, true, get_css_pin(channel));
    //    SPI_Write(ADDR_APGAIN, (uint32_t)AP_GAIN, true, get_css_pin(channel));

    //    SPI_Write(ADDR_BPGAIN, BP_GAIN, true, get_css_pin(channel));
    //    SPI_Write(ADDR_CPGAIN, CP_GAIN, true, get_css_pin(channel));

    /****************************************************************************************************/
}

void current_offset_calibration(CurrentRMSRegs *Data, SPI_CSS_Pin css_pin)
{
    /*
     * Formula for Calibration of IRMS
     * xRMS = sqrt(xRMS0^2 + xRMOSOS)
     */

    ReadCurrentRMSOSRegs(&current_RMS_OS, css_pin);
    //    channelConfig[j-1].setIOffset
    float tmp;
    //    tmp = pow(Data->CurrentRMSValue_A,2) + (pow(2,15)*(current_RMS_OS.CurrentRMSOSValue_A));
    //    tmp = pow(Data->CurrentRMSValue_A,2) + (pow(2,15)*(channelConfig[css_pin-1].setIOffset / 1000));
    tmp = pow(Data->CurrentRMSValue_A, 2) + (channelConfig[css_pin - 1].setIOffset);
    Data->CurrentRMSValue_A = tmp > 0 ? sqrt(tmp) : 0;

    tmp = pow(Data->CurrentRMSValue_B, 2) + (channelConfig[css_pin - 1].setIOffset);
    Data->CurrentRMSValue_B = tmp > 0 ? sqrt(tmp) : 0;

    tmp = pow(Data->CurrentRMSValue_C, 2) + (channelConfig[css_pin - 1].setIOffset);
    Data->CurrentRMSValue_C = tmp > 0 ? sqrt(tmp) : 0;
}

void current_offset_write(int8_t channel, int32_t offset)
{

    SPI_Write(ADDR_AIRMSOS, offset, true, get_css_pin(channel));
    SPI_Write(ADDR_BIRMSOS, offset, true, get_css_pin(channel));
    SPI_Write(ADDR_CIRMSOS, offset, true, get_css_pin(channel));

    //    current_RMS_OS.CurrentRMSOSValue_A = offset;
}

void voltage_offset_calibration(VoltageRMSRegs *Data, SPI_CSS_Pin css_pin)
{
    /*
     * Formula for Calibration of VRMS
     * xRMS = sqrt(xRMS0^2 + 2^15 � xRMOSOS)
     */

    ReadVoltageRMSOSRegs(&voltage_RMS_OS, css_pin);

    float tmp;
    tmp = pow(Data->VoltageRMSValue_A, 2) + (pow(2, 15) * (voltage_RMS_OS.VoltageRMSOSValue_A));
    Data->VoltageRMSValue_A = tmp > 0 ? sqrt(tmp) : 0;

    tmp = pow(Data->VoltageRMSValue_B, 2) + pow(2, 15) * (voltage_RMS_OS.VoltageRMSOSValue_B);
    Data->VoltageRMSValue_B = tmp > 0 ? sqrt(tmp) : 0;

    tmp = pow(Data->VoltageRMSValue_C, 2) + pow(2, 15) * (voltage_RMS_OS.VoltageRMSOSValue_C);
    Data->VoltageRMSValue_C = tmp > 0 ? sqrt(tmp) : 0;
}
void voltage_offset_write(int8_t channel, int32_t offset)
{

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

SPI_CSS_Pin get_css_pin(uint8_t channel)
{
    switch (channel)
    {
    case 1:
        return SPI_CSS1;
    case 2:
        return SPI_CSS2;
    case 3:
        return SPI_CSS3;
    case 4:
        return SPI_CSS4;
    case 5:
        return SPI_CSS5;
    case 6:
        return SPI_CSS6;
    case 7:
        return SPI_CSS7;
    case 8:
        return SPI_CSS8;
    case 9:
        return SPI_CSS9;
    case 10:
        return SPI_CSS10;
    case 11:
        return SPI_CSS11;
    case 12:
        return SPI_CSS12;
    default:
        return SPI_CSS1;
    }
}