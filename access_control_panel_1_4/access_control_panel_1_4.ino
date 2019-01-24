/*
Mid City Tower Access Control Project
NFC Reader Program

Written By: Stuart Rogers
*/

#include "access_control_panel_1_4.h"

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <EEPROM.h>
#include <TimeLib.h>

uint8_t device_id[DEVICE_ID_LENGTH];

PN532 nfc(10);

volatile long int_count = 0;

uint16_t custom_animation[49][2]; //196 bytes

uint16_t (*active_animation)[2] = NULL;

uint16_t comm_timeout_timer, tap_timeout_timer;
uint8_t comm_flag = false;

uint16_t tap_timeout_time = 4000;

//device commands
uint8_t nack_command = 0x0;
uint8_t ack_command = 0x1;

uint8_t get_active_card_properties_command = 0x2;
uint8_t clear_card_command = 0x3;
uint8_t display_animation_command = 0x4;
uint8_t get_uptime_command = 0x5;
uint8_t get_device_id_command = 0x6;

uint8_t set_custom_animation_command = 0x7;

uint8_t get_panel_status = 0x8;

uint8_t set_time_command = 0x9;
uint8_t get_time_command = 0xA;

uint8_t get_debug_stats = 0xB;

uint8_t set_mifare_sec_access_conf = 0xC;
uint8_t get_mifare_sec_access_conf = 0xD;

uint8_t get_device_version = 0xE;

uint8_t restart_command = 0xF;

//get passive target uid
//format access control card
//format student id
//read block
//write block

uint8_t acknowledge_packet[] = {0x1, 0x1, 0x9a}; //0x1
uint8_t not_acknowledge_packet[] = {0x1, 0x0, 0xc4}; //0x0

uint8_t card_present = 0;
card_properties active_card; //memory placeholder

//led stuff
///////////////////////////////
int red_counter_maximum;
int green_counter_maximum;
int blue_counter_maximum;

int red_counter = -2;
int green_counter = -1;
int blue_counter = 0;

uint32_t animation_time;

uint32_t red_advance_time = 0;
uint32_t green_advance_time = 0;
uint32_t blue_advance_time = 0;

int completed = 0;

int num_iterations = 0;
int iteration_counter = 0;

bool led_lock = false;
/////////////////////////////////

//uint8_t comm_buffer[18]; //1 byte for length, 16 bytes for data, 1 byte for crc8
//uint8_t comm_buffer_pos = 0;

uint8_t uptime[8];
uint16_t uptime_timer;

struct mifare_secure_access_configuration
{
    uint8_t mifare_key_number;
    uint8_t mifare_key_data[6];
    
    uint8_t secure_records_sector;

} mifare_sec_conf; 

struct dev_flags
{
    uint8_t auto_poll_enabled : 1;
    uint8_t auto_format_cards : 1;


} flags;


void setup()
{ 
    for(int i = 0; i < sizeof(unused_pins); i++)
        pinMode(pgm_read_byte_near(unused_pins + i), OUTPUT);
    
    Serial.begin(250000);

    pinMode(13, OUTPUT);
    digitalWrite(13, HIGH);
    
    animateLED((uint16_t*)power_on_animation, sizeof(power_on_animation), true, 1, false, true);
    blockingLEDRefresh();

    loadConfiguration();
    
    nfc.begin();
    
    nfc.setPassiveActivationRetries(1);
    nfc.SAMConfig();

    flags.auto_format_cards = true;
    flags.auto_poll_enabled = true;
}

void loop()
{       
    if(flags.auto_poll_enabled)
        cardPresenceRefresh();

    refreshLED();

    updateTimes();

	commManager();
}

uint8_t commManager()
{
	if (Serial.available())
	{
			if (!comm_flag)
			{
				comm_flag = true;
				comm_timeout_timer = millis();
			}

			uint8_t peek_value = Serial.peek();
            uint8_t packet_size = peek_value + 2;

			if ((peek_value <= MAXIMUM_PACKET_LENGTH && peek_value > 0) && (uint16_t)((int16_t)millis() - comm_timeout_timer) < COMM_TIMEOUT)
			{
				if (Serial.available() >= packet_size) //continue if we've received the full packet
				{
					uint8_t comm_buffer[packet_size];

					for (int index_counter = 0; index_counter < packet_size; index_counter++)
						comm_buffer[index_counter] = Serial.read();

					processPacket(comm_buffer, packet_size);

					comm_flag = false;
				}
                else
                    return PARTIAL_PACKET_RECEIVED;
			}
			else
			{
				Serial.read();

				comm_flag = false;

                return PACKET_PROCESSING_RX_TIMEOUT_ERROR;
			}
	}
	else
	{
		comm_flag = false;
		comm_timeout_timer = 0;

        return NO_PACKET_RECEIVED;
	}
}

uint8_t processPacket(uint8_t* comm_buffer, uint8_t packet_size)
{
	if (CRC8(comm_buffer, packet_size - 1) == comm_buffer[packet_size - 1])
	{
		uint8_t command_byte = comm_buffer[1];

		if (command_byte == get_active_card_properties_command)
		{
			if(card_present)
            {
                Serial.write(acknowledge_packet, 3);
                
                packageDataAndWritePacket((uint8_t*)&active_card, sizeof(card_properties));

                if(awaitAck())
                    return COMMAND_EXECUTION_ERROR;
                else
                    return COMMAND_PROCESSED_WITHOUT_ERROR;
            }
            else
                Serial.write(not_acknowledge_packet, sizeof(not_acknowledge_packet));
		}
		else
			if (command_byte == display_animation_command)
			{
				uint8_t animation_id = comm_buffer[2];

				switch (animation_id)
				{
					case 0: if(custom_animation != NULL)
                            {
                                animateLED((uint16_t*)custom_animation, sizeof(custom_animation), 1, false, true, true);
                                break;
                            }
                            else
                            {
                                Serial.write(not_acknowledge_packet, 3);
                                return COMMAND_EXECUTION_ERROR;
                            }

                    case 1: animateLED((uint16_t*)card_declined, sizeof(card_declined), true, 1, true, true); break;
					case 2: animateLED((uint16_t*)alt_card_approved, sizeof(alt_card_approved), true, 1, true, true); break;
					case 3: animateLED((uint16_t*)card_approved, sizeof(card_approved), true, 1, true, true); break;
					case 4: animateLED((uint16_t*)blank_led, sizeof(blank_led), true, 1, true, true); break;

                    default:
                        Serial.write(not_acknowledge_packet, 3); 
                        return COMMAND_EXECUTION_ERROR;
				}

                Serial.write(acknowledge_packet, 3);

                return COMMAND_PROCESSED_WITHOUT_ERROR;
			}
			else
				if (command_byte == get_uptime_command)
				{
					Serial.write(acknowledge_packet, 3);
				
					packageDataAndWritePacket(uptime, 8);

                    if(awaitAck())
                        return COMMAND_EXECUTION_ERROR;
                    else
                        return COMMAND_PROCESSED_WITHOUT_ERROR;
				}
				else
					if (command_byte == clear_card_command)
					{
						clearCardPresence(comm_buffer[2]);

                        Serial.write(acknowledge_packet, 3);

                        return COMMAND_PROCESSED_WITHOUT_ERROR;
					}
					else
					    if (command_byte == restart_command)
					    {
						    Serial.write(acknowledge_packet, 3);

						    restart();

                            //no need to return due to the restart
					    }
                        else
                            if(command_byte == get_device_id_command)
                            {
                                Serial.write(acknowledge_packet, 3);

                                packageDataAndWritePacket(device_id, DEVICE_ID_LENGTH);

                                if(awaitAck())
                                    return COMMAND_EXECUTION_ERROR;
                                else
                                    return COMMAND_PROCESSED_WITHOUT_ERROR;
                            }
                            else
                                if(command_byte = set_custom_animation_command)
                                {
                                    //the payload will only contain the number of bytes to await. this must be less than 196 bytes. 32 animation bytes will be transferred at a time. each transfer will have a trailing crc byte

                                    if(comm_buffer[2] == 0 || comm_buffer[2] > 196)
                                    {
                                        Serial.write(not_acknowledge_packet, sizeof(not_acknowledge_packet));
                                        return COMMAND_EXECUTION_ERROR;
                                    }
                                    else
                                        Serial.write(acknowledge_packet, 3);

                                    uint8_t * animation_buffer = (uint8_t*)malloc(comm_buffer[2]);

                                    uint8_t num_itr = comm_buffer[2] % 32 > 0 ? comm_buffer[2]/32 + 1 : comm_buffer[2]/32; 
                                    for(int i = 0; i < num_itr; i++)
                                    {
                                        uint8_t to_await_now = num_itr - 1 > 0 ? 33 : (32 * num_itr - comm_buffer[2]) + 1;

                                        uint8_t i_buff[to_await_now];

                                        if(awaitBytes(to_await_now))
                                        {
                                            Serial.write(not_acknowledge_packet, sizeof(not_acknowledge_packet));
                                            return COMMAND_EXECUTION_ERROR;
                                        }

                                        Serial.readBytes((char*)i_buff, to_await_now);

                                        if(CRC8(i_buff, to_await_now-1) == i_buff[to_await_now-1])
                                            memcpy(animation_buffer+i*32, i_buff, to_await_now-1);
                                        else
                                        {
                                            Serial.write(not_acknowledge_packet, sizeof(not_acknowledge_packet));
                                            return COMMAND_EXECUTION_ERROR;
                                        }

                                        Serial.write(acknowledge_packet, 3);
                                    }

                                    memcpy(custom_animation, animation_buffer, sizeof(animation_buffer));

                                    //await the number of bytes specified in the payload
                                    free(animation_buffer);   
                                }
                                else
                                    if(command_byte == set_time_command)
                                    {
                                        //time_t is a 32 bit unsigned integer

                                        uint32_t received_time = *(uint32_t*)comm_buffer+2;

                                        setTime(received_time);

                                        Serial.write(acknowledge_packet, 3);

                                        return COMMAND_PROCESSED_WITHOUT_ERROR;
                                    }
                                    else
                                        if(command_byte == get_time_command)
                                        {
                                            Serial.write(acknowledge_packet, 3);
                                            
                                            uint32_t current_time = now();

                                            packageDataAndWritePacket((uint8_t*)&current_time, 4);

                                            if(awaitAck())
                                                return COMMAND_EXECUTION_ERROR;
                                            else
                                                return COMMAND_PROCESSED_WITHOUT_ERROR;
                                        }
                                        else
                                            if(command_byte == get_panel_status)
                                            {
                                                //[0]:0 - master sync needed
                                                //[0]:1 - time sync needed
                                                //[0]:2 - card presence detected
                                                //[0]:3 -  
                                            }
                                            else
                                                if(command_byte == get_debug_stats)
                                                {
                                                    //return debug stats struct
                                                }
                                                else
                                                    if(command_byte == set_mifare_sec_access_conf)
                                                    {
                                                        memcpy((uint8_t*)&mifare_sec_conf, comm_buffer+2, sizeof(mifare_secure_access_configuration));
                                                        
                                                        Serial.write(acknowledge_packet, sizeof(acknowledge_packet));        
                                                    }
                                                    else
                                                        if(command_byte == get_mifare_sec_access_conf)
                                                        {
                                                            Serial.write(acknowledge_packet, sizeof(acknowledge_packet));

                                                            packageDataAndWritePacket((uint8_t*)&mifare_sec_conf, sizeof(mifare_secure_access_configuration));

                                                            if(awaitAck())
                                                                return COMMAND_EXECUTION_ERROR;
                                                            else
                                                                return COMMAND_PROCESSED_WITHOUT_ERROR;
                                                        }
                                                        else
                                                        {
                                                            //command not found
                                                            Serial.write(not_acknowledge_packet, 3);

                                                            return COMMAND_PROCESSING_ERROR_COMMAND_NOT_FOUND; 
                                                        }
	}
}

//3 invalid resonse received
//2 nack received
// 1 no response received
//0 ack received
uint8_t awaitAck()
{
    if(awaitBytes(sizeof(acknowledge_packet)))
        return 1;

    uint8_t buff[sizeof(acknowledge_packet)];
    Serial.readBytes((char*)buff, sizeof(acknowledge_packet));

    if(compareArrays(acknowledge_packet, buff, sizeof(acknowledge_packet)))
        return 0;
    else
        if(compareArrays(not_acknowledge_packet, buff, sizeof(acknowledge_packet)))
            return 2;
        else
            return 3;
}

// 0 - received awaited number of bytes
//1 - a timeout occured
uint8_t awaitBytes(uint8_t num_bytes)
{
    uint16_t timeout_timer_1 = millis();
    while(Serial.available() < num_bytes)
    {
        if( (uint16_t)((int16_t)millis() - timeout_timer_1) >= COMM_TIMEOUT)
        return 0;
    }
    
    return 1;    
}

bool compareArrays(uint8_t * arr1, uint8_t * arr2, uint8_t arr_len)
{
    for(int i = 0; i < arr_len; i++)
        if(*(arr1 + i) != *(arr2 +i))
            return false;

    return true;
}

void packageDataAndWritePacket(uint8_t* data_buffer, uint8_t data_length)
{
	uint8_t * packet_buffer = (uint8_t*)calloc(data_length + 2, 1);

	packet_buffer[0] = data_length;

	memcpy(packet_buffer+1, data_buffer, data_length);

	packet_buffer[data_length + 1] = CRC8(packet_buffer, data_length+1);

    Serial.write(packet_buffer, data_length + 2);

    free(packet_buffer);
}

void cardPresenceRefresh()
{
	uint8_t nfc_buffer[8];

	if(nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, nfc_buffer+1, nfc_buffer))
    {
    if(!compareArrays(nfc_buffer+1, nfc_buffer[0] == 4 ? active_card.card_uid+3 : active_card.card_uid, nfc_buffer[0]))
	{
    	animateLED((uint16_t*)card_recognized, sizeof(card_recognized), true, 1, false, false);

        uint8_t interaction_success = false;

        //read all security blocks into a buffer

        uint8_t * o_sec_rec_contents = (uint8_t*)malloc(16*3);
        uint8_t * n_sec_rec_contents = (uint8_t*)malloc(16*2);

        if(nfc.mifareclassic_AuthenticateBlock(nfc_buffer+1, nfc_buffer[0], mifare_sec_conf.secure_records_sector*4, mifare_sec_conf.mifare_key_number, (uint8_t*)mifare_sec_conf.mifare_key_data))
            if(nfc.mifareclassic_ReadDataBlock(mifare_sec_conf.secure_records_sector*4, o_sec_rec_contents))
                if(nfc.mifareclassic_ReadDataBlock(mifare_sec_conf.secure_records_sector*4+1, o_sec_rec_contents+16))
                    if(nfc.mifareclassic_ReadDataBlock(mifare_sec_conf.secure_records_sector*4+2, o_sec_rec_contents+16*2))
                    {
                            uint8_t record_buffer[16];

                            for(int i = 0; i < 16; i++)
                                record_buffer[i] = 0;

                            memcpy(record_buffer, device_id, DEVICE_ID_LENGTH);

                            *(uint32_t*)(record_buffer+DEVICE_ID_LENGTH) = now();

                            if(nfc.mifareclassic_WriteDataBlock(mifare_sec_conf.secure_records_sector*4+1, record_buffer))
                                if(nfc.mifareclassic_IncrementValueBlock(mifare_sec_conf.secure_records_sector*4+2))
                                    if(nfc.mifareclassic_TransferValueBlock(mifare_sec_conf.secure_records_sector*4+2))
                                        if(nfc.mifareclassic_ReadDataBlock(mifare_sec_conf.secure_records_sector*4+1, n_sec_rec_contents))
                                            if(nfc.mifareclassic_ReadDataBlock(mifare_sec_conf.secure_records_sector*4+2, n_sec_rec_contents+16))
                                                if(compareArrays(record_buffer, n_sec_rec_contents, 16))
                                                {
                                                    uint32_t pres0 = *(uint32_t*)(o_sec_rec_contents+16*2);
                                                    uint32_t pres1 = *(uint32_t*)(n_sec_rec_contents+16);

                                                    if(pres0+1 == pres1)
                                                        interaction_success = true;
                                                            
                                                }
                    }
                         
        free(n_sec_rec_contents);
        
        clearCardPresence(false);  

        if(interaction_success)
        {
            memcpy(((uint8_t*)active_card.card_uid) + (nfc_buffer[0] == 4 ? 3 : 0), nfc_buffer + 1, nfc_buffer[0]); //pad if the uid is only 4 bytes
            memcpy((uint8_t*)active_card.sec_block_contents, o_sec_rec_contents, 16);
            memcpy((uint8_t*)active_card.rec_block_contents, o_sec_rec_contents+16, 16);

            active_card.counter_value = *(uint32_t*)(o_sec_rec_contents+16*2);

            card_present = true;
            tap_timeout_timer = millis();    
        }
        else
        {
            //failed to read the card for some reason
            if(flags.auto_format_cards)
            {
                animateLED((uint16_t*)card_operation_pending, sizeof(card_operation_pending), true, 1, false, false);
                
                //one reason that a card read will fail is due to an unformatted card.
                //check the card to see if its state indicates a read failure due to this reason

                //if the current configuration is the default configuration then exit

                //new cards will have the slot zero key set to default
                if(nfc.inListPassiveTarget())
                    if(nfc.mifareclassic_AuthenticateBlock(nfc_buffer+1, nfc_buffer[0], default_secure_records_sector*4, (uint8_t)default_mifare_key_position, (uint8_t*)default_mifare_key_data))
                    {
                        //the default key worked. this means that the card is most likely a new card and needs to be formatted.
                        //due to the fact that it's possible that we're using the cards without a secure key we need to perform additional checks.
                        //check the access control key block to see if a key has been written to the card. a valid key is any key other than 0.
                        
                        if(nfc.mifareclassic_ReadDataBlock(default_secure_records_sector*4, o_sec_rec_contents))
                        {
                            if(!checkAccessKey(o_sec_rec_contents))
                            {
                                //the key is invalid. this card requires formatting.

                                //copy the default access key to the buffer
                                memcpy_P(o_sec_rec_contents, default_access_key_block_contents, sizeof(default_access_key_block_contents));
                                //write the buffer to the access key block 
                                nfc.mifareclassic_WriteDataBlock(default_secure_records_sector*4, o_sec_rec_contents);
                                //format the lifetime use counter block
                                nfc.mifareclassic_FormatValueBlock(default_secure_records_sector*4+MIFARE_VALUE_BLOCK_OFFSET);
                                //copy the default sector trailer to the buffer
                                memcpy_P(o_sec_rec_contents, default_security_sector_trailer_block_contents, sizeof(default_security_sector_trailer_block_contents));
                                //fill the buffer with the key that is currently in use
                                memcpy(o_sec_rec_contents, mifare_sec_conf.mifare_key_data, sizeof(mifare_sec_conf.mifare_key_data)); //key a
                                memcpy(o_sec_rec_contents+10, mifare_sec_conf.mifare_key_data, sizeof(mifare_sec_conf.mifare_key_data)); //key b
                                //write the buffer to the trailer block
                                nfc.mifareclassic_WriteDataBlock(default_secure_records_sector*4+3, o_sec_rec_contents);

                                //the card has now been formatted
                            }
                            else
                            {
                                //the default sector encryption key worked but the access key is valid. the card has most likely been formatted with the default encryption key.
                                //the key needs to be updated

                                //copy the default sector trailer to the buffer
                                memcpy_P(o_sec_rec_contents, default_security_sector_trailer_block_contents, sizeof(default_security_sector_trailer_block_contents));
                                //fill the buffer with the key that is currently in use
                                memcpy(o_sec_rec_contents, mifare_sec_conf.mifare_key_data, sizeof(mifare_sec_conf.mifare_key_data)); //key a
                                memcpy(o_sec_rec_contents+10, mifare_sec_conf.mifare_key_data, sizeof(mifare_sec_conf.mifare_key_data)); //key b
                                //write the buffer to the trailer block
                                nfc.mifareclassic_WriteDataBlock(default_secure_records_sector*4+3, o_sec_rec_contents);

                                //the card has now been formatted
                            
                            }
                        }
                        }
                        else
                        {
                             //copy ndef key to buffer
                             //memcpy_P(o_sec_rec_contents, public_ndef_key, sizeof(public_ndef_key));

                             if(nfc.inListPassiveTarget())
                             {
                                nfc.mifareclassic_AuthenticateBlock(nfc_buffer+1, nfc_buffer[0], default_secure_records_sector*4, 1, (uint8_t*)default_mifare_key_data);
                             
                                 memcpy_P(o_sec_rec_contents, factory_default_sector_trailer, sizeof(factory_default_sector_trailer));

                                 nfc.mifareclassic_WriteDataBlock(default_secure_records_sector*4+3, o_sec_rec_contents);
                             }

                                
                        }
                    
            }
        }

        free(o_sec_rec_contents);      
	}
    }
    else
        if(!card_present)
            clearCardPresence(false);
    
    if (card_present)
	{
		if ((uint16_t)((int16_t)millis() - tap_timeout_timer) >= tap_timeout_time)
		{
			clearCardPresence(true);
			animateLED((uint16_t*)general_system_failure, sizeof(general_system_failure), true, 1, true, false);
		}
		else
			animateLED((uint16_t*)card_accepted, sizeof(card_accepted), true, 1, false, false);
	}
        else
          animateLED((uint16_t*)blank_led, sizeof(blank_led), true, 1, false, false); //this is going to run all the time. there must be a faster way of making sure the led is always in a known state
}

//0 invalid key
//1 valud key
uint8_t checkAccessKey(uint8_t * key_data)
{
    uint32_t byte_sum = 0;
    for(int i = 0; i < 16; i++)
        byte_sum += key_data[i];

    if(byte_sum > 0)
        return 1;
    else
        return 0;
}

void updateTimes()
{
	*(uint64_t*)&uptime += (uint16_t)((int16_t)millis() - uptime_timer);
	uptime_timer = millis();

    now();
}

void clearCardPresence(uint8_t ret_uid)
{
	card_present = false;
	tap_timeout_timer = 0;

    uint8_t uid_buf[7];

    if(ret_uid)
        memcpy(uid_buf, active_card.card_uid, 7);

	//clear card data buffer
	for(int i = 0; i < sizeof(card_properties); i++)
        *((uint8_t*)&active_card + i) = 0;

    if(ret_uid)
        memcpy(active_card.card_uid, uid_buf, 7);
}

void initLEDCycle()
{
	red_counter = -2;
	green_counter = -1;
	blue_counter = 0;
	
	red_advance_time = 0;
	green_advance_time = 0;
	blue_advance_time = 0;
	
	completed = 0;
}

void blockingLEDRefresh()
{
	while(iteration_counter < num_iterations)
		refreshLED();
}

void refreshLED()
{
	if (iteration_counter < num_iterations)
	{
		if (completed < 2)
		{
			animation_time = millis();

			if (animation_time >= red_advance_time)
			{
				if (red_counter > -3)
				{
					red_counter += 3;

					if (red_counter > red_counter_maximum)
						red_counter = -3;
					else
					{
						analogWrite(RED_LED_PIN, active_animation[red_counter][0]); //write next red value
						red_advance_time = animation_time + active_animation[red_counter][1]; //calculate the timing
					}
				}
				else
					completed++;
			}

			if (animation_time >= green_advance_time)
			{
				if (green_counter > -3)
				{
					green_counter += 3;
					if (green_counter > green_counter_maximum)
					{
						green_counter = -3;
					}
					else
					{
						analogWrite(GREEN_LED_PIN, active_animation[green_counter][0]); //write next green value
						green_advance_time = animation_time + active_animation[green_counter][1]; //calculate the timing
					}
				}
				else
					completed++;
			}

			if (animation_time >= blue_advance_time)
			{
				if (blue_counter > -3)
				{
					blue_counter += 3;
					if (blue_counter > blue_counter_maximum)
						blue_counter = -3;
					else
					{
						analogWrite(BLUE_LED_PIN, active_animation[blue_counter][0]); //write next blue value
						blue_advance_time = animation_time + active_animation[blue_counter][1]; //calculate the timing
					}
				}
				else
					completed++;
			}
		}
		else
		{
			iteration_counter++;

			initLEDCycle();
		}
	}
	else
    {
		led_lock = false;
        free(active_animation);
        active_animation = NULL;
    }
}

//fade is not implemented yet
void animateLED(uint16_t * animation_pointer, int animation_size, bool isPROGMEM, int iterations, bool lock_led, bool skip_lock)
{
	if (!led_lock || skip_lock)
	{
		led_lock = lock_led;
		
        if(active_animation != NULL)
            free(active_animation);

		active_animation = (uint16_t(*)[2])malloc(animation_size);

		if(isPROGMEM)
		    memcpy_P(active_animation, animation_pointer, animation_size);
		else
		    memcpy(active_animation, animation_pointer, animation_size);

		red_counter_maximum = active_animation[0][0] * 3 - 2;
		green_counter_maximum = active_animation[0][0] * 3 - 1;
		blue_counter_maximum = active_animation[0][0] * 3;

		num_iterations = iterations;

		iteration_counter = 0;

		initLEDCycle();

		refreshLED(); //kick it off
	}
}

//CRC-8 - based on the CRC8 formulas by Dallas/Maxim
//code released under the therms of the GNU GPL 3.0 license
byte CRC8(const byte *data, byte len)
{
	byte crc = 0x00;
	while (len--)
	{
		byte extract = *data++;
		for (byte tempI = 8; tempI; tempI--)
		{
			byte sum = (crc ^ extract) & 0x01;
			crc >>= 1;
			if (sum)
			{
				crc ^= 0x8C;
			}
			extract >>= 1;
		}
	}
	return crc;
}

void restart()
{
	delay(10);
	
	asm volatile ("  jmp 0");
}

void checkDevID()
{
    if(EEPROM.read(0) != BRINGUP_FLAG)
    {
        randomSeed(analogRead(A0));

        for(uint8_t i = 0; i < DEVICE_ID_LENGTH; i++)
            EEPROM.write(i+1, random(256));
            
        EEPROM.write(0, BRINGUP_FLAG);   
    }

    for(uint8_t i = 0; i < DEVICE_ID_LENGTH; i++)
        device_id[i] = EEPROM.read(i+1);
}

void loadConfiguration()
{
    checkDevID(); 

    memcpy(mifare_sec_conf.mifare_key_data, default_mifare_key_data, sizeof(default_mifare_key_data));
    mifare_sec_conf.mifare_key_number = default_mifare_key_position;
    mifare_sec_conf.secure_records_sector = 2;
}
