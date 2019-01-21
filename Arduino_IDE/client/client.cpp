//
// --------------------------------------------------------------------------
//  Gurux Ltd
//
//
//
//
// Version:         $Revision: 10346 $,
//                  $Date: 2018-10-29 16:08:18 +0200 (Mon, 29 Oct 2018) $
//                  $Author: gurux01 $
//
// Copyright (c) Gurux Ltd
//
//---------------------------------------------------------------------------
//
//  DESCRIPTION
//
// This file is a part of Gurux Device Framework.
//
// Gurux Device Framework is Open Source software; you can redistribute it
// and/or modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; version 2 of the License.
// Gurux Device Framework is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// More information of Gurux DLMS/COSEM Director: http://www.gurux.org/GXDLMSDirector
//
// This code is licensed under the GNU General Public License v2.
// Full text may be retrieved at http://www.gnu.org/licenses/gpl-2.0.txt
//---------------------------------------------------------------------------

// Building flags defined in platformio.ini

#include "connection.h"

#ifndef SERIAL_MODE
//Client don't need this.
unsigned char svr_isTarget(
    dlmsSettings *settings,
    unsigned long serverAddress,
    unsigned long clientAddress)
{
  return 0;
}

//Client don't need this.
int svr_connected(
    dlmsServerSettings *settings)
{
  return 0;
}

void time_now(
    gxtime *value)
{
  //Get local time somewhere.
  time_t tm1 = time(NULL);
  struct tm dt = *localtime(&tm1);
  time_init2(value, &dt);
}

gxClock clock1;

// Custom settings for the connection
#define LOGICAL_NAMES 1
#define CLIENT_ADDR 122
#define SERVER_ADDR 32767
#define AUTH_DLMS DLMS_AUTHENTICATION_LOW
#define PASS_DLMS "WSD2129c"
#endif

void setup()
{
  // start serial port at 9600 bps:
  MAIN_SERIAL.begin(9600);  //Main traffic between microcontroller and dlms/cosem meter
  AUX_SERIAL.begin(9600);   //To replicate DEBUG traffic
  DEBUG_SERIAL.begin(9600); //For Human-readable error messages
  while (!(MAIN_SERIAL && AUX_SERIAL && DEBUG_SERIAL))
  {
    ; // wait for serial port to connect. Needed for native USB port only
  }

#ifndef SERIAL_MODE
#ifdef TIVAboard
  initTime();
#endif

  bb_init(&frameData);
  //Set frame size.
  bb_capacity(&frameData, 128);
  cl_init(&meterSettings, LOGICAL_NAMES, CLIENT_ADDR, SERVER_ADDR, AUTH_DLMS, PASS_DLMS, DLMS_INTERFACE_TYPE_HDLC);
  cosem_init(&clock1.base, DLMS_OBJECT_TYPE_CLOCK, "0.0.1.0.0.255");
#endif
}

#ifdef SERIAL_MODE
int available = 0;
char data[256];
#endif

void loop()
{
#ifdef SERIAL_MODE
  available = AUX_SERIAL.available();
  if (available != 0)
  {
    AUX_SERIAL.readBytes(data, available); //TODO: desacoplar Arduino
    MAIN_SERIAL.write((const uint8_t *)data, available);
  }

  available = MAIN_SERIAL.available();
  if (available != 0)
  {
    MAIN_SERIAL.readBytes(data, available); //TODO: desacoplar Arduino
    AUX_SERIAL.write((const uint8_t *)data, available);
  }
#else
  int ret;
  //Initialize connection.
  ret = com_initializeConnection();
  if (ret != DLMS_ERROR_CODE_OK)
  {
    DEBUG_SERIAL.println("Init Error\n\r");
    delay(2000);
    return;
  }
  //Read clock.
  ret = com_read(&clock1.base, 2);
  if (ret != DLMS_ERROR_CODE_OK)
  {
    DEBUG_SERIAL.println("Read Error\n\r");
    delay(2000);
    return;
  }
  DEBUG_SERIAL.println(time_toUnixTime(&(clock1.time.value)));
  com_close();
  delay(5000);
#endif
}
