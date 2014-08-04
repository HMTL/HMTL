/*
 * Listen for HMTL formatted messages
 */

#include "EEPROM.h"
#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>
#include "SPI.h"
#include "Adafruit_WS2801.h"
#include "Wire.h"

#define DEBUG_LEVEL DEBUG_HIGH
#include "Debug.h"

#include "GeneralUtils.h"
#include "EEPromUtils.h"

#include "HMTLTypes.h"
#include "HMTLMessaging.h"

#include "PixelUtil.h"
#include "RS485Utils.h"
#include "MPR121.h"

#include "HMTL_Module.h"


/* Program states */
typedef struct {
  hmtl_program_blink_t msg;
  boolean on;
  unsigned long next_change;
} state_blink_t;

typedef struct {
  hmtl_program_timed_change_t msg;
  unsigned long change_time;
} state_timed_change_t;

/* List of available programs */
hmtl_program_t program_functions[] = {
  { HMTL_PROGRAM_NONE, NULL, NULL},
  { HMTL_PROGRAM_BLINK, program_blink, program_blink_init },
  { HMTL_PROGRAM_TIMED_CHANGE, program_timed_change, program_timed_change_init }
};
#define NUM_PROGRAMS (sizeof (program_functions) / sizeof (hmtl_program_t))

/* Track the currently active programs */
program_tracker_t *active_programs[HMTL_MAX_OUTPUTS];


RS485Socket rs485;
config_rgb_t rgb_output;
config_value_t value_output;

config_hdr_t config;
output_hdr_t *outputs[HMTL_MAX_OUTPUTS];
config_max_t readoutputs[HMTL_MAX_OUTPUTS];
void *objects[HMTL_MAX_OUTPUTS];

PixelUtil pixels;

#define SEND_BUFFER_SIZE (sizeof (rs485_socket_hdr_t) + 64)
byte databuffer[SEND_BUFFER_SIZE];
byte *send_buffer;

void setup() {
  //  Serial.begin(115200);
  Serial.begin(57600);

  for (byte i = 0; i < HMTL_MAX_OUTPUTS; i++) {
    active_programs[i] = NULL;
  }

  int32_t outputs_found = hmtl_setup(&config, readoutputs, 
				     outputs, objects, HMTL_MAX_OUTPUTS,
				     &rs485, &pixels, NULL,
				     &rgb_output, &value_output,
				     NULL);

  if (!(outputs_found & (1 << HMTL_OUTPUT_RS485))) {
    DEBUG_ERR("No RS485 config found");
    DEBUG_ERR_STATE(1);
  }

  /* Setup the RS485 connection */  
  rs485.setup();
  send_buffer = rs485.initBuffer(databuffer);

  DEBUG_PRINTLN(DEBUG_LOW, "HMTL MSG Test initialized");
  DEBUG_PRINTLN(DEBUG_LOW, "ready")
}

int cycle = 0;

#define MSG_MAX_SZ (sizeof(msg_hdr_t) + sizeof(msg_max_t))
byte msg[MSG_MAX_SZ];
byte offset = 0;

void loop() {
  boolean update = false;
  boolean serial = false;
  
  /* Check for messages on the serial interface */
  msg_hdr_t *msg_hdr = (msg_hdr_t *)msg;
  if (hmtl_serial_getmsg(msg, MSG_MAX_SZ, &offset)) {
    serial = true;
    /* Received a complete message */
    DEBUG_VALUE(DEBUG_TRACE, "Received msg len=", offset);
    DEBUG_PRINT(DEBUG_TRACE, " ");
    DEBUG_COMMAND(DEBUG_TRACE, 
		  print_hex_string(msg, offset)
		  );
    DEBUG_PRINT_END();
    DEBUG_PRINTLN(DEBUG_HIGH, "ok");

    if ((msg_hdr->address != config.address) ||
	(msg_hdr->address == RS485_ADDR_ANY)) {
      /* Forward the message over the rs485 interface */
      DEBUG_VALUELN(DEBUG_HIGH, "Forwarding serial msg to ", msg_hdr->address);
      memcpy(send_buffer, msg, offset);
      rs485.sendMsgTo(msg_hdr->address, send_buffer, offset);
    }

    if ((msg_hdr->address == config.address) ||
	(msg_hdr->address == RS485_ADDR_ANY)) {

      output_hdr_t *out_hdr = (output_hdr_t *)(msg_hdr + 1);
      if (out_hdr->type == HMTL_OUTPUT_PROGRAM) {
	// XXX: This stuff should be moved into the framework somehow
	setup_program(outputs, active_programs, (msg_program_t *)out_hdr);
      } else {
	hmtl_handle_msg(msg_hdr, &config, outputs, objects);
      }
      update = true;
    }

    offset = 0;
  }

  /* Check for message over RS485 */
  unsigned int msglen;
  msg_hdr = hmtl_rs485_getmsg(&rs485, &msglen, config.address);
  if (msg_hdr != NULL) {
    DEBUG_VALUE(DEBUG_TRACE, "Received rs485 msg len=", msglen);
    DEBUG_PRINT(DEBUG_TRACE, " ");
    DEBUG_COMMAND(DEBUG_TRACE, 
		  print_hex_string((byte *)msg_hdr, msglen)
		  );
    DEBUG_PRINT_END();

    if ((msg_hdr->address == config.address) ||
	(msg_hdr->address == RS485_ADDR_ANY)) {

      output_hdr_t *out_hdr = (output_hdr_t *)(msg_hdr + 1);
      if (out_hdr->type == HMTL_OUTPUT_PROGRAM) {
	// XXX: This stuff should be moved into the framework somehow
	setup_program(outputs, active_programs, (msg_program_t *)out_hdr);
      } else {
	hmtl_handle_msg(msg_hdr, &config, outputs, objects);
      }
      update = true;
    }
  }

  /* Execute any active programs */
  if (run_programs(outputs, active_programs)) {
    update = true;
  }

  if (update) {
    /* Update the outputs */
    for (byte i = 0; i < config.num_outputs; i++) {
      hmtl_update_output(outputs[i], objects[i]);
    }
  }
}

/*
 * Code for program execution
 */

/* Setup a program from a HMTL program message */
boolean setup_program(output_hdr_t *outputs[],
		      program_tracker_t *trackers[],
		      msg_program_t *msg) {

  DEBUG_VALUE(DEBUG_HIGH, "setup_program: program=", msg->type);
  DEBUG_VALUELN(DEBUG_HIGH, " output=", msg->hdr.output);

  /* Find the program to be executed */ 
  hmtl_program_t *program = NULL;
  for (byte i = 0; i < NUM_PROGRAMS; i++) {
    if (program_functions[i].type == msg->type) {
      program = &program_functions[i];
      break;
    }
  }
  if (program == NULL) {
    DEBUG_VALUELN(DEBUG_ERROR, "setup_program: invalid type: ",
		  msg->type);
    return false;
  }

   /* Setup the tracker */
  if (msg->hdr.output > HMTL_MAX_OUTPUTS) {
    DEBUG_VALUELN(DEBUG_ERROR, "setup_program: invalid output: ",
		  msg->hdr.output);
    return false;
  }
  if (outputs[msg->hdr.output] == NULL) {
    DEBUG_VALUELN(DEBUG_ERROR, "setup_program: NULL output: ",
		  msg->hdr.output);
    return false;
  }

  program_tracker_t *tracker = trackers[msg->hdr.output];

  if (program->type == HMTL_PROGRAM_NONE) {
    free_tracker(trackers, msg->hdr.output);
    return true;
  }

  if (tracker != NULL) {
    DEBUG_PRINTLN(DEBUG_TRACE, "setup_program: reusing old tracker");
    if (tracker->state) {
      DEBUG_PRINTLN(DEBUG_TRACE, "setup_program: deleting old state");
      free(tracker->state);
    }
  } else {
    tracker = (program_tracker_t *)malloc(sizeof (program_tracker_t));
    trackers[msg->hdr.output] = tracker;
  }

  tracker->program = program;
  tracker->done = false;
  tracker->program->setup(msg, tracker);

  return true;
}

/* Free a single program tracker */
void free_tracker(program_tracker_t *trackers[], int index) {
  program_tracker_t *tracker = trackers[index];
  if (tracker != NULL) {
      DEBUG_VALUELN(DEBUG_HIGH, "free_tracker: clearing program for ", 
		    index);
      if (tracker->state) free(tracker->state);
      free(tracker);
    }
    trackers[index] = NULL;
}

/* Execute all active programs */
boolean run_programs(output_hdr_t *outputs[],
		     program_tracker_t *trackers[]) {
  boolean updated = false;

  for (byte i = 0; i < HMTL_MAX_OUTPUTS; i++) {
    program_tracker_t *tracker = trackers[i];
    if (tracker != NULL) {
      if (tracker->done) {
	free_tracker(trackers, i);
	continue;
      }

      if (tracker->program->program(outputs[i], tracker)) {
	updated = true;
      }
    }
  }

  return updated;
}

/*
 * Program function to turn an output on and off
 */
boolean program_blink_init(msg_program_t *msg, program_tracker_t *tracker) {
  DEBUG_PRINT(DEBUG_HIGH, "Initializing blink program state");

  state_blink_t *state = (state_blink_t *)malloc(sizeof (state_blink_t));  
  memcpy(&state->msg, msg->values, sizeof (state->msg)); // ??? Correct size?
  state->on = false;
  state->next_change = millis();

  tracker->state = state;

  DEBUG_VALUE(DEBUG_HIGH, " on_period:", state->msg.on_period);
  DEBUG_VALUELN(DEBUG_HIGH, " off_period:", state->msg.off_period);

  return true;
}

boolean program_blink(output_hdr_t *output, program_tracker_t *tracker) {
  boolean changed = false;
  unsigned long now = millis();
  state_blink_t *state = (state_blink_t *)tracker->state;

  DEBUG_PRINT(DEBUG_TRACE, "Blink");
  DEBUG_VALUE(DEBUG_TRACE, " now:", now);
  DEBUG_VALUE(DEBUG_TRACE, " next:", state->next_change);
  DEBUG_VALUE(DEBUG_TRACE, " on: ", state->on);

  if (now >= state->next_change) {
    if (state->on) {
      // Turn off the output
      hmtl_set_output(output, state->msg.off_value);
      
      state->on = false;
      state->next_change += state->msg.off_period;
    } else {
      // Turn on the output
      hmtl_set_output(output, state->msg.on_value);

      state->on = true;
      state->next_change += state->msg.on_period;
    }

    changed = true;
  }

  DEBUG_PRINT_END();

  return changed;
}


/*
 * Program which sets a value and waits for a period setting another value
 */
boolean program_timed_change_init(msg_program_t *msg,
				  program_tracker_t *tracker) {
  DEBUG_PRINT(DEBUG_HIGH, "Initializing timed change program");

  state_timed_change_t *state = (state_timed_change_t *)malloc(sizeof (state_timed_change_t));  

  DEBUG_VALUE(DEBUG_HIGH, " msgsz=", sizeof (state->msg));

  memcpy(&state->msg, msg->values, sizeof (state->msg)); // ??? Correct size?
  state->change_time = 0;

  tracker->state = state;

  DEBUG_VALUELN(DEBUG_HIGH, " change_period:", state->msg.change_period);

  return true;
}

boolean program_timed_change(output_hdr_t *output, program_tracker_t *tracker) {
  boolean changed = false;
  unsigned long now = millis();
  state_timed_change_t *state = (state_timed_change_t *)tracker->state;

  if (state->change_time == 0) {
    // Set the initial color
    hmtl_set_output(output, state->msg.start_value);
    state->change_time = now + state->msg.change_period;
    changed = true;
  }

  if (now > state->change_time) {
    // Set the final color
    hmtl_set_output(output, state->msg.stop_value);
    
    // Disable the program
    tracker->done = true;
    changed = true;
  }

  return changed;
}

/* Set the indicated output to a 3 byte value */
void hmtl_set_output(output_hdr_t *output, uint8_t value[3]) {
  switch (output->type) {
  case HMTL_OUTPUT_VALUE:{
    config_value_t *val = (config_value_t *)output;
    val->value = value[0];
    break;
  }
  }
}
