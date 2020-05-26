/*******************************************************************************
 * Author: Adam Phelps
 * License: Create Commons Attribution-Non-Commercial
 * Copyright: 2020
 *
 * REST API endpoints for Wifi-Enabled modules
 ******************************************************************************/

#include <Arduino.h>

#include "HMTL_Module_API.h"

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL DEBUG_HIGH
#endif
#include "Debug.h"
#include "Socket.h"

#include "HMTLTypes.h"
#include "HMTLMessaging.h"
#include "HMTLPrograms.h"
#include "MessageHandler.h"

#define ESP32
#if defined(ESP32)
#include <WiFiBase.h>

WiFiBase wfb;
#endif

static Socket **sockets;
static MessageHandler *handler;
static config_hdr_t *config;

void
setup_HMTL_API(Socket **s, MessageHandler *h, config_hdr_t *c) {
  sockets = s;
  handler = h;
  config = c;

  DEBUG4_PRINTLN("Initializing up API");

  /* Startup a connection and add a wifi socket */
  wfb = WiFiBase();
  wfb.configureAccessPoint("HMTL_Module", "12345678");
  wfb.startup();

  wfb.addRESTEndpoint("/clear", clearHandler, "\"description\":\"clear all patterns\"");
  wfb.addRESTEndpoint("/rgb", rgbHandler,
                      "\"description\":\"set to a single color\",\"args\":[\"r\",\"g\",\"b\"]");
  wfb.addRESTEndpoint("/circular", circularHandler,
                      "\"description\":\"run circular pattern\",\"args\":[\"pattern\",\"length\",\"period\"]");
  wfb.addRESTEndpoint("/sparkle", sparkleHandler,
                      "\"description\":\"run sparkle pattern\",\"args\":[\"r\",\"g\",\"b\",\"threshold\",\"bg\",\"hum_min\",\"hue_max\",\"sat_min\",\"sat_max\",\"val_min\",\"val_max\"]");

  wfb.addRESTEndpoint("/RGB", rgbColorPicker, "");
}

void api_status() {
#if defined(ESP32)
  DEBUG3_VALUE(" * wifi connected:", wfb.connected());
    DEBUG3_VALUE(" ssid:", WiFi.SSID());
    DEBUG3_VALUE(" ip:", WiFi.localIP().toString());
    DEBUG3_VALUE(" ap_ip:", WiFi.softAPIP().toString());
#endif
}

void api_check() {
  wfb.checkServer();
}

#if defined(ESP32)

void clearHandler() {
  WebServer *server = wfb.getServer();

  DEBUG3_PRINTLN("/clear");

  hmtl_program_cancel_fmt(sockets[0]->send_buffer,
                          sockets[0]->send_data_size,
                          config->address, HMTL_ALL_OUTPUTS);
  auto *msg = (msg_hdr_t *)sockets[0]->send_buffer;
  handler->process_msg(msg, sockets[0], nullptr, config);

  server->send(200, "application/json", "ok");
}

void circularHandler() {
  WebServer *server = wfb.getServer();

  uint8_t pattern = 0;
  if (server->hasArg("pattern")) {
    pattern = server->arg("pattern").toInt();
  }

  byte output = handler->manager->lookup_output_by_type(HMTL_OUTPUT_PIXELS);
  if (output != HMTL_NO_OUTPUT) {
    auto *pixels = (PixelUtil *)handler->manager->objects[output];

    uint16_t length = pixels->numPixels() / 8;
    if (server->hasArg("length")) {
      length = server->arg("length").toInt();
    }

    uint16_t period = 25;
    if (server->hasArg("period")) {
      period = server->arg("period").toInt();
    }

    DEBUG3_VALUE("/circular pattern=", pattern);
    DEBUG3_VALUE(" length=", length);
    DEBUG3_VALUELN(" period=", period);

    DEBUG4_VALUELN("/circular: output ", output);
    program_circular_fmt(sockets[0]->send_buffer, sockets[0]->send_data_size,
                         config->address, output,
                         period, length, CRGB::Black,
                         pattern, 0);
    msg_hdr_t *msg = (msg_hdr_t *)sockets[0]->send_buffer;
    handler->process_msg(msg, sockets[0], NULL, config);
  }

  DEBUG3_PRINTLN("/circular done");

  server->send(200, "application/json", "ok");
}

void sparkleHandler() {
  WebServer *server = wfb.getServer();

  uint16_t period = 50;
  if (server->hasArg("period")) {
    period = server->arg("period").toInt();
  }

  uint8_t r = 0, g = 0, b = 0;
  if (server->hasArg("r")) r = server->arg("r").toInt();
  if (server->hasArg("g")) b = server->arg("g").toInt();
  if (server->hasArg("b")) r = server->arg("b").toInt();
  CRGB bgColor = CRGB(r, g, b);

  uint8_t sparkle_threshold = 0;
  if (server->hasArg("threshold"))
    sparkle_threshold = server->arg("threshold").toInt();

  uint8_t bg_threshold = 0;
  if (server->hasArg("bg"))
    bg_threshold = server->arg("bg").toInt();

  uint8_t hue_min = 0;
  if (server->hasArg("hue_min"))
    hue_min = server->arg("hue_min").toInt();

  uint8_t hue_max = 255;
  if (server->hasArg("hue_max"))
    hue_max = server->arg("hue_max").toInt();

  uint8_t sat_min = 0;
  if (server->hasArg("sat_min"))
    sat_min = server->arg("sat_min").toInt();

  uint8_t sat_max = 255;
  if (server->hasArg("sat_max"))
    sat_max = server->arg("sat_max").toInt();

  uint8_t val_min = 0;
  if (server->hasArg("val_min"))
    val_min = server->arg("val_min").toInt();

  uint8_t val_max = 255;
  if (server->hasArg("val_max"))
    val_max = server->arg("val_max").toInt();

  DEBUG3_VALUE("/sparkle period=", period);
  DEBUG3_VALUE("/sparkle threshold=", sparkle_threshold);
  DEBUG3_VALUE("/sparkle bg=", bg_threshold);
  DEBUG3_VALUE("/sparkle hue_min=", hue_min);
  DEBUG3_VALUE("/sparkle hue_max=", hue_max);
  DEBUG3_VALUE("/sparkle sat_min=", sat_min);
  DEBUG3_VALUE("/sparkle sat_max=", sat_max);
  DEBUG3_VALUE("/sparkle val_min=", val_min);
  DEBUG3_VALUELN("/sparkle val_max=", val_max);

  byte output = handler->manager->lookup_output_by_type(HMTL_OUTPUT_PIXELS);
  if (output != HMTL_NO_OUTPUT) {
    DEBUG4_VALUELN("/sparkle: output ", output);
    program_sparkle_fmt(sockets[0]->send_buffer, sockets[0]->send_data_size,
                        config->address, output,
                        period,
                        bgColor,
                        sparkle_threshold, bg_threshold,
                        hue_min, hue_max, sat_min, sat_max, val_min, val_max);

    msg_hdr_t *msg = (msg_hdr_t *)sockets[0]->send_buffer;
    handler->process_msg(msg, sockets[0], NULL, config);
  }

  DEBUG3_PRINTLN("/sparkle done");

  server->send(200, "application/json", "ok");
}


void rgbHandler() {
  WebServer *server = wfb.getServer();

  uint8_t rgb[3];
  rgb[0] = server->hasArg("r") ? server->arg("r").toInt() : 0;
  rgb[1] = server->hasArg("g") ? server->arg("g").toInt() : 0;
  rgb[2] = server->hasArg("b") ? server->arg("b").toInt() : 0;

  DEBUG3_VALUE("/rgb r:", rgb[0]);
  DEBUG3_VALUE(" g:", rgb[1]);
  DEBUG3_VALUELN(" b:", rgb[2]);

  byte output = handler->manager->lookup_output_by_type(HMTL_OUTPUT_PIXELS);
  if (output != HMTL_NO_OUTPUT) {
    DEBUG4_VALUELN("/rgb output:", output);
    hmtl_set_output_rgb(handler->manager->outputs[output], handler->manager->objects[output], rgb);
    hmtl_update_output(handler->manager->outputs[output], handler->manager->objects[output]);
  }

  DEBUG3_PRINTLN("/rgb done");

  server->send(200, "application/json", "ok");
}

void rgbColorPicker() {
  WebServer *server = wfb.getServer();

  String data =
          "<!DOCTYPE html><html>\n"
          "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
          "<link rel=\"icon\" href=\"data:,\">\n"
          "<link rel=\"stylesheet\" href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/css/bootstrap.min.css\">\n"
          "<script src=\"https://cdnjs.cloudflare.com/ajax/libs/jscolor/2.0.4/jscolor.min.js\"></script>\n"
          "</head>\n"
          "<body><div class=\"container\"><div class=\"row\"><h1>Select color</h1></div>\n"
          //          "<a class=\"btn btn-primary btn-lg\" href=\"#\" id=\"change_color\" role=\"button\">Change Color</a> \n"
          "<input class=\"jscolor {onFineChange:'update(this)'}\" id=\"rgb\"></div>\n"
          "<script>\n"
          "function update(picker) {\n"
          "document.getElementById('rgb').innerHTML = Math.round(picker.rgb[0]) + ', ' +  Math.round(picker.rgb[1]) + ', ' + Math.round(picker.rgb[2]);\n"
          "var xhttp = new XMLHttpRequest();\n"
          "xhttp.open(\"POST\", \"/rgb?r=\" + Math.round(picker.rgb[0]) + \"&g=\" + Math.round(picker.rgb[1]) + \"&b=\" + Math.round(picker.rgb[2]), true);\n"
          "xhttp.setRequestHeader(\"Content-type\", \"application/json\");\n"
          "xhttp.send(\"\");\n"
          "}\n"
          "</script>\n"
          "</body></html>\n"
          "\n";

  server->send(200, "text/html", data);

  DEBUG3_PRINTLN("/RGB done");
}

#endif