/**
 * \file
 *
 * \brief Empty user application template
 *
 */

/**
 * \mainpage User Application template doxygen documentation
 *
 * \par Empty user application template
 *
 * Bare minimum empty user application template
 *
 * \par Content
 *
 * -# Include the ASF header files (through asf.h)
 * -# Minimal main function that starts with a call to system_init()
 * -# "Insert application code here" comment
 *
 */

/*
 * Include header files for all drivers that have been imported from
 * Atmel Software Framework (ASF).
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */
#include <asf.h>
#include <port.h>
#include "ade9000regs.h"

#define LED_1	PIN_PA25
#define LED_2	PIN_PA28
#define LED_3	PIN_PB23
#define LED_4	PIN_PA27

int main(void)
{
	system_init(); // Initializes clock and ASF drivers
	delay_init();

	// Configure PA25 as output
	struct port_config pin_conf;
	port_get_config_defaults(&pin_conf); // Load default settings

	pin_conf.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(LED_1, &pin_conf); // Apply config to PA25
	port_pin_set_config(LED_2, &pin_conf); // Apply config to PA25
	port_pin_set_config(LED_3, &pin_conf); // Apply config to PA25
	port_pin_set_config(LED_4, &pin_conf); // Apply config to PA25
	
	while (1)
	{
		port_pin_toggle_output_level(LED_1);
		delay_ms(100);
		port_pin_toggle_output_level(LED_2);
		delay_ms(100);
		port_pin_toggle_output_level(LED_3);
		delay_ms(100);
		port_pin_toggle_output_level(LED_4);
		delay_ms(100);
	}
}