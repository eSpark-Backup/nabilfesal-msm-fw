#ifndef ADE9000_H
#define ADE9000_H

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */

#include "main.h" 
#include "ADE9000RegMap.h" 
#include "ADE9000Calibration.h" 

// *****************************************************************************
// *****************************************************************************
// Section: Configuration Definitions
// *****************************************************************************
// *****************************************************************************

#define ADE9000_PART_ID  0xE0100A96 
/*Configuration registers*/
#define ADE9000_PGA_GAIN 0x0000   	    /*PGA@0x0000. Gain of all channels=1*/
#define ADE9000_CONFIG0 0x00000000		/*Integrator disabled*/
#define ADE9000_CONFIG1	0x0003			/*CF3/ZX pin outputs Zero crossing */
#define ADE9000_CONFIG2	0x0C00			/*Default High pass corner frequency of 1.25Hz*/
#define ADE9000_CONFIG3	0x0000			/*Peak and overcurrent detection disabled*/
#define ADE9000_ACCMODE 0x0000			/*60Hz operation, 3P4W Wye configuration, signed accumulation*/
										/*Clear bit 8 i.e. ACCMODE=0x00xx for 50Hz operation*/
										/*ACCMODE=0x0x9x for 3Wire delta when phase B is used as reference*/	
#define ADE9000_TEMP_CFG 0x000C			/*Temperature sensor enabled*/
#define ADE9000_ZX_LP_SEL 0x001E		/*Line period and zero crossing obtained from combined signals VA,VB and VC*/	
#define ADE9000_MASK0 0x00000001		/*Enable EGYRDY interrupt*/				
#define ADE9000_MASK1 0x00000000		/*MASK1 interrupts disabled*/
#define ADE9000_EVENT_MASK 0x00000000	/*Events disabled */
#define ADE9000_VLEVEL	0x0022EA28		/*Assuming Vnom=1/2 of full scale. 
										/*Refer Technical reference manual for detailed calculations.*/
#define ADE9000_DICOEFF 0x00000000 		/* Set DICOEFF= 0xFFFFE000 when integrator is enabled*/

/*Constant Definitions***/
#define ADE90xx_FDSP 8000   			/*ADE9000 FDSP: 8000sps, ADE9078 FDSP: 4000sps*/
#define ADE9000_RUN_ON 0x0001			/*DSP ON*/
/*Energy Accumulation Settings*/



//#define ADE9000_EP_CFG 0x0011			/*Enable energy accumulation, accumulate samples at 8ksps*/
										/*latch energy accumulation after EGYRDY*/
										/*If accumulation is changed to half line cycle mode, change EGY_TIME*/
#define ADE9000_EGY_TIME 0x1F3F 				/*Accumulate 8000 samples*/


#define ADE9000_EP_CFG 0x0011          /*Enable energy accumulation, 8ksps, latch after EGYRDY*/
/* Set to ~10s accumulation window for testing */
//#define ADE9000_EGY_TIME 0x13880      /* 80,000 samples ? 10 seconds at 8kSPS */

//#define ADE9000_EGY_TIME 0x3FF      /* 80,000 samples ? 10 seconds at 8kSPS */



 
/*Waveform buffer Settings*/
#define ADE9000_WFB_CFG 0x1000			/*Neutral current samples enabled, Resampled data enabled*/
/*Burst all channels*/
#define WFB_ELEMENT_ARRAY_SIZE 512  	/*size of buffer to read. 512 Max.Each element IA,VA...IN has max 512 points 
										/*[Size of waveform buffer/number of sample sets = 2048/4 = 512]*/
/*(Refer ADE9000 technical reference manual for more details)*/

/*Size of array reading calibration constants from EEPROM*/
#define CALIBRATION_CONSTANTS_ARRAY_SIZE 13

// *****************************************************************************
// *****************************************************************************
// Section: Data Types
// *****************************************************************************
// *****************************************************************************
typedef enum {
    SPI_CSS1 = 1,
    SPI_CSS2,
    SPI_CSS3,
    SPI_CSS4,
    SPI_CSS5,
    SPI_CSS6,
    SPI_CSS7,
    SPI_CSS8,
    SPI_CSS9,
    SPI_CSS10,
    SPI_CSS11,
    SPI_CSS12
} SPI_CSS_Pin;

typedef struct {
    float cal_irms_cc;
    float cal_vrms_cc;
    float cal_power_cc;
    float cal_energy_cc;
} Calibration_regs;

typedef struct {
    float ActivePowerValue_A;
    float ActivePowerValue_B;
    float ActivePowerValue_C;
} ActivePowerRegs;

typedef struct {
    float ReactivePowerValue_A;
    float ReactivePowerValue_B;
    float ReactivePowerValue_C;
} ReactivePowerRegs;

typedef struct {
    float ApparentPowerValue_A;
    float ApparentPowerValue_B;
    float ApparentPowerValue_C;
} ApparentPowerRegs;

typedef struct {
    uint64_t ActiveEnergyReg_A;
    uint64_t ActiveEnergyReg_B;
    uint64_t ActiveEnergyReg_C;
    float PActiveEnergyValue_kWh_A;
    float PActiveEnergyValue_kWh_B;
    float PActiveEnergyValue_kWh_C;
    float NActiveEnergyValue_kWh_A;
    float NActiveEnergyValue_kWh_B;
    float NActiveEnergyValue_kWh_C;
    float ACC_PActiveEnergyValue_kWh_A;
    float ACC_PActiveEnergyValue_kWh_B;
    float ACC_PActiveEnergyValue_kWh_C;
    float ACC_NActiveEnergyValue_kWh_A;
    float ACC_NActiveEnergyValue_kWh_B;
    float ACC_NActiveEnergyValue_kWh_C;

    uint64_t ReactiveEnergyReg_A;
    uint64_t ReactiveEnergyReg_B;
    uint64_t ReactiveEnergyReg_C;
    float ReactiveEnergyValue_kWh_A;
    float ReactiveEnergyValue_kWh_B;
    float ReactiveEnergyValue_kWh_C;
    float ACC_ReactiveEnergyValue_kWh_A;
    float ACC_ReactiveEnergyValue_kWh_B;
    float ACC_ReactiveEnergyValue_kWh_C;
} EnergyRegs;

typedef struct {
    float VoltageRMSValue_A;
    float VoltageRMSValue_B;
    float VoltageRMSValue_C;
} VoltageRMSRegs;

typedef struct {
    float VoltageRMSOSValue_A;
    float VoltageRMSOSValue_B;
    float VoltageRMSOSValue_C;
} VoltageRMSOSRegs;

typedef struct {
    float CurrentRMSOSValue_A;
    float CurrentRMSOSValue_B;
    float CurrentRMSOSValue_C;
} CurrentRMSOSRegs;


typedef struct {
    float LineVoltage_AB;
    float LineVoltage_BC;
    float LineVoltage_CA;
} LineVoltageRegs;

typedef struct {
    float CurrentRMSValue_A;
    float CurrentRMSValue_B;
    float CurrentRMSValue_C;
} CurrentRMSRegs;

typedef struct {
    float VoltageTHDValue_A;
    float VoltageTHDValue_B;
    float VoltageTHDValue_C;
} VoltageTHDRegs;

typedef struct {
    float CurrentTHDValue_A;
    float CurrentTHDValue_B;
    float CurrentTHDValue_C;
} CurrentTHDRegs;

typedef struct {
    float PowerFactorValue_A;
    float PowerFactorValue_B;
    float PowerFactorValue_C;
} PowerFactorRegs;

typedef struct {
    float FrequencyValue_A;
    float FrequencyValue_B;
    float FrequencyValue_C;
} PeriodRegs;

typedef struct 
{
	// float AngleValue_VA_VB;
	// float AngleValue_VB_VC;
	// float AngleValue_VA_VC;
	float AngleValue_VA_IA;
	float AngleValue_VB_IB;
	float AngleValue_VC_IC;
	// float AngleValue_IA_IB;
	// float AngleValue_IB_IC;
	// float AngleValue_IA_IC;
}AngleRegs;   

typedef struct {
    float Temperature;
} TemperatureRegs;

typedef struct {
    VoltageRMSRegs voltageRMS;
    LineVoltageRegs lineVoltage;
    CurrentRMSRegs currentRMS;
    PeriodRegs periodFrequency;
    ActivePowerRegs activePower;
    ReactivePowerRegs reactivePower;
    ApparentPowerRegs apparentPower;
    PowerFactorRegs powerFactor;
    EnergyRegs activeEnergy;
    VoltageTHDRegs voltageTHD;
    CurrentTHDRegs currentTHD;
    AngleRegs angleDegree;
    TemperatureRegs temperature;
} ADE9000UnitMeasurements;

typedef struct{
    uint16_t ctRatio;
    float setIOffset;
    int32_t setVOffset;
} ChannelConfig;
extern ChannelConfig channelConfig[12];

// *****************************************************************************
// *****************************************************************************
// Section: System Functions
// *****************************************************************************
// *****************************************************************************
void SPI_CSS_Control(SPI_CSS_Pin pin, bool set) ;
void SPI_Write(uint16_t address, uint32_t data, bool is32Bit, SPI_CSS_Pin css_pin);
uint32_t SPI_Read(uint32_t address, bool is32Bit, SPI_CSS_Pin css_pin);

bool IsADE9000(SPI_CSS_Pin css_pin);
void ADE9000_Initialize(SPI_CSS_Pin css_pin);

//void ct_ratio_calibration(uint16_t ratio);
void ct_ratio_calibration(uint16_t ratio, uint8_t channel, CurrentRMSRegs Data);

void current_offset_calibration(CurrentRMSRegs *Data, SPI_CSS_Pin css_pin);
void current_offset_write(int8_t channel, int32_t offset);

void voltage_offset_calibration(VoltageRMSRegs *Data, SPI_CSS_Pin css_pin);
void voltage_offset_write(int8_t channel, int32_t offset);


void ReadVoltageRMSRegs(VoltageRMSRegs *Data, SPI_CSS_Pin css_pin);
void ReadVoltageRMSOSRegs(VoltageRMSOSRegs *VRMS_OS, SPI_CSS_Pin css_pin);
void ReadLineVoltages(VoltageRMSRegs *rmsData, LineVoltageRegs *lineData);
void ReadCurrentRMSRegs(CurrentRMSRegs *Data, SPI_CSS_Pin css_pin);
void ReadPeriodRegsnValues(PeriodRegs *Data, VoltageRMSRegs *RMSData, SPI_CSS_Pin css_pin);

void ReadActivePowerRegs(ActivePowerRegs *Data, SPI_CSS_Pin css_pin);
void ReadReactivePowerRegs(ReactivePowerRegs *Data1,ApparentPowerRegs *Data2,PowerFactorRegs *Data3, SPI_CSS_Pin css_pin);
void ReadApparentPowerRegs(ApparentPowerRegs *Data, SPI_CSS_Pin css_pin);
void ReadPowerFactorRegsnValues(PowerFactorRegs *Data, VoltageRMSRegs *rmsData, CurrentRMSRegs *currentData, SPI_CSS_Pin css_pin);

void ReadEnergyRegs(EnergyRegs *Data1,ActivePowerRegs *Data2, SPI_CSS_Pin css_pin);

void ReadVoltageTHDRegsnValues(VoltageTHDRegs *Data, SPI_CSS_Pin css_pin);
void ReadCurrentTHDRegsnValues(CurrentTHDRegs *Data, SPI_CSS_Pin css_pin);
void ReadAngleRegsnValues(AngleRegs *Data, SPI_CSS_Pin css_pin);

void ReadTempRegnValue(TemperatureRegs *Data, SPI_CSS_Pin css_pin);

SPI_CSS_Pin get_css_pin(uint8_t channel);
#endif

/* *************************************************************************** */
