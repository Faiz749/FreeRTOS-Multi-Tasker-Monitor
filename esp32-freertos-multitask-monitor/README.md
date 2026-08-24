# Day 9: FreeRTOS Multi-Tasker Monitor

## Project Description

A real-time monitoring system built on an **ESP32 using FreeRTOS**. It reads temperature, humidity, and object detection data, processes them through separate tasks, and controls LEDs, a buzzer, and an OLED based on the system state.

I built this project mainly to get hands-on practice with **FreeRTOS and embedded firmware**, especially task management, communication between tasks, synchronization, and timers.

## What It Does

* Reads temperature and humidity using a DHT11
* Detects objects using an IR sensor
* Processes sensor data through separate FreeRTOS tasks
* Detects alert conditions
* Shows live readings on an OLED
* Uses LEDs and a buzzer to indicate the current state
* Prints system information through the Serial Monitor
* Sends a heartbeat message every 5 seconds

## System Architecture

```text
       DHT11 + IR Sensor
              │
              ▼
       ┌──────────────┐
       │  Sensor Task │
       └──────┬───────┘
              │
         Sensor Queue
              │
              ▼
       ┌──────────────┐
       │ Controller   │
       │     Task     │
       └──────┬───────┘
              │
        Analysis Queue
              │
              ▼
       ┌──────────────┐
       │ Analysis Task│
       └──────┬───────┘
              │
       ┌──────┴──────────┐
       ▼                 ▼
  Task Notification   OLED Display
       │
       ▼
┌──────────────┐
│  Output Task │
└──────┬───────┘
       │
   ┌───┴───┐
   ▼       ▼
 LEDs    Buzzer
```

Each task has a separate job. This keeps sensor reading, processing, decision-making, and output control separate instead of putting everything inside one `loop()`.

## FreeRTOS Tasks

| Task            | Priority | Job                                            |
| --------------- | -------: | ---------------------------------------------- |
| Sensor Task     |        2 | Reads the DHT11 and IR sensor                  |
| Controller Task |        2 | Receives and forwards sensor data              |
| Analysis Task   |        3 | Checks conditions and decides the system state |
| Output Task     |        1 | Controls the LEDs and buzzer                   |

All four tasks are pinned to **ESP32 Core 1**.

The Analysis Task has the highest priority because it handles the main system decision.

## FreeRTOS Communication

### Queues

Two queues are used to move sensor data between tasks.

```text
Sensor Task
    │
    ▼
sensorReading
    │
    ▼
Controller Task
    │
    ▼
analysisQueue
    │
    ▼
Analysis Task
```

The readings are stored in one structure:

```cpp
struct Data {
    int IR_value;
    float temperature;
    float humidity;
};
```

This allows the three sensor values to be passed between tasks together.

### Event Groups

The Analysis Task uses an Event Group to keep track of three conditions:

```text
Bit 0 → High temperature
Bit 1 → Object detected
Bit 2 → High humidity
```

If any of these conditions is active, the system changes to `ALERT`.

### Task Notifications

Once the Analysis Task decides the current state, it sends that state directly to the Output Task.

```cpp
xTaskNotify(
    outputTaskHandle,
    currentState,
    eSetValueWithOverwrite
);
```

The Output Task waits for this notification and changes the LEDs and buzzer.

### Mutex

A FreeRTOS mutex is used when accessing the shared Serial output and counter.

```cpp
xSemaphoreTake(serialMutex, portMAX_DELAY);

// shared resource

xSemaphoreGive(serialMutex);
```

This prevents the tasks from accessing the shared resource at the same time.

### Software Timer

A FreeRTOS software timer runs every 5 seconds and prints a heartbeat message.

```text
Every 5 seconds
      │
      ▼
Heartbeat callback
      │
      ▼
System status printed
```

## System States

The system has three states:

```text
             ┌──────────┐
             │   IDLE   │
             └────┬─────┘
                  │
            Valid readings
                  │
                  ▼
             ┌──────────┐
             │  NORMAL  │
             └────┬─────┘
                  │
          Alert condition
                  │
                  ▼
             ┌──────────┐
             │  ALERT   │
             └──────────┘
```

### IDLE

The DHT11 returns an invalid temperature or humidity reading.

### NORMAL

* Temperature ≤ 35°C
* Humidity ≤ 75%
* No object detected

### ALERT

The system enters alert mode when **any** of these conditions is true:

* Temperature > 35°C
* Humidity > 75%
* An object is detected

## Outputs

| State  | Green LED | Red LED | Buzzer |
| ------ | --------- | ------- | ------ |
| IDLE   | OFF       | OFF     | OFF    |
| NORMAL | ON        | OFF     | OFF    |
| ALERT  | OFF       | ON      | ON     |

The OLED displays:

* IR value
* Temperature
* Humidity
* Current state
* Counter

## Hardware

* ESP32 development board
* DHT11
* IR obstacle sensor
* 0.96" SSD1306 OLED
* Green LED
* Red LED
* Buzzer
* Breadboard
* Jumper wires

## Libraries

* DHT sensor library
* Adafruit GFX Library
* Adafruit SSD1306 Library

## Pin Configuration

| Component  | ESP32 Pin |
| ---------- | --------- |
| DHT11 Data | GPIO 4    |
| IR Sensor  | GPIO 34   |
| Green LED  | GPIO 25   |
| Red LED    | GPIO 13   |
| Buzzer     | GPIO 32   |
| OLED SDA   | GPIO 21   |
| OLED SCL   | GPIO 22   |

## Demo Files

- `circuit-diagram.png`
- `serial-monitor.png`
- `demo-video.mp4`

## Embedded Systems Relevance

This project gave me practical experience with **FreeRTOS-based embedded firmware**, where different tasks handle different parts of the system.

Software timers are useful for periodic operations such as system monitoring and timeout handling.

Task notifications provide a simple way for one task to signal another when something happens.

Queues, event groups, and mutexes are also used to move data between tasks and keep shared resources safe.

These are useful building blocks for embedded systems where sensor processing, monitoring, and hardware outputs need to work together in real time.

## Data Flow

```text
DHT11 + IR
    │
    ▼
Sensor Task
    │
    │ Queue
    ▼
Controller Task
    │
    │ Queue
    ▼
Analysis Task
    │
    ├── Check readings
    ├── Update Event Group
    ├── Decide state
    ├── Update OLED
    └── Notify Output Task
                │
                ▼
           Output Task
                │
          ┌─────┴─────┐
          ▼           ▼
        LEDs        Buzzer
```

## Project Structure

```text
FreeRTOS-Multi-Tasker-Monitor/
│
├── FreeRTOS-Multi-Tasker-Monitor.ino
└── README.md
```

## Why I Built It

I built this project to practice **FreeRTOS in an actual embedded application**.

Instead of handling everything in one `loop()`, I separated sensor reading, data handling, analysis, and output control into different tasks. This helped me understand how an RTOS can be used to organize and coordinate different parts of an embedded system.
