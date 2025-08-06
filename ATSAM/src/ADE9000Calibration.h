#ifndef ADE9000CALIBRATION_H
#define ADE9000CALIBRATION_H

#define NOMINAL_INPUT_VOLTAGE 200       //Nominal Calibrating RMS Voltage on all phases (Vrms)
#define NOMINAL_INPUT_CURRENT 5         //Nominal Calibrating RMS Current on all phases (Irms)
#define INPUT_FREQUENCY 	  50              //Nominal Calibrating frequency. Used to calculate Omega for phase calibration.Ensure ACCMODE register is set to correct frequency
#define CALIBRATION_ANGLE_DEGREES 60	//Calibration Angle in Degrees.Used for phase calibration
/*Transfer function*/

/*Full scale Codes referred from Datasheet.Respective digital codes are produced when ADC inputs are at full scale. Donot Change. */
#define ADE9000_RMS_FULL_SCALE_CODES       52702092
#define ADE9000_WATT_FULL_SCALE_CODES      20694066
#define ADE9000_RESAMPLED_FULL_SCALE_CODES 18196
#define ADE9000_PCF_FULL_SCALE_CODES       74532013
/****************************************************************************************************************
									Current Transfer Function
*****************************************************************************************************************
It is the voltage at the ADC input pins per input current(A) (Volts/Amp)
E.g: For a current output current transformer with Turns Ratio of 400:5. Burden resistor on board = .656*2=11.312 Ohm.
Current transfer function= 1/2500*Total Burden = 5/400*11.312= 0.1414
****************************************************************************************************************/
#define R_BURDEN (0.3535f * 2.0f)             //double the pcb value 
#define CT_PRI 400.0f                         //primary current
#define CT_SEC 5.00f                          //secondary current
//#define CURRENT_TRANSFER_FUNCTION ((R_BURDEN * CT_SEC) / CT_PRI)   //The RMS voltage at the ADC input pins per input RMS current (V/A).)
// #define CURRENT_TRANSFER_FUNCTION 0.00088375f
#define CURRENT_TRANSFER_FUNCTION 0.00707f
/****************************************************************************************************************
									Voltage Transfer Function
*****************************************************************************************************************
It is the voltage at the ADC input pins per input Voltage(V)(Volts/Volts)
E.g. The defaul atteunation factor on board is 801. 
Voltage transfer function = 1/801= 0.001248 ~=0.00125
****************************************************************************************************************/ 
#define R_HIGH 800000.0f
#define R_LOW 1000.0f
//#define VOLTAGE_TRANSFER_FUNCTION  ((1.0f / (R_HIGH + R_LOW)) * R_LOW) //The RMS voltage at the ADC input pins per input RMS voltage (V/V) 
#define VOLTAGE_TRANSFER_FUNCTION  0.001248439f
/****************************************************************************************************************
									Constants: Do not change 
*****************************************************************************************************************/
#define F_DSP 8000                         //Signal update Rate
#define CALIBRATION_EGY_CFG 0xF011         //Latch after EGYRDY. Sample based accumulation. Read with reset disabled. Accumulation enabled
#define EGYACCTIME 0x1F3F                  //Accumulate for a total of 8000 (EGY_TIME+1) samples.
#define CALIBRATION_ACC_TIME 1             //if EGYACCTIME= 0x1F3F, Accumulation time is 1sec. Change this if EGYACCTIME is changed.
#define CAL_ANGLE_RADIANS(x) (x*3.14159/180)
#define SQRT_2 sqrt(2.0)
/****************************************************************************************************************
									Conversion Constants
*****************************************************************************************************************
If calibration is done with the the calibration sketch, the following conversion constants apply:
If CURRENT_TRANSFER_FUNCTION or VOLTAGE_TRANSFER_FUNCTION are changed, update the conversion constants

CAL_IRMS_CC in uA/code 				= 10^6/(CURRENT_TRANSFER_FUNCTION*ADE9000_RMS_FULL_SCALE_CODES*SQRT(2)) 
CAL_VRMS_CC in uV/code 				= 10^6/(VOLTAGE_TRANSFER_FUNCTION*ADE9000_RMS_FULL_SCALE_CODES*SQRT(2))
CAL_POWER_CC in mW/code 			= 10^3/(CURRENT_TRANSFER_FUNCTION*VOLTAGE_TRANSFER_FUNCTION*ADE9000_WATT_FULL_SCALE_CODES*2)
CAL_ENERGY_CC in uWhr/xTHR_HI code 	= 10^6/(CURRENT_TRANSFER_FUNCTION*VOLTAGE_TRANSFER_FUNCTION*ADE9000_WATT_FULL_SCALE_CODES*2*8000*3600*2^-13)

If conversion constants are set correctly, the register codes are converted to physical parameters as:
e.g Channel A Vrms = (AVRMS(register)*CAL_VRMS_CC/10^6)
	Channel A Active Power = (AWATT(register)*CAL_POWER_CC/10^3)	
*****************************************************************************************************************/
/*
#define CURRENT_CONST (CURRENT_TRANSFER_FUNCTION / ADE9000_RMS_FULL_SCALE_CODES * 2.0f)
#define VOLTAGE_CONST (VOLTAGE_TRANSFER_FUNCTION / ADE9000_RMS_FULL_SCALE_CODES * 2.0f)
#define POWER_CONST (((VOLTAGE_TRANSFER_FUNCTION / 0.707f)  * (CURRENT_TRANSFER_FUNCTION / 0.707f)  * ADE9000_WATT_FULL_SCALE_CODES) * 4.0f)
#define CAL_IRMS_CC		3.28849					// (uA/code)
#define CAL_VRMS_CC		10.73364				// (uV/code)	
#define CAL_POWER_CC 	4.73755					// (mW/code) Applicable for Active, reactive and apparent power
#define CAL_ENERGY_CC	1.34757					// (uWhr/xTHR_HI code)Applicable for Active, reactive and apparent energy
*/


//#define CAL_IRMS_CC		(1000000.0 / (CURRENT_TRANSFER_FUNCTION * ADE9000_RMS_FULL_SCALE_CODES * SQRT_2))				
//#define CAL_VRMS_CC		(1000000.0 / (VOLTAGE_TRANSFER_FUNCTION * ADE9000_RMS_FULL_SCALE_CODES * SQRT_2))						
/*
#define CAL_POWER_CC 	(1000.0 / (VOLTAGE_TRANSFER_FUNCTION * CURRENT_TRANSFER_FUNCTION * ADE9000_WATT_FULL_SCALE_CODES * 2.0f))			
#define CAL_ENERGY_CC	(1000000.0 / (VOLTAGE_TRANSFER_FUNCTION * CURRENT_TRANSFER_FUNCTION * ADE9000_WATT_FULL_SCALE_CODES * 2.0f * 8000.0f * 3600.0f *0.0001220703125f))	
*/

//400:5 Outgoing Channel One
//#define OG_CAL_IRMS_CC		15.18195573f
//#define OG_CAL_VRMS_CC		10.74705975f						
//#define OG_CAL_POWER_CC 	21.89915027f			
//#define OG_CAL_ENERGY_CC	6.229091633f

//800:5 Incoming Channel One
#define IC_CAL_IRMS_CC		30.36391145f
#define IC_CAL_VRMS_CC		10.74705975f						
#define IC_CAL_POWER_CC 	43.79830055f			
#define IC_CAL_ENERGY_CC	12.45818327f

//500:5 Incoming Channel One
// #define IC_CAL_IRMS_CC		1.8977444f
// #define IC_CAL_VRMS_CC		10.74705975f						
// #define IC_CAL_POWER_CC 	43.79830055f			
// #define IC_CAL_ENERGY_CC	12.45818327f


//400:5 Outgoing Channel One unit2
#define OG_CAL_IRMS_CC		38.33443821f
#define OG_CAL_VRMS_CC		10.74705975f						
#define OG_CAL_POWER_CC 	55.29535444f			
#define OG_CAL_ENERGY_CC	15.72845637f

#endif