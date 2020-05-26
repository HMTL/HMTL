/*******************************************************************************
 * Author: Adam Phelps
 * License: Create Commons Attribution-Non-Commercial
 * Copyright: 2020
 *
 * REST API endpoints for Wifi-Enabled modules
 ******************************************************************************/

#ifndef HMTL_MODULE_API_H
#define HMTL_MODULE_API_H

#include "Socket.h"

#include "HMTLPrograms.h"
#include "MessageHandler.h"

void setup_HMTL_API(Socket **s, MessageHandler *h, config_hdr_t *c);

void clearHandler();
void circularHandler();
void sparkleHandler();
void rgbHandler();
void rgbColorPicker();

void api_status();
void api_check();

#endif // HMTL_MODULE_API_H
