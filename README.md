# mppsolar-lib-protocol-to-jk-bms-rs485
An ESP-IDF based implementation for adapting MPP Solar's Lib protocol to the JK BMS RS485 based protocol.

This work is based on:
| Source nr. | Source URL | Description |
| :--------- | :--------- | :---------- |
| 1. | https://github.com/esphome/esphome | From here I took the UART related code and got inspiration for the code structure. I might at some point make the implementation fully compatible with ESP Home. |
| 2. | https://github.com/syssi/esphome-jk-bms/ | From here I took the |

All you need to get going is Visual Studio Code and the ESP IDF extension installed.

You should have a look in KConfig.projbuild and in main.cpp and adjust things as you please.

DISCLAIMER: As right now the entire thing is a work in progress, not yet production ready, don't blame me if we you take it blindly and it works for like an hour or two and afterwards your battery blows up.

So, to get to business: 
**components/jk_bms/*** and **components/jk_modbus/*** headers and .cpp files are modified copies of the code from Syssi.

**components/uart/*** and **/core/*** headers and .cpp files are modified copies of ESP home code.

**main/*** headers and .cpp files is code written by me for handling the communication with Lib protocol based inverters, like the MPP Solar's PIP 4024MT that I own. Inside the main folder you can find the implementation of a Lib protocol UART handler that is based on IDFUartComponent and a data adapter with a mock implementation. The code from components/jk_bms implements the data adapter.

On the electronics side of things, the ESP32 is connected directly to an RS485 to TTL adapter that is powered from the ESP32's board 3.3V rail, and the A and B pins are connected directly to the inverter. The JK BMS is connected directly to the ESP32 as it uses 3.3V based signalling, so all good. I'll add a schematic in the future.
