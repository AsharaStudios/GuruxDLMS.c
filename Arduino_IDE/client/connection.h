#ifndef CONNECTION_H
#define CONNECTION_H

#include <dlmssettings.h>
#include <variant.h>
#include <cosem.h>
#include <client.h>
#include <converters.h>
#include <gxobjects.h>

#if defined NANOboard || defined MEGAboard
#include <Arduino.h>
#elif defined TIVAboard
#include <Energia.h>
#include "rtc_func.h"
#endif

#if defined MEGAboard || defined TIVAboard
#define MAIN_SERIAL Serial1
#define AUX_SERIAL Serial
#define DEBUG_SERIAL Serial2
#endif

//Received data.
extern gxByteBuffer frameData;
extern dlmsSettings meterSettings;

//Initialize connection to the meter.
int com_initializeConnection();

int com_close();

int com_readSerialPort(unsigned char eop);

int com_readDataBlock(message *messages, gxReplyData *reply);

//Report error on output;
void com_reportError(const char *description, gxObject *object, unsigned char attributeOrdinal, int ret);

//Get Association view.
int com_getAssociationView();

//Read object.
int com_read(gxObject *object, unsigned char attributeOrdinal);

int com_write(gxObject *object, unsigned char attributeOrdinal);

int com_method(gxObject *object, unsigned char attributeOrdinal, dlmsVARIANT *params);

//Read objects.
int com_readList(gxArray *list);

int com_readRowsByEntry(gxProfileGeneric *object, unsigned long index, unsigned long count);

int com_readRowsByRange(gxProfileGeneric *object, struct tm *start, struct tm *end);

//Read scalers and units. They are static so they are read only once.
int com_readScalerAndUnits();

//Read profile generic columns. They are static so they are read only once.
int com_readProfileGenericColumns();

//Read profile generics rows.
int com_readProfileGenerics();

int com_readValue(gxObject *object, unsigned char index);

// This function reads ALL objects that meter have excluded profile generic objects.
// It will loop all object's attributes.
int com_readValues();

//This function reads ALL objects that meter have. It will loop all object's attributes.
int com_readAllObjects();

// Read DLMS Data frame from the device.
int readDLMSPacket(gxByteBuffer *data, gxReplyData *reply);

#endif