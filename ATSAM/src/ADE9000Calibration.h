#ifndef ADE9000CALIBRATION_H
#define ADE9000CALIBRATION_H

#define PI 3.14159

/* Select CT ratio to be used as default */
#define CT_400_5	0
#define CT_500_5	1
#define CT_800_5	0

/*Full scale Codes referred from Datasheet.Respective digital codes are produced when ADC inputs are at full scale. Donot Change. */
#define ADE9000_RMS_FULL_SCALE_CODES       52702092				// used for IRMS and VRMS, (also Fast RMS 1/2 and 10/12 cycle RMS)
#define ADE9000_WATT_FULL_SCALE_CODES      20694066				// used for WATT, VAR and VA
#define ADE9000_RESAMPLED_FULL_SCALE_CODES 18196				// used for resampled waveforms
#define ADE9000_PCF_FULL_SCALE_CODES       74532013				// used for 

#define R_BURDEN 			(0.002*2)            //double the pcb value (0.002 Ohm) for 1A CTs
#define FULL_SCALE_VOLTAGE	1000.0
#ifdef CT_400_5
	#define CT_PRI 400.0f      						 //primary current
	#define CT_SEC 5.00f       						 //secondary current
#elif CT_500_5
	#define CT_PRI 500.0f      						 //primary current
	#define CT_SEC 5.00f       						 //secondary current
#elif CT_800_5
	#define CT_PRI 800.0f      						 //primary current
	#define CT_SEC 5.00f       						 //secondary current
#else
    #error "No valid CT ratio selected! Please define one CT_xxx_5 as 1."
#endif

#define NOMINAL_INPUT_VOLTAGE 		230       // Nominal Calibrating RMS Voltage on all phases (Vrms)
#define NOMINAL_INPUT_CURRENT 		10        // Nominal Calibrating RMS Current on all phases (Irms)
#define INPUT_FREQUENCY 	  		50        // Nominal Calibrating frequency. Used to calculate Omega for phase calibration.Ensure ACCMODE register is set to correct frequency
#define CALIBRATION_ANGLE_DEGREES 	60		  // Calibration Angle in Degrees.Used for phase calibration

#define ACCUMULATION_TIME 			1 		  // EP_CFG = 0x0011 and EGY_TIME = 7999 (1 sec accumulation)
#define R_HIGH 						800000.0f	// Voltage divider high side resistor (Ohms)
#define R_LOW 						1000.0f		// Voltage divider low side resistor (Ohms)

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
									Current Transfer Function
*****************************************************************************************************************
It is the voltage at the ADC input pins per input current(A) (Volts/Amp)
E.g: For a current output current transformer with Turns Ratio of 400:5. Burden resistor on board = .656*2=11.312 Ohm.
Current transfer function= 1/2500*Total Burden = 5/400*11.312= 0.1414
****************************************************************************************************************/
#define CURRENT_TRANSFER_FUNCTION ((R_BURDEN) / (CT_PRI / CT_SEC))   //The RMS voltage at the ADC input pins per input RMS current (V/A).)

/****************************************************************************************************************
									Voltage Transfer Function
*****************************************************************************************************************
It is the voltage at the ADC input pins per input Voltage(V)(Volts/Volts)
E.g. The defaul atteunation factor on board is 801. 
Voltage transfer function = 1/801= 0.001248 ~=0.00125
****************************************************************************************************************/ 
#define VOLTAGE_TRANSFER_FUNCTION  (R_LOW / (R_HIGH + R_LOW)) //The RMS voltage at the ADC input pins per input RMS voltage (V/V) 


/****************************************************************************************************************
									Conversion Constants
*****************************************************************************************************************
If calibration is done with the the calibration sketch, the following conversion constants apply:
If CURRENT_TRANSFER_FUNCTION or VOLTAGE_TRANSFER_FUNCTION are changed, update the conversion constants

CAL_IRMS_CC in uA/code 				= 10^6/(CURRENT_TRANSFER_FUNCTION*ADE9000_RMS_FULL_SCALE_CODES*SQRT(2)) 
CAL_VRMS_CC in uV/code 				= 10^6/(VOLTAGE_TRANSFER_FUNCTION*ADE9000_RMS_FULL_SCALE_CODES*SQRT(2))
CAL_POWER_CC in mW/code 			= 10^3/(CURRENT_TRANSFER_FUNCTION*VOLTAGE_TRANSFER_FUNCTION*ADE9000_WATT_FULL_SCALE_CODES*2)
CAL_ENERGY_CC in uWhr/xTHR_HI code 	= 10^6/(CURRENT_TRANSFER_FUNCTION*VOLTAGE_TRANSFER_FUNCTION*ADE9000_WATT_FULL_SCALE_CODES*2*8000*3600*2^-13)

CAL_IRMS_CC in uA/code 				= (I_nominal*10^6)/(Ifsp*full_scale_rms_codes) 
CAL_VRMS_CC in uV/code 				= (V_nominal*10^6)/(Vfsp*full_scale_rms_codes)
CAL_POWER_CC in mW/code 			= (V_nominal*I_nominal*10^3)/(Vfsp*Ifsp*full_scale_watt_codes)
CAL_ENERGY_CC in uWhr/xTHR_HI code 	= (V_nominal*I_nominal*10^6)/(Vfsp*Ifsp*full_scale_watt_codes*3600*accumulation_time*8000*2^-13)

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
#define ADC_FULL_SCALE_VOLTAGE		0.707f			//Full scale voltage at ADC input pins with gain 1 (Volts)
#define I_FSP			((CURRENT_TRANSFER_FUNCTION * NOMINAL_INPUT_CURRENT)/(ADC_FULL_SCALE_VOLTAGE))
#define V_FSP			((VOLTAGE_TRANSFER_FUNCTION * NOMINAL_INPUT_VOLTAGE)/(ADC_FULL_SCALE_VOLTAGE))

#define CAL_IRMS_CC		((NOMINAL_INPUT_CURRENT * 1000000)/(I_FSP * ADE9000_RMS_FULL_SCALE_CODES))									// (uA/code)
#define CAL_VRMS_CC		((NOMINAL_INPUT_VOLTAGE * 1000000)/(V_FSP * ADE9000_RMS_FULL_SCALE_CODES))									// (uV/code)
#define CAL_POWER_CC	((NOMINAL_INPUT_CURRENT * NOMINAL_INPUT_VOLTAGE * 1000)/(I_FSP * V_FSP * ADE9000_WATT_FULL_SCALE_CODES))	// (mW/code) Applicable for Active, reactive and apparent power
#define CAL_ENERGY_CC(x)	((NOMINAL_INPUT_CURRENT * NOMINAL_INPUT_VOLTAGE * 1000000.0f)/(I_FSP * V_FSP * ADE9000_WATT_FULL_SCALE_CODES * 3600.0f * CALIBRATION_ACC_TIME * 8000.0f * 0.000122070313f))	// (uWhr/xTHR_HI code)Applicable for Active, reactive and apparent energy


/***************************************************************************************************************
		  Pre-Calculated Values for different CT Ratios using nominal voltage/current settings from above
****************************************************************************************************************/
#ifdef CT_400_5
//400:5 Outgoing Channel One
#define OG_CAL_IRMS_CC		268.300551f
#define OG_CAL_VRMS_CC		10.745439f						
#define OG_CAL_POWER_CC 	386.950637f			
#define OG_CAL_ENERGY_CC	110.065956f
#elif CT_500_5
//500:5 Incoming Channel One
#define IC_CAL_IRMS_CC		335.375689f
#define IC_CAL_VRMS_CC		10.745439f						
#define IC_CAL_POWER_CC 	483.688297f			
#define IC_CAL_ENERGY_CC	137.582443f
#elif CT_800_5
//800:5 Incoming Channel One
#define IC_CAL_IRMS_CC		536.601103f
#define IC_CAL_VRMS_CC		10.745439f	
#define IC_CAL_POWER_CC 	773.901275f			
#define IC_CAL_ENERGY_CC	220.131912f
#else
    #error "No valid CT ratio selected! Please define one CT_xxx_5 as 1."
#endif

#endif /* ADE9000CALIBRATION_H */