/*
 * This header contains transport-agnostic message types
 */

#ifndef HMTLMESSAGES_H
#define HMTLMESSAGES_H

/*
 * Generic message to set a given pin to the indicated value
 */
typedef struct {
  byte output;
  byte value;
} msg_output_value_t;

#endif
