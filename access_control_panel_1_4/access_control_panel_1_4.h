/*
 * access_control_panel_1_4.h
 *
 * Created: 7/11/2016 2:51:26 PM
 *  Author: Stuart
 */ 
#include <inttypes.h>
#include <avr/pgmspace.h>
#include <pins_arduino.h>

#ifndef ACCESS_CONTROL_PANEL_1_4_H_
#define ACCESS_CONTROL_PANEL_1_4_H_

#define DEVICE_VERSION 4
#define DEVICE_ID_LENGTH 8
#define BRINGUP_FLAG 0xAB

#define MIFARE_KEY_BLOCK_OFFSET 0
#define MIFARE_RECORD_BLOCK_OFFSET 1
#define MIFARE_VALUE_BLOCK_OFFSET 2

#define RED_LED_PIN 6
#define GREEN_LED_PIN 5
#define BLUE_LED_PIN 3

#define MAXIMUM_PACKET_LENGTH 16
#define COMM_TIMEOUT 1000

#define NO_PACKET_RECEIVED 0
#define PARTIAL_PACKET_RECEIVED 1
#define PACKET_PROCESSED_WITHOUT_ERROR 2
#define PACKET_PROCESSING_RX_TIMEOUT_ERROR 3

#define COMMAND_PROCESSED_WITHOUT_ERROR 0
#define COMMAND_PROCESSING_ERROR_COMMAND_NOT_FOUND 1
#define COMMAND_EXECUTION_ERROR 2

//Led Animation Sequences
//[][0] pwm setting, [][1] time in milliseconds
//all sequences must follow a rgb pattern
//the first value of the pwm setting array must contain the sequence length (number of rgb values)
//the first value of the timing array will be ignored
//the last rgb value of the sequence will be held

const uint16_t general_system_failure[][2] PROGMEM = {{10,0}, { 254, 250 }, { 0, 250 },{ 0, 250 },
                                                              { 0, 200 }, { 0, 200 }, { 0, 200 },
                                                              { 254, 250 }, { 0, 250 }, { 0, 250 },
                                                              { 0, 200 }, { 0, 200 }, { 0, 200 },
                                                              { 254, 250 }, { 0, 250 }, { 0, 250 },
                                                              { 0, 200 }, { 0, 200 }, { 0, 200 },
                                                              { 254, 250 }, { 0, 250 }, { 0, 250 },
                                                              { 0, 200 }, { 0, 200 }, { 0, 200 },
                                                              { 254, 250 }, { 0, 250 }, { 0, 250 },
                                                              { 0, 200 }, { 0, 200 }, { 0, 200 } };

const uint16_t card_recognized[][2] PROGMEM = {{1, 0},{0,0},{0,0},{1,0}};

const uint16_t card_accepted[][2] PROGMEM = {{1, 0},{142,0},{112,0},{0,0}};

const uint16_t card_operation_pending[][2] PROGMEM = {{1, 0},{255,0},{127,0},{1,0}};

const uint16_t card_approved[][2] PROGMEM = {{2, 0},{0, 2000},{0, 2000},{254, 2000},
{0, 0},  {0, 0}, {0, 0}};

const uint16_t alt_card_approved[][2] PROGMEM = {{2, 0},{0, 2000},{254, 2000},{0, 2000},
{0, 0},  {0, 0}, {0, 0}};

const uint16_t card_declined[][2] PROGMEM = {{2, 0},{254, 2000},{0, 2000},{0, 2000},
{0 , 0}, {0, 0}, {0, 0}};

const uint16_t power_on_animation[][2] PROGMEM = {{4, 0},{254, 250},{0 , 250} ,{0 , 250},
{127, 500},{127, 500},{0 , 500},
{85, 1000} ,{85, 1000} ,{85, 1000},
{0 , 0} ,{0 , 0} ,{0 , 0}};

const uint16_t blank_led[][2] PROGMEM = { { 1, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } };

const uint8_t unused_pins[] PROGMEM = {A1, A2, A3, 10, 11, 12, 13, 4, 7, 8, A6, A7};    //9, 2, 6 ,5 ,3

//const uint8_t defualt_mifare_block_key[] PROGMEM = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
//const uint8_t default_mifare_value_block_contents[] PROGMEM = {0, 0, 0, 0, ~0, ~0, ~0, ~0, 0, 0, 0, 0, block_num, ~block_num, block_num, ~block_num};

const uint8_t default_access_key_block_contents[] PROGMEM = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

const uint8_t default_security_sector_trailer_block_contents[] PROGMEM = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                                                          0xFF, 0b00000111, 0b10000000, 0xFF,
                                                                          0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

const uint8_t factory_default_sector_trailer[] PROGMEM = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                                    0xFF, 0x07, 0x80, 0x69,
                                                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

const uint8_t default_mifare_key_position = 0;
const uint8_t default_mifare_key_data[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
const uint8_t default_secure_records_sector = 2;

//const uint8_t default_security_block_num = 8;
//const uint8_t default_record_block_num = 9;
//const uint8_t default_counter_block_num = 10;

struct card_properties
{
    uint8_t card_uid[7];

    uint8_t sec_block_contents[16];
    uint8_t rec_block_contents[16];

    uint32_t counter_value;
};

#endif /* ACCESS_CONTROL_PANEL_1_4_H_ */