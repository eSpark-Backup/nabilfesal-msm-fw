/*
 * CFile1.c
 *
 * Created: 7/23/2025 1:40:41 PM
 *  Author: eSpark
 */ 
/*
 * CFile2.c
 *
 * Created: 7/23/2025 1:38:11 PM
 *  Author: eSpark
 */ 

#include <asf.h>
#include <stdio.h>
#include "ade9000.h"

uint16_t chip_id = 10;

struct spi_module spi_instance;

uint8_t ade9000_ss_pin[12] = {
	ADE9000_CSS1_PIN,
	ADE9000_CSS2_PIN,
	ADE9000_CSS3_PIN,
	ADE9000_CSS4_PIN,
	ADE9000_CSS5_PIN,
	ADE9000_CSS6_PIN,
	ADE9000_CSS7_PIN,
	ADE9000_CSS8_PIN,
	ADE9000_CSS9_PIN,
	ADE9000_CSS10_PIN,
	ADE9000_CSS11_PIN,
	ADE9000_CSS12_PIN
};

void configure_spi(void)
{
	// Initialize SPI configuration structure
	struct spi_config config_spi;
	spi_get_config_defaults(&config_spi);

	// Set SPI parameters based on ADE9000 datasheet (Mode 0)
	config_spi.mux_setting = SPI_SIGNAL_MUX_SETTING_D;				// MISO, MOSI, SCK pins
	config_spi.mode = SPI_MODE_MASTER;								// SPI Mode Master
	config_spi.transfer_mode = SPI_TRANSFER_MODE_0;					// SPI Mode 0 (CPOL=0, CPHA=0)
	config_spi.mode_specific.master.baudrate = 1000000;									// 1 MHz SPI clock speed
	config_spi.pinmux_pad0 = PINMUX_PA07D_SERCOM0_PAD3;				// MISO pin
	config_spi.pinmux_pad1 = PINMUX_PA05D_SERCOM0_PAD1;				// SCK pin
	config_spi.pinmux_pad2 = PINMUX_UNUSED;							// No pad used
	config_spi.pinmux_pad3 = PINMUX_PA04D_SERCOM0_PAD0;				// MOSI pin

	// Initialize the SPI module
	spi_init(&spi_instance, SERCOM0, &config_spi);
	spi_enable(&spi_instance);
}

void ade9000_select(uint8_t ADE9000_CSS_PIN)
{
	port_pin_set_output_level(ADE9000_CSS_PIN, false);  // CS LOW to select ADE9000
}

void ade9000_deselect(uint8_t ADE9000_CSS_PIN)
{
	port_pin_set_output_level(ADE9000_CSS_PIN, true);   // CS HIGH to deselect ADE9000
}

/*
void ade9000_write(uint16_t reg_addr, uint16_t value)
{
	uint8_t tx[4];  

	tx[0] = (reg_addr >> 8) & 0xFF;   // High byte of register address
	tx[1] = reg_addr & 0xFF;          // Low byte of register address
	tx[2] = (value >> 8) & 0xFF;      // High byte of value
	tx[3] = value & 0xFF;             // Low byte of value

	// Select ADE9000, send data, and deselect ADE9000
	ade9000_select(ADE9000_CSS10_PIN);
	spi_write_buffer_wait(&spi_instance, tx, 4);
	ade9000_deselect(ADE9000_CSS10_PIN);
}
*/

void config_ade9000(){
	static struct port_config pin_conf;
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
}

uint32_t read_ade9000_data(uint8_t ade_css_pin, uint16_t reg_address){
	uint32_t ade9000_data;
	uint8_t rx_buf_2[4];
	uint8_t tx_buf[2];
	
	
	tx_buf[0] = (reg_address >> 4 ) & 0xFF;
	tx_buf[1] = (reg_address << 4) | 0x8; // read byte 0x8 added for write it should be 0x0

	port_pin_set_output_level(ade_css_pin, false);
	spi_write_buffer_wait(&spi_instance, tx_buf, 2);
	spi_read_buffer_wait(&spi_instance, rx_buf_2, 4, 0x00);  // Receive 2 bytes of data
	port_pin_set_output_level(ade_css_pin, true);
	
	// Combine response bytes
	// ade9000_data = (rx_buf_2[1] << 16) | (rx_buf_2[2] << 8) | rx_buf_2[3];
	ade9000_data = ((uint32_t)rx_buf_2[0] << 24) |
	((uint32_t)rx_buf_2[1] << 16) |
	((uint32_t)rx_buf_2[2] << 8)  |
	((uint32_t)rx_buf_2[3]);
	return ade9000_data;
}