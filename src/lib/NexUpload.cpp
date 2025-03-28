/**
 * @file NexUpload.cpp
 *
 * The implementation of download tft file for nextion. 
 *
 * @author  Chen Zengpeng (email:<zengpeng.chen@itead.cc>)
 * @date    2016/3/29
 * @copyright 
 * Copyright (C) 2014-2015 ITEAD Intelligent Systems Co., Ltd. \n
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */
#include "NexUpload.h"
#include "HardwareSerial.h"
HardwareSerial shw(0);

//#define USE_SOFTWARE_SERIAL
#ifdef USE_SOFTWARE_SERIAL
SoftwareSerial dbSerial(3, 2); /* RX:D3, TX:D2 */
#define DEBUG_SERIAL_ENABLE
#endif

#ifdef DEBUG_SERIAL_ENABLE
#define dbSerialPrint(a)    shw.print(a)
#define dbSerialPrintln(a)  shw.println(a)
#define dbSerialBegin(a)    shw.begin(a)
#else
#define dbSerialPrint(a)    do{}while(0)
#define dbSerialPrintln(a)  do{}while(0)
#define dbSerialBegin(a)    do{}while(0)
#endif

NexUpload::NexUpload(const char *file_name,const uint8_t SD_chip_select,uint32_t download_baudrate)
{
    _file_name = file_name; 
    _SD_chip_select = SD_chip_select;
    _download_baudrate = download_baudrate;
}

NexUpload::NexUpload(const String file_Name,const uint8_t SD_chip_select,uint32_t download_baudrate)
{
    NexUpload(file_Name.c_str(),SD_chip_select,download_baudrate);
}

void NexUpload::upload(void)
{
    shw.begin(921600);
    if(!_checkFile())
    {
        shw.println("the file is error");
        return;
    }
    else
    {
        shw.println("File ok\r\n");
    }
    if(_getBaudrate() == 0)
    {
        shw.println("get baudrate error");
        return;
    }
    else
    {
        shw.println("Baudrate ok\r\n");
    }
    if(!_setDownloadBaudrate(_download_baudrate))
    {
        shw.println("modify baudrate error");
        return;
    }
    else
    {
        shw.println("Modify baudrate ok\r\n");
    }
    if(!_downloadTftFile())
    {
        shw.println("download file error");
        return;
    }
    else
    {
        shw.println("Download file ok\r\n");
    }
    shw.println("download ok\r\n");
}


void NexUpload::_writeString(String stringData) 
{
    shw.write(0xff);
    shw.write(0xff);
    shw.write(0xff);

    for (int i = 0; i < stringData.length(); i++) 
    {
        // Enviamos los datos
        shw.write(stringData[i]);
    }

    // Indicamos a la pantalla que ya hemos enviado todos
    // los datos, con este triple uint8_t 0xFF
    shw.write(0xff);
    shw.write(0xff);
    shw.write(0xff);

}

uint16_t NexUpload::_getBaudrate(void)
{
    uint32_t baudrate_array[12] = {921600,512000,256000,250000,230400,115200,19200,9600,57600,38400,4800,2400};
    for(uint8_t i = 0; i < 7; i++)
    {
        if(_searchBaudrate(baudrate_array[i]))
        {
            _baudrate = baudrate_array[i];
            shw.println("get baudrate");
            break;
        }
    }
    return _baudrate;
}

bool NexUpload::_checkFile(void)
{
    shw.println("start _checkFile");
    if(!SD.begin(_SD_chip_select))
    {
        shw.println("init sd failed");
        return 0;
    }
    if(!SD.exists(_file_name))
    {
        shw.println("file is not exit");
    }
   _myFile = SD.open(_file_name);
    _undownloadByte = _myFile.size();
    shw.println("tft file size is:");
    shw.println(_undownloadByte);
    shw.println("check file ok");
    return 1;
}

bool NexUpload::_searchBaudrate(uint32_t baudrate)
{
    String string = String("");  
    shw.begin(baudrate);
    this->sendCommand("");
    this->sendCommand("connect");
    this->recvRetString(string);  
    if(string.indexOf("comok") != -1)
    {
        return 1;
    } 
    return 0;
}

void NexUpload::sendCommand(const char* cmd)
{

    while (shw.available())
    {
        shw.read();
    }

    shw.print(cmd);
    shw.write(0xFF);
    shw.write(0xFF);
    shw.write(0xFF);
}

uint16_t NexUpload::recvRetString(String &string, uint32_t timeout,bool recv_flag)
{
    uint16_t ret = 0;
    uint8_t c = 0;
    long start;
    bool exit_flag = false;
    start = millis();
    while (millis() - start <= timeout)
    {
        while (shw.available())
        {
            c = shw.read(); 
            if(c == 0)
            {
                continue;
            }
            string += (char)c;
            if(recv_flag)
            {
                if(string.indexOf(0x05) != -1)
                { 
                    exit_flag = true;
                } 
            }
        }
        if(exit_flag)
        {
            break;
        }
    }
    ret = string.length();
    return ret;
}

bool NexUpload::_setDownloadBaudrate(uint32_t baudrate)
{
    String string = String(""); 
    String cmd = String("");
    
    String filesize_str = String(_undownloadByte,10);
    String baudrate_str = String(baudrate,10);
    cmd = "whmi-wris " + filesize_str + "," + baudrate_str + ",1";
    
    shw.println(cmd);
    this->sendCommand("");
    this->sendCommand(cmd.c_str());
    delay(50);
    shw.begin(baudrate);
    this->recvRetString(string,500);  
    if(string.indexOf(0x05) != -1)
    { 
        return 1;
    } 
    return 0;
}

bool NexUpload::_downloadTftFile(void)
{
    uint8_t c;
    uint16_t send_timer = 0;
    uint16_t last_send_num = 0;
    String string = "";
    send_timer = _undownloadByte / 4096 + 1;
    last_send_num = _undownloadByte % 4096;

    while(send_timer)
    {

        if(send_timer == 1)
        {
            for(uint16_t j = 1; j <= 4096; j++)
            {
                if(j <= last_send_num)
                {
                    c = _myFile.read();
                    shw.write(c);
                }
                else
                {
                    break;
                }
            }

        }
        else
        {
            for(uint16_t i = 1; i <= 4096; i++)
            {
                c = _myFile.read();
                shw.write(c);
            }

        }

        String txt = "Left: " + String(_undownloadByte) + " bytes";
        _writeString("statusLCD.txt=\"" + txt + "\"");

        this->recvRetString(string,500,true);  
        if(string.indexOf(0x05) != -1)
        { 
            string = "";
        } 
        else
        {
            return 0;
        }
         --send_timer;
    }  
}

