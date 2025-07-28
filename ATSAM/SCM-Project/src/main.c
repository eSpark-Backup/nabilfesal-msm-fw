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
#include <stdio.h>
#include <spi.h>
#include <string.h>
#include <usart.h>
#include <string.h>
//#include <uart_config.h>
#include "ade9000regs.h"
#include "ade9000.h"

#define LED_1	PIN_PA25
#define LED_2	PIN_PA28
#define LED_3	PIN_PB23
#define LED_4	PIN_PA27
uint8_t led[4] = {LED_1, LED_2, LED_3, LED_4};

#define check_id 0x100000
#define target_id 0x00100000

#define SPI_BAUDRATE 1000000
#define ADE9000_READ_CMD  0x01  // Read Command for ADE9000
#define ADE9000_WRITE_CMD 0x00  // Write Command for ADE9000
#define ADE9000_REG_ADDRESS 0x03 // example address to read data

uint32_t ade9000_data[12];
uint16_t temp_offset[12];
uint16_t temp_gain[12];
uint16_t temp_result[12];
uint32_t ade9000_dataa;


struct usart_config usart_configuration;
struct usart_module usart_configuration_module; 

bool led_1_status = true,led_2_status,led_3_status,led_4_status;
uint16_t chip_address = 10;


void debug_output(uint32_t data);

static void configure_usart(void) {
	 usart_get_config_defaults(&usart_configuration);
	struct usart_config options = {
		.baudrate = 115200,
		.character_size = USART_CHARACTER_SIZE_8BIT,
		.parity = USART_PARITY_NONE,
		.stopbits = USART_STOPBITS_1,
	};
	usart_serial_init(&usart_configuration ,SERCOM1 , &options);
	
}


void debug_output(uint32_t data) {
    printf("ade9000_data: 0x%08lX\n", data);
}

int main(void)
{
	system_init(); // Initializes clock and ASF drivers
	delay_init();

	//usart_get_config_defaults(&usart_configuration);
	configure_usart();      // Initialize USART for debugging
	configure_spi();        // Initialize SPI for communication with ADE9000
	//stdio_serial_init(&usart_instance, CONF_STDIO_USART_MODULE, &config_usart);
	
	struct port_config pin_conf;
	port_get_config_defaults(&pin_conf); // Load default settings

	pin_conf.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(LED_1, &pin_conf); // Apply config to PA25
	port_pin_set_config(LED_2, &pin_conf); // Apply config to PA28
	port_pin_set_config(LED_3, &pin_conf); // Apply config to PA23
	port_pin_set_config(LED_4, &pin_conf); // Apply config to PA27
	
	// config_ade9000();
	
	port_pin_set_config(ADE9000_CSS12_PIN, &pin_conf);
	port_pin_set_config(ADE9000_CSS11_PIN, &pin_conf);
	port_pin_set_config(ADE9000_CSS10_PIN, &pin_conf);
	port_pin_set_config(ADE9000_CSS9_PIN, &pin_conf);
	port_pin_set_config(ADE9000_CSS8_PIN, &pin_conf);
	port_pin_set_config(ADE9000_CSS7_PIN, &pin_conf);
	port_pin_set_config(ADE9000_CSS6_PIN, &pin_conf);
	port_pin_set_config(ADE9000_CSS5_PIN, &pin_conf);
	port_pin_set_config(ADE9000_CSS4_PIN, &pin_conf);
	port_pin_set_config(ADE9000_CSS3_PIN, &pin_conf);
	port_pin_set_config(ADE9000_CSS2_PIN, &pin_conf);
	port_pin_set_config(ADE9000_CSS1_PIN, &pin_conf);
	ade9000_deselect(ADE9000_CSS12_PIN);
	ade9000_deselect(ADE9000_CSS11_PIN);
	ade9000_deselect(ADE9000_CSS10_PIN);
	ade9000_deselect(ADE9000_CSS9_PIN);
	ade9000_deselect(ADE9000_CSS8_PIN);
	ade9000_deselect(ADE9000_CSS7_PIN);
	ade9000_deselect(ADE9000_CSS6_PIN);
	ade9000_deselect(ADE9000_CSS5_PIN);
	ade9000_deselect(ADE9000_CSS4_PIN);
	ade9000_deselect(ADE9000_CSS3_PIN);
	ade9000_deselect(ADE9000_CSS2_PIN);
	ade9000_deselect(ADE9000_CSS1_PIN);

	while (1)
	{	
		/*
		port_pin_toggle_output_level(LED_1);
		delay_ms(100);
		port_pin_toggle_output_level(LED_2);
		delay_ms(100);
		port_pin_toggle_output_level(LED_3);
		delay_ms(100);
		port_pin_toggle_output_level(LED_4);
		delay_ms(100);
		*/
		
		//////////////////////////// Read ADE9000 ID /////////////////////////////
		
		for (int i = 0; i < 12; i++){
			//ade9000_data[i] = read_ade9000_data(ade9000_ss_pin[i],0x00000472);	// ADE9000 1 - 12 read address of ID 0x472
			ade9000_data[i] = read_ade9000_data(ade9000_ss_pin[i],ADDR_STATUS0);	// ADE9000 1 - 12 read address of ID 0x472
			ade9000_data[i];
		}
		ade9000_data[0];
		
		for (int i = 0;i < 4; i++){
			if(((ade9000_data[i*3] >> 20) & 0xF) == 0x1 || ((ade9000_data[i*3 + 1] >> 20) & 0xF) == 0x1 || ((ade9000_data[i*3 + 2] >> 20) & 0xF) == 0x1){
				port_pin_set_output_level(led[i], false);
				/*
				ade9000_data[i*3] = ade9000_data[i*3] & check_id;
				ade9000_data[i*3 + 1] = ade9000_data[i*3 + 1] & check_id;
				ade9000_data[i*3 + 2] = ade9000_data[i*3 + 2] & check_id;
				*/
				ade9000_data[i*3];
				ade9000_data[i*3 + 1];
				ade9000_data[i*3 + 2];
				ade9000_data[i*3] = 1;
				ade9000_data[i*3 + 1] = 1;
				ade9000_data[i*3 + 2] = 1;
				ade9000_data[i*3];
				ade9000_data[i*3 + 1];
				ade9000_data[i*3 + 2];
			}
			else{
				port_pin_set_output_level(led[i], true);
			}
		}
		for (int i = 0; i < 12; i++){
			ade9000_data[i] = NULL;	// ADE9000 1 - 12 empty array for new value for testing
			ade9000_data[i];
		}
		
		/*

		ade9000_data[0] = read_ade9000_data(ADE9000_CSS1_PIN,0x472);	// 1 ADE9000
		ade9000_data[0]; // Output Decimal Value: 4294967295 , Hex Value: FFFF FFFF
		
		ade9000_data[1] = read_ade9000_data(ADE9000_CSS2_PIN,0x472);	// 2 ADE9000
		ade9000_data[1]; // Output Decimal Value: 4294967295 , Hex Value: FFFF FFFF
		
		ade9000_data[2] = read_ade9000_data(ADE9000_CSS3_PIN,0x472);	// 3 ADE9000
		ade9000_data[2]; // Output Decimal Value: 4294967295 , Hex Value: FFFF FFFF
		
		ade9000_data[3] = read_ade9000_data(ADE9000_CSS4_PIN,0x472);	// 4 ADE9000
		ade9000_data[3]; // Output Decimal Value: 4294967295 , Hex Value: FFFF FFFF
		
		ade9000_data[4] = read_ade9000_data(ADE9000_CSS5_PIN,0x472);	// 5 ADE9000
		ade9000_data[4]; // Output Decimal Value: 4294967295 , Hex Value: FFFF FFFF
		
		ade9000_dataa = read_ade9000_data(ADE9000_CSS6_PIN,0x472);	// 6 ADE9000
		ade9000_dataa; // Output Decimal Value: 4294967295 , Hex Value: FFFF FFFF
		
		ade9000_dataa = read_ade9000_data(ADE9000_CSS7_PIN,0x472);	// 7 ADE9000
		ade9000_dataa; // Output Decimal Value: 3759147670 , Hex Value: E010 0A96
		
		ade9000_dataa = read_ade9000_data(ADE9000_CSS8_PIN,0x472);	// 8 ADE9000
		ade9000_dataa; // Output Decimal Value: 3759147670 , Hex Value: E010 0A96
				
		ade9000_dataa = read_ade9000_data(ADE9000_CSS9_PIN,0x472);	// 9 ADE9000
		ade9000_dataa; // Output Decimal Value: 3759147670 , Hex Value: E010 0A96
		
		ade9000_dataa = read_ade9000_data(ADE9000_CSS10_PIN,0x472);	// 10 ADE9000
		ade9000_dataa; // Output Decimal Value: 3759147671 , Hex Value: E010 0A97
		
		ade9000_dataa = read_ade9000_data(ADE9000_CSS11_PIN,0x472);	// 11 ADE9000
		ade9000_dataa; // Output Decimal Value: 3759147669 , Hex Value: E010 0A95
		
		ade9000_dataa = read_ade9000_data(ADE9000_CSS12_PIN,0x472);	// 12 ADE9000
		ade9000_dataa; // Output Decimal Value: 3759147669 , Hex Value: E010 0A95
		*/
		
		/*
		*/

/*
		1110 0000 0001 0000 1001 0000 0000 0000
		x         0001 0000 0000 0000 0000 0000 AND Operation to check ID
		---------------------------------------
		0000 0000 0001 0000 0000 0000 0000 0000		
		---------------------------------------
*/
		
		/////////////////////////////////////////////////////////////////////////
		for (int i = 0; i < 12; i++){
			ade9000_data[i] = read_ade9000_data(ade9000_ss_pin[i],0x411);	// ADE9000 1 - 12 read address 0x411
			ade9000_data[i];
		}
		ade9000_data[0];
		for (int i = 0; i < 12; i++){
			ade9000_data[i] = NULL;	// ADE9000 1 - 12 empty array for new value for testing
			ade9000_data[i];
		}
		for (int i = 0; i < 12; i++){
			ade9000_data[i] = read_ade9000_data(ade9000_ss_pin[i],0x409);	// ADE9000 1 - 12 read address 0x409
			ade9000_data[i];
		}
		ade9000_data[0];
		for (int i = 0; i < 12; i++){
			ade9000_data[i] = NULL;	// ADE9000 1 - 12 empty array for new value for testing
			ade9000_data[i];
		}
		for (int i = 0; i < 12; i++){
			ade9000_data[i] = read_ade9000_data(ade9000_ss_pin[i],0x474);	// ADE9000 1 - 12 read address 0x474 TEMP_TRIM
			// [31:16] TEMP_OFFSET Offset of temperature sensor, calculated during the manufacturing process. 0xX R
			// [15:0] TEMP_GAIN Gain of temperature sensor, calculated during the manufacturing process. 0xX Read
			temp_offset[i] = (uint16_t)ade9000_data[i] >> 16;
			temp_gain[i] = (uint16_t)ade9000_data[i];
			ade9000_data[i];
		}
		ade9000_data[0];
		for (int i = 0; i < 12; i++){
			ade9000_data[i] = NULL;	// ADE9000 1 - 12 empty array for new value for testing
			ade9000_data[i];
		}
		for (int i = 0; i < 12; i++){
			ade9000_data[i] = read_ade9000_data(ade9000_ss_pin[i],0x4B7);	// ADE9000 1 - 12 read address 0x4B7
			ade9000_data[i];
		}
		ade9000_data[0];
		/*
		ade9000_data[0] = read_ade9000_data(ADE9000_CSS1_PIN,0x411);	// 1 ADE9000
		ade9000_data[0]; // Output Decimal Value: 4294967295 , Hex Value: FFFF FFFF
				
		ade9000_data[1] = read_ade9000_data(ADE9000_CSS2_PIN,0x411);	// 2 ADE9000
		ade9000_data[1]; // Output Decimal Value: 4294967295 , Hex Value: FFFF FFFF
				
		ade9000_data[4] = read_ade9000_data(ADE9000_CSS5_PIN,0x411);	// 5 ADE9000
		ade9000_data[4]; // Output Decimal Value: 4294967295 , Hex Value: FFFF FFFF
				
		ade9000_data[5] = read_ade9000_data(ADE9000_CSS6_PIN,0x411);	// 6 ADE9000
		ade9000_data[5]; // Output Decimal Value: 4294967295 , Hex Value: FFFF FFFF
				
		ade9000_data[7] = read_ade9000_data(ADE9000_CSS8_PIN,0x411);	// 8 ADE9000
		ade9000_data[7]; // Output Decimal Value: 8388607 , Hex Value: 7F FFFF
				
		ade9000_data[8] = read_ade9000_data(ADE9000_CSS9_PIN,0x411);	// 9 ADE9000
		ade9000_data[8]; // Output Decimal Value: 8388607 , Hex Value: 7F FFFF
				
		ade9000_data[9] = read_ade9000_data(ADE9000_CSS10_PIN,0x411);	// 10 ADE9000
		ade9000_data[9]; // Output Decimal Value: 8388607 , Hex Value: 7F FFFF
				
		ade9000_data[10] = read_ade9000_data(ADE9000_CSS11_PIN,0x411);	// 11 ADE9000
		ade9000_data[10]; // Output Decimal Value: 8388607 , Hex Value: 7F FFFF
				
		ade9000_data[11] = read_ade9000_data(ADE9000_CSS12_PIN,0x411);	// 12 ADE9000
		ade9000_data[11]; // Output Decimal Value: 8388607 , Hex Value: 7F FFFF
		*/
		/////////////////////////////////////////////////////////////////////////
		for (int i = 0; i < 12; i++){
			ade9000_data[i] = NULL;	// ADE9000 1 - 12 empty array for new value for testing
			ade9000_data[i];
		}
		
		delay_ms(100);
		delay_ms(1000);
	}
}








//struct spi_module spi_master_instance;

/*
int main(void)
{
	system_init(); // Initializes clock and ASF drivers
	delay_init();

	usart_get_config_defaults(&usart_configuration);
	configure_spi();       // Set up SPI communication

	//stdio_serial_init(&usart_instance, CONF_STDIO_USART_MODULE, &config_usart);

	chip_address = test_communication();  // Read the chip ID and print it

	
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
		led_1_status = port_pin_get_output_level(LED_1);
		//printf("World");
		led_2_status = port_pin_get_output_level(LED_2);
		led_3_status = port_pin_get_output_level(LED_3);
		led_4_status = port_pin_get_output_level(LED_4);
		//printf("LED 1: %d, LED 2: %d, LED 3: %d, LED 4: %d\n",
		led_1_status, led_2_status, led_3_status, led_4_status);
		//printf("Chip Address: %d\n", chip_address);
		delay_ms(1000);
	}
}
*/


//#include <asf.h>  // ASF Header (Atmel Software Framework)
//#include <stdio.h>
/*
#include <string.h>

#define SPI_BAUDRATE 1000000
#define ADE9000_READ_CMD  0x01  // Read Command for ADE9000
#define ADE9000_WRITE_CMD 0x00  // Write Command for ADE9000

#define ADE9000_REG_ADDRESS 0x03 // example address to read data

void debug_output(uint32_t data);

static void configure_usart(void) {
	//////
	usart_configuration options = {
		.baudrate = 115200,
		.character_size = USART_CHARACTER_SIZE_8BIT,
		.parity = USART_PARITY_NONE,
		.stopbits = USART_STOPBITS_1,
	};

	usart_serial_init(&usart_configuration, &options);  // Initialize USART with specified settings
	//////	
	 usart_get_config_defaults(&usart_configuration);
	struct usart_config options = {
		.baudrate = 115200,
		.character_size = USART_CHARACTER_SIZE_8BIT,
		.parity = USART_PARITY_NONE,
		.stopbits = USART_STOPBITS_1,
	};
	usart_serial_init(&usart_configuration ,SERCOM1 , &options);
	
}
/*
/*
void configure_usart() {
    struct usart_config config_usart;
    usart_get_config_defaults(&config_usart);

    // Set baud rate, data bits, etc.
    config_usart.baudrate = 9600;
    config_usart.char_size = USART_CHAR_SIZE_8_BITS;
    config_usart.parity = USART_PARITY_NONE;
    config_usart.stop_bits = USART_STOP_BITS_1;

    // Initialize USART
    stdio_serial_init(&usart_instance, CONF_STDIO_USART_MODULE, &config_usart);
}
*/
/*
static void configure_spi(void) {

	struct spi_config spi_device_cfg;
	//spi_device_configure(&spi_device_cfg);  // SPI configuration
	//spi_get_config_defaults(&config_spi);
	spi_get_config_defaults(&spi_device_cfg);
	spi_init(&spi_module, &spi_device_cfg);       // Initialize SPI interface with configuration
	spi_enable(&spi_module);                   // Enable SPI peripheral

	
	struct spi_config spi_device_cfg;
	spi_get_config_defaults(&spi_device_cfg);
	spi_init(&SERCOM0, &spi_device_cfg);  // Ensure SERCOM0 or the appropriate SPI module is used
	spi_enable(&SERCOM0);
	
}
*/
/*
uint32_t read_ade9000(uint8_t reg_address) {

	uint8_t tx_buffer[4];  // Transmit buffer for SPI
	uint8_t rx_buffer[4];  // Receive buffer for SPI
	uint32_t value = 0;

	tx_buffer[0] = ADE9000_READ_CMD;  // Command byte (read)
	tx_buffer[1] = reg_address;       // Register address to read from
	tx_buffer[2] = 0x00;              // Dummy byte (to shift out data)
	tx_buffer[3] = 0x00;              // Dummy byte (to shift out data)

	spi_transceive(&SPIM, tx_buffer, 4, rx_buffer, 4);
	
	value = (rx_buffer[1] << 16) | (rx_buffer[2] << 8) | rx_buffer[3];

	return value;
	
	
	uint8_t tx_buffer[4] = {ADE9000_READ_CMD, reg_address, 0x00, 0x00};
	uint8_t rx_buffer[4];
	ade9000_select(ADE9000_CSS10_PIN);
	spi_write_buffer_wait(&spi_instance, tx, 2);
	spi_read_buffer_wait(&spi_instance, rx, 2, 0x00);  // Receive 2 bytes of data
	ade9000_deselect(ADE9000_CSS10_PIN);
	return (rx_buffer[1] << 16) | (rx_buffer[2] << 8) | rx_buffer[3];
	
}
*/
/*
void debug_output(uint32_t data) {
	char buffer[100];
	uint32_t USART[100];
	snprintf(buffer, sizeof(buffer), "Received Data: %lu Data Complete\r\n", data);
	usart_serial_write_packet(&USART, (uint8_t*)buffer, strlen(buffer));  // Write data to USART terminal
}
*/

/*
int main(void) {
	system_init();          // Initialize system clock
	board_init();           // Initialize the board
	configure_usart();      // Initialize USART for debugging
	configure_spi();        // Initialize SPI for communication with ADE9000

	struct port_config pin_conf;
	port_get_config_defaults(&pin_conf); // Load default settings

	pin_conf.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(LED_1, &pin_conf); // Apply config to PA25
	port_pin_set_config(LED_2, &pin_conf); // Apply config to PA25
	port_pin_set_config(LED_3, &pin_conf); // Apply config to PA25
	port_pin_set_config(LED_4, &pin_conf); // Apply config to PA25

	uint32_t ade9000_data;

	while (1) {
		port_pin_toggle_output_level(LED_1);
		delay_ms(100);
		port_pin_toggle_output_level(LED_2);
		delay_ms(100);
		ade9000_data = ade9000_read(ADE9000_REG_ADDRESS,0); // read data from 0x03

		debug_output(ade9000_data);
		
		delay_ms(100);
		port_pin_toggle_output_level(LED_3);
		delay_ms(100);
		port_pin_toggle_output_level(LED_4);
		delay_ms(1000);  // Delay for 1 second
	}
}
*/
/*
void debug_output(uint32_t data) {
    printf("ade9000_data: 0x%08lX\n", data);
}

int main(void)
{
	system_init(); // Initializes clock and ASF drivers
	delay_init();

	//usart_get_config_defaults(&usart_configuration);
	configure_usart();      // Initialize USART for debugging
	configure_spi();        // Initialize SPI for communication with ADE9000
	//stdio_serial_init(&usart_instance, CONF_STDIO_USART_MODULE, &config_usart);
	
	struct port_config pin_conf;
	port_get_config_defaults(&pin_conf); // Load default settings

	pin_conf.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(LED_1, &pin_conf); // Apply config to PA25
	port_pin_set_config(LED_2, &pin_conf); // Apply config to PA25
	port_pin_set_config(LED_3, &pin_conf); // Apply config to PA25
	port_pin_set_config(LED_4, &pin_conf); // Apply config to PA25
	
	config_ade9000();
	/*
	port_pin_set_config(ADE9000_CSS12_PIN, &pin_conf);
	port_pin_set_config(ADE9000_CSS11_PIN, &pin_conf);
	port_pin_set_config(ADE9000_CSS10_PIN, &pin_conf);
	port_pin_set_config(ADE9000_CSS9_PIN, &pin_conf);
	port_pin_set_config(ADE9000_CSS8_PIN, &pin_conf);
	port_pin_set_config(ADE9000_CSS7_PIN, &pin_conf);
	port_pin_set_config(ADE9000_CSS6_PIN, &pin_conf);
	port_pin_set_config(ADE9000_CSS5_PIN, &pin_conf);
	port_pin_set_config(ADE9000_CSS4_PIN, &pin_conf);
	port_pin_set_config(ADE9000_CSS3_PIN, &pin_conf);
	port_pin_set_config(ADE9000_CSS2_PIN, &pin_conf);
	port_pin_set_config(ADE9000_CSS1_PIN, &pin_conf);
	ade9000_deselect(ADE9000_CSS12_PIN);
	ade9000_deselect(ADE9000_CSS11_PIN);
	ade9000_deselect(ADE9000_CSS10_PIN);
	ade9000_deselect(ADE9000_CSS9_PIN);
	ade9000_deselect(ADE9000_CSS8_PIN);
	ade9000_deselect(ADE9000_CSS7_PIN);
	ade9000_deselect(ADE9000_CSS6_PIN);
	ade9000_deselect(ADE9000_CSS5_PIN);
	ade9000_deselect(ADE9000_CSS4_PIN);
	ade9000_deselect(ADE9000_CSS3_PIN);
	ade9000_deselect(ADE9000_CSS2_PIN);
	ade9000_deselect(ADE9000_CSS1_PIN);
	// ADE9000_CSS10_PIN;
	// ADE9000_CSS11_PIN;
	// ADE9000_CSS9_PIN;
	// ADE9000_CSS8_PIN;
*/

	
	// volatile uint32_t ade9000_data[12];
/*
	while (1)
	{
		*/
		/*
		port_pin_toggle_output_level(LED_1);
		delay_ms(100);
		port_pin_toggle_output_level(LED_2);
		delay_ms(100);
		port_pin_toggle_output_level(LED_3);
		delay_ms(100);
		port_pin_toggle_output_level(LED_4);
		delay_ms(100);
		*/
		
		///////////////////////////////////////////////////////////////////////////////////////////////////////////
		/*
		uint8_t tx_buf[2] = {0};
		uint8_t rx_buf[2] = {0};
		uint8_t rx_buf_2[4] = {0};
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////////
		/*
		ade9000_data = 20;
		ade9000_data;
		//	0x472 + 0b1000(Read) = (0100 0111 0010) 1000
		tx_buf[0] = 0b01001111;
		tx_buf[1] = 0b00101000;

		port_pin_set_output_level(ADE9000_CSS8_PIN, false);
		spi_transceive_buffer_wait(&spi_instance, tx_buf, rx_buf, 2);
		port_pin_set_output_level(ADE9000_CSS8_PIN, true);
		
		// Combine response bytes
		ade9000_data = ((uint16_t)rx_buf[0] << 8) | rx_buf[1];
		ade9000_data;
		
		ade9000_data = 20;
		ade9000_data;
		
		//	0x472 + 0b1000(Read) = (0100 0111 0010) 1000
		tx_buf[0] = 0b01000111;
		tx_buf[1] = 0b00101000;

		port_pin_set_output_level(ADE9000_CSS8_PIN, false);
		spi_transceive_buffer_wait(&spi_instance, tx_buf, rx_buf, 2);
		port_pin_set_output_level(ADE9000_CSS8_PIN, true);
		
		// Combine response bytes
		ade9000_data = ((uint16_t)rx_buf[0] << 8) | rx_buf[1];
		ade9000_data;
		*/
		
		/*
		ade9000_data = 20;
		ade9000_data;
		rx_buf_2[0] = NULL;
		rx_buf_2[1] = NULL;
		rx_buf_2[2] = NULL;
		rx_buf_2[3] = NULL;
		
		// 0100 0111 0010
		//	0x472 + 0b1000(Read) = (0100 0111 0010) 1000
		tx_buf[0] = 0b01000111;
		tx_buf[1] = 0b00101000;
		
		uint16_t register_address = 0x472;
		
		tx_buf[0] = (register_address >> 8 ) & 0xFF;
		tx_buf[1] = (register_address << 4) | 0x8;

		tx_buf[1];
		port_pin_set_output_level(ADE9000_CSS8_PIN, false); // select ade 9000
		spi_write_buffer_wait(&spi_instance, tx_buf, 2);
		spi_read_buffer_wait(&spi_instance, rx_buf_2, 4, 0x00);  // Receive 2 bytes of data
		port_pin_set_output_level(ADE9000_CSS8_PIN, true); // deselect ade 9000
		
		// Combine response bytes
		// ade9000_data = (rx_buf_2[1] << 16) | (rx_buf_2[2] << 8) | rx_buf_2[3];
		//ade9000_data = ((uint32_t)rx_buf_2[0] << 24) |
          //              ((uint32_t)rx_buf_2[1] << 16) |
            //            ((uint32_t)rx_buf_2[2] << 8)  |
              //          ((uint32_t)rx_buf_2[3]);
		//ade9000_data;
		
		ade9000_data = 20;
		ade9000_data;
		rx_buf_2[0] = NULL;
		rx_buf_2[1] = NULL;
		rx_buf_2[2] = NULL;
		rx_buf_2[3] = NULL;
		
		//	0x472 + 0b1000(Read) = (0100 0111 0010) 1000
		tx_buf[0] = 0x47;
		tx_buf[1] = 0x28;

		port_pin_set_output_level(ADE9000_CSS8_PIN, false);
		spi_write_buffer_wait(&spi_instance, tx_buf, 2);
		spi_read_buffer_wait(&spi_instance, rx_buf_2, 4, 0x00);  // Receive 2 bytes of data
		port_pin_set_output_level(ADE9000_CSS8_PIN, true);
		
		// Combine response bytes
		// ade9000_data = (rx_buf_2[1] << 16) | (rx_buf_2[2] << 8) | rx_buf_2[3];
		ade9000_data = ((uint32_t)rx_buf_2[0] << 24) |
                        ((uint32_t)rx_buf_2[1] << 16) |
                        ((uint32_t)rx_buf_2[2] << 8)  |
                        ((uint32_t)rx_buf_2[3]);
		ade9000_data;
		*/
		/*
		ade9000_data = 20;
		ade9000_data;
		*/
		/*
		rx_buf_2[0] = NULL;
		rx_buf_2[1] = NULL;
		rx_buf_2[2] = NULL;
		rx_buf_2[3] = NULL;
		
		//	0x411 + 0b1000(Read)
		tx_buf[0] = 0x41; // 0100 0001
		tx_buf[1] = 0x18; // 0001 1000 
		
		uint16_t  register_address = 0x411; // 0100 0001 0001
		
		tx_buf[0] = (register_address >> 4 ) & 0xFF; // 0100 0001
		tx_buf[1] = (register_address << 4) | 0x8; // 0001 1000

		port_pin_set_output_level(ADE9000_CSS8_PIN, false);
		spi_write_buffer_wait(&spi_instance, tx_buf, 2);
		spi_read_buffer_wait(&spi_instance, rx_buf_2, 4, 0x00);  // Receive 2 bytes of data
		port_pin_set_output_level(ADE9000_CSS8_PIN, true);
		
		// Combine response bytes
		// ade9000_data = (rx_buf_2[1] << 16) | (rx_buf_2[2] << 8) | rx_buf_2[3];
		ade9000_data = ((uint32_t)rx_buf_2[0] << 24) |
                        ((uint32_t)rx_buf_2[1] << 16) |
                        ((uint32_t)rx_buf_2[2] << 8)  |
                        ((uint32_t)rx_buf_2[3]);
		ade9000_data;
		
		ade9000_data = 20;
		ade9000_data;
		rx_buf_2[0] = NULL;
		rx_buf_2[1] = NULL;
		rx_buf_2[2] = NULL;
		rx_buf_2[3] = NULL;
		
		//	0x411 + 0b1000(Read)
		tx_buf[0] = 0x41;
		tx_buf[1] = 0x18;
		
		port_pin_set_output_level(ADE9000_CSS8_PIN, false);
		spi_write_buffer_wait(&spi_instance, tx_buf, 2);
		spi_read_buffer_wait(&spi_instance, rx_buf_2, 4, 0x00);  // Receive 2 bytes of data
		port_pin_set_output_level(ADE9000_CSS8_PIN, true);
		
		// Combine response bytes
		// ade9000_data = (rx_buf_2[1] << 16) | (rx_buf_2[2] << 8) | rx_buf_2[3];
		ade9000_data = ((uint32_t)rx_buf_2[0] << 24) |
                        ((uint32_t)rx_buf_2[1] << 16) |
                        ((uint32_t)rx_buf_2[2] << 8)  |
                        ((uint32_t)rx_buf_2[3]);
		ade9000_data;
		*/
		
		
		//////////////////////////// Read ADE9000 ID /////////////////////////////
		/*
		for (int i = 0; i < 12; i++){
			ade9000_data[i] = read_ade9000_data(ade9000_ss_pin[i],0x472);	// ADE9000 1 - 12
			ade9000_data[i];
		}
		
		for (int i = 0;i < 4; i++){
			if(ade9000_data[i*3] & check_id == target_id || ade9000_data[i*3 + 1] & check_id == target_id || ade9000_data[i*3 + 2] & check_id == target_id){
				port_pin_set_output_level(led[i], LOW);
			}
			else{
				port_pin_set_output_level(led[i], HIGH);
			}
		}
		*/
		
		/*

		ade9000_data[0] = read_ade9000_data(ADE9000_CSS1_PIN,0x472);	// 1 ADE9000
		ade9000_data[0]; // Output Decimal Value: 4294967295 , Hex Value: FFFF FFFF
		
		ade9000_data[1] = read_ade9000_data(ADE9000_CSS2_PIN,0x472);	// 2 ADE9000
		ade9000_data[1]; // Output Decimal Value: 4294967295 , Hex Value: FFFF FFFF
		
		ade9000_data[2] = read_ade9000_data(ADE9000_CSS3_PIN,0x472);	// 3 ADE9000
		ade9000_data[2]; // Output Decimal Value: 4294967295 , Hex Value: FFFF FFFF
		
		ade9000_data[3] = read_ade9000_data(ADE9000_CSS4_PIN,0x472);	// 4 ADE9000
		ade9000_data[3]; // Output Decimal Value: 4294967295 , Hex Value: FFFF FFFF
		
		ade9000_data[4] = read_ade9000_data(ADE9000_CSS5_PIN,0x472);	// 5 ADE9000
		ade9000_data[4]; // Output Decimal Value: 4294967295 , Hex Value: FFFF FFFF
		
		ade9000_data[5] = read_ade9000_data(ADE9000_CSS6_PIN,0x472);	// 6 ADE9000
		ade9000_data[5]; // Output Decimal Value: 4294967295 , Hex Value: FFFF FFFF
		
		ade9000_data[6] = read_ade9000_data(ADE9000_CSS7_PIN,0x472);	// 7 ADE9000
		ade9000_data[6]; // Output Decimal Value: 3759147670 , Hex Value: E010 0A96
		
		ade9000_data[7] = read_ade9000_data(ADE9000_CSS8_PIN,0x472);	// 8 ADE9000
		ade9000_data[7]; // Output Decimal Value: 3759147670 , Hex Value: E010 0A96
				
		ade9000_data[8] = read_ade9000_data(ADE9000_CSS9_PIN,0x472);	// 9 ADE9000
		ade9000_data[8]; // Output Decimal Value: 3759147670 , Hex Value: E010 0A96
		
		ade9000_data[9] = read_ade9000_data(ADE9000_CSS10_PIN,0x472);	// 10 ADE9000
		ade9000_data[9]; // Output Decimal Value: 3759147671 , Hex Value: E010 0A97
		
		ade9000_data[10] = read_ade9000_data(ADE9000_CSS11_PIN,0x472);	// 11 ADE9000
		ade9000_data[10]; // Output Decimal Value: 3759147669 , Hex Value: E010 0A95
		
		ade9000_data[11] = read_ade9000_data(ADE9000_CSS12_PIN,0x472);	// 12 ADE9000
		ade9000_data[11]; // Output Decimal Value: 3759147669 , Hex Value: E010 0A95
		
		*/

/*
		1110 0000 0001 0000 1001 0000 0000 0000
		x         0001 0000 0000 0000 0000 0000 AND Operation to check ID
		---------------------------------------
		0000 0000 0001 0000 0000 0000 0000 0000		
		---------------------------------------
*/
/*
	i = 0; i*3 + 0
	i = 1; i*3 + 1
	i = 2; i*3 + 2
	i = 3; i*3 + 0
	i = 4;
	i = 5;
	i = 6;
	i = 7;
	i = 8;
	i = 9;
	i = 10;
	i = 11;
	i = 12;
*/
/*
		for (int i = 0;i < 4; i++){
			if(ade9000_data[i*3] & check_id == target_id || ade9000_data[i*3 + 1] & check_id == target_id || ade9000_data[i*3 + 2] & check_id == target_id){
				port_pin_set_output_level(led[i], LOW);
			}
			else{
				port_pin_set_output_level(led[i], HIGH);
			}
		}
	*/	
		/*
		if(ade9000_data[0] & 0x100000 == 0x00100000 || ade9000_data[1] & 0x100000 == 0x00100000 || ade9000_data[2] & 0x100000 == 0x00100000){	// LED 1 Turns On
			ade9000_data[0] & 0x100000;
			ade9000_data[1] & 0x100000;
			ade9000_data[2] & 0x100000;
			port_pin_set_output_level(LED_1, LOW);
		}
		else{																																	// LED 1 Turns Off
			port_pin_set_output_level(LED_1, HIGH);
		}
		
		if(ade9000_data[3] & 0x100000 == 0x00100000 || ade9000_data[4] & 0x100000 == 0x00100000 || ade9000_data[5] & 0x100000 == 0x00100000){	// LED 2 Turns On
			ade9000_data[3] & 0x100000;
			ade9000_data[4] & 0x100000;
			ade9000_data[5] & 0x100000;
			port_pin_set_output_level(LED_2, LOW);
		}
		else{																																	// LED 2 Turns Off
			port_pin_set_output_level(LED_2, HIGH);
		}
		
		if(ade9000_data[6] & 0x100000 == 0x00100000 || ade9000_data[7] & 0x100000 == 0x00100000 || ade9000_data[8] & 0x100000 == 0x00100000){	// LED 3 Turns On
			ade9000_data[6] & 0x100000;
			ade9000_data[7] & 0x100000;
			ade9000_data[8] & 0x100000;
			port_pin_set_output_level(LED_3, LOW);
		}
		else{																																	// LED 3 Turns Off
			port_pin_set_output_level(LED_3, HIGH);
		}
		
		if(ade9000_data[9] & 0x100000 == 0x00100000 || ade9000_data[10] & 0x100000 == 0x00100000 || ade9000_data[11] & 0x100000 == 0x00100000){	// LED 4 Turns On
			ade9000_data[9] & 0x100000;
			ade9000_data[10] & 0x100000;
			ade9000_data[11] & 0x100000;
			port_pin_set_output_level(LED_4, LOW);
		}
		else{																																	// LED 4 Turns Off
			port_pin_set_output_level(LED_4, HIGH);
		}
		*/


		
		/////////////////////////////////////////////////////////////////////////
		/*
		ade9000_data[0] = read_ade9000_data(ADE9000_CSS1_PIN,0x411);	// 1 ADE9000
		ade9000_data[0]; // Output Decimal Value: 4294967295 , Hex Value: FFFF FFFF
				
		ade9000_data[1] = read_ade9000_data(ADE9000_CSS2_PIN,0x411);	// 2 ADE9000
		ade9000_data[1]; // Output Decimal Value: 4294967295 , Hex Value: FFFF FFFF
				
		ade9000_data[4] = read_ade9000_data(ADE9000_CSS5_PIN,0x411);	// 5 ADE9000
		ade9000_data[4]; // Output Decimal Value: 4294967295 , Hex Value: FFFF FFFF
				
		ade9000_data[5] = read_ade9000_data(ADE9000_CSS6_PIN,0x411);	// 6 ADE9000
		ade9000_data[5]; // Output Decimal Value: 4294967295 , Hex Value: FFFF FFFF
				
		ade9000_data[7] = read_ade9000_data(ADE9000_CSS8_PIN,0x411);	// 8 ADE9000
		ade9000_data[7]; // Output Decimal Value: 8388607 , Hex Value: 7F FFFF
				
		ade9000_data[8] = read_ade9000_data(ADE9000_CSS9_PIN,0x411);	// 9 ADE9000
		ade9000_data[8]; // Output Decimal Value: 8388607 , Hex Value: 7F FFFF
				
		ade9000_data[9] = read_ade9000_data(ADE9000_CSS10_PIN,0x411);	// 10 ADE9000
		ade9000_data[9]; // Output Decimal Value: 8388607 , Hex Value: 7F FFFF
				
		ade9000_data[10] = read_ade9000_data(ADE9000_CSS11_PIN,0x411);	// 11 ADE9000
		ade9000_data[10]; // Output Decimal Value: 8388607 , Hex Value: 7F FFFF
				
		ade9000_data[11] = read_ade9000_data(ADE9000_CSS12_PIN,0x411);	// 12 ADE9000
		ade9000_data[11]; // Output Decimal Value: 8388607 , Hex Value: 7F FFFF
		*/
		
		/////////////////////////////////////////////////////////////////////////
		
		/*
		ade9000_data = 20;
		ade9000_data;
		rx_buf_2[0] = NULL;
		rx_buf_2[1] = NULL;
		rx_buf_2[2] = NULL;
		rx_buf_2[3] = NULL;
		
		//	0x411 + 0b1000(Read)
		tx_buf[0] = 0x41;
		tx_buf[1] = 0x18;

		port_pin_set_output_level(ADE9000_CSS2_PIN, false);
		spi_write_buffer_wait(&spi_instance, tx_buf, 2);
		spi_read_buffer_wait(&spi_instance, rx_buf_2, 4, 0x00);  // Receive 2 bytes of data
		port_pin_set_output_level(ADE9000_CSS2_PIN, true);
		
		// Combine response bytes
		// ade9000_data = (rx_buf_2[1] << 16) | (rx_buf_2[2] << 8) | rx_buf_2[3];
		ade9000_data = ((uint32_t)rx_buf_2[0] << 24) |
		((uint32_t)rx_buf_2[1] << 16) |
		((uint32_t)rx_buf_2[2] << 8)  |
		((uint32_t)rx_buf_2[3]);
		ade9000_data;
		
		ade9000_data = 20;
		ade9000_data;
		rx_buf_2[0] = NULL;
		rx_buf_2[1] = NULL;
		rx_buf_2[2] = NULL;
		rx_buf_2[3] = NULL;
		
		//	0x472 + 0b1000(Read) = (0100 0111 0010) 1000
		tx_buf[0] = 0x47;
		tx_buf[1] = 0x28;

		port_pin_set_output_level(ADE9000_CSS11_PIN, false);
		spi_write_buffer_wait(&spi_instance, tx_buf, 2);
		spi_read_buffer_wait(&spi_instance, rx_buf_2, 4, 0x00);  // Receive 2 bytes of data
		port_pin_set_output_level(ADE9000_CSS11_PIN, true);
		
		// Combine response bytes
		// ade9000_data = (rx_buf_2[1] << 16) | (rx_buf_2[2] << 8) | rx_buf_2[3];
		ade9000_data = ((uint32_t)rx_buf_2[0] << 24) |
                        ((uint32_t)rx_buf_2[1] << 16) |
                        ((uint32_t)rx_buf_2[2] << 8)  |
                        ((uint32_t)rx_buf_2[3]);
		ade9000_data;
		
		ade9000_data = 20;
		ade9000_data;
		rx_buf_2[0] = NULL;
		rx_buf_2[1] = NULL;
		rx_buf_2[2] = NULL;
		rx_buf_2[3] = NULL;
		
		//	0x472 + 0b1000(Read) = (0100 0111 0010) 1000
		tx_buf[0] = 0x47;
		tx_buf[1] = 0x28;

		port_pin_set_output_level(ADE9000_CSS6_PIN, false);
		spi_write_buffer_wait(&spi_instance, tx_buf, 2);
		spi_read_buffer_wait(&spi_instance, rx_buf_2, 4, 0x00);  // Receive 2 bytes of data
		port_pin_set_output_level(ADE9000_CSS6_PIN, true);
		
		// Combine response bytes
		// ade9000_data = (rx_buf_2[1] << 16) | (rx_buf_2[2] << 8) | rx_buf_2[3];
		ade9000_data = ((uint32_t)rx_buf_2[0] << 24) |
                        ((uint32_t)rx_buf_2[1] << 16) |
                        ((uint32_t)rx_buf_2[2] << 8)  |
                        ((uint32_t)rx_buf_2[3]);
		ade9000_data;
		
				
		ade9000_data = 20;
		ade9000_data;
		rx_buf_2[0] = NULL;
		rx_buf_2[1] = NULL;
		rx_buf_2[2] = NULL;
		rx_buf_2[3] = NULL;
		
		//	0x472 + 0b1000(Read) = (0100 0111 0010) 1000
		tx_buf[0] = 0x47;
		tx_buf[1] = 0x28;

		port_pin_set_output_level(PIN_PA13, false);
		spi_write_buffer_wait(&spi_instance, tx_buf, 2);
		spi_read_buffer_wait(&spi_instance, rx_buf_2, 4, 0x00);  // Receive 2 bytes of data
		port_pin_set_output_level(PIN_PA13, true);
		
		// Combine response bytes
		// ade9000_data = (rx_buf_2[1] << 16) | (rx_buf_2[2] << 8) | rx_buf_2[3];
		ade9000_data = ((uint32_t)rx_buf_2[0] << 24) |
                        ((uint32_t)rx_buf_2[1] << 16) |
                        ((uint32_t)rx_buf_2[2] << 8)  |
                        ((uint32_t)rx_buf_2[3]);
		ade9000_data;
		*/
		

		/*  Wrong
		Bytes to send:
[0]   0xC4      ? High byte of 0xC472
[1]   0x72      ? Low byte of 0xC472
[2]   0x00      ? dummy byte 1 (for receiving MSB of data)
[3]   0x00      ? dummy byte 2
[4]   0x00      ? dummy byte 3
[5]   0x00      ? dummy byte 4 (for receiving LSB)

Received bytes:
[2] = 0x00  ? MSB
[3] = 0x1C
[4] = 0x9E
[5] = 0x57  ? LSB

? Resulting value: 0x001C9E57
? In binary: 00000000 00011100 10011110 01010111
? Bit 20 = 1 ? it's an ADE9000

*/

/*
		ade9000_data = 20;
		ade9000_data;
		
		tx_buf[0] = 0b01000111;
		tx_buf[1] = 0b01000111;
		rx_buf[0] = 0b01000111;
		rx_buf[1] = 0b01000111;

		port_pin_set_output_level(ADE9000_CSS8_PIN, false);
		spi_write_buffer_wait(&spi_instance, tx_buf, 2);
		spi_read_buffer_wait(&spi_instance, rx_buf, 2, 0x00);  // Receive 2 bytes of data
		port_pin_set_output_level(ADE9000_CSS8_PIN, true);
		
		// Combine response bytes
		ade9000_data = ((uint16_t)rx_buf[0] << 8) | rx_buf[1];
		ade9000_data;
		
		ade9000_data = 20;
		ade9000_data;
		
		tx_buf[0] = 0b01000111;
		tx_buf[1] = 0b00101000;

		port_pin_set_output_level(ADE9000_CSS8_PIN, false);
		spi_transceive_buffer_wait(&spi_instance, tx_buf, rx_buf, 2);
		port_pin_set_output_level(ADE9000_CSS8_PIN, true);
		
		// Combine response bytes
		ade9000_data = ((uint16_t)rx_buf[0] << 8) | rx_buf[1];
		ade9000_data;
		
		ade9000_data = 20;
		ade9000_data;

		//////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		//ade9000_data = ade9000_read(ADE9000_CSS10_PIN,ADE9000_REG_ADDRESS,0); // read data from 0x02
		//ade9000_data;		
		
		ade9000_data = ade9000_read(ADE9000_CSS10_PIN,0x472,0); // read data from 0x472
		ade9000_data;
		
		ade9000_data = 20;
		ade9000_data;
		
		//////////////////////////////////////////////////////////////////////////////////////////////////////////

		
		//ade9000_data = ade9000_read(ADE9000_CSS11_PIN,ADE9000_REG_ADDRESS,0); // read data from 0x03
		//ade9000_data;
	
		ade9000_data = ade9000_read(ADE9000_CSS11_PIN,0x472,0); // read data from 0x472
		ade9000_data;
		
		
			 
		tx_buf[0] = 0b01000111;
		tx_buf[1] = 0b00101000;

		port_pin_set_output_level(ADE9000_CSS8_PIN, false);
		spi_transceive_buffer_wait(&spi_instance, tx_buf, rx_buf, 2);
		port_pin_set_output_level(ADE9000_CSS8_PIN, true);
		
		// Combine response bytes
		ade9000_data = ((uint16_t)rx_buf[0] << 8) | rx_buf[1];
		ade9000_data;
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////////
		/*
		ade9000_data = ade9000_read(ADE9000_CSS12_PIN,ADE9000_REG_ADDRESS,0); // read data from 0x03
		ade9000_data;
		
		ade9000_data = ade9000_read(ADE9000_CSS12_PIN,0x472,0); // read data from 0x03
		ade9000_data;

		//////////////////////////////////////////////////////////////////////////////////////////////////////////
				
		ade9000_data = 20;
		ade9000_data;	
		ade9000_data = ade9000_read(ADE9000_CSS9_PIN,ADE9000_REG_ADDRESS,0); // read data from 0x03
		ade9000_data;
		
		ade9000_data = ade9000_read(ADE9000_CSS9_PIN,0x472,0); // read data from 0x03
		ade9000_data;
		
		//////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		
		ade9000_data = ade9000_read(ADE9000_CSS8_PIN,ADE9000_REG_ADDRESS,0); // read data from 0x03
		ade9000_data;
		
		ade9000_data = ade9000_read(ADE9000_CSS8_PIN,0x472,0); // read data from 0x03
		ade9000_data;
		
		//////////////////////////////////////////////////////////////////////////////////////////////////////////
		*/
		// ade9000_data = 20;
		// ade9000_data;
				/*
		ade9000_data = ade9000_read(ADE9000_CSS7_PIN,ADE9000_REG_ADDRESS,0); // read data from 0x03
		ade9000_data;
		
		ade9000_data = ade9000_read(ADE9000_CSS7_PIN,0x472,0); // read data from 0x03
		ade9000_data;
		
		//////////////////////////////////////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		
		ade9000_data = ade9000_read(ADE9000_CSS6_PIN,ADE9000_REG_ADDRESS,0); // read data from 0x03
		ade9000_data;
		
		ade9000_data = ade9000_read(ADE9000_CSS6_PIN,0x472,0); // read data from 0x03
		ade9000_data;
		
		//////////////////////////////////////////////////////////////////////////////////////////////////////////
		ade9000_data = 20;
				ade9000_data;
		ade9000_data = ade9000_read(ADE9000_CSS5_PIN,ADE9000_REG_ADDRESS,0); // read data from 0x03
		ade9000_data;
		
		ade9000_data = ade9000_read(ADE9000_CSS5_PIN,0x472,0); // read data from 0x03
		ade9000_data;
		
		//////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		
				
		ade9000_data = 20;
		ade9000_data;

		////////////////
		////////////////
		led_1_status = port_pin_get_output_level(LED_1);
		//printf("World");
		led_2_status = port_pin_get_output_level(LED_2);
		led_3_status = port_pin_get_output_level(LED_3);
		led_4_status = port_pin_get_output_level(LED_4);
		////////////////
		//printf("LED 1: %d, LED 2: %d, LED 3: %d, LED 4: %d\n",
		//led_1_status, led_2_status, led_3_status, led_4_status);
		
		//printf("Read Data: %lu (Decimal), 0x%08lX (Hexadecimal)\n", ade9000_data, ade9000_data);
		//printf("Decimal %lu in lowercase hex: %lx\n", ade9000_data, ade9000_data);
		//printf("Data (Hex): 0x%08lX\n", ade9000_data);
		//for (int i = 0; i < 4; i++) {
			//uint8_t byte = (ade9000_data >> (8 * i)) & 0xFF;
			//printf("Byte %d: 0x%02lX\n", i, byte);
		//}
		
		//debug_output(ade9000_data);
		
		//snprintf()
		//debug_output("Hello World");
		//printf("Chip Address: %d\n", chip_address);
		delay_ms(100);
		delay_ms(1000);
	}
}
*/


	