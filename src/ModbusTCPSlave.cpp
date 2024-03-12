#include <Arduino.h>
#include "ModbusTCPSlave.h"

ModbusTCPSlave::ModbusTCPSlave(void) : server(MB_PORT)

{
}

void ModbusTCPSlave::begin()
{
    server.begin();
}

void ModbusTCPSlave::Stop()
{

    //  server.close();
}

void ModbusTCPSlave::Run()
{
    boolean flagClientConnected = 0;
    byte byteFN = 0;
    int Start;
    int WordDataLength;
    int ByteDataLength;
    int MessageLength;

    //****************** Read from socket ****************

    EthernetClient client = server.available();

    while (client.connected())
    {
        if (client.available())
        {
            flagClientConnected = 1;
            int i = 0;
            while (client.available())
            {
                ByteArray[i] = client.read();
                i++;
            }

            client.flush();

            byteFN = ByteArray[MB_TCP_FUNC];
            Start = word(ByteArray[MB_TCP_REGISTER_START], ByteArray[MB_TCP_REGISTER_START + 1]);
            WordDataLength = word(ByteArray[MB_TCP_REGISTER_NUMBER], ByteArray[MB_TCP_REGISTER_NUMBER + 1]);
        }

        // Handle request

        switch (byteFN)
        {
        case 0:
            break;

        case 3: // 03 Read Holding Registers
            ByteDataLength = WordDataLength * 2;
            ByteArray[5] = ByteDataLength + 3; // Number of bytes after this one.
            ByteArray[8] = ByteDataLength;     // Number of bytes after this one (or number of bytes of data).
            for (int i = 0; i < WordDataLength; i++)
            {
                ByteArray[9 + i * 2] = highByte(MBHoldingRegister[Start + i]);
                ByteArray[10 + i * 2] = lowByte(MBHoldingRegister[Start + i]);
            }
            MessageLength = ByteDataLength + 9;
            client.write((const uint8_t *)ByteArray, MessageLength);

            //  client.stop();
            byteFN = 0;
            break;

        case 4: // 04 Read Input Registers
            Start = word(ByteArray[MB_TCP_REGISTER_START], ByteArray[MB_TCP_REGISTER_START + 1]);
            WordDataLength = word(ByteArray[MB_TCP_REGISTER_NUMBER], ByteArray[MB_TCP_REGISTER_NUMBER + 1]);
            ByteDataLength = WordDataLength * 2;
            ByteArray[5] = ByteDataLength + 3; // Number of bytes after this one.
            ByteArray[8] = ByteDataLength;     // Number of bytes after this one (or number of bytes of data).
            for (int i = 0; i < WordDataLength; i++)
            {
                ByteArray[9 + i * 2] = highByte(MBInputRegister[Start + i]);
                ByteArray[10 + i * 2] = lowByte(MBInputRegister[Start + i]);
            }
            MessageLength = ByteDataLength + 9;
            client.write((const uint8_t *)ByteArray, MessageLength);

            //  client.stop();
            byteFN = 0;
            break;

        case 6: // 06 Write Holding Register
            MBHoldingRegister[Start] = word(ByteArray[MB_TCP_REGISTER_NUMBER], ByteArray[MB_TCP_REGISTER_NUMBER + 1]);
            ByteArray[5] = 6; // Number of bytes after this one.
            MessageLength = 12;
            client.write((const uint8_t *)ByteArray, MessageLength);

            //  client.stop();
            byteFN = 0;
            break;

        case 16: // 16 Write Holding Registers
            ByteDataLength = WordDataLength * 2;
            ByteArray[5] = ByteDataLength + 3; // Number of bytes after this one.
            for (int i = 0; i < WordDataLength; i++)
            {
                MBHoldingRegister[Start + i] = word(ByteArray[13 + i * 2], ByteArray[14 + i * 2]);
            }
            MessageLength = 12;
            client.write((const uint8_t *)ByteArray, MessageLength);

            //  client.stop();
            byteFN = 0;

            break;
        }
    }

    //  client.stop();
}
