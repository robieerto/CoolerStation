/*
    ModbusTCPSlave.h - an Arduino library for a Modbus TCP slave.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// Note: The Arduino IDE does not respect conditional included
// header files in the main sketch so you have to select your
// here.
//
#ifndef ModbusTCPSlave_h
#define ModbusTCPSlave_h

#define MB_PORT 502  


#define MB_ESP32



#include "Arduino.h"


#ifdef MB_ESP32
#include <EthernetENC.h>
#endif

#define maxInputRegister      30
#define maxHoldingRegister    30
//
// MODBUS Function Codes
//
/*#define MB_FC_NONE 0
#define MB_FC_READ_COILS 1
#define MB_FC_READ_DISCRETE_INPUT 2
#define MB_FC_READ_REGISTERS 3              //implemented
#define MB_FC_READ_INPUT_REGISTERS 4        //implemented
#define MB_FC_WRITE_COIL 5
#define MB_FC_WRITE_REGISTER 6              //implemented
#define MB_FC_WRITE_MULTIPLE_COILS 15
#define MB_FC_WRITE_MULTIPLE_REG 16   //implemented
*/
//
// MODBUS Error Codes
//
#define MB_EC_NONE 0
#define MB_EC_ILLEGAL_FUNCTION 1
#define MB_EC_ILLEGAL_DATA_ADDRESS 2
#define MB_EC_ILLEGAL_DATA_VALUE 3
#define MB_EC_SLAVE_DEVICE_FAILURE 4
//
// MODBUS MBAP offsets
//
#define MB_TCP_TID          0
#define MB_TCP_PID          2
#define MB_TCP_LEN          4
#define MB_TCP_UID          6
#define MB_TCP_FUNC         7
#define MB_TCP_REGISTER_START         8
#define MB_TCP_REGISTER_NUMBER         10

class ModbusTCPSlave
{
public:
  ModbusTCPSlave(void);

#ifdef MB_ESP32
    void begin();
    #endif
    void Run();
  void Stop();
    unsigned int  MBInputRegister[maxInputRegister];
    unsigned int  MBHoldingRegister[maxHoldingRegister];

private: 
    byte ByteArray[260];
    bool ledPinStatus = LOW;
    

    
#ifdef MB_ESP32
    EthernetServer server;
#endif
};

#endif
