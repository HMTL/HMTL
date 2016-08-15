/*******************************************************************************
 * Author: Adam Phelps
 * License: MIT
 * Copyright: 2014
 */

#ifndef HMTL_MODULE_H
#define HMTL_MODULE_H

#include <Arduino.h>
#include <HMTLTypes.h>
#include <ProgramManager.h>

boolean program_level_value(output_hdr_t *output, void *object,
                            program_tracker_t *tracker);
boolean program_level_value_init(msg_program_t *msg,
                                 program_tracker_t *tracker,
                                 output_hdr_t *output);
boolean program_sound_value_init(msg_program_t *msg,
                                 program_tracker_t *tracker,
                                 output_hdr_t *output);
boolean program_sound_value(output_hdr_t *output, void *object,
                            program_tracker_t *tracker);
boolean program_sensor_data_init(msg_program_t *msg,
                                 program_tracker_t *tracker,
                                 output_hdr_t *output);
boolean process_sensor_data(output_hdr_t *output,
                            void *object,
                            program_tracker_t *tracker);



#endif
