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

/* Provide access to a time synchronization object */
extern TimeSync timesync;

/*******************************************************************************
 * Function prototypes for a HMTL program and program configuration
 */
typedef struct program_tracker program_tracker_t;
class ProgramManager;

typedef boolean (*hmtl_program_func)(output_hdr_t *output,
                                     void *object, // TODO: This and output should go away
                                     program_tracker_t *tracker);
typedef boolean (*hmtl_program_setup)(msg_program_t *msg,
                                      program_tracker_t *tracker,
                                      output_hdr_t *output,
                                      void *object,
                                      ProgramManager *manager);

typedef struct {
  byte type;
  hmtl_program_func program;
  hmtl_program_setup setup;
} hmtl_program_t;

#define PROGRAM_TRACKER_DONE  0x1 // The running program has completed

// The program state should be deallocated when done
#define PROGRAM_DEALLOC_STATE 0x2

/* Structure used to track the state of currently active programs */
struct program_tracker {
  byte program_index;
  byte flags;
  output_hdr_t *output;
  void *state;
  void *object;
};

#define IS_RUNNING_PROGRAM(tracker) \
  ((tracker != NULL) && (tracker->program_index != NO_PROGRAM))

/*******************************************************************************
 * Program tracking, configuration, etc
 */

class ProgramManager {
 public:
  static const byte NO_PROGRAM = (byte)-1;

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

  byte program_from_tracker(program_tracker_t *tracker);

  byte lookup_output_by_type(uint8_t type, uint8_t num = 0);

  void *get_program_state(program_tracker_t *tracker, byte size,
                          void *preallocated = nullptr);
  void free_program_state(program_tracker_t *tracker);

 private:
  program_tracker_t* get_tracker(int index);
  void free_tracker(int index);

  byte lookup_function(byte type);

  hmtl_program_t *functions;
  byte num_programs;

  program_tracker_t **trackers;
};

#endif
