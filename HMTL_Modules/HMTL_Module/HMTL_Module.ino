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
#include "PixelUtil.h"
#include "RS485Utils.h"
#include "MPR121.h"

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
  
  /* Check for messages on the serial interface */
  msg_hdr_t *msg_hdr = (msg_hdr_t *)msg;
  if (hmtl_serial_getmsg(msg, MSG_MAX_SZ, &offset)) {
    /* Received a complete message */
    DEBUG_VALUE(DEBUG_HIGH, "Received msg len=", offset);
    DEBUG_PRINT(DEBUG_HIGH, " ");
    DEBUG_COMMAND(DEBUG_HIGH, 
		  print_hex_string(msg, offset)
		  );
    DEBUG_PRINTLN(DEBUG_HIGH, "");
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
      if (msg->hdr.type == HMTL_OUTPUT_PROGRAM) {
	// XXX - Setup programs
	dispatch_program(outputs, ()msg, states, true);
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
		  print_hex_string((byte *)&msg, msglen)
		  );
    DEBUG_PRINTLN(DEBUG_HIGH, "");
    DEBUG_PRINTLN(DEBUG_HIGH, "ok");

    if ((msg_hdr->address == config.address) ||
	(msg_hdr->address == RS485_ADDR_ANY)) {
      hmtl_handle_msg(msg_hdr, &config, outputs, objects);
      update = true;
    }
  }

  // XXX: Run programs

  if (update) {
    for (int i = 0; i < config.num_outputs; i++) {
      hmtl_update_output(outputs[i], objects[i]);
    }
  }
}

/*
 * Code for program execution
 */

#define PROGRAM_BLINK_VALUE 0x1
typedef struct {
  uint16_t on_period;
  uint8_t on_value[3];
  uint16_t off_period;
  uint8_t off_value[3];
} program_blink_value_t;
typedef struct {
  boolean on;
  unsigned long next_change;
} state_blink_value_t;

typedef boolean (*hmtl_program_func)(output_hdr_t *output, 
				  msg_program_t *program_ptr, 
				  void *state_ptr,
				  boolean init);
typedef struct {
  byte type;
  hmtl_program_func program;
} hmtl_program_t;

hmtl_program_t program_functions[] = {
  { PROGRAM_BLINK_VALUE, program_blink }
};
#define NUM_PROGRAMS (sizeof (program_functions))

boolean dispatch_program(output_hdr_t *outputs[], msg_program_t *program_ptr, 
			 void *state_ptrs[], boolean init) {
  if (program_ptr->hdr.output > HMTL_MAX_OUTPUTS) {
    DEBUG_VALUELN(DEBUG_ERROR, "dispatch_program: invalid output: ",
		  program_ptr->hdr.output);
    return false;
  }
  output_hdr_t *output = outputs[program_ptr->hdr.output];
  if (output == NULL) {
    DEBUG_VALUELN(DEBUG_ERROR, "dispatch_program: null output: ",
		  program_ptr->hdr.output);
    return false;
  }
  void *state_ptr = state_ptrs[program_ptr->hdr.output];

  for (byte i = 0; i < NUM_PROGRAMS; i++) {
    if (program_functions[i].type == program_ptr->type) {
      return program_functions[i].program(output, program_ptr, 
					  state_ptr, init);
    }
  }

  DEBUG_VALUELN(DEBUG_ERROR, "dispatch_program: unknown program type: ",
		program_ptr->type);

  return false;
}


/*
 * Program function to turn an output on and off
 */
boolean program_blink(output_hdr_t *output, msg_program_t *program_ptr, 
		      void *state_ptr, boolean init) {
  unsigned long now = millis();
  program_blink_value_t *program = (program_blink_value_t *)program_ptr->values;
  state_blink_value_t *state = (state_blink_value_t *)state_ptr;

  if (init) {
    state->on = false;
    state->next_change = 0;
  }

  if (now >= state->next_change) {
    if (state->on) {
      // Turn off the output
      switch (output->type) {
      case HMTL_OUTPUT_VALUE:{
	config_value_t *val = (config_value_t *)output;
	val->value = program->off_value[0];
	break;
      }
      }
      state->next_change += program->off_period;
    } else {
      // Turn on the output
      switch (output->type) {
      case HMTL_OUTPUT_VALUE:{
	config_value_t *val = (config_value_t *)output;
	val->value = program->on_value[0];
	break;
      }
      }
      state->next_change += program->on_period;
    }
    return true;
  }

  return false;
}
