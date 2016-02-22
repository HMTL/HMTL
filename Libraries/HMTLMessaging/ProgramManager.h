/*******************************************************************************
 * Author: Adam Phelps
 * License: MIT
 * Copyright: 2015
 *
 * This provides a class to configure, track the state of, and execute simple
 * program functions.
 ******************************************************************************/

#ifndef PROGRAMMANAGER_H
#define PROGRAMMANAGER_H

#include "HMTLMessaging.h"
#include "TimeSync.h"

extern TimeSync time;


/*******************************************************************************
 * Function prototypes for a HMTL program and program configuration
 */
typedef struct program_tracker program_tracker_t;

typedef boolean (*hmtl_program_func)(output_hdr_t *output,
                                     void *object,
                                     program_tracker_t *tracker);
typedef boolean (*hmtl_program_setup)(msg_program_t *msg,
                                      program_tracker_t *tracker,
                                      output_hdr_t *output);

typedef struct {
  byte type;
  hmtl_program_func program;
  hmtl_program_setup setup;
} hmtl_program_t;

/* Structure used to track the state of currently active programs */
#define PROGRAM_TRACKER_DONE 0x1
struct program_tracker {
  hmtl_program_t *program;
  void *state;
  byte flags;
};

/*******************************************************************************
 * Program tracking, configuration, etc
 */

class ProgramManager {
 public:

  ProgramManager();
  ProgramManager(output_hdr_t **_outputs,
                 program_tracker_t **_trackers,
                 void **_objects,
                 byte _num_outputs,
                 hmtl_program_t *_functions, byte _num_programs);

  boolean handle_msg(msg_program_t *msg);

  boolean run_program(byte type, void *arg);

  boolean run();

  output_hdr_t **outputs;
  void **objects;
  byte num_outputs;

 private:
  program_tracker_t* new_tracker(int index);
  void free_tracker(int index);

  hmtl_program_t *lookup_function(byte type);

  hmtl_program_t *functions;
  byte num_programs;

  program_tracker_t **trackers;
};

/* Provide access to a time synchronization object */
extern TimeSync time;

#endif
