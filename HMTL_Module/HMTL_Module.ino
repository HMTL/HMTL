/*******************************************************************************
 * Author: Adam Phelps
 * License: Create Commons Attribution-Non-Commercial
 * Copyright: 2014
 *
 * Code for a fully contained module which handles HMTL formatted messages
 * from a serial or RS485 connection.
 ******************************************************************************/

#include "EEPROM.h"
#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>
#include "SPI.h"
#include "FastLED.h"
#include "Wire.h"

#define DEBUG_LEVEL DEBUG_HIGH
#include "Debug.h"

#include "GeneralUtils.h"
#include "EEPromUtils.h"

#include "HMTLTypes.h"
#include "HMTLMessaging.h"
#include "HMTLProtocol.h"

#include "PixelUtil.h"

#include "Socket.h"
#include "RS485Utils.h"
#include "MPR121.h"

#include "HMTL_Module.h"

/* Auto update build number */
#define HMTL_MODULE_BUILD 12 // %META INCR

#define TYPE_HMTL_MODULE 0x1

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

#define SEND_BUFFER_SIZE 64
byte databuffer[RS485_BUFFER_TOTAL(SEND_BUFFER_SIZE)];
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

  DEBUG2_VALUELN("HMTL Module initialized, v", HMTL_MODULE_BUILD);
  Serial.println(F(HMTL_READY));
}

int cycle = 0;

#define MSG_MAX_SZ (sizeof(msg_hdr_t) + sizeof(msg_max_t))
byte msg[MSG_MAX_SZ];
byte offset = 0;

unsigned long last_msg_ms = 0;

void loop() {
  unsigned long now = millis();
  boolean update = false;

  if ((now - last_msg_ms > 10000) &&
      (now % 1000 == 0)) {
    /*
     * If the module has never received a message (last_msg_ms == 0) or it has
     * been a long time since the last message, itermittently resend the 'ready'
     * message.  This allows for connection methods that may miss the first
     * 'ready' to catch this one (such as Bluetooth).
     */
    Serial.println(F(HMTL_READY));
  }
  
  /* Check for messages on the serial interface */
  msg_hdr_t *msg_hdr = (msg_hdr_t *)msg;
  if (hmtl_serial_getmsg(msg, MSG_MAX_SZ, &offset)) {
    boolean forwarded = false;

    /* Received a complete message */
    DEBUG5_VALUE("Received msg len=", offset);
    DEBUG5_PRINT(" ");
    DEBUG5_COMMAND(
                  print_hex_string(msg, offset)
                  );
    DEBUG_PRINT_END();
    Serial.println(F(HMTL_ACK));

    if ((msg_hdr->address != config.address) ||
        (msg_hdr->address == RS485_ADDR_ANY)) {
      /* Forward the message over the rs485 interface */
      if (offset > SEND_BUFFER_SIZE) {
        DEBUG_ERR("Message larger than send buffer");
      } else {
        DEBUG4_VALUELN("Forwarding serial msg to ", msg_hdr->address);
        memcpy(send_buffer, msg, offset);
        rs485.sendMsgTo(msg_hdr->address, send_buffer, offset);
        forwarded = true;
      }
    }

    if (process_msg(msg_hdr, false, forwarded)) {
      update = true;
    }

    offset = 0;
    last_msg_ms = now;
  }

  /* Check for message over RS485 */
  unsigned int msglen;
  msg_hdr = hmtl_rs485_getmsg(&rs485, &msglen, config.address);
  if (msg_hdr != NULL) {
    DEBUG5_VALUE("Received rs485 msg len=", msglen);
    DEBUG5_PRINT(" ");
    DEBUG5_COMMAND(
                  print_hex_string((byte *)msg_hdr, msglen)
                  );
    DEBUG_PRINT_END();

    if (process_msg(msg_hdr, true, false)) {
      update = true;
    }

    last_msg_ms = now;
  }

  /* Execute any active programs */
  if (run_programs(outputs, objects, active_programs)) {
    update = true;
  }

  if (update) {
    /* Update the outputs */
    for (byte i = 0; i < config.num_outputs; i++) {
      hmtl_update_output(outputs[i], objects[i]);
    }
  }
}

/* Process a message if it is for this module */
boolean process_msg(msg_hdr_t *msg_hdr, boolean from_rs485, boolean forwarded) {
  if (msg_hdr->version != HMTL_MSG_VERSION) {
    DEBUG_ERR("Invalid message version");
    return false;
  }

  if ((msg_hdr->address == config.address) ||
      (msg_hdr->address == RS485_ADDR_ANY)) {

    if (msg_hdr->flags & MSG_FLAG_ACK) {
      /* 
       * This is an ack message that is not for us, resend it over serial in
       * case that was the original source.
       * TODO: Maybe this should check address as well, and serial needs to be
       * assigned an address?
       */
      DEBUG4_PRINTLN("Forwarding ack to serial");
      Serial.write((byte *)msg_hdr, msg_hdr->length);

      return false;
    }

    switch (msg_hdr->type) {

    case MSG_TYPE_OUTPUT: {
      output_hdr_t *out_hdr = (output_hdr_t *)(msg_hdr + 1);
      if (out_hdr->type == HMTL_OUTPUT_PROGRAM) {
        // TODO: This program stuff should be moved into the framework
        setup_program(outputs, active_programs, (msg_program_t *)out_hdr);
      } else {
        hmtl_handle_output_msg(msg_hdr, &config, outputs, objects);
      }

      return true;
    }

    case MSG_TYPE_POLL: {
      // TODO: This should be in framework as well
      uint16_t source_address = 0;
      uint16_t recv_buffer_size = 0;
      if (from_rs485) {
        // The response will be going over RS485, get the source address
        source_address = RS485_SOURCE_FROM_DATA(msg_hdr);
        recv_buffer_size = rs485.recvLimit;
      } else {
        recv_buffer_size = MSG_MAX_SZ;
      }

      DEBUG3_VALUELN("Poll req src:", source_address);

      // Format the poll response
      uint16_t len = hmtl_poll_fmt(send_buffer, SEND_BUFFER_SIZE,
                                   source_address,
                                   msg_hdr->flags, TYPE_HMTL_MODULE,
                                   &config, outputs, recv_buffer_size);

      // Respond to the appropriate source
      if (from_rs485) {
        if (msg_hdr->address == RS485_ADDR_ANY) {
          // If this was a broadcast address then do not respond immediately,
          // delay for time based on our address.
          int delayMs = config.address * 2;
          DEBUG3_VALUELN("Delay resp: ", delayMs)
          delay(delayMs);
        }

        rs485.sendMsgTo(source_address, send_buffer, len);
      } else {
        Serial.write(send_buffer, len);
      }

      break;
    }

    case MSG_TYPE_SET_ADDR: {
      /* Handle an address change message */
      msg_set_addr_t *set_addr = (msg_set_addr_t *)(msg_hdr + 1);
      if ((set_addr->device_id == 0) ||
          (set_addr->device_id == config.device_id)) {
        config.address = set_addr->address;
        rs485.sourceAddress = config.address;
        DEBUG2_VALUELN("Address changed to ", config.address);
      }
    }
    }
  }

  if (forwarded) {
    // Special handling for messages that we've forwarded along
    if (msg_hdr->flags & MSG_FLAG_RESPONSE) {
      // The sender expects a response
    }
  }

  return false;
}

/*
 * Code for program execution
 */

/* Setup a program from a HMTL program message */
boolean setup_program(output_hdr_t *outputs[],
                      program_tracker_t *trackers[],
                      msg_program_t *msg) {

  DEBUG4_VALUE("setup_program: program=", msg->type);
  DEBUG4_VALUELN(" output=", msg->hdr.output);

  /* Find the program to be executed */ 
  hmtl_program_t *program = NULL;
  for (byte i = 0; i < NUM_PROGRAMS; i++) {
    if (program_functions[i].type == msg->type) {
      program = &program_functions[i];
      break;
    }
  }
  if (program == NULL) {
    DEBUG1_VALUELN("setup_program: invalid type: ",
		  msg->type);
    return false;
  }

   /* Setup the tracker */
  if (msg->hdr.output > HMTL_MAX_OUTPUTS) {
    DEBUG1_VALUELN("setup_program: invalid output: ",
		  msg->hdr.output);
    return false;
  }
  if (outputs[msg->hdr.output] == NULL) {
    DEBUG1_VALUELN("setup_program: NULL output: ",
		  msg->hdr.output);
    return false;
  }

  program_tracker_t *tracker = trackers[msg->hdr.output];

  if (program->type == HMTL_PROGRAM_NONE) {
    free_tracker(trackers, msg->hdr.output);
    return true;
  }

  if (tracker != NULL) {
    DEBUG5_PRINTLN("setup_program: reusing old tracker");
    if (tracker->state) {
      DEBUG5_PRINTLN("setup_program: deleting old state");
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
      DEBUG3_VALUELN("free_tracker: clearing program for ", 
		    index);
      if (tracker->state) free(tracker->state);
      free(tracker);
    }
    trackers[index] = NULL;
}

/* Execute all active programs */
boolean run_programs(output_hdr_t *outputs[],
                     void *objects[],
                     program_tracker_t *trackers[]) {
  boolean updated = false;

  for (byte i = 0; i < HMTL_MAX_OUTPUTS; i++) {
    program_tracker_t *tracker = trackers[i];
    if (tracker != NULL) {
      if (tracker->done) {
        free_tracker(trackers, i);
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
 * Program function to turn an output on and off
 */
boolean program_blink_init(msg_program_t *msg, program_tracker_t *tracker) {
  DEBUG3_PRINT("Initializing blink program state");

  state_blink_t *state = (state_blink_t *)malloc(sizeof (state_blink_t));  
  memcpy(&state->msg, msg->values, sizeof (state->msg)); // ??? Correct size?
  state->on = false;
  state->next_change = millis();

  tracker->state = state;

  DEBUG3_VALUE(" on_period:", state->msg.on_period);
  DEBUG3_VALUELN(" off_period:", state->msg.off_period);

  return true;
}

boolean program_blink(output_hdr_t *output, void *object, program_tracker_t *tracker) {
  boolean changed = false;
  unsigned long now = millis();
  state_blink_t *state = (state_blink_t *)tracker->state;

  DEBUG5_PRINT("Blink");
  DEBUG5_VALUE(" now:", now);
  DEBUG5_VALUE(" next:", state->next_change);
  DEBUG5_VALUE(" on: ", state->on);

  if (now >= state->next_change) {
    if (state->on) {
      // Turn off the output
      hmtl_set_output_rgb(output, object, state->msg.off_value);
      
      state->on = false;
      state->next_change += state->msg.off_period;
    } else {
      // Turn on the output
      hmtl_set_output_rgb(output, object, state->msg.on_value);

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
  DEBUG3_PRINT("Initializing timed change program");

  state_timed_change_t *state = (state_timed_change_t *)malloc(sizeof (state_timed_change_t));  

  DEBUG3_VALUE(" msgsz=", sizeof (state->msg));

  memcpy(&state->msg, msg->values, sizeof (state->msg)); // ??? Correct size?
  state->change_time = 0;

  tracker->state = state;

  DEBUG3_VALUELN(" change_period:", state->msg.change_period);

  return true;
}

boolean program_timed_change(output_hdr_t *output, void *object, 
                             program_tracker_t *tracker) {
  boolean changed = false;
  unsigned long now = millis();
  state_timed_change_t *state = (state_timed_change_t *)tracker->state;

  if (state->change_time == 0) {
    // Set the initial color
    hmtl_set_output_rgb(output, object, state->msg.start_value);
    state->change_time = now + state->msg.change_period;
    changed = true;
  }

  if (now > state->change_time) {
    // Set the final color
    hmtl_set_output_rgb(output, object, state->msg.stop_value);
    
    // Disable the program
    tracker->done = true;
    changed = true;
  }

  return changed;
}
