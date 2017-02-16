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
#include <HMTLTypes.h>
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
#include "HMTLPrograms.h"
#include "HMTLProtocol.h"
#include "ProgramManager.h"
#include "MessageHandler.h"

#include "PixelUtil.h"

#include "Socket.h"
#include "RS485Utils.h"
#include "XBeeSocket.h"
#include "MPR121.h"

#include "TimeSync.h"
#include "HMTL_Module.h"

/* Auto update build number */
#define HMTL_MODULE_BUILD 29 // %META INCR

/*
 * Communications
 */

#define SEND_BUFFER_SIZE 64 // The data size for transmission buffers

RS485Socket rs485;
byte rs485_data_buffer[RS485_BUFFER_TOTAL(SEND_BUFFER_SIZE)];

XBeeSocket xbee;
byte xbee_data_buffer[RS485_BUFFER_TOTAL(SEND_BUFFER_SIZE)];

#ifdef USE_RFM69
#include "RFM69.h"
#include "RFM69Socket.h"

// TODO: This should be in the configuration
#define NETWORK 100

#ifndef IRQ_PIN
#define IRQ_PIN 2
#endif

#ifndef IS_RFM69HW
#define IS_RFM69HW true
#endif
// TODO: End of stuff for configuration

RFM69Socket rfm69;
#define RFM69_SEND_BUFFER_SIZE RFM69_DATA_LENGTH(RF69_MAX_DATA_LEN)
byte databuffer[RF69_MAX_DATA_LEN];
#endif


#define MAX_SOCKETS 2
Socket *sockets[MAX_SOCKETS] = { NULL, NULL };

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

/*
 * Program management
 */

/* List of available programs */
hmtl_program_t program_functions[] = {
  { HMTL_PROGRAM_NONE, NULL, NULL},
  { HMTL_PROGRAM_BLINK, program_blink, program_blink_init },
  { HMTL_PROGRAM_TIMED_CHANGE, program_timed_change, program_timed_change_init },
  { HMTL_PROGRAM_FADE, program_fade, program_fade_init },
  { HMTL_PROGRAM_SPARKLE, program_sparkle, program_sparkle_init },
  { PROGRAM_BRIGHTNESS, NULL,  program_brightness },

  { HMTL_PROGRAM_LEVEL_VALUE, program_level_value, program_level_value_init },
  { HMTL_PROGRAM_SOUND_VALUE, program_sound_value, program_sound_value_init },
  { PROGRAM_SENSOR_DATA, process_sensor_data, program_sensor_data_init }
};
#define NUM_PROGRAMS (sizeof (program_functions) / sizeof (hmtl_program_t))

program_tracker_t *active_programs[HMTL_MAX_OUTPUTS];
ProgramManager manager;
MessageHandler handler;

#ifdef ENABLE_PUSH_BUTTON
  #define PUSH_BUTTON_PIN 8
#endif

// Prototypes
void additional_setup();
void additional_loop();

void setup() {

  //  Serial.begin(115200);
  Serial.begin(57600);

  int32_t outputs_found = hmtl_setup(&config, readoutputs,
                                     outputs, objects, HMTL_MAX_OUTPUTS,
                                     &rs485, 
                                     &xbee,
                                     &pixels,
                                     NULL, // MPR121
                                     NULL, // RGB
                                     NULL, // Value
                                     NULL);

  byte num_sockets = 0;

  if (outputs_found & (1 << HMTL_OUTPUT_RS485)) {
    /* Setup the RS485 connection */  
    rs485.setup();
    rs485.initBuffer(rs485_data_buffer, SEND_BUFFER_SIZE);
    sockets[num_sockets++] = &rs485;
  }

  if (outputs_found & (1 << HMTL_OUTPUT_XBEE)) {
    /* Setup the RS485 connection */  
    xbee.setup();
    xbee.initBuffer(xbee_data_buffer, SEND_BUFFER_SIZE);
    sockets[num_sockets++] = &xbee;
  }

#ifdef USE_RFM69
  if (true) {
    // XXX: RFM69!
    rfm69.init(config.address, NETWORK, IRQ_PIN, IS_RFM69HW, RF69_915MHZ); // TODO: Put this in config
    rfm69.setup();
    rfm69.initBuffer(databuffer, RFM69_SEND_BUFFER_SIZE);
    sockets[num_sockets++] = &rfm69;
  }
#endif

  if (num_sockets == 0) {
    DEBUG_ERR("No sockets configured");
    DEBUG_ERR_STATE(2);
  }

  /* Setup the program manager */
  manager = ProgramManager(outputs, active_programs, objects, HMTL_MAX_OUTPUTS,
                           program_functions, NUM_PROGRAMS);

  handler = MessageHandler(config.address, &manager, sockets, num_sockets);

  additional_setup();

  DEBUG2_VALUELN("HMTL Module initialized, v", HMTL_MODULE_BUILD);

  // Indicate that module is ready for action
  Serial.println(F(HMTL_READY));
}

void additional_setup() {
#ifdef ENABLE_PUSH_BUTTON
  pinMode(PUSH_BUTTON_PIN, INPUT_PULLUP);
#endif

#ifdef STARTUP_COMMANDS
  startup_commands();
#endif
}

#ifdef ENABLE_PUSH_BUTTON
byte get_button_value() {
  return digitalRead(PUSH_BUTTON_PIN);
}
#endif

#ifdef STARTUP_COMMANDS
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

    handler.process_msg(startupmsg[i], NULL, serial_socket, &config);
  }

  /* Free the startup messages */
  for (byte i = 0; i < STARTUP_MSGS; i++) {
    free(startupmsg[i]);
  }
}
#endif // STARTUP_COMMANDS


#define MSG_MAX_SZ (sizeof(msg_hdr_t) + sizeof(msg_max_t))
byte msg[MSG_MAX_SZ];
byte serial_offset = 0;


/*******************************************************************************
 * The main event loop
 *
 * - Checks for and handles messages over all interfaces
 * - Runs any enabled programs
 * - Updates any outputs
 */
void loop() {

  // Check and send a serial-ready message if needed
  handler.serial_ready();

  /*
   * Check the serial device and all sockets for messages, forwarding them and
   * processing them if they are for this module.
   */
  boolean update = handler.check(&config);

  additional_loop();

  /* Execute any active programs */
  if (manager.run()) {
    update = true;
  }

  if (update) {
    /* Update the outputs */
    for (byte i = 0; i < config.num_outputs; i++) {
      hmtl_update_output(outputs[i], objects[i]);
    }
  }
}

void additional_loop() {
#ifdef ENABLE_PUSH_BUTTON
  //Use a push button to override a particular output
  static byte prev_value = HIGH;
  if (get_button_value() != prev_value) {
    prev_value = get_button_value();
    DEBUG1_VALUELN("Push button to ", prev_value);
    if (prev_value == LOW) {
      uint32_t value = (prev_value == LOW ? pixel_color(255,255,255) : 0);
      hmtl_program_timed_change_fmt(rs485.send_buffer,  rs485.send_data_size,
                             config.address, (byte)3,
                             500, value,
                             0);
      handler.process_msg((msg_hdr_t *)rs485.send_buffer, &rs485, NULL, &config);

    }
  }
#endif
}

/*******************************************************************************
 * Program handler for processing incoming sensor data
 */

boolean program_sensor_data_init(msg_program_t *msg,
                                 program_tracker_t *tracker,
                                 output_hdr_t *output) {
  DEBUG3_PRINTLN("Initializing sensor data handler");
  return true;
}

boolean process_sensor_data(output_hdr_t *output,
                            void *object,
                            program_tracker_t *tracker) {
  msg_sensor_data_t *sensor = (msg_sensor_data_t *)object;
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
      sound_channels = sensor->data_len / (byte)sizeof (uint16_t);
      DEBUG4_COMMAND(
                     DEBUG4_VALUE(" SOUND:", sound_channels);
                     for (byte i = 0; i < sound_channels; i++) {
                       DEBUG4_HEXVAL(" ", sound_data[i]);
                     }
                     );
      return true;
      break;
    }
    case HMTL_SENSOR_LIGHT: {
      light_data = *(uint16_t *)data;
      DEBUG4_VALUE(" LIGHT:", light_data);
      return true;
      break;
    }
    case HMTL_SENSOR_POT: {
      level_data = *(uint16_t *)data;
      DEBUG4_VALUE(" LEVEL:", level_data);
      return true;
      break;
    }
    default: {
      DEBUG1_PRINT(" ERROR: UNKNOWN TYPE");
      return false;
      break;
    }
  }
}




/*******************************************************************************
 * Program to set the value level based on the most recent sensor data
 */
typedef struct {
  uint16_t value;
} state_level_value_t;

boolean program_level_value_init(msg_program_t *msg,
                                 program_tracker_t *tracker,
                                 output_hdr_t *output) {
  if ((output == NULL) || !IS_HMTL_RGB_OUTPUT(output->type)) {
    return false;
  }

  DEBUG3_PRINTLN("Initializing level value state");

  state_level_value_t *state = (state_level_value_t *)malloc(sizeof (state_level_value_t));
  tracker->flags |= PROGRAM_DEALLOC_STATE;

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

typedef struct {
  uint16_t value;
  uint32_t max;
} state_sound_value_t;

boolean program_sound_value_init(msg_program_t *msg,
                                 program_tracker_t *tracker,
                                 output_hdr_t *output) {
  if ((output == NULL) || !IS_HMTL_RGB_OUTPUT(output->type)) {
    return false;
  }

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

