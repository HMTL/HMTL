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
hmtl_program_t *ProgramManager::lookup_function(byte id) {
  /* Find the program to be executed */
  for (byte i = 0; i < num_programs; i++) {
    if (functions[i].type == id) {
      return &functions[i];
    }
  }
  return NULL;
}

/*
 * Process a program configuration message
 */
boolean ProgramManager::handle_msg(msg_program_t *msg) {
  DEBUG4_VALUE("handle_msg: program=", msg->type);
  DEBUG4_VALUELN(" output=", msg->hdr.output);

  /* Find the program to be executed */
  hmtl_program_t *program = lookup_function(msg->type);
  if (program == NULL) {
    DEBUG1_VALUELN("handle_msg: invalid type: ",
		  msg->type);
    return false;
  }

   /* Setup the tracker */
  if (msg->hdr.output > num_outputs) {
    DEBUG1_VALUELN("handle_msg: invalid output: ",
		  msg->hdr.output);
    return false;
  }
  if (outputs[msg->hdr.output] == NULL) {
    DEBUG1_VALUELN("handle_msg: NULL output: ",
		  msg->hdr.output);
    return false;
  }

  program_tracker_t *tracker = trackers[msg->hdr.output];

  if (program->type == HMTL_PROGRAM_NONE) {
    /* This is a message to clear the existing program so free the tracker */
    free_tracker(msg->hdr.output);
    return true;
  }

  if (tracker != NULL) {
    DEBUG5_PRINTLN("handle_msg: reusing old tracker");
    if (tracker->state) {
      DEBUG5_PRINTLN("handle_msg: deleting old state");
      free(tracker->state);
    }
  } else {
    tracker = (program_tracker_t *)malloc(sizeof (program_tracker_t));
    trackers[msg->hdr.output] = tracker;
  }

  tracker->program = program;
  tracker->flags = 0x0;
  tracker->program->setup(msg, tracker);

  return true;
}

/*
 * Free a single program tracker
 */
void ProgramManager::free_tracker(int index) {
  program_tracker_t *tracker = trackers[index];
  if (tracker != NULL) {
      DEBUG3_VALUELN("free_tracker: clearing program for ",
		    index);
      if (tracker->state) free(tracker->state);
      free(tracker);
    }
    trackers[index] = NULL;
}

/*
 * Execute all configured program functions
 */
boolean ProgramManager::run() {
  boolean updated = false;

  for (byte i = 0; i < num_outputs; i++) {
    program_tracker_t *tracker = trackers[i];
    if (tracker != NULL) {

      if (tracker->flags & PROGRAM_TRACKER_DONE) {
        /* If this program has been set as done then free its tracker */
        free_tracker(i);
        continue;
      }

      if (tracker->program->program(outputs[i], objects[i], tracker)) {
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
  hmtl_program_t *program = lookup_function(type);

  program->program(NULL, arg, NULL);

  return false;
}