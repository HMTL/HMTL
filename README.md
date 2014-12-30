HMTL
====

## History

This project was originally started to develop a hardware/software platform for controlling 12V LED strips and 12V powered propane flame effects for a Burning Man art car (Ku, the Heavy Metal Tiki Lounge/God, https://www.facebook.com/KuHMTL), however the capabilities of this project have now expanded well past that.

## Overview

This project contains protocols designed to make it easy to build networked modules which read sensors and can activate various external devices.  Currently supported types include:

  * Value (Single-color LEDs, solenoids and ignitors for flame effects)
  * RGB (RGB LEDs and LED strips)
  * Pixels (WS2801 style LEDs, easily expandable to any type supported by the FastLED library, https://github.com/FastLED/FastLED)
  * MPR121 (MPR121 capacitive touch sensors)
  * RS485 (RS485 communication, such as ST485 or MAX485 chips)

The configuration for these modules are stored in EEPROM, is loaded during module startup and the devices are trivially setup and configured.

A general purpose protocol is used to communicate with the devices over any supported hardware layer, which currently includes RS485 for longer-distance two-wire connections and standard serial connections for communication via USB, Bluetooth modules such as Adafruit's Bluefruit EZ-Link (http://www.adafruit.com/product/1588), or serial-enabled wireless devices such as XBee radios.

## Hardware

While many features of these libraries can be used with generic Arduinos or breadboarded project, several special purpose PCBs have been designed for use with this code:

* Trigger Module
* HMTL Controller

## Example projects
* HMTL Fire Control box: https://github.com/HMTL/HMTL_Fire_Control
* Adam's Cube and Triangle lights: https://github.com/aphelps/ObjectLights

To Use
------

In order to compile these sketches the contents of the Libraries must be linked to from within your Arduino/libaries directory, in addition several dependent libraries must also be installed:

Several additional libraries must be installed to make use of this project:
* MPR121, PixelUtils, etc: https://github.com/aphelps/ArduinoLibs
* Nick Gammon's RS485 library: http://www.gammon.com.au/Arduino/RS485_non_blocking.zip

Networking
----------

HMTL Module Code
----------------

Python tools
------------

Wiring
------

## 4-Pin XLR

Pin | Use      | 4-wire color
--- | -------- | -------
1   | GND      | White
2   | Data 1/A | Black
3   | Data 2/B | Green
4   | VCC (12V)| Red    

Future work
-----------