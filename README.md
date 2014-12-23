HMTL
====

This project contains protocols designed to make it easy to build networked modules which trigger external actions, read sensors, etc

* Store module configuration in EEPROM
* Load configuration and automatically set up the configured inputs and outputs
...

To Use
======

In order to compile these sketches the contents of the Libraries must be linked to from within your Arduino/libaries directory.

They also depend on several libraries that can be found here:
  * https://github.com/aphelps/ArduinoLibs

Contents
========


Wiring
======

## 4-Pin XLR

Pin | Use      | 4-wire color
------------------------
1   | GND      | White
2   | Data 1/A | Black
3   | Data 2/B | Green
4   | VCC (12V)| Red    
