#ifndef EXTERNAL_EEPROM_H
#define EXTERNAL_EEPROM_H

#define SLAVE_READ		0xA1
#define SLAVE_WRITE		0xA0


#define SEC_ADDR		0x00
#define MIN_ADDR		0x01
#define HOUR_ADDR		0x02
#define DAY_ADDR		0x03
#define DATE_ADDR		0x04
#define MONTH_ADDR		0x05
#define YEAR_ADDR		0x06
#define CNTL_ADDR		0x07


void write_eeprom(unsigned char address1,  unsigned char data);
unsigned char read_eeprom(unsigned char address1);

#endif