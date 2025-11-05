/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */

#include "main.h"
#include "ADE9000.h"

#define CONST_2_27                  134217728.0f // 2^27
#define MIN_CURRENT_CAP             2.99f
#define RMS_VOLTAGE_LIMIT 			50        // RMS Voltage limit on all phases

bool SECONDHEX = false;

Calibration_regs calibration_data[TOTAL_ADE9000_UNITS];
VoltageRMSOSRegs voltage_RMS_OS;
CurrentRMSOSRegs current_RMS_OS;

// *****************************************************************************
// *****************************************************************************
// Section: ADE9000 SPI Functions
// *****************************************************************************
// *****************************************************************************

SPI_CSS_Pin get_css_pin(uint8_t channel)
{
    switch (channel)
    {
        case 0:
            return SPI_CSS1;
        case 1:
            return SPI_CSS2;
        case 2:
            return SPI_CSS3;
        case 3:
            return SPI_CSS4;
        case 4:
            return SPI_CSS5;
        case 5:
            return SPI_CSS6;
        case 6:
            return SPI_CSS7;
        case 7:
            return SPI_CSS8;
        case 8:
            return SPI_CSS9;
        case 9:
            return SPI_CSS10;
        case 10:
            return SPI_CSS11;
        case 11:
            return SPI_CSS12;
        default:
            return SPI_CSS1;
    }
}

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

static const uint16_t phase_offset_table[] = {
    [VOLTAGE_PARAMETER]    = ADE9000_OFFSET_RMS_POWER,
    [CURRENT_PARAMETER]    = ADE9000_OFFSET_RMS_POWER,
    [POWER_PARAMETER]      = ADE9000_OFFSET_RMS_POWER,
    [ENERGY_PARAMETER]     = ADE9000_OFFSET_ENERGY,
    [PERIOD_PARAMETER]     = ADE9000_OFFSET_PERIOD,
    [WAVEFORM_PARAMETER]   = ADE9000_OFFSET_WAVEFORM,
    [CALIBRATION_PARAMETER]= ADE9000_OFFSET_CALIBRATION
};
/**
 * @brief Returns register address for given base address, phase, and parameter type.
 *
 * @param base_addr Base register address for Phase A (e.g., ADDR_AIRMS or ADDR_AVGAIN)
 * @param phase     Phase (PHASE_A, PHASE_B, PHASE_C)
 * @param parameter Parameter type (determines phase offset rule)
 * @return uint32_t Final register address for the given phase and parameter type.
 */
uint32_t ade9000_get_phase_register(uint32_t base_addr, ADE9000_phase phase, ADE9000_parameter_type parameter)
{
    if (phase >= TOTAL_PHASES)
        return base_addr;           // fallback to base address if invalid phase

    return base_addr + (phase * phase_offset_table[parameter]);
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

    // Set Gain registers to zero, calibration should be performed later
    SPI_Write(ade9000_get_phase_register(ADDR_AVGAIN, PHASE_A, CALIBRATION_PARAMETER), 0x00000000, true, css_pin);
    SPI_Write(ade9000_get_phase_register(ADDR_AVGAIN, PHASE_B, CALIBRATION_PARAMETER), 0x00000000, true, css_pin);
    SPI_Write(ade9000_get_phase_register(ADDR_AVGAIN, PHASE_C, CALIBRATION_PARAMETER), 0x00000000, true, css_pin);
    // SPI_Write(ADDR_BVGAIN, 0x00000000, true, css_pin);
    // SPI_Write(ADDR_CVGAIN, 0x00000000, true, css_pin);
    
    SPI_Write(ade9000_get_phase_register(ADDR_AIGAIN, PHASE_A, CALIBRATION_PARAMETER), 0x00000000, true, css_pin);
    SPI_Write(ade9000_get_phase_register(ADDR_AIGAIN, PHASE_B, CALIBRATION_PARAMETER), 0x00000000, true, css_pin);
    SPI_Write(ade9000_get_phase_register(ADDR_AIGAIN, PHASE_C, CALIBRATION_PARAMETER), 0x00000000, true, css_pin);
    // SPI_Write(ADDR_AIGAIN, 0x00000000, true, css_pin);
    // SPI_Write(ADDR_BIGAIN, 0x00000000, true, css_pin);
    // SPI_Write(ADDR_CIGAIN, 0x00000000, true, css_pin);
    
    SPI_Write(ade9000_get_phase_register(ADDR_APGAIN, PHASE_A, CALIBRATION_PARAMETER), 0x00000000, true, css_pin);
    SPI_Write(ade9000_get_phase_register(ADDR_APGAIN, PHASE_B, CALIBRATION_PARAMETER), 0x00000000, true, css_pin);
    SPI_Write(ade9000_get_phase_register(ADDR_APGAIN, PHASE_C, CALIBRATION_PARAMETER), 0x00000000, true, css_pin);
    // SPI_Write(ADDR_APGAIN, 0x00000000, true, css_pin);
    // SPI_Write(ADDR_BPGAIN, 0x00000000, true, css_pin);
    // SPI_Write(ADDR_CPGAIN, 0x00000000, true, css_pin);
    
    SPI_Write(ade9000_get_phase_register(ADDR_APHCAL0, PHASE_A, CALIBRATION_PARAMETER), 0x00000000, true, css_pin);
    SPI_Write(ade9000_get_phase_register(ADDR_APHCAL0, PHASE_B, CALIBRATION_PARAMETER), 0x00000000, true, css_pin);
    SPI_Write(ade9000_get_phase_register(ADDR_APHCAL0, PHASE_C, CALIBRATION_PARAMETER), 0x00000000, true, css_pin);
    // SPI_Write(ADDR_APHCAL0, 0xFF3593B3, true, css_pin);     // supposed to be calibrated instead of hardcoded, also check MTEN bit in CONFIG0 register
    // SPI_Write(ADDR_BPHCAL0, 0xFF3593B3, true, css_pin);     // supposed to be calibrated instead of hardcoded, also check MTEN bit in CONFIG0 register
    // SPI_Write(ADDR_CPHCAL0, 0xFF3593B3, true, css_pin);     // supposed to be calibrated instead of hardcoded, also check MTEN bit in CONFIG0 register

    SPI_Write(ADDR_APP_NL_LVL, 0x00002B1C, true, css_pin);
    SPI_Write(ADDR_ACT_NL_LVL,   0x00002B1C, true, css_pin);
    SPI_Write(ADDR_REACT_NL_LVL, 0x00000000, true, css_pin);

    SPI_Write(ADDR_EP_CFG, ADE9000_EP_CFG, false, css_pin);
    SPI_Write(ADDR_RUN, ADE9000_RUN_ON, false, css_pin);
    SPI_Write(ADDR_TEMP_CFG, ADE9000_TEMP_CFG, false, css_pin);

    calibration_data[css_pin-1].cal_vrms_cc = (1000000.0 * 0.707) / (VOLTAGE_TRANSFER_FUNCTION * ADE9000_RMS_FULL_SCALE_CODES);
}

// *****************************************************************************
// *****************************************************************************
// Section: ADE9000 Measurement Functions
// *****************************************************************************
// *****************************************************************************

/***************************GENERAL VALUES*****************************/
void ReadVoltageRMSRegs(VoltageRMSRegs *Data, SPI_CSS_Pin css_pin)
{
    if(css_pin > SPI_CSS12) return;

    Data->VoltageRMSValue_A = (float)((int32_t)(SPI_Read(ADDR_AVRMS, true, css_pin)) * (calibration_data[css_pin-1].cal_vrms_cc)) / 1000000.0f;
    Data->VoltageRMSValue_B = (float)((int32_t)(SPI_Read(ADDR_BVRMS, true, css_pin)) * (calibration_data[css_pin-1].cal_vrms_cc)) / 1000000.0f;
    Data->VoltageRMSValue_C = (float)((int32_t)(SPI_Read(ADDR_CVRMS, true, css_pin)) * (calibration_data[css_pin-1].cal_vrms_cc)) / 1000000.0f;
    if (Data->VoltageRMSValue_A < RMS_VOLTAGE_LIMIT)
    {
        Data->VoltageRMSValue_A = 0.00f;
    }
    if (Data->VoltageRMSValue_B < RMS_VOLTAGE_LIMIT)
    {
        Data->VoltageRMSValue_B = 0.00f;
    }
    if (Data->VoltageRMSValue_C < RMS_VOLTAGE_LIMIT)
    {
        Data->VoltageRMSValue_C = 0.00f;
    }
}

void ReadLineVoltages(VoltageRMSRegs *rmsData, LineVoltageRegs *lineData)
{
    lineData->LineVoltage_AB = rmsData->VoltageRMSValue_A * sqrtf(3.0f);
    lineData->LineVoltage_BC = rmsData->VoltageRMSValue_B * sqrtf(3.0f);
    lineData->LineVoltage_CA = rmsData->VoltageRMSValue_C * sqrtf(3.0f);
}

void ReadVoltageRMSOSRegs(VoltageRMSOSRegs *VRMS_OS, SPI_CSS_Pin css_pin)
{
    if(css_pin > SPI_CSS12) return;

    VRMS_OS->VoltageRMSOSValue_A = (float)(int32_t(SPI_Read(ADDR_AVRMSOS, true, css_pin)));
    VRMS_OS->VoltageRMSOSValue_B = (float)(int32_t(SPI_Read(ADDR_BVRMSOS, true, css_pin)));
    VRMS_OS->VoltageRMSOSValue_C = (float)(int32_t(SPI_Read(ADDR_CVRMSOS, true, css_pin)));
}

void ReadCurrentRMSOSRegs(CurrentRMSOSRegs *IRMS_OS, SPI_CSS_Pin css_pin)
{
    if(css_pin > SPI_CSS12) return;

    IRMS_OS->CurrentRMSOSValue_A = (float)(int32_t(SPI_Read(ADDR_AIRMSOS, true, css_pin))) / 1000;
    IRMS_OS->CurrentRMSOSValue_B = (float)(int32_t(SPI_Read(ADDR_BIRMSOS, true, css_pin))) / 1000;
    IRMS_OS->CurrentRMSOSValue_C = (float)(int32_t(SPI_Read(ADDR_CIRMSOS, true, css_pin))) / 1000;
}

void ReadCurrentRMSRegs(CurrentRMSRegs *Data, SPI_CSS_Pin css_pin)
{
    if(css_pin > SPI_CSS12) return;

    Data->CurrentRMSValue_A = (float)((int32_t)(SPI_Read(ADDR_AIRMS, true, css_pin)) * (float)calibration_data[css_pin - 1].cal_irms_cc) / 1000000.0f;
    Data->CurrentRMSValue_B = (float)((int32_t)(SPI_Read(ADDR_BIRMS, true, css_pin)) * (float)calibration_data[css_pin - 1].cal_irms_cc) / 1000000.0f;
    Data->CurrentRMSValue_C = (float)((int32_t)(SPI_Read(ADDR_CIRMS, true, css_pin)) * (float)calibration_data[css_pin - 1].cal_irms_cc) / 1000000.0f;
    if (Data->CurrentRMSValue_A < MIN_CURRENT_CAP)
    {
        Data->CurrentRMSValue_A = 0.00f;
    }
    if (Data->CurrentRMSValue_B < MIN_CURRENT_CAP)
    {
        Data->CurrentRMSValue_B = 0.00f;
    }
    if (Data->CurrentRMSValue_C < MIN_CURRENT_CAP)
    {
        Data->CurrentRMSValue_C = 0.00f;
    }
}

void ReadPeriodRegsnValues(PeriodRegs *Data, VoltageRMSRegs *RMSData, SPI_CSS_Pin css_pin)
{
    if(css_pin > SPI_CSS12) return;

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
void ReadActivePowerRegs(ActivePowerRegs *Data, VoltageRMSRegs *rmsData, CurrentRMSRegs *currentData, SPI_CSS_Pin css_pin)
{
    if(css_pin > SPI_CSS12) return;

    if (rmsData->VoltageRMSValue_A > RMS_VOLTAGE_LIMIT && currentData->CurrentRMSValue_A > MIN_CURRENT_CAP)
    {
        Data->ActivePowerValue_A = (float)((int32_t)(SPI_Read(ADDR_AWATT, true, css_pin)) / (calibration_data[css_pin - 1].cal_power_cc * 1000.0f));
    }
    else
    {
        Data->ActivePowerValue_A = 0.0f;
    }

    if (rmsData->VoltageRMSValue_B > RMS_VOLTAGE_LIMIT && currentData->CurrentRMSValue_B > MIN_CURRENT_CAP)
    {
        Data->ActivePowerValue_B = (float)((int32_t)(SPI_Read(ADDR_BWATT, true, css_pin)) / (calibration_data[css_pin - 1].cal_power_cc * 1000.0f));
    }
    else
    {
        Data->ActivePowerValue_B = 0.0f;
    }

    if (rmsData->VoltageRMSValue_C > RMS_VOLTAGE_LIMIT && currentData->CurrentRMSValue_C > MIN_CURRENT_CAP)
    {
        Data->ActivePowerValue_C = (float)((int32_t)(SPI_Read(ADDR_CWATT, true, css_pin)) / (calibration_data[css_pin - 1].cal_power_cc * 1000.0f));
    }
    else
    {
        Data->ActivePowerValue_C = 0.0f;
    }
    // Data->ActivePowerValue_A = (float)(int32_t(SPI_Read(ADDR_AWATT, true, css_pin))) / (calibration_data[css_pin - 1].cal_power_cc * 1000.0f);
    // Data->ActivePowerValue_B = (float)(int32_t(SPI_Read(ADDR_BWATT, true, css_pin))) / (calibration_data[css_pin - 1].cal_power_cc * 1000.0f);
    // Data->ActivePowerValue_C = (float)(int32_t(SPI_Read(ADDR_CWATT, true, css_pin))) / (calibration_data[css_pin - 1].cal_power_cc * 1000.0f);
}

void ReadReactivePowerRegs(ReactivePowerRegs *Data1, VoltageRMSRegs *rmsData, CurrentRMSRegs *currentData, SPI_CSS_Pin css_pin)
{
    if(css_pin > SPI_CSS12) return;

    if (rmsData->VoltageRMSValue_A > RMS_VOLTAGE_LIMIT && currentData->CurrentRMSValue_A > MIN_CURRENT_CAP)
    {
        Data1->ReactivePowerValue_A = (float)((int32_t)(SPI_Read(ADDR_AVAR, true, css_pin)) / (calibration_data[css_pin - 1].cal_power_cc * 1000.0f));
    }
    else
    {
        Data1->ReactivePowerValue_A = 0.0f;
    }

    if (rmsData->VoltageRMSValue_B > RMS_VOLTAGE_LIMIT && currentData->CurrentRMSValue_B > MIN_CURRENT_CAP)
    {
        Data1->ReactivePowerValue_B = (float)((int32_t)(SPI_Read(ADDR_BVAR, true, css_pin)) / (calibration_data[css_pin - 1].cal_power_cc * 1000.0f));
    }
    else
    {
        Data1->ReactivePowerValue_B = 0.0f;
    }

    if (rmsData->VoltageRMSValue_C > RMS_VOLTAGE_LIMIT && currentData->CurrentRMSValue_C > MIN_CURRENT_CAP)
    {
        Data1->ReactivePowerValue_C = (float)((int32_t)(SPI_Read(ADDR_CVAR, true, css_pin)) / (calibration_data[css_pin - 1].cal_power_cc * 1000.0f));
    }
    else
    {
        Data1->ReactivePowerValue_C = 0.0f;
    }
}

void ReadApparentPowerRegs(ApparentPowerRegs *Data, VoltageRMSRegs *rmsData, CurrentRMSRegs *currentData, SPI_CSS_Pin css_pin)
{
    if(css_pin > SPI_CSS12) return;

    if (rmsData->VoltageRMSValue_A > RMS_VOLTAGE_LIMIT && currentData->CurrentRMSValue_A > MIN_CURRENT_CAP)
    {
        Data->ApparentPowerValue_A = (float)((int32_t)(SPI_Read(ADDR_AVA, true, css_pin)) / (calibration_data[css_pin - 1].cal_power_cc * 1000.0f));
    }
    else
    {
        Data->ApparentPowerValue_A = 0.0f;
    }

    if (rmsData->VoltageRMSValue_B > RMS_VOLTAGE_LIMIT && currentData->CurrentRMSValue_B > MIN_CURRENT_CAP)
    {
        Data->ApparentPowerValue_B = (float)((int32_t)(SPI_Read(ADDR_BVA, true, css_pin)) / (calibration_data[css_pin - 1].cal_power_cc * 1000.0f));
    }
    else
    {
        Data->ApparentPowerValue_B = 0.0f;
    }

    if (rmsData->VoltageRMSValue_C > RMS_VOLTAGE_LIMIT && currentData->CurrentRMSValue_C > MIN_CURRENT_CAP)
    {
        Data->ApparentPowerValue_C = (float)((int32_t)(SPI_Read(ADDR_CVA, true, css_pin)) / (calibration_data[css_pin - 1].cal_power_cc * 1000.0f));
    }
    else
    {
        Data->ApparentPowerValue_C = 0.0f;
    }
}

void ReadPowerFactorRegsnValues(PowerFactorRegs *Data, VoltageRMSRegs *rmsData, CurrentRMSRegs *currentData, SPI_CSS_Pin css_pin)
{
    if(css_pin > SPI_CSS12) return;

    if (rmsData->VoltageRMSValue_A > RMS_VOLTAGE_LIMIT && currentData->CurrentRMSValue_A > MIN_CURRENT_CAP)
    {
        Data->PowerFactorValue_A = (float)((int32_t)SPI_Read(ADDR_APF, true, css_pin)) / CONST_2_27;
    }
    else
    {
        Data->PowerFactorValue_A = 1.0f;
    }

    if (rmsData->VoltageRMSValue_B > RMS_VOLTAGE_LIMIT && currentData->CurrentRMSValue_B > MIN_CURRENT_CAP)
    {
        Data->PowerFactorValue_B = (float)((int32_t)SPI_Read(ADDR_BPF, true, css_pin)) / CONST_2_27;
    }
    else
    {
        Data->PowerFactorValue_B = 1.0f;
    }

    if (rmsData->VoltageRMSValue_C > RMS_VOLTAGE_LIMIT && currentData->CurrentRMSValue_C > MIN_CURRENT_CAP)
    {
        Data->PowerFactorValue_C = (float)((int32_t)SPI_Read(ADDR_CPF, true, css_pin)) / CONST_2_27;
    }
    else
    {
        Data->PowerFactorValue_C = 1.0f;
    }
}

/***************************ENERGY VALUES*****************************/
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

        if (css_pin)
        {
            // rawValue = SPI_Read(ADDR_AFWATTHR_HI, true, css_pin);
            // Data1->ActiveEnergyReg_A = (rawValue & 0x80000000) ? -((int32_t)(~rawValue + 1)) : (int32_t)rawValue;
            // rawValue = SPI_Read(ADDR_BFWATTHR_HI, true, css_pin);
            // Data1->ActiveEnergyReg_B = (rawValue & 0x80000000) ? -((int32_t)(~rawValue + 1)) : (int32_t)rawValue;
            // rawValue = SPI_Read(ADDR_CFWATTHR_HI, true, css_pin);
            // Data1->ActiveEnergyReg_C = (rawValue & 0x80000000) ? -((int32_t)(~rawValue + 1)) : (int32_t)rawValue;
            // rawValue = SPI_Read(ADDR_AFVARHR_HI, true, css_pin);
            // Data1->ReactiveEnergyReg_A = (rawValue & 0x80000000) ? -((int32_t)(~rawValue + 1)) : (int32_t)rawValue;
            // rawValue = SPI_Read(ADDR_BFVARHR_HI, true, css_pin);
            // Data1->ReactiveEnergyReg_B = (rawValue & 0x80000000) ? -((int32_t)(~rawValue + 1)) : (int32_t)rawValue;
            // rawValue = SPI_Read(ADDR_CFVARHR_HI, true, css_pin);
            // Data1->ReactiveEnergyReg_C = (rawValue & 0x80000000) ? -((int32_t)(~rawValue + 1)) : (int32_t)rawValue;
            uint32_t rawActive;
//            uint32_t rawActive = SPI_Read(ADDR_AWATTHR_HI, true, css_pin);
//            Data1->ActiveEnergyReg_A = (rawActive & 0x80000000) ? ((int32_t)(~rawActive + 1)) : (int32_t)rawActive;
//            Data1->PActiveEnergyValue_kWh_A = ((float) Data1->ActiveEnergyReg_A * (calibration_data[css_pin - 1].cal_energy_cc)) / 1000000.0f / 1000.0f;
            uint32_t rawReactive;
//            uint32_t rawReactive = SPI_Read_Energy(ADDR_AFVARHR_HI, true, css_pin);

            
//            rawActive = SPI_Read(ADDR_BWATTHR_HI, true, css_pin);
//            Data1->ActiveEnergyReg_B = (rawActive & 0x80000000) ? ((int32_t)(~rawActive + 1)) : (int32_t)rawActive;
//            Data1->PActiveEnergyValue_kWh_B = ((float) Data1->ActiveEnergyReg_B * (calibration_data[css_pin - 1].cal_energy_cc)) / 1000000.0f / 1000.0f;
//            rawReactive = SPI_Read_Energy(ADDR_BFVARHR_HI, true, css_pin);
            
            
//             rawActive = SPI_Read(ADDR_AWATTHR_HI, true, css_pin);
////            Data1->ActiveEnergyReg_C = (rawActive & 0x80000000) ? ((int32_t)(~rawActive + 1)) : (int32_t)rawActive;
//            if(rawActive & 0x80000000){
//                Data1->ActiveEnergyReg_A = ((int32_t)(~rawActive + 1));
//                Data1->NActiveEnergyValue_kWh_A = ((float) Data1->ActiveEnergyReg_A * (calibration_data[css_pin - 1].cal_energy_cc)) / 1000000.0f / 1000.0f;
//
//            }
//            else{
//                Data1->ActiveEnergyReg_A = (int32_t)rawActive;
//            Data1->PActiveEnergyValue_kWh_A = ((float) Data1->ActiveEnergyReg_A * (calibration_data[css_pin - 1].cal_energy_cc)) / 1000000.0f / 1000.0f;
//
//            }
//             
//             
//            
//            rawActive = SPI_Read(ADDR_BWATTHR_HI, true, css_pin);
////            Data1->ActiveEnergyReg_C = (rawActive & 0x80000000) ? ((int32_t)(~rawActive + 1)) : (int32_t)rawActive;
//            if(rawActive & 0x80000000){
//                Data1->ActiveEnergyReg_B = ((int32_t)(~rawActive + 1));
//                Data1->NActiveEnergyValue_kWh_B = ((float) Data1->ActiveEnergyReg_B * (calibration_data[css_pin - 1].cal_energy_cc)) / 1000000.0f / 1000.0f;
//
//            }
//            else{
//                Data1->ActiveEnergyReg_B = (int32_t)rawActive;
//            Data1->PActiveEnergyValue_kWh_B = ((float) Data1->ActiveEnergyReg_B * (calibration_data[css_pin - 1].cal_energy_cc)) / 1000000.0f / 1000.0f;
//
//            }
            
            //----------------------------------------------------------------------
            // Reading Active Energy
            //----------------------------------------------------------------------

            if (CurrentData->CurrentRMSValue_A >= MIN_CURRENT_CAP){            
                rawActive = SPI_Read(ADDR_AWATTHR_HI, true, css_pin);
                Data1->ActiveEnergyReg_A = (int32_t)rawActive; // Converting to in32_t for signed check

                if (Data1->ActiveEnergyReg_A < 0) {
                    Data1->NActiveEnergyValue_kWh_A = ((float) Data1->ActiveEnergyReg_A *
                        calibration_data[css_pin - 1].cal_energy_cc) / 1000000.0f / 1000.0f;
                } else {
                    Data1->PActiveEnergyValue_kWh_A = ((float) Data1->ActiveEnergyReg_A *
                        calibration_data[css_pin - 1].cal_energy_cc) / 1000000.0f / 1000.0f;
                }   
            }
            
            if (CurrentData->CurrentRMSValue_B >= MIN_CURRENT_CAP){            
                rawActive = SPI_Read(ADDR_BWATTHR_HI, true, css_pin);
                Data1->ActiveEnergyReg_B = (int32_t)rawActive;

                if (Data1->ActiveEnergyReg_B < 0) {
                    Data1->NActiveEnergyValue_kWh_B = ((float) Data1->ActiveEnergyReg_B *
                        calibration_data[css_pin - 1].cal_energy_cc) / 1000000.0f / 1000.0f;
                } else {
                    Data1->PActiveEnergyValue_kWh_B = ((float) Data1->ActiveEnergyReg_B *
                        calibration_data[css_pin - 1].cal_energy_cc) / 1000000.0f / 1000.0f;
                }
            }
            
            
            if (CurrentData->CurrentRMSValue_C >= MIN_CURRENT_CAP){            
                rawActive = SPI_Read(ADDR_CWATTHR_HI, true, css_pin);
                Data1->ActiveEnergyReg_C = (int32_t)rawActive;

                if (Data1->ActiveEnergyReg_C < 0) {
                    Data1->NActiveEnergyValue_kWh_C = ((float) Data1->ActiveEnergyReg_C *
                        calibration_data[css_pin - 1].cal_energy_cc) / 1000000.0f / 1000.0f;
                } else {
                    Data1->PActiveEnergyValue_kWh_C = ((float) Data1->ActiveEnergyReg_C *
                        calibration_data[css_pin - 1].cal_energy_cc) / 1000000.0f / 1000.0f;
                }
                Data1->PActiveEnergyValue_kWh_C = ((float) Data1->ActiveEnergyReg_C * (calibration_data[css_pin - 1].cal_energy_cc)) / 1000000.0f / 1000.0f;
            }
            //----------------------------------------------------------------------
            
            //----------------------------------------------------------------------
            // Reading Reactive Energy
            //----------------------------------------------------------------------
            if (CurrentData->CurrentRMSValue_A >= MIN_CURRENT_CAP){  
                rawActive = SPI_Read(ADDR_AVAHR_HI, true, css_pin);
                Data1->ReactiveEnergyReg_A = (int32_t)rawActive; // converting to int32 for signed check
                Data1->ReactiveEnergyValue_kWh_A = ((float) Data1->ReactiveEnergyReg_A * (calibration_data[css_pin - 1].cal_energy_cc)) / 1000000.0f / 1000.0f;
            }
            
            if (CurrentData->CurrentRMSValue_B >= MIN_CURRENT_CAP){ 
                rawActive = SPI_Read(ADDR_BVAHR_HI, true, css_pin);
                Data1->ReactiveEnergyReg_B = (int32_t)rawActive;
                Data1->ReactiveEnergyValue_kWh_B = ((float) Data1->ReactiveEnergyReg_B * (calibration_data[css_pin - 1].cal_energy_cc)) / 1000000.0f / 1000.0f;
            }
            
            if (CurrentData->CurrentRMSValue_C >= MIN_CURRENT_CAP){
                rawActive = SPI_Read(ADDR_CVAHR_HI, true, css_pin);
                Data1->ReactiveEnergyReg_C = (int32_t)rawActive;
                Data1->ReactiveEnergyValue_kWh_C = ((float) Data1->ReactiveEnergyReg_C * (calibration_data[css_pin - 1].cal_energy_cc)) / 1000000.0f / 1000.0f;
            }
            //----------------------------------------------------------------------
            
            
//            rawReactive = SPI_Read_Energy(ADDR_CFVARHR_HI, true, css_pin);
        
            // Data1->ActiveEnergyReg_A = ((int32_t)rawActive & 0x7FFFFFFF);
            
//            Data1->ReactiveEnergyReg_A = (uint64_t)(rawReactive & 0x80000000) ? -((int32_t)(~rawReactive + 1)) : (int32_t)rawReactive;
            
        }
        else
        {
            // Data1->ActiveEnergyReg_A = SPI_Read(ADDR_AFWATTHR_HI, true, css_pin);
            // Data1->ActiveEnergyReg_B = SPI_Read(ADDR_BFWATTHR_HI, true, css_pin);
            // Data1->ActiveEnergyReg_C = SPI_Read(ADDR_CFWATTHR_HI, true, css_pin);
            // Data1->ReactiveEnergyReg_A = SPI_Read(ADDR_AFVARHR_HI, true, css_pin);
            // Data1->ReactiveEnergyReg_B = SPI_Read(ADDR_BFVARHR_HI, true, css_pin);
            // Data1->ReactiveEnergyReg_C = SPI_Read(ADDR_CFVARHR_HI, true, css_pin);
        }
        // Data1->PActiveEnergyValue_kWh_A = (float) Data1->ActiveEnergyReg_A;
//         if (CurrentData->CurrentRMSValue_A >= MIN_CURRENT_CAP)
        // {
            // Data1->PActiveEnergyValue_kWh_A = Data1->ActiveEnergyReg_A * (float)(calibration_data[css_pin - 1].cal_energy_cc) / 1000000 / 1000;
        // }
        // if (CurrentData->CurrentRMSValue_B >= MIN_CURRENT_CAP)
        // {
        //     Data1->PActiveEnergyValue_kWh_B = Data1->ActiveEnergyReg_B * (float)(calibration_data[css_pin - 1].cal_energy_cc) / 1000000 / 1000;
        // }
        // if (CurrentData->CurrentRMSValue_C >= MIN_CURRENT_CAP)
        // {
        //     Data1->PActiveEnergyValue_kWh_C = Data1->ActiveEnergyReg_C * (float)(calibration_data[css_pin - 1].cal_energy_cc) / 1000000 / 1000;
        // }

        // Data1->ReactiveEnergyValue_kWh_A = Data1->ReactiveEnergyReg_A * (float)(calibration_data[css_pin - 1].cal_energy_cc) / 1000000 / 1000;
        // Data1->ReactiveEnergyValue_kWh_B = Data1->ReactiveEnergyReg_B * (float)(calibration_data[css_pin - 1].cal_energy_cc) / 1000000 / 1000;
        // Data1->ReactiveEnergyValue_kWh_C = Data1->ReactiveEnergyReg_C * (float)(calibration_data[css_pin - 1].cal_energy_cc) / 1000000 / 1000;
        
        SPI_Write(ADDR_STATUS0, (status0 | 0x01), true, css_pin);
    }
    else
    {
    }
}


/***************************THRESHOLD VALUES*****************************/
void ReadVoltageTHDRegsnValues(VoltageTHDRegs *Data, SPI_CSS_Pin css_pin)
{
    uint32_t rawValue;
    int32_t tempReg;

    rawValue = SPI_Read(ADDR_AVTHD, true, css_pin);
    tempReg = *(int32_t *)&rawValue;
    Data->VoltageTHDValue_A = (float)tempReg * 100 / 134217728.0f; // Multiplied by 2^-27

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

    //    Data->AngleValue_VA_IA = abs(((int32_t(SPI_Read(ADDR_ANGL_VA_IA, false, css_pin))) * 0.017578125) - 360) * (( 22/7 ) / 180); // in Radians
    //    Data->AngleValue_VB_IB = abs(360 - ((int32_t(SPI_Read(ADDR_ANGL_VB_IB, false, css_pin))) * 0.017578125)); // in Degrees

    Data->AngleValue_VA_IA = (abs(360 - ((int32_t(SPI_Read(ADDR_ANGL_VA_IA, false, css_pin))) * 0.017578125)) * (PI / 180)); // in Radians
    Data->AngleValue_VB_IB = (abs(360 - ((int32_t(SPI_Read(ADDR_ANGL_VB_IB, false, css_pin))) * 0.017578125)) * (PI / 180)); // in Radians
    Data->AngleValue_VC_IC = (abs(360 - ((int32_t(SPI_Read(ADDR_ANGL_VC_IC, false, css_pin))) * 0.017578125)) * (PI / 180)); // in Radians

    //    Data->AngleValue_VB_IB = (int32_t(SPI_Read(ADDR_ANGL_VB_IB, true, css_pin))) * 0.017578125; // mulConstant
    //    Data->AngleValue_VC_IC = (int32_t(SPI_Read(ADDR_ANGL_VC_IC, true, css_pin))) * 0.017578125; // mulConstant

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
int32_t calculate_gain_reg_value(ADE9000_parameter_type parameter, ADE9000_phase phase, uint32_t expected_reg_value, uint8_t channel)
{   // helper function for calculating gain register value

    uint32_t reg_addr = ade9000_get_phase_register(((parameter == CURRENT_PARAMETER) ? ADDR_AIRMS : ((parameter == VOLTAGE_PARAMETER) ? ADDR_AVRMS : 0)),
                                                     phase, parameter);

    uint32_t measured_reg_value = SPI_Read(reg_addr, true, get_css_pin(channel));
    double ratio = ((double)expected_reg_value / (double)measured_reg_value) - 1.0;
    int32_t gain_value = (int32_t)(ratio * CONST_2_27);

    return gain_value;
}

void current_gain_calibration(float current, uint8_t channel, ADE9000_phase phase)
{

    // NOMINAL_INPUT_CURRENT and NOMINAL_INPUT_VOLTAGE are also supposed to be received from host controller (V3S)
    float i_nominal = current;

    float current_transfer_func = (float)(R_BURDEN / (float)(channelConfig[channel].ctRatio));
    float input_current_ADC = (i_nominal * current_transfer_func);
    float i_fsp = (input_current_ADC) / 0.707; // @ gain 1      (ideally fetch the set gain value from the register)

    // Now calibrate the register values for all 3 phases
    uint32_t expected_irms_value = (i_fsp * ADE9000_RMS_FULL_SCALE_CODES);

    // Reset gain registers to 0 first
    SPI_Write(ade9000_get_phase_register(ADDR_AIGAIN, phase, CALIBRATION_PARAMETER), (uint32_t)0x00, true, get_css_pin(channel));
    // SPI_Write(ADDR_BIGAIN, (uint32_t)0x00, true, get_css_pin(channel));
    // SPI_Write(ADDR_CIGAIN, (uint32_t)0x00, true, get_css_pin(channel));

    delay_ms(1); // wait for some time after resetting gain registers to 0

    reg_values[channel][phase] = (uint32_t)calculate_gain_reg_value(CURRENT_PARAMETER, phase, expected_irms_value, channel);

    SPI_Write(ade9000_get_phase_register(ADDR_AIGAIN, phase, CALIBRATION_PARAMETER), reg_values[channel][phase], true, get_css_pin(channel));
    // SPI_Write(ADDR_BIGAIN, (uint32_t)calculate_gain_reg_value(CURRENT_PARAMETER, PHASE_B, expected_irms_value, channel), true, get_css_pin(channel));
    // SPI_Write(ADDR_CIGAIN, (uint32_t)calculate_gain_reg_value(CURRENT_PARAMETER, PHASE_C, expected_irms_value, channel), true, get_css_pin(channel));
    
    /* Conversion Constants */
    // Current Conversion Constant = (I nominal * 10^6)/(Ifsp*full_scale_rms_codes);
    // calibration_data[channel].cal_irms_cc = (i_nominal * 1000000.0) / (expected_irms_value);         // only dependant on CT-ratio

}

void voltage_gain_calibration(float voltage, ADE9000_phase phase)
{
    // NOMINAL_INPUT_CURRENT and NOMINAL_INPUT_VOLTAGE are also supposed to be received from host controller (V3S)
    float v_nominal = voltage;

    // float voltage_transfer_func = (float)((R_LOW)/(R_LOW+R_HIGH));
    float input_voltage_ADC = (v_nominal * VOLTAGE_TRANSFER_FUNCTION);
    float v_fsp = (input_voltage_ADC) / 0.707; // @ gain 1      (ideally fetch the set gain value from the register)

    // // Now calibrate the register values
    uint32_t expected_vrms_value = (v_fsp * ADE9000_RMS_FULL_SCALE_CODES);
    
    for(uint8_t channel=0; channel<TOTAL_ADE9000_UNITS; channel++)
    {
        SPI_Write(ade9000_get_phase_register(ADDR_AVGAIN, phase, CALIBRATION_PARAMETER), 0x00000000, true, get_css_pin(channel));
        delay_ms(1); // wait for some time after resetting gain registers to 0
        reg_values[channel][phase] = (uint32_t)calculate_gain_reg_value(VOLTAGE_PARAMETER, phase, expected_vrms_value, channel);
        SPI_Write(ade9000_get_phase_register(ADDR_AVGAIN, phase, CALIBRATION_PARAMETER),
                  reg_values[channel][phase],
                  true, get_css_pin(channel)
                 );
    }

    /* Conversion Constants */          // no need to calculate here, all phases and channels have same voltage conversion constant
    // Current Conversion Constant = (V nominal * 10^6)/(Vfsp*full_scale_rms_codes);
    // calibration_data[channel].cal_vrms_cc = (v_nominal * 1000000.0) / (v_fsp * ADE9000_RMS_FULL_SCALE_CODES);
    // calibration_data[channel].cal_vrms_cc = (1000000.0 * 0.707) / (voltage_transfer_func * ADE9000_RMS_FULL_SCALE_CODES);
}

void ct_ratio_calibration(uint16_t ratio, uint8_t channel)
{

    /* Conversion Constants */
    // calibration_data[channel].cal_irms_cc = (SQRT_2 * current_transfer_func * ADE9000_RMS_FULL_SCALE_CODES);
    calibration_data[channel].cal_irms_cc = (1000000.0 * 0.707 * ratio) / (R_BURDEN * (float)ADE9000_RMS_FULL_SCALE_CODES);
    calibration_data[channel].cal_power_cc = (1000.0 * 0.707 * 0.707 * ratio) / (ADE9000_WATT_FULL_SCALE_CODES * R_BURDEN * VOLTAGE_TRANSFER_FUNCTION);
    calibration_data[channel].cal_energy_cc = (1000000.0 * 0.707 * 0.707 * ratio) / (ADE9000_WATT_FULL_SCALE_CODES * R_BURDEN * VOLTAGE_TRANSFER_FUNCTION * 3600.0f * 8000.0f * 0.0001220703125f); // 2^(-13) = 0.0001220703125f
    // need to use accumulation time here instead of 8000

}

void current_offset_calibration(CurrentRMSRegs *Data, SPI_CSS_Pin css_pin)
{
    /*
     * Formula for Calibration of IRMS
     * xRMS = sqrt(xRMS0^2 + xRMOSOS)
     */

    /*  xIRMSOS
     *  xIRMS_expected = (i_fsp_calibration * ADE9000_RMS_FULL_SCALE_CODES)
     *  xIRMSOS = ((xIRMS_expected^2) - (xIRMS_measured^2)) / (2^15)    // convert to register value
     */

    ReadCurrentRMSOSRegs(&current_RMS_OS, css_pin);
    //    channelConfig[j-1].setIOffset
    float tmp;
    //    tmp = pow(Data->CurrentRMSValue_A,2) + (pow(2,15)*(current_RMS_OS.CurrentRMSOSValue_A));
    //    tmp = pow(Data->CurrentRMSValue_A,2) + (pow(2,15)*(channelConfig[css_pin-1].setIOffset / 1000));
    //    tmp = pow(Data->CurrentRMSValue_A, 2) + (channelConfig[css_pin - 1].setIOffset);
    //    Data->CurrentRMSValue_A = tmp > 0 ? sqrt(tmp) : 0;
    //
    //    tmp = pow(Data->CurrentRMSValue_B, 2) + (channelConfig[css_pin - 1].setIOffset);
    //    Data->CurrentRMSValue_B = tmp > 0 ? sqrt(tmp) : 0;
    //
    //    tmp = pow(Data->CurrentRMSValue_C, 2) + (channelConfig[css_pin - 1].setIOffset);
    //    Data->CurrentRMSValue_C = tmp > 0 ? sqrt(tmp) : 0;

    //        tmp = pow(Data->CurrentRMSValue_A, 2) + (channelConfig[css_pin - 1].setIOffset);
    if (Data->CurrentRMSValue_A >= MIN_CURRENT_CAP)
    {
        Data->CurrentRMSValue_A += (channelConfig[css_pin - 1].setIOffset);
    }
    if (Data->CurrentRMSValue_B >= MIN_CURRENT_CAP)
    {
        Data->CurrentRMSValue_B += (channelConfig[css_pin - 1].setIOffset);
    }
    if (Data->CurrentRMSValue_C >= MIN_CURRENT_CAP)
    {
        Data->CurrentRMSValue_C += (channelConfig[css_pin - 1].setIOffset);
    }

    //    Data->CurrentRMSValue_B += (channelConfig[css_pin - 1].setIOffset);
    //
    //    Data->CurrentRMSValue_C += (channelConfig[css_pin - 1].setIOffset);
}

void current_offset_write(int8_t channel, int32_t offset)
{

    // GAIN and OFFSET registers needs to be re-evaluated for better accuracy
    // We need to apply the same offset to all 3 phases

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

    /*  xVRMSOS
     *  xVRMS_expected = (v_fsp_calibration * ADE9000_RMS_FULL_SCALE_CODES)
     *  xVRMSOS = ((xVRMS_expected^2) - (xVRMS_measured^2)) / (2^15)    // convert to register value
     */

    ReadVoltageRMSOSRegs(&voltage_RMS_OS, css_pin);

    //    float tmp;
    //    tmp = pow(Data->VoltageRMSValue_A, 2) + (pow(2, 15) * (voltage_RMS_OS.VoltageRMSOSValue_A));
    //    Data->VoltageRMSValue_A = tmp > 0 ? sqrt(tmp) : 0;
    //
    //    tmp = pow(Data->VoltageRMSValue_B, 2) + pow(2, 15) * (voltage_RMS_OS.VoltageRMSOSValue_B);
    //    Data->VoltageRMSValue_B = tmp > 0 ? sqrt(tmp) : 0;
    //
    //    tmp = pow(Data->VoltageRMSValue_C, 2) + pow(2, 15) * (voltage_RMS_OS.VoltageRMSOSValue_C);
    //    Data->VoltageRMSValue_C = tmp > 0 ? sqrt(tmp) : 0;

    //    float tmp;
    if (Data->VoltageRMSValue_A >= RMS_VOLTAGE_LIMIT)
    {
        Data->VoltageRMSValue_A += (voltage_RMS_OS.VoltageRMSOSValue_A);
    }

    if (Data->VoltageRMSValue_B >= RMS_VOLTAGE_LIMIT)
    {
        Data->VoltageRMSValue_B += (voltage_RMS_OS.VoltageRMSOSValue_B);
    }

    if (Data->VoltageRMSValue_C >= RMS_VOLTAGE_LIMIT)
    {
        Data->VoltageRMSValue_C += (voltage_RMS_OS.VoltageRMSOSValue_C);
    }

    //    Data->VoltageRMSValue_B += (voltage_RMS_OS.VoltageRMSOSValue_B);
    //
    //    Data->VoltageRMSValue_C += (voltage_RMS_OS.VoltageRMSOSValue_C);
}
void voltage_offset_write(int8_t channel, int32_t offset)
{

    // GAIN and OFFSET registers needs to be re-evaluated for better accuracy
    // We need to apply the same offset to all 3 phases

    SPI_Write(ADDR_AVRMSOS, offset, true, get_css_pin(channel));
    SPI_Write(ADDR_BVRMSOS, offset, true, get_css_pin(channel));
    SPI_Write(ADDR_CVRMSOS, offset, true, get_css_pin(channel));

}
