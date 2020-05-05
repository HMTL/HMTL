/*******************************************************************************
 * Author: Adam Phelps
 * License: MIT
 * Copyright: 2019
 *
 * Hard-coded startup programs that can be specified in the upload
 * script.
 ******************************************************************************/

#ifndef HMTL_MODULE_STARTUP_COMMANDS_H
#define HMTL_MODULE_STARTUP_COMMANDS_H

#ifndef PLATFORMIO
#define STARTUP_COMMANDS
//#define STARTUP_BLINK
//#define STARTUP_SPARKLE
//#define STARTUP_CIRCULAR
//#define STARTUP_VALUE
//#define STARTUP_FADE_ALL
#endif

void startup_commands(ProgramManager *manager, MessageHandler *handler,
                      Socket **sockets, config_hdr_t *config);

#endif //HMTL_MODULE_STARTUP_COMMANDS_H
