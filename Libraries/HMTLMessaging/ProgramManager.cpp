/*******************************************************************************
 * Author: Adam Phelps
 * License: MIT
 * Copyright: 2015
 *
 * This provides a class to configure, track the state of, and execute simple
 * program functions.
 ******************************************************************************/

#include <Arduino.h>

#ifndef DEBUG_LEVEL
  #define DEBUG_LEVEL DEBUG_ERROR
#endif
#include "Debug.h"

#include "HMTLPrograms.h"
#include "ProgramManager.h"

/*******************************************************************************
 * Program tracking, configuration, etc
 */

/* Provide access to a time synchronization object */
TimeSync time;

ProgramManager::ProgramManager() {
};

ProgramManager::ProgramManager(output_hdr_t **_outputs,
                               program_tracker_t **_trackers,
                               void **_objects,
                               byte _num_outputs,

                               hmtl_program_t *_functions,
                               byte _num_programs) {
  outputs = _outputs;
  trackers = _trackers;
  objects = _objects;
  num_outputs = _num_outputs;

  functions = _functions;
  num_programs = _num_programs;

  for (byte i = 0; i < num_outputs; i++) {
    trackers[i] = NULL;
  }

  DEBUG3_VALUE("ProgramManager: outputs:", num_outputs);
  DEBUG3_VALUELN(" programs:", num_programs);
}

/*
 * Lookup a program in the manager based on its ID
 */
byte ProgramManager::lookup_function(byte id) {
  /* Find the program to be executed */
  for (byte i = 0; i < num_programs; i++) {
    if (functions[i].type == id) {
      return i;
    }
  }
  return NO_PROGRAM;
}

/*
 * Process a program configuration message
 */
boolean ProgramManager::handle_msg(msg_program_t *msg) {
  DEBUG4_VALUE("handle_msg: program=", msg->type);
  DEBUG4_VALUELN(" output=", msg->hdr.output);

  /* Find the program to be executed */
  byte program = lookup_function(msg->type);
  if (program == NO_PROGRAM) {
    DEBUG1_VALUELN("handle_msg: invalid type: ", msg->type);
    return false;
  }

   /* Setup the tracker */
  byte starting_output, stop_output;

  if (msg->hdr.output == HMTL_ALL_OUTPUTS) {
    /* This should be applied to all outputs that can handle the message type */
    starting_output = 0;
    stop_output = num_outputs;
  } else if (msg->hdr.output > num_outputs) {
    DEBUG1_VALUELN("handle_msg: invalid output: ",
                   msg->hdr.output);
    return false;
  } else if (outputs[msg->hdr.output] == NULL) {
    DEBUG1_VALUELN("handle_msg: NULL output: ", msg->hdr.output);
    return false;
  } else {
    /* Only apply to the specified output */
    starting_output = msg->hdr.output;
    stop_output = starting_output + 1;
  }

  for (byte output = starting_output; output < stop_output; output++) {

    if (outputs[output] == NULL)
      continue;

    program_tracker_t *tracker = trackers[output];

    if (msg->type == HMTL_PROGRAM_NONE) {
      /* This is a message to clear the existing program so free the tracker */
      DEBUG3_VALUELN("handle_msg: clear ", output);
      free_tracker(output);
      continue;
    }

    /* If there was an active program on this output then clear the tracker */
    free_tracker(output);

    /* Setup a tracker for this program */
    tracker = new_tracker(output);
    tracker->program_index = program;
    tracker->flags = 0x0;

    /* Attempt to setup the program */
    boolean success = functions[tracker->program_index].setup(msg, tracker,
                                                              outputs[output]);
    if (!success) {
      DEBUG4_VALUELN("handle_msg: NA on ", output);
      free_tracker(output);
      continue;
    }
    DEBUG3_VALUELN("handle_msg: setup on ", output);
  }

  return true;
}

/*
 * Allocate a new program tracker
 */
program_tracker_t * ProgramManager::new_tracker(int index) {
  if (trackers[index] == NULL) {
    DEBUG3_VALUELN("new_tracker:", index);
    // TODO: Should these be allocated or assigned when manager is initialized?
    program_tracker_t *tracker = (program_tracker_t *) malloc(
            sizeof(program_tracker_t));
    trackers[index] = tracker;
  }

  memset(trackers[index], 0, sizeof (program_tracker_t));

  return trackers[index];
}

/*
 * Free a single program tracker.  This
 */
void ProgramManager::free_tracker(int index) {
  program_tracker_t *tracker = trackers[index];
  if (IS_RUNNING_PROGRAM(tracker)) {
    DEBUG3_VALUELN("free_tracker:", index);
    if (tracker->flags & PROGRAM_DEALLOC_STATE) {
      /*
       * If the tracker's flags indicate that the state should be deallocated
       * then do so now.
       */
      if (tracker->state) {
        free(tracker->state);
      }
    }

    /* Clear the tracker and set its program to NO_PROGRAM */
    memset(tracker, 0, sizeof (program_tracker_t));
    tracker->program_index = NO_PROGRAM;
  }
}

/*
 * Execute all configured program functions
 */
boolean ProgramManager::run() {
  boolean updated = false;

  for (byte i = 0; i < num_outputs; i++) {
    program_tracker_t *tracker = trackers[i];
    if (IS_RUNNING_PROGRAM(tracker)) {

      if (tracker->flags & PROGRAM_TRACKER_DONE) {
        /* If this program has been set as done then free its tracker */
        free_tracker(i);
        continue;
      }

      if (functions[tracker->program_index].program(outputs[i],
                                                    objects[i],
                                                    tracker)) {
        updated = true;
      }
    }
  }

  return updated;
}

/*
 * Run a single program without an object or tracker.  This is used for
 * providing custom sensor handlers (PROGRAM_SENSOR_DATA) and similar
 * external functions.
 */
boolean ProgramManager::run_program(byte type, void *arg) {
  byte program = lookup_function(type);
  if (program != NO_PROGRAM) {
    functions[program].program(NULL, arg, NULL);
  }

  return false;
}