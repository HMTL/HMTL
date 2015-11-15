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

#include "HMTLPrograms.h"

/*******************************************************************************
 * Program tracking, configuration, etc
 */

class ProgramManager {
 public:
  ProgramManager();
  ProgramManager(output_hdr_t **_outputs,
                 program_tracker_t **_trackers,
                 byte _num_outputs,
                 hmtl_program_t *_functions, byte _num_programs);

  boolean handle_msg(msg_program_t *msg);

  boolean run(void **objects);

 private:
  void free_tracker(int index);

  output_hdr_t **outputs;
  byte num_outputs;

  hmtl_program_t *functions;
  byte num_programs;

  program_tracker_t **trackers;
};

#endif
