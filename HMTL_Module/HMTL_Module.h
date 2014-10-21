#ifndef HMTL_MODULE_H
#define HMTL_MODULE_H

typedef struct program_tracker program_tracker_t;

typedef boolean (*hmtl_program_func)(output_hdr_t *outputs,
				     program_tracker_t *tracker);
typedef boolean (*hmtl_program_setup)(msg_program_t *msg,
				      program_tracker_t *tracker);

typedef struct {
  byte type;
  hmtl_program_func program;
  hmtl_program_setup setup;
} hmtl_program_t;

/* Track the currently active programs */
struct program_tracker {
  hmtl_program_t *program;
  void *state;
  boolean done;
};


#endif
