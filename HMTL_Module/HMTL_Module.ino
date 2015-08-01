/*******************************************************************************
 * Author: Adam Phelps
 * License: MIT
 * Copyright: 2014
 *
 * Code for a fully contained module which handles HMTL formatted messages
 * from a serial, RS485, or XBee connection.   It also functions as a bridge
 * router, retransmitting messages from one connections over any others that
 * are present.
 ******************************************************************************/

#include "EEPROM.h"
#include <RS485_non_blocking.h>
#include <SoftwareSerial.h>
#include "SPI.h"
#include "FastLED.h"
#include "Wire.h"
#include "XBee.h"



#ifndef DEBUG_LEVEL
  #define DEBUG_LEVEL DEBUG_HIGH
#endif
#include "Debug.h"

#include "GeneralUtils.h"
#include "EEPromUtils.h"

#include "HMTLTypes.h"
#include "HMTLMessaging.h"
#include "HMTLProtocol.h"

#include "PixelUtil.h"

#include "Socket.h"
#include "RS485Utils.h"
#include "XBeeSocket.h"
#include "MPR121.h"

#include "HMTL_Module.h"

/* Auto update build number */
#define HMTL_MODULE_BUILD 24 // %META INCR

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

typedef struct {
  uint16_t value;
} state_level_value_t;

typedef struct {
  uint16_t value;
  uint32_t max;
} state_sound_value_t;

/* List of available programs */
hmtl_program_t program_functions[] = {
  { HMTL_PROGRAM_NONE, NULL, NULL},
  { HMTL_PROGRAM_BLINK, program_blink, program_blink_init },
  { HMTL_PROGRAM_TIMED_CHANGE, program_timed_change, program_timed_change_init },
  { HMTL_PROGRAM_LEVEL_VALUE, program_level_value, program_level_value_init },
  { HMTL_PROGRAM_SOUND_VALUE, program_sound_value, program_sound_value_init },
  { HMTL_PROGRAM_FADE, program_fade, program_fade_init },
};
#define NUM_PROGRAMS (sizeof (program_functions) / sizeof (hmtl_program_t))

/* Track the currently active programs */
program_tracker_t *active_programs[HMTL_MAX_OUTPUTS];


#define SEND_BUFFER_SIZE 64 // The data size for transmission buffers

boolean has_rs485;
RS485Socket rs485;
byte rs485_data_buffer[RS485_BUFFER_TOTAL(SEND_BUFFER_SIZE)];

boolean has_xbee;
XBeeSocket xbee;
byte xbee_data_buffer[RS485_BUFFER_TOTAL(SEND_BUFFER_SIZE)];

Socket *serial_socket = NULL;

config_hdr_t config;
output_hdr_t *outputs[HMTL_MAX_OUTPUTS];
config_max_t readoutputs[HMTL_MAX_OUTPUTS];
void *objects[HMTL_MAX_OUTPUTS];

PixelUtil pixels;



/*
 * Data from sensors, set to highest analog value
 */
uint16_t level_data = 1023;
uint16_t light_data = 1023;

byte sound_channels = 0;
#define SOUND_CHANNELS 8
uint16_t sound_data[SOUND_CHANNELS] = { 0, 0, 0, 0, 0, 0, 0, 0 };

void setup() {

  //  Serial.begin(115200);
  Serial.begin(57600);

  for (byte i = 0; i < HMTL_MAX_OUTPUTS; i++) {
    active_programs[i] = NULL;
  }

  int32_t outputs_found = hmtl_setup(&config, readoutputs,
                                     outputs, objects, HMTL_MAX_OUTPUTS,
                                     &rs485, 
                                     &xbee,
                                     &pixels, 
                                     NULL, // MPR121
                                     NULL, // RGB
                                     NULL, // Value
                                     NULL);

  if (!(outputs_found & (1 << HMTL_OUTPUT_RS485))) {
    DEBUG_ERR("No RS485 config found");
    DEBUG_ERR_STATE(1);
  }

  if (outputs_found & (1 << HMTL_OUTPUT_RS485)) {
    /* Setup the RS485 connection */  
    rs485.setup();
    rs485.initBuffer(rs485_data_buffer, SEND_BUFFER_SIZE);
    has_rs485 = true;
    serial_socket = &rs485;
  } else {
    has_rs485 = false;
  }

  if (outputs_found & (1 << HMTL_OUTPUT_XBEE)) {
    /* Setup the RS485 connection */  
    xbee.setup();
    xbee.initBuffer(xbee_data_buffer, SEND_BUFFER_SIZE);
    has_xbee = true;
    serial_socket = &xbee;
  } else {
    has_xbee = false;
  }


  DEBUG2_VALUELN("HMTL Module initialized, v", HMTL_MODULE_BUILD);
  Serial.println(F(HMTL_READY));

  //startup_commands();
}

/*******************************************************************************
 * Execute any startup commands
 */
void startup_commands() {
  // TODO: These should be stored in EEPROM

  #define STARTUP_MSGS 4
  msg_hdr_t *startupmsg[STARTUP_MSGS];

  for (byte i = 0; i < STARTUP_MSGS; i++) {
    byte length = sizeof (msg_hdr_t) + sizeof (msg_program_t);
    startupmsg[i] = (msg_hdr_t *)malloc(length);

    startupmsg[i]->version = HMTL_MSG_VERSION;
    startupmsg[i]->type = MSG_TYPE_OUTPUT;
    startupmsg[i]->flags = 0;
    startupmsg[i]->address = 0; // XXX: This address?

#if 1
    msg_value_t *value = (msg_value_t *)(startupmsg[i] + 1);
    value->hdr.type = HMTL_OUTPUT_VALUE;
    value->hdr.output = i;
    value->value = 128;
#else
    msg_program_t *program = (msg_program_t *)(startupmsg[i] + 1);
    programs->hdr.type = HMTL_OUTPUT_PROGRAM;
    programs->hdr.output = i;
    programs->type = ;
#endif
  }

  /* 
   * Process the startup commands, forwarding them if they're broadcast or
   * to some other address, and then apply them locally
   */
  for (byte i = 0; i < STARTUP_MSGS; i++) {
    boolean forwarded = false;
    if (has_rs485) {
      forwarded = check_and_forward(startupmsg[i], &rs485);
    }
    if (has_xbee) {
      forwarded = check_and_forward(startupmsg[i], &xbee);
    }

    process_msg(startupmsg[i], false, forwarded);
  }

  /* Free the startupup messages */
  for (byte i = 0; i < STARTUP_MSGS; i++) {
    free(startupmsg[i]);
  }
}

#define MSG_MAX_SZ (sizeof(msg_hdr_t) + sizeof(msg_max_t))
byte msg[MSG_MAX_SZ];
byte offset = 0;

#define READY_THRESHOLD 10000
#define READY_RESEND_PERIOD 1000
unsigned long last_serial_ms = 0;
unsigned long last_ready_ms = 0;

/*******************************************************************************
 * The main event loop
 *
 * - Checks for and handles messages over all interfaces
 * - Runs any enabled programs
 * - Updates any outputs
 */
void loop() {
  unsigned long now = millis();
  boolean update = false;

  if ((now - last_serial_ms > READY_THRESHOLD) && 
      (now - last_ready_ms > READY_RESEND_PERIOD)) {
    /*
     * If the module has never received a message (last_serial_ms == 0) or it has
     * been a long time since the last message, itermittently resend the 'ready'
     * message.  This allows for connection methods that may miss the first
     * 'ready' to catch this one (such as Bluetooth).
     */
    Serial.println(F(HMTL_READY));
    last_ready_ms = now;
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

    if (has_rs485) {
      /* Forward the message over RS485 if needed */
      forwarded = check_and_forward(msg_hdr, &rs485);
    }
    if (has_xbee) {
      /* Forward the message over XBee if needed */
      forwarded = check_and_forward(msg_hdr, &xbee);
    }


    if (process_msg(msg_hdr, NULL, forwarded)) {
      update = true;
    }

    offset = 0;
    last_serial_ms = now;
  }

  if (has_rs485) {
    /* Check for message over RS485 */
    unsigned int msglen;
    msg_hdr = hmtl_socket_getmsg(&rs485, &msglen, config.address);
    if (msg_hdr != NULL) {
      DEBUG5_VALUE("Received rs485 msg len=", msglen);
      DEBUG5_PRINT(" ");
      DEBUG5_COMMAND(
                     print_hex_string((byte *)msg_hdr, msglen)
                     );
      DEBUG_PRINT_END();

      if (process_msg(msg_hdr, &rs485, false)) {
        update = true;
      }
    }
  }

  if (has_xbee) {
    /* Check for message over Xbee */
    unsigned int msglen;
    msg_hdr = hmtl_socket_getmsg(&xbee, &msglen, config.address);
    if (msg_hdr != NULL) {
      DEBUG5_VALUE("Received XBee msg len=", msglen);
      DEBUG5_PRINT(" ");
      DEBUG5_COMMAND(
                     print_hex_string((byte *)msg_hdr, msglen)
                     );
      DEBUG_PRINT_END();
      
      if (process_msg(msg_hdr, &xbee, false)) {
        update = true;
      }
    }

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
boolean process_msg(msg_hdr_t *msg_hdr, Socket *src, boolean forwarded) {
  if (msg_hdr->version != HMTL_MSG_VERSION) {
    DEBUG_ERR("Invalid message version");
    return false;
  }

  if ((msg_hdr->address == config.address) ||
      (msg_hdr->address == SOCKET_ADDR_ANY)) {

    if ((msg_hdr->flags & MSG_FLAG_ACK) && 
        (msg_hdr->address != SOCKET_ADDR_ANY)) {
      /* 
       * This is an ack message that is not for us, resend it over serial in
       * case that was the original source.
       * TODO: Maybe this should check address as well, and serial needs to be
       * assigned an address?
       */
      DEBUG4_PRINTLN("Forwarding ack to serial");
      Serial.write((byte *)msg_hdr, msg_hdr->length);

      if (msg_hdr->type != MSG_TYPE_SENSOR) { // Sensor broadcasts are for everyone
        return false;
      }
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
        Socket *sock;

        if (src != NULL) {
          // The response will be going over a socket, get the source address
          source_address = src->sourceFromData(msg_hdr);
          recv_buffer_size = src->recvLimit;
          sock = src;
        } else {
          recv_buffer_size = MSG_MAX_SZ;
          sock = serial_socket;
        }

        DEBUG3_VALUELN("Poll req src:", source_address);

        // Format the poll response
        uint16_t len = hmtl_poll_fmt(sock->send_buffer, sock->send_data_size,
                                     source_address,
                                     msg_hdr->flags, TYPE_HMTL_MODULE,
                                     &config, outputs, recv_buffer_size);

        // Respond to the appropriate source
        if (src != NULL) {
          if (msg_hdr->address == SOCKET_ADDR_ANY) {
            // If this was a broadcast address then do not respond immediately,
            // delay for time based on our address.
            int delayMs = config.address * 2;
            DEBUG3_VALUELN("Delay resp: ", delayMs)
              delay(delayMs);
          }

          src->sendMsgTo(source_address, sock->send_buffer, len);
        } else {
          Serial.write(sock->send_buffer, len);
        }

        break;
      }

      case MSG_TYPE_SET_ADDR: {
        /* Handle an address change message */
        msg_set_addr_t *set_addr = (msg_set_addr_t *)(msg_hdr + 1);
        if ((set_addr->device_id == 0) ||
            (set_addr->device_id == config.device_id)) {
          config.address = set_addr->address;
          src->sourceAddress = config.address;
          DEBUG2_VALUELN("Address changed to ", config.address);
        }
        break;
      }

      case MSG_TYPE_SENSOR: {
        if (msg_hdr->flags & MSG_FLAG_ACK) {
          /*
           * This is a sensor response, record relevant values for usage
           * elsewhere.
           */
          msg_sensor_data_t *sensor = NULL;
          while ((sensor = hmtl_next_sensor(msg_hdr, sensor))) {
            process_sensor_data(sensor);
          }
          DEBUG_PRINT_END();
        }
        break;
      }
    }
  }

  if (forwarded) {
    // Special handling for messages that we've forwarded along
    if (msg_hdr->flags & MSG_FLAG_RESPONSE) {
      // The sender expects a response
    }
    // TODO: Why is this section here?
  }

  return false;
}

/*
 * Check if a message should be forwarded and transmit it over the
 * a socket if so.
 */
boolean check_and_forward(msg_hdr_t *msg_hdr, Socket *socket) {
  if ((msg_hdr->address != config.address) ||
      (msg_hdr->address == SOCKET_ADDR_ANY)) {
    /*
     * Messages that are not to this module's address or are on the broadcast
     * address should be forwarded.
     */
    if (msg_hdr->length > socket->send_data_size) {
      DEBUG1_VALUELN("Message larger than send buffer:", msg_hdr->length);
    } else {
      DEBUG4_HEXVALLN("Forwarding serial msg to ", msg_hdr->address);
      memcpy(socket->send_buffer, msg_hdr, msg_hdr->length);
      socket->sendMsgTo(msg_hdr->address, socket->send_buffer, msg_hdr->length);
      return true;
    }
  }

  // The message was not forwarded over the socket
  return false;
}

/*
 * Record relevant sensor data for programatic usage
 */
void process_sensor_data(msg_sensor_data_t *sensor) {
  void *data = (void *)&sensor->data;

  switch (sensor->sensor_type) {
    case HMTL_SENSOR_SOUND: {
      byte copy_len;
      if (sizeof (uint16_t) * SOUND_CHANNELS < sensor->data_len) {
        copy_len = sizeof (uint16_t) * SOUND_CHANNELS;
      } else {
        copy_len = sensor->data_len;
      }

      memcpy(sound_data, data, copy_len);
      sound_channels = sensor->data_len / sizeof (uint16_t);
      DEBUG4_COMMAND(
                     DEBUG4_VALUE(" SOUND:", sound_channels);
                     for (byte i = 0; i < sound_channels; i++) {
                       DEBUG4_HEXVAL(" ", sound_data[i]);
                     }
                     );
      break;
    }
    case HMTL_SENSOR_LIGHT: {
      light_data = *(uint16_t *)data;
      DEBUG4_VALUE(" LIGHT:", light_data);
      break;
    }
    case HMTL_SENSOR_POT: {
      level_data = *(uint16_t *)data;
      DEBUG4_VALUE(" LEVEL:", level_data);
      break;
    }
    default: {
      DEBUG1_PRINT(" ERROR: UNKNOWN TYPE");
      break;
    }
  }
}

/*******************************************************************************
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

/*******************************************************************************
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

boolean program_blink(output_hdr_t *output, void *object, 
                      program_tracker_t *tracker) {
  boolean changed = false;
  unsigned long now = millis();
  state_blink_t *state = (state_blink_t *)tracker->state;

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

    DEBUG4_PRINT("Blink");
    DEBUG4_VALUE(" now:", now);
    DEBUG4_VALUE(" next:", state->next_change);
    DEBUG4_VALUE(" on: ", state->on);

    changed = true;
  }

  DEBUG_PRINT_END();

  return changed;
}

/*******************************************************************************
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

/*******************************************************************************
 * Program to set the value level based on the most recent sensor data
 */

boolean program_level_value_init(msg_program_t *msg,
                                 program_tracker_t *tracker) {
  DEBUG3_PRINTLN("Initializing level value state");

  state_level_value_t *state = (state_level_value_t *)malloc(sizeof (state_level_value_t));
  state->value = 0;
  
  tracker->state = state;

  return true;
}

boolean program_level_value(output_hdr_t *output, void *object,
                            program_tracker_t *tracker) {
  state_level_value_t *state = (state_level_value_t *)tracker->state;
  if (state->value != level_data) {
    state->value = level_data;
    uint8_t mapped = map(level_data, 0, 1023, 0, 255);
    uint8_t values[3] = {mapped, mapped, mapped};
    hmtl_set_output_rgb(output, object, values);

    DEBUG3_VALUELN("Level value:", mapped);

    return true;
  }

  return false;
}

/*******************************************************************************
 * Program to set the value based on the most recent sound data
 */

boolean program_sound_value_init(msg_program_t *msg,
                                 program_tracker_t *tracker) {
  DEBUG3_PRINTLN("Initializing sound value state");

  state_sound_value_t *state = (state_sound_value_t *)malloc(sizeof (state_sound_value_t));
  state->value = 0;
  state->max = 0;
  
  tracker->state = state;

  return true;
}

boolean program_sound_value(output_hdr_t *output, void *object,
                            program_tracker_t *tracker) {
  state_sound_value_t *state = (state_sound_value_t *)tracker->state;

  uint32_t total = 0;
  for (int i = 0; i < sound_channels; i++) {
    total += sound_data[i];
  }
  if (total > state->max) {
    state->max = total;
    DEBUG3_VALUELN("Sound max:", total);
  }

  DEBUG3_VALUE("Check out:", output->output);
  DEBUG3_VALUELN(" value:", total);

  uint8_t mapped = 0;
  if (total > 2) {
    // To avoid noise while quiet set a minimumza
    mapped = map(total, 0, state->max, 0, 255);
  }
  if (mapped != state->value) {
    state->value = mapped;

    uint8_t values[3] = {mapped, mapped, mapped};
    hmtl_set_output_rgb(output, object, values);

    return true;
  }

  return false;
}

/*******************************************************************************
 * Program to fade between two values
 */

typedef struct {
  hmtl_program_fade_t msg;
  unsigned long start_time;
} state_fade_t;

boolean program_fade_init(msg_program_t *msg,
                          program_tracker_t *tracker) {
  DEBUG3_PRINT("Initializing fade program:");

  state_fade_t *state = (state_fade_t *)malloc(sizeof (state_fade_t));
  memcpy(&state->msg, msg->values, sizeof (state->msg));
  tracker->state = state;

  DEBUG3_VALUE(" ", state->msg.period);
  DEBUG3_VALUE(" ", state->msg.start_value[0]);
  DEBUG3_VALUE(",", state->msg.start_value[1]);
  DEBUG3_VALUE(",", state->msg.start_value[2]);
  DEBUG3_VALUE(" ", state->msg.stop_value[0]);
  DEBUG3_VALUE(",", state->msg.stop_value[1]);
  DEBUG3_VALUELN(",", state->msg.stop_value[2]);

  state->start_time = 0;

  return true;
}

boolean program_fade(output_hdr_t *output, void *object, 
                     program_tracker_t *tracker) {
  boolean changed = false;
  unsigned long now = millis();
  state_fade_t *state = (state_fade_t *)tracker->state;

  if (state->start_time == 0) {
    // Set the initial color
    hmtl_set_output_rgb(output, object, state->msg.start_value);
    changed = true;
    state->start_time = now;
    DEBUG5_VALUELN("Fade ms:", now);
  } else {
    // Calculate the color at this time
    CRGB start = CRGB(state->msg.start_value[0],
                      state->msg.start_value[1],
                      state->msg.start_value[2]);
    CRGB stop = CRGB(state->msg.stop_value[0],
                     state->msg.stop_value[1],
                     state->msg.stop_value[2]);

    // TODO: There is probably a more efficient way to do this
    unsigned int elapsed = now - state->start_time;
    if (elapsed >= state->msg.period) {
      // Disable the program
      tracker->done = true;
      elapsed = state->msg.period;
    }
    fract8 fraction = map(elapsed, 0, state->msg.period, 0, 255);

    CRGB current = blend(start, stop, fraction);
    hmtl_set_output_rgb(output, object, current.raw);
    changed = true;

    DEBUG5_VALUE("Fade ms:", now);
    DEBUG5_VALUE(" elapsed:", elapsed);
    DEBUG5_VALUELN(" fract:", fraction);
  }

  return changed;
}
