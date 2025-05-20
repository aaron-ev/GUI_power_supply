# GUI Power Supply

A Qt-based graphical user interface for controlling and monitoring a programmable power supply.

## Features

- Real-time voltage and current monitoring
- Adjustable output settings
- User settings persistence
- User-friendly interface

## Application Overview

![Power Supply Info](/img/powerSupplyInfo.png)

## Architecture

![Power Supply Architecture](/img/powerSupplyArch.png)

## Power Supply C++ user space driver

Power supply device operations such as reading, opening, and writing are
handled by the user space driver `drv_power_supply.cpp`.
This driver provides a C++ class interface for interacting with the
power supply hardware. Communication is implemented using the standard
VISA C API, with linking performed via the `visa64.lib` library
provided by National Instruments.
