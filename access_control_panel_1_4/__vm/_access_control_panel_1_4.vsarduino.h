/* 
	Editor: http://www.visualmicro.com
	        visual micro and the arduino ide ignore this code during compilation. this code is automatically maintained by visualmicro, manual changes to this file will be overwritten
	        the contents of the Visual Micro sketch sub folder can be deleted prior to publishing a project
	        all non-arduino files created by visual micro and all visual studio project or solution files can be freely deleted and are not required to compile a sketch (do not delete your own code!).
	        note: debugger breakpoints are stored in '.sln' or '.asln' files, knowledge of last uploaded breakpoints is stored in the upload.vmps.xml file. Both files are required to continue a previous debug session without needing to compile and upload again
	
	Hardware: Arduino Pro or Pro Mini (5V, 16 MHz) w/ ATmega328, Platform=avr, Package=arduino
*/

#define __AVR_ATmega328p__
#define __AVR_ATmega328P__
#define ARDUINO 105
#define ARDUINO_MAIN
#define F_CPU 16000000L
#define __AVR__
extern "C" void __cxa_pure_virtual() {;}

//
//
uint8_t commManager();
uint8_t processPacket(uint8_t* comm_buffer, uint8_t packet_size);
uint8_t awaitAck();
uint8_t awaitBytes(uint8_t num_bytes);
bool compareArrays(uint8_t * arr1, uint8_t * arr2, uint8_t arr_len);
void packageDataAndWritePacket(uint8_t* data_buffer, uint8_t data_length);
void cardPresenceRefresh();
uint8_t checkAccessKey(uint8_t * key_data);
void updateTimes();
void clearCardPresence(uint8_t ret_uid);
void initLEDCycle();
void blockingLEDRefresh();
void refreshLED();
void animateLED(uint16_t * animation_pointer, int animation_size, bool isPROGMEM, int iterations, bool lock_led, bool skip_lock);
byte CRC8(const byte *data, byte len);
void restart();
void checkDevID();
void loadConfiguration();

#include "X:\Stuart\Desktop\Desktop Stuff\arduino-1.0.5-r2 (Panels v1r3)\hardware\arduino\variants\standard\pins_arduino.h" 
#include "X:\Stuart\Desktop\Desktop Stuff\arduino-1.0.5-r2 (Panels v1r3)\hardware\arduino\cores\arduino\arduino.h"
#include <..\access_control_panel_1_4\access_control_panel_1_4.ino>
#include <..\access_control_panel_1_4\access_control_panel_1_4.h>
