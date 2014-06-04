/*
 * Utility functions for working with the transport-agnostic messages formats
 */

#include <Arduino.h>

#define DEBUG_LEVEL DEBUG_HIGH
#include "Debug.h"

#include "GeneralUtils.h"
#include "EEPromUtils.h"
#include "HMTLTypes.h"
#include "PixelUtil.h"
#include "MPR121.h"
#include "RS485Utils.h"

int hmtl_output_size(output_hdr_t *output) 
{
  switch (output->type) {
      case HMTL_OUTPUT_VALUE:
        return sizeof (config_value_t);
      case HMTL_OUTPUT_RGB:
        return sizeof (config_rgb_t);
      case HMTL_OUTPUT_PROGRAM:
        return sizeof (config_program_t);
      case HMTL_OUTPUT_PIXELS:
        return sizeof (config_pixels_t);
      case HMTL_OUTPUT_MPR121:
        return sizeof (config_mpr121_t);
      case HMTL_OUTPUT_RS485:
        return sizeof (config_rs485_t);
      default:
        DEBUG_ERR("hmtl_output_size: bad output type");
        return -1;    
  }
}

/*
 * Read in the HMTL config, returning the EEProm address following
 * what was read.
 */
int hmtl_read_config(config_hdr_t *hdr, config_max_t outputs[],
                     int max_outputs) 
{
  int addr;

  /* Zero the outputs */
  for (int i = 0; i < max_outputs; i++) {
    outputs[i].hdr.type = HMTL_OUTPUT_NONE;
  }

  addr = EEPROM_safe_read(HMTL_CONFIG_ADDR,
                          (uint8_t *)hdr, sizeof (config_hdr_t));
  if (addr < 0) {
    DEBUG_ERR("hmtl_read_config: error reading config from eeprom");
    return -1;
  }

  if (hdr->magic != HMTL_CONFIG_MAGIC) {
    DEBUG_ERR("hmtl_read_config: read config with invalid magic");
    return -2;
  }

  if (hdr->protocol_version != HMTL_CONFIG_VERSION) {
    DEBUG_ERR("hmtl_read_config: hdr has wrong protocol version");
    return -3;
  }

  if ((hdr->num_outputs > 0) && (max_outputs != 0)) {
    /* Read in the outputs if any were indicated and a buffer was provided */
    if (max_outputs < hdr->num_outputs) {
      DEBUG_ERR("hmtl_read_config: not enough outputs");
      return -4;
    }
    for (int i = 0; i < hdr->num_outputs; i++) {
      addr = EEPROM_safe_read(addr,
                              (uint8_t *)&outputs[i], sizeof (config_max_t));
      if (addr <= 0) {
        DEBUG_ERR("hmtl_read_config: error reading outputs");
	return -5;
      }
    }
  }

  DEBUG_VALUE(DEBUG_LOW, "hmtl_read_config: size=", addr - HMTL_CONFIG_ADDR);
  DEBUG_VALUE(DEBUG_LOW, " end=", addr);
  DEBUG_VALUELN(DEBUG_LOW, " module address=", hdr->address);

  return addr;
}

/*
 * Write out the HMTL config, returning the EEProm address following
 * what was written.
 */
int hmtl_write_config(config_hdr_t *hdr, output_hdr_t *outputs[])
{
  int addr;
  hdr->magic = HMTL_CONFIG_MAGIC;
  hdr->protocol_version = HMTL_CONFIG_VERSION;
  addr = EEPROM_safe_write(HMTL_CONFIG_ADDR,
                           (uint8_t *)hdr, sizeof (config_hdr_t));
  if (addr < 0) {
    DEBUG_ERR("hmtl_write_config: failed to write config to EEProm");
    return -1;
  }

  if (outputs != NULL) {
    for (int i = 0; i < hdr->num_outputs; i++) {
      output_hdr_t *output = outputs[i];
      addr = EEPROM_safe_write(addr, (uint8_t *)output,
			       hmtl_output_size(output));
      if (addr < 0) {
	DEBUG_ERR("hmtl_write_config: failed to write outputs to EEProm");
	return -2;
      }
    }
  }

  DEBUG_VALUE(DEBUG_LOW, "hmtl_write_config: size=", addr - HMTL_CONFIG_ADDR);
  DEBUG_VALUELN(DEBUG_LOW, " end=", addr);

  return addr;
}

/* Initialized the pins of an output */
int hmtl_setup_output(output_hdr_t *hdr, void *data)
{
  DEBUG_VALUE(DEBUG_HIGH, "hmtl_setup_output: type=", hdr->type);
  switch (hdr->type) {
      case HMTL_OUTPUT_VALUE: 
      {
        config_value_t *out = (config_value_t *)hdr;
        DEBUG_PRINT(DEBUG_HIGH, " value");
        pinMode(out->pin, OUTPUT);
        break;
      }
      case HMTL_OUTPUT_RGB:
      {
        config_rgb_t *out = (config_rgb_t *)hdr;
        DEBUG_PRINT(DEBUG_HIGH, " rgb");
        for (int j = 0; j < 3; j++) {
          pinMode(out->pins[j], OUTPUT);
        }
        break;
      }
      case HMTL_OUTPUT_PROGRAM:
      {
//        config_program_t *out = (config_program_t *)hdr;
        DEBUG_PRINT(DEBUG_HIGH, " program");
        break;
      }
      case HMTL_OUTPUT_PIXELS:
      {
        DEBUG_PRINT(DEBUG_HIGH, " pixels");
        if (data != NULL) {
          config_pixels_t *out = (config_pixels_t *)hdr;
          PixelUtil *pixels = (PixelUtil *)data;
          pixels->init(out->numPixels,
		       out->dataPin,
		       out->clockPin,
		       out->type);
        } else {
          DEBUG_ERR("Expected PixelUtil data struct for pixel configs");
          return -1;
        }
        break;
      }
      case HMTL_OUTPUT_MPR121:
      {
        DEBUG_PRINTLN(DEBUG_HIGH, " mpr121");
        if (data != NULL) {
          config_mpr121_t *out = (config_mpr121_t *)hdr;
	  MPR121 *capSensor = (MPR121 *)data;
	  capSensor->init(out->irqPin,
			  out->useInterrupt,
			  START_ADDRESS,  // XXX - Only single address
			  false);         // XXX - No touch times
	  for (int i = 0; i < MAX_MPR121_PINS; i++) {
	    byte touch = out->thresholds[i] & 0x0F;
	    byte release = (out->thresholds[i] & 0xF0) >> 4;
	    if (touch || release) {
	      capSensor->setThreshold(i, touch, release);
	    }
	  }
        } else {
          DEBUG_ERR("Expected MPR121 data struct for mpr121 configs");
          return -1;
        }
        break;
      }
      case HMTL_OUTPUT_RS485:
      {
        DEBUG_PRINT(DEBUG_HIGH, " rs485");
        if (data != NULL) {
          config_rs485_t *out = (config_rs485_t *)hdr;
	  RS485Socket *rs485 = (RS485Socket *)data;
	  rs485->init(out->recvPin, out->xmitPin, out->enablePin,
		     false); // Set to true to enable debugging
        } else {
          DEBUG_ERR("Expected RS485Socket data struct for RS485 configs");
          return -1;
        }
        break;
      }
      default:
      {
        DEBUG_VALUELN(DEBUG_ERROR, "Invalid type", hdr->type);
        return -1;
      }
  }

  DEBUG_PRINTLN(DEBUG_HIGH, "");

  return 0;
}

/* Perform an update of an output */
int hmtl_update_output(output_hdr_t *hdr, void *data) 
{
  switch (hdr->type) {
      case HMTL_OUTPUT_VALUE: 
      {
        config_value_t *out = (config_value_t *)hdr;

	// On a non-PWM pin this outputs HIGH if value >= 128
	analogWrite(out->pin, out->value);
	DEBUG_VALUE(DEBUG_TRACE, "hmtl_update_output: val pin=", out->pin);
	DEBUG_VALUELN(DEBUG_TRACE, " val=", out->value);
        break;
      }
      case HMTL_OUTPUT_RGB:
      {
        config_rgb_t *out = (config_rgb_t *)hdr;
        for (int j = 0; j < 3; j++) {
          analogWrite(out->pins[j], out->values[j]);
        }
        break;
      }
      case HMTL_OUTPUT_PROGRAM:
      {
//          config_program_t *out = (config_program_t *)hdr;
        break;
      }
      case HMTL_OUTPUT_PIXELS:
      {
	if (data) {
	  PixelUtil *pixels = (PixelUtil *)data;
	  pixels->update();
	}
        break;
      }
      case HMTL_OUTPUT_MPR121:
      {
	// XXX - Should this be reading the inputs?
	break;
      }
      case HMTL_OUTPUT_RS485:
      {
	// XXX - Should this be checking for data?
	break;
      }
      default: 
      {
        DEBUG_ERR("hmtl_update_output: unknown type");
        return -1;
      }
  }

  return 0;
}

/* Update the output with test data */
#define TEST_MAX_VAL 128
#define TEST_PWM_STEP  1
int hmtl_test_output(output_hdr_t *hdr, void *data) 
{
  switch (hdr->type) {
      case HMTL_OUTPUT_VALUE: 
      {
        config_value_t *out = (config_value_t *)hdr;
        out->value = 255;
        //out->value = (out->value + TEST_PWM_STEP) % TEST_MAX_VAL;
        break;
      }
      case HMTL_OUTPUT_RGB:
      {
        config_rgb_t *out = (config_rgb_t *)hdr;
        for (int j = 0; j < 3; j++) {
          out->values[j] = (out->values[j] + TEST_PWM_STEP + j) % TEST_MAX_VAL;
        }
        break;
      }
      case HMTL_OUTPUT_PROGRAM:
      {
//          config_program_t *out = (config_program_t *)hdr;
        break;
      }
      case HMTL_OUTPUT_PIXELS:
      {
//          config_pixels_t *out = (config_pixels_t *)hdr;
        PixelUtil *pixels = (PixelUtil *)data;
        static int currentPixel = 0;
        pixels->setPixelRGB(currentPixel, 0, 0, 0);
        currentPixel = (currentPixel + 1) % pixels->numPixels();
        pixels->setPixelRGB(currentPixel, 255, 0, 0);
        break;
      }
      case HMTL_OUTPUT_MPR121: break; // Nothing to do here
      case HMTL_OUTPUT_RS485:  break; // Nothing to do here
      default: 
      {
        DEBUG_ERR("hmtl_test_output: unknown type");
        return -1;
      }

  }

  return 0;
}

int hmtl_test_output_car(output_hdr_t *hdr, void *data) 
{
  switch (hdr->type) {
      case HMTL_OUTPUT_VALUE: 
      {
        config_value_t *out = (config_value_t *)hdr;
        out->value = (out->value + TEST_PWM_STEP) % TEST_MAX_VAL;
        break;
      }
      case HMTL_OUTPUT_RGB:
      {
        config_rgb_t *out = (config_rgb_t *)hdr;
        out->values[0] = TEST_MAX_VAL;
        out->values[1] = 0;
        out->values[2] = 0;
        break;
      }
      case HMTL_OUTPUT_PROGRAM:
      {
//          config_program_t *out = (config_program_t *)hdr;
        break;
      }
      case HMTL_OUTPUT_PIXELS:
      {
//          config_pixels_t *out = (config_pixels_t *)hdr;
#if 0
        PixelUtil *pixels = (PixelUtil *)data;
        static int prevPixel = pixels->numPixels() - 1;
        static int currPixel = 0;
        static int nextPixel = 1;
        pixels->setPixelRGB(prevPixel, 0, 0, 0);
        pixels->setPixelRGB(currPixel, 128, 0, 0);
        pixels->setPixelRGB(nextPixel, 0, 255, 0);

        prevPixel = (prevPixel + 1) % pixels->numPixels();
        currPixel = (currPixel + 1) % pixels->numPixels();
        nextPixel = (nextPixel + 1) % pixels->numPixels();

        pixels->setPixelRGB(nextPixel, 0, 0, 125);
#endif

#if 0
#define TEST_MAX_RAINBOW 128
	static int rainbow = 0;
        for (byte i = 0; i < pixels->numPixels(); i++) {
	  pixels->setPixelRGB(i, 
			      pixel_wheel(((i * 256 / pixels->numPixels()) + rainbow) % 256, TEST_MAX_RAINBOW) );
	}
	rainbow = (rainbow + 1) % (256 * 5);
#endif

#if 0
#define TEST_PERIOD_MS    100

	
#define TEST_PATTERN_SIZE 9
        static byte pattern[TEST_PATTERN_SIZE][3] = {
          {64,  0,   128},
	  {128, 0,   64 },
	  {255, 0,   0  },
	  {128, 64,  0  },
	  {64,  128, 0  },
	  {0,   255, 0  },
	  {0,   128, 64 },
          {0,   64,  128},
          {0,   0,   255},
        };
	
	/*
#define TEST_PATTERN_SIZE 5
        static byte pattern[TEST_PATTERN_SIZE][3] = {
	{64, 64, 64},
	  {128, 128, 128},
	    {256, 256, 256},
	      {128, 128, 128},
		{64, 64, 64},
		  };
	*/
	/*
#define TEST_PATTERN_SIZE 9
        static byte pattern[TEST_PATTERN_SIZE][3] = {
          {16, 00, 00},
          {32, 00, 00},
	  {64, 00, 00},
	  {128, 00, 00},
	  {256, 00, 00},
          {128, 00, 00},
          {64, 00, 00},
	  {32, 00, 00},
	  {16, 00, 00},
	};
	*/
	static long next_time = millis() + TEST_PERIOD_MS;
        static byte current = 0;

	long now = millis();
	if (now > next_time) {
          pixels->setPixelRGB(current % pixels->numPixels(), 0, 0, 0);
	  current++;
	  next_time += TEST_PERIOD_MS;
        }

        for (byte i = 0; i <  TEST_PATTERN_SIZE; i++) {
	  pixels->setPixelRGB((current + i) % pixels->numPixels(),
			      pattern[i][0], pattern[i][1], pattern[i][2]);
	}
#endif
        break;
      }
      case HMTL_OUTPUT_MPR121: break; // Nothing to do here
      case HMTL_OUTPUT_RS485:  break; // Nothing to do here
      default: 
      {
        DEBUG_ERR("hmtl_test_output: unknown type");
        return -1;
      }

  }

  return 0;
}

/* Fill in a config with default values */
void hmtl_default_config(config_hdr_t *hdr)
{
  hdr->magic = HMTL_CONFIG_MAGIC;
  hdr->protocol_version = HMTL_CONFIG_VERSION;
  hdr->hardware_version = 0;
  hdr->address = 0;
  hdr->num_outputs = 0;
  hdr->flags = 0;
  DEBUG_VALUELN(DEBUG_LOW, "hmtl_default_config: address=", hdr->address);
}

/******************************************************************************
 * Configuration validation
 */

boolean hmtl_validate_header(config_hdr_t *hdr) {
  if (hdr->magic != HMTL_CONFIG_MAGIC) return false;
  switch (hdr->protocol_version) {
  case 1: {
    config_hdr_v1_t *hdr_v1 = (config_hdr_v1_t *)hdr;
    if (hdr_v1->num_outputs > HMTL_MAX_OUTPUTS) return false;
    break;
  }
  case 2: {
    config_hdr_v2_t *hdr_v2 = (config_hdr_v2_t *)hdr;
    if (hdr_v2->num_outputs > HMTL_MAX_OUTPUTS) return false;
    break;
  }
  default: return false;
  }

  return true;
}

boolean hmtl_validate_value(config_value_t *val) {
  if (val->pin > 13) return false;
  return true;
}

boolean hmtl_validate_rgb(config_rgb_t *rgb) {
  uint32_t pinmap = 0;
  uint32_t pinbit;

  for (int pin = 0; pin < 3; pin++) {
    if (rgb->pins[pin] > 13) return false;
    pinbit = (1 << rgb->pins[pin]);
    if (pinmap & pinbit) return false;
    pinmap |= pinbit;
  }
  return true;
}

boolean hmtl_validate_pixels(config_pixels_t *pixels) {
  if (pixels->clockPin > 13) return false;
  if (pixels->dataPin > 13) return false;
  if (pixels->clockPin == pixels->dataPin) return false;
  if ((pixels->type != 0) && (pixels->type != WS2801_RGB)) return false;
  return true;
}

boolean hmtl_validate_mpr121(config_mpr121_t *mpr121) {
  if (mpr121->irqPin > 13) return false;
  return true;
}

boolean hmtl_validate_rs485(config_rs485_t *rs485) {
  if (rs485->recvPin > 13) return false;
  if (rs485->xmitPin > 13) return false;
  if (rs485->enablePin > 13) return false;
  if (rs485->recvPin == rs485->xmitPin) return false;
  if (rs485->recvPin == rs485->enablePin) return false;
  if (rs485->enablePin == rs485->xmitPin) return false;
  return true;
}

boolean hmtl_validate_config(config_hdr_t *hdr, output_hdr_t *outputs[],
			     int num_outputs) {
  uint32_t pinmap = 0;
  uint32_t pinbit;

  if (!hmtl_validate_header(hdr)) goto VALIDATE_ERROR;
  if (hdr->num_outputs != num_outputs) {
    DEBUG_ERR("Number of outputs does not match");
    return false;
  }

  /* Verify that all pins are unique */
  for (int i = 0; i < num_outputs; i++) {
    output_hdr_t *out = outputs[i];
    switch (out->type) {
    case HMTL_OUTPUT_VALUE: {
      config_value_t *out2 = (config_value_t *)out;
    if (!hmtl_validate_value(out2)) goto VALIDATE_ERROR;
      pinbit = (1 << out2->pin);
      if (pinmap & pinbit) goto PIN_ERROR;
      pinmap |= pinbit;
      break;
    }
    case HMTL_OUTPUT_RGB: {
      config_rgb_t *out2 = (config_rgb_t *)out;
      if (!hmtl_validate_rgb(out2)) goto VALIDATE_ERROR;
      for (int pin = 0; pin < 3; pin++) {
	pinbit = (1 << out2->pins[pin]);
	if (pinmap & pinbit) goto PIN_ERROR;
	pinmap |= pinbit;
      }
      break;
    }
    case HMTL_OUTPUT_PIXELS: {
      config_pixels_t *out2 = (config_pixels_t *)out;
      if (!hmtl_validate_pixels(out2)) goto VALIDATE_ERROR;
      pinbit = (1 << out2->clockPin);
      if (pinmap & pinbit) goto PIN_ERROR;
      pinmap |= pinbit;
      pinbit = (1 << out2->dataPin);
      if (pinmap & pinbit) goto PIN_ERROR;
      pinmap |= pinbit;
      break;
    }
    case HMTL_OUTPUT_MPR121: {
      config_mpr121_t *out2 = (config_mpr121_t *)out;
      if (!hmtl_validate_mpr121(out2)) goto VALIDATE_ERROR;
      pinbit = (1 << out2->irqPin);
      if (pinmap & pinbit) goto PIN_ERROR;
      pinmap |= pinbit;
      break;
    }
    case HMTL_OUTPUT_RS485: {
      config_rs485_t *out2 = (config_rs485_t *)out;
      if (!hmtl_validate_rs485(out2)) goto VALIDATE_ERROR;
      pinbit = (1 << out2->recvPin);
      if (pinmap & pinbit) goto PIN_ERROR;
      pinmap |= pinbit;

      pinbit = (1 << out2->xmitPin);
      if (pinmap & pinbit) goto PIN_ERROR;
      pinmap |= pinbit;

      pinbit = (1 << out2->enablePin);
      if (pinmap & pinbit) goto PIN_ERROR;
      pinmap |= pinbit;
      break;
    }
    default: {
      DEBUG_ERR("Invalid output type");
      return false;
    }
    }
  }

  return true;

 PIN_ERROR:
  DEBUG_ERR("Pin used multiple times");
  return false;
 VALIDATE_ERROR:
  DEBUG_ERR("A config was invalid");
  return false;

}


/******************************************************************************
 * Debug printing of configuration
 */

void hmtl_print_header(config_hdr_t *hdr) {
#ifdef DEBUG_LEVEL
  DEBUG_VALUE(DEBUG_LOW, "  header: mag: ", hdr->magic);
  DEBUG_VALUE(DEBUG_LOW, " protocol_version: ", hdr->protocol_version);
  DEBUG_VALUE(DEBUG_LOW, " hardware_version: ", hdr->hardware_version);
  DEBUG_VALUE(DEBUG_LOW, " address: ", hdr->address);
  DEBUG_VALUE(DEBUG_LOW, " outputs: ", hdr->num_outputs);
  DEBUG_VALUELN(DEBUG_LOW, " flags: ", hdr->flags);
#endif
}

void hmtl_print_output(output_hdr_t *out) {
#ifdef DEBUG_LEVEL
  DEBUG_VALUE(DEBUG_LOW, "  output ", out->output);
  DEBUG_VALUE(DEBUG_LOW, " offset=", (int)out);
  DEBUG_VALUE(DEBUG_LOW, " type=", out->type);
  DEBUG_PRINT(DEBUG_LOW, " - ");
  switch (out->type) {
  case HMTL_OUTPUT_VALUE:
    {
      config_value_t *out2 = (config_value_t *)out;
      DEBUG_VALUE(DEBUG_LOW, "value pin=", out2->pin);
      DEBUG_VALUELN(DEBUG_LOW, " val=", out2->value);
      break;
    }
  case HMTL_OUTPUT_RGB:
    {
      config_rgb_t *out2 = (config_rgb_t *)out;
      DEBUG_VALUE(DEBUG_LOW, "rgb pin0=", out2->pins[0]);
      DEBUG_VALUE(DEBUG_LOW, " pin1=", out2->pins[1]);
      DEBUG_VALUE(DEBUG_LOW, " pin2=", out2->pins[2]);
      DEBUG_VALUE(DEBUG_LOW, " val0=", out2->values[0]);
      DEBUG_VALUE(DEBUG_LOW, " val1=", out2->values[1]);
      DEBUG_VALUELN(DEBUG_LOW, " val2=", out2->values[2]);
      break;
    }
  case HMTL_OUTPUT_PROGRAM:
    {
      config_program_t *out2 = (config_program_t *)out;
      DEBUG_PRINTLN(DEBUG_LOW, "program");
      for (int i = 0; i < MAX_PROGRAM_VAL; i++) {
	DEBUG_VALUELN(DEBUG_LOW, " val=", out2->values[i]);
      }
      break;
    }
  case HMTL_OUTPUT_PIXELS:
    {
      config_pixels_t *out2 = (config_pixels_t *)out;
      DEBUG_VALUE(DEBUG_LOW, "pixels clock=", out2->clockPin);
      DEBUG_VALUE(DEBUG_LOW, " data=", out2->dataPin);
      DEBUG_VALUE(DEBUG_LOW, " num=", out2->numPixels);
      DEBUG_VALUELN(DEBUG_LOW, " type=", out2->type);
      break;
    }
  case HMTL_OUTPUT_MPR121:
    {
      config_mpr121_t *out2 = (config_mpr121_t *)out;
      DEBUG_VALUE(DEBUG_LOW, "mpr121 irq=", out2->irqPin);
      DEBUG_VALUE(DEBUG_LOW, " useInt=", out2->useInterrupt);
      for (int i = 0; i < MAX_MPR121_PINS; i++) {
	byte touch = out2->thresholds[i] & 0x0F;
	byte release = (out2->thresholds[i] & 0xF0) >> 4;
	if (touch || release) {
	  DEBUG_VALUE(DEBUG_LOW, " thresh=", i);
	  DEBUG_VALUE(DEBUG_LOW, ",", touch);
	  DEBUG_VALUE(DEBUG_LOW, ",", release);
	}
      }
      DEBUG_PRINT_END();
      break;
    }
  case HMTL_OUTPUT_RS485:
    {
      config_rs485_t *out2 = (config_rs485_t *)out;
      DEBUG_VALUE(DEBUG_LOW, "rs485 recv=", out2->recvPin);
      DEBUG_VALUE(DEBUG_LOW, " xmit=", out2->xmitPin);
      DEBUG_VALUE(DEBUG_LOW, " enable=", out2->enablePin);
      DEBUG_PRINT_END();
      break;
    }
  default:
    {
      DEBUG_PRINTLN(DEBUG_LOW, "Unknown type");
      break;
    }
  }
#endif
}

/* Print out details of a config */
void hmtl_print_config(config_hdr_t *hdr, output_hdr_t *outputs[])
{
#ifdef DEBUG_LEVEL
  DEBUG_PRINTLN(DEBUG_LOW, "hmtl_print_config:");
  hmtl_print_header(hdr);

  if (outputs == NULL) 
    return;

  for (int i = 0; i < hdr->num_outputs; i++) {
    output_hdr_t *out = (output_hdr_t *)outputs[i];
    if (out == NULL) {
      DEBUG_VALUE(DEBUG_LOW, "Output ", i);
      DEBUG_PRINTLN(DEBUG_LOW, " is NULL");
      continue;
    }
    hmtl_print_output(out);
  }
#endif
}



/*
 * Read in the EEProm config and initialize ouputs
 */
int32_t hmtl_setup(config_hdr_t *config, 
		   config_max_t readoutputs[], 

		   output_hdr_t *outputs[], 
		   void *objects[],
		   byte num_outputs,
		   
		   RS485Socket *rs485,
		   PixelUtil *pixels,
		   config_rgb_t *rgb_output,
		   config_value_t *value_output,
		   
		   int *configOffset
	       ) {
  int32_t outputs_found = 0;

  int offset = hmtl_read_config(config,
				readoutputs, 
				num_outputs);
  if (offset < 0) {
    DEBUG_ERR("Failed to read configuration");
    DEBUG_ERR_STATE(12);
  }
  if (configOffset) *configOffset = offset;

  DEBUG_VALUELN(DEBUG_LOW, "Read config.  offset=", offset);

  /* Setup the output pointers array */
  for (int i = 0; i < config->num_outputs; i++) {
    if (i >= num_outputs) {
      DEBUG_VALUELN(DEBUG_ERROR, "Too many outputs:", config->num_outputs);
      DEBUG_ERR_STATE(13);
    }
    if (readoutputs[i].hdr.type == HMTL_OUTPUT_NONE) {
      outputs[i] = NULL;
    } else {
      outputs[i] = (output_hdr_t *)&readoutputs[i];
    }
  }

  DEBUG_COMMAND(DEBUG_LOW, hmtl_print_config(config, outputs));

  /* Initialize the outputs */
  for (int i = 0; i < config->num_outputs; i++) {
    void *data = NULL;
    byte type = ((output_hdr_t *)outputs[i])->type;
    switch (type) {
    case HMTL_OUTPUT_PIXELS: {
      if (pixels == NULL) continue;
      data = pixels;
      break;
    }
    case HMTL_OUTPUT_RS485: {
      if (rs485 == NULL) continue;
      data = rs485;
      break;
    }
    case HMTL_OUTPUT_RGB: {
      if (rgb_output == NULL) continue;
      memcpy(rgb_output, outputs[i], sizeof (config_rgb_t));
      break;
    }
    case HMTL_OUTPUT_VALUE: {
      if (value_output == NULL) continue;
      memcpy(value_output, outputs[i], sizeof (config_value_t));
      break;
    }
    }

    if (objects) objects[i] = data;

    hmtl_setup_output((output_hdr_t *)outputs[i], data);
    outputs_found |= (1 << type);
  }

  return outputs_found;
}
