

#ifndef ADE9000_H
#define ADE9000_H

#define ADE9000_CSS1_PIN		PIN_PA13
#define ADE9000_CSS2_PIN		PIN_PA12
#define ADE9000_CSS3_PIN		PIN_PB11
#define ADE9000_CSS4_PIN		PIN_PB10
#define ADE9000_CSS5_PIN		PIN_PA17
#define ADE9000_CSS6_PIN		PIN_PA16
#define ADE9000_CSS7_PIN		PIN_PA15
#define ADE9000_CSS8_PIN		PIN_PA14
#define ADE9000_CSS9_PIN		PIN_PA21
#define ADE9000_CSS10_PIN		PIN_PA20
#define ADE9000_CSS11_PIN		PIN_PA19
#define ADE9000_CSS12_PIN		PIN_PA18

extern uint8_t ade9000_ss_pin[12];

extern struct spi_module spi_instance;

void configure_spi(void);

void ade9000_select(uint8_t ADE9000_CSS_PIN);

void ade9000_deselect(uint8_t ADE9000_CSS_PIN);

void ade9000_write(uint16_t reg_addr, uint16_t value);

void config_ade9000();

uint32_t read_ade9000_data(uint8_t ade_css_pin, uint16_t reg_address);

#endif
