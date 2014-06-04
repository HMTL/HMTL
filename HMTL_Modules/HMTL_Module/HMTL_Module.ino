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


/* message body and state for basic blink program */
#define PROGRAM_NONE  0x0
#define PROGRAM_BLINK 0x1
typedef struct {
  uint16_t on_period;
  uint8_t on_value[3];
  uint16_t off_period;
  uint8_t off_value[3];
} program_blink_t;
typedef struct {
  program_blink_t msg;
  boolean on;
  unsigned long next_change;
} state_blink_t;

/* List of available programs */
hmtl_program_t program_functions[] = {
  { PROGRAM_NONE, NULL, NULL},
  { PROGRAM_BLINK, program_blink, program_blink_init }
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
  Serial.begin(9600);

  for (byte i = 0; i < HMTL_MAX_OUTPUTS; i++) {
    active_programs[i] = NULL;
  }

  int32_t outputs_found = hmtl_setup(&config, readoutputs, 
				     outputs, objects, HMTL_MAX_OUTPUTS,
			     &rs485, &pixels, &rgb_output, &value_output,
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
    DEBUG_VALUE(DEBUG_HIGH, "Received msg len=", offset);
    DEBUG_PRINT(DEBUG_HIGH, " ");
    DEBUG_COMMAND(DEBUG_HIGH, 
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

      output_hdr_t *out_hdr = (output_hdr_t *)(++msg_hdr);
      if (out_hdr->type == HMTL_OUTPUT_PROGRAM) {
	// XXX: This stuff should be moved into the framework somehow
	DEBUG_PRINTLN(DEBUG_HIGH, "Received program command"); // XXX
	setup_program(outputs, active_programs, (msg_program_t *)out_hdr);
      } else {
	hmtl_handle_msg((msg_hdr_t *)&msg, &config, outputs, objects);
      }
      update = true;
    }

    offset = 0;
  }

  /* Check for message over RS485 */
  unsigned int msglen;
  msg_hdr = hmtl_rs485_getmsg(&rs485, &msglen, RS485_ADDR_ANY);
  if (msg_hdr != NULL) {
    DEBUG_VALUE(DEBUG_HIGH, "Received rs485 msg len=", msglen);
    DEBUG_PRINT(DEBUG_HIGH, " ");
    DEBUG_COMMAND(DEBUG_HIGH, 
		  print_hex_string((byte *)msg_hdr, msglen)
		  );
    DEBUG_PRINT_END();

    if ((msg_hdr->address == config.address) ||
	(msg_hdr->address == RS485_ADDR_ANY)) {
      hmtl_handle_msg(msg_hdr, &config, outputs, objects);
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

  if (program->type == PROGRAM_NONE) {
    if (tracker != NULL) {
      DEBUG_VALUELN(DEBUG_HIGH, "setup_progam: clearing program for", 
		    msg->hdr.output);
      if (tracker->state) free(tracker->state);
      free(tracker);
    }
    trackers[msg->hdr.output] = NULL;
    return true;
  }

  if (tracker != NULL) {
    DEBUG_PRINTLN(DEBUG_HIGH, "setup_program: reusing old tracker");
    if (tracker->state) {
      DEBUG_PRINTLN(DEBUG_HIGH, "setup_program: deleting old state");
      free(tracker->state);
    }
  } else {
    tracker = (program_tracker_t *)malloc(sizeof (program_tracker_t));
    trackers[msg->hdr.output] = tracker;
  }

  tracker->program = program;
  tracker->program->setup(msg, tracker);

  return true;
}

/* Execute all active programs */
boolean run_programs(output_hdr_t *outputs[],
		     program_tracker_t *trackers[]) {
  boolean updated = false;

  for (byte i = 0; i < HMTL_MAX_OUTPUTS; i++) {
    program_tracker_t *tracker = trackers[i];
    if (tracker != NULL) {
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
  memcpy(&state->msg, msg->values, MAX_PROGRAM_VAL);
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
      switch (output->type) {
      case HMTL_OUTPUT_VALUE:{
	config_value_t *val = (config_value_t *)output;
	val->value = state->msg.off_value[0];
	DEBUG_VALUE(DEBUG_TRACE, " Turned off:", val->value);
	break;
      }
      }
      
      state->on = false;
      state->next_change += state->msg.off_period;
    } else {
      // Turn on the output
      switch (output->type) {
      case HMTL_OUTPUT_VALUE:{
	config_value_t *val = (config_value_t *)output;
	val->value = state->msg.on_value[0];
	DEBUG_VALUE(DEBUG_TRACE, " Turned on:", val->value);
	break;
      }
      }

      state->on = true;
      state->next_change += state->msg.on_period;
    }

    changed = true;
  }

  DEBUG_PRINT_END();

  return changed;
}
