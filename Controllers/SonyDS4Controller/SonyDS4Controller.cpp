/*-----------------------------------------*\
|  SonyDS4Controller.cpp                    |
|                                           |
|  Controller for Sony Dualshock 4          |
|                                           |
|  Pol Rius (alpemwarrior) 24/09/2020       |
\*-----------------------------------------*/

#include <CRC.h>
#include <hidapi/hidapi.h>
#include "SonyDS4Controller.h"

SonyDS4Controller::SonyDS4Controller(hid_device * device_handle, const char * device_path)
{
    this->device_handle = device_handle;
    unsigned char readBuffer[64];
    unsigned char reportBuffer[64];
    reportBuffer[0] = 0x02;

    hid_get_feature_report(device_handle, reportBuffer, 64);
    for (int i = 0; i < 5; i++)
    {
        hid_read(device_handle, readBuffer, 64);
        if (readBuffer[0] == 17)
        {
            is_bluetooth = true;
            break;
        }
    }
    
    location = device_path;
}

std::string SonyDS4Controller::GetLocation()
{
    return("HID: " + location);
}

void SonyDS4Controller::SetColors(unsigned char red, unsigned char green, unsigned char blue)
{
    if(is_bluetooth)
    {
        sendReportBT(red, green, blue);
    }
    else
    {
        sendReportUSB(red, green, blue);
    }
}

void SonyDS4Controller::sendReportBT(unsigned char red, unsigned char green, unsigned char blue)
{
    unsigned char buffer[79] =
    {
        0xa2, 0x11, 0xC0, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, red, green, blue, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    
    uint32_t crc = CRCPP::CRC::Calculate(buffer, 75, CRCPP::CRC::CRC_32());
    unsigned char outbuffer[78];

    /*-------------------------------------------------*\
    | The report has to be signed with the byte 0xa2.   |
    | However, hidapi already adds 0xa2 to the report,  |
    | so we need to remove it from the buffer.          |
    \*-------------------------------------------------*/
    for(unsigned int i = 1; i < 79; i++)
    {
        outbuffer[i - 1] = buffer[i];
    }

    /*-------------------------------------------------*\
    | Add the crc32 to the end of the buffer            |
    \*-------------------------------------------------*/
    outbuffer[74] = (0x000000FF & crc);
    outbuffer[75] = (0x0000FF00 & crc) >> 8;
    outbuffer[76] = (0x00FF0000 & crc) >> 16;
    outbuffer[77] = (0xFF000000 & crc) >> 24;
    
    hid_write(device_handle, outbuffer, 78);
}

void SonyDS4Controller::sendReportUSB(unsigned char red, unsigned char green, unsigned char blue)
{
    uint8_t buffer[11] =
    {
        0x05,
        0x07,
        0x00,
        0x00,
        0x00,
        0x00,
        red,
        green,
        blue,
        0x00,
        0x00
    };

    hid_write(device_handle, buffer, 11);
}
