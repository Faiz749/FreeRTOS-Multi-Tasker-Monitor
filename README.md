# FreeRTOS Multi-Tasker Monitor

A real-time monitoring project built with an **ESP32 and FreeRTOS**. It reads temperature, humidity, and object detection data and processes them using multiple FreeRTOS tasks.

The project was built to get practical experience with **RTOS-based embedded firmware** and task communication.

## Features

* DHT11 temperature and humidity monitoring
* IR object detection
* 4 FreeRTOS tasks
* Queues and event groups
* Task notifications
* Mutex synchronization
* Software timer
* OLED display
* LED and buzzer alerts
* System state handling

## Hardware

* ESP32
* DHT11
* IR obstacle sensor
* 0.96-inch SSD1306 OLED
* Green LED
* Red LED
* Buzzer
* Breadboard and jumper wires

## Libraries

* DHT sensor library
* Adafruit GFX Library
* Adafruit SSD1306 Library

## Run the Project

1. Install the required libraries.
2. Connect the hardware using the pin configuration in the project README.
3. Open `main.ino`.
4. Select the ESP32 board and correct port.
5. Upload the code.
6. Open Serial Monitor at `115200` baud.

## Demo

* `circuit-diagram.png.jpeg`
* `serial-monitor.png`
* `demo-video.mp4.mp4`

## Technical Details

For the complete FreeRTOS architecture, task design, communication methods, state logic, pin configuration, and data flow, see:

**[Detailed Project README](esp32-freertos-multitask-monitor/README-9.md)**

## Project Status

Complete and tested on real ESP32 hardware.
