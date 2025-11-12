# Wireless E-Stop System

A reliable wireless emergency stop system for robotics and industrial applications, featuring separate transmitter (TX) and receiver (RX) units communicating via E32 LoRa radio modules.

## Overview

This system provides a safety-critical wireless emergency stop solution with the following components:

- **Handheld Transmitter (TX)**: Portable unit with emergency stop button and status LED
- **Receiver (RX)**: Stationary unit controlling up to 4 relays for machine safety interlocks

## Features

### Transmitter (TX)
- Emergency stop button with LED feedback
- Visual status indication (flashing when stopped, solid when running)
- Supports up to 4 switch inputs
- Continuous heartbeat transmission
- Built on Arduino-compatible hardware

### Receiver (RX)
- Controls 4 independent safety relays
- Heartbeat monitoring with configurable timeout
- Audible alarm (buzzer) on communication loss
- Visual status indication via onboard LED
- Requires minimum consecutive RUN packets before releasing safety relays
- Failsafe operation - defaults to STOP state on power-up or communication loss

### Safety Features
- **Heartbeat Monitoring**: 1-second timeout triggers alarm
- **Packet Counting**: Requires 20 consecutive valid RUN packets before releasing relays
- **Persistent Alarm**: Buzzer continues even after communication is restored
- **LED Indicators**: 
  - Solid LED on power-up
  - Blinking LED on each heartbeat received
- **Failsafe Design**: System defaults to STOP state on any fault condition

## Hardware Requirements

### Both Units
- Arduino-compatible microcontroller (tested with Arduino)
- E32 LoRa radio module (433MHz/868MHz/915MHz depending on region)
- Power supply appropriate for your hardware

### Transmitter
- Emergency stop button (normally open)
- LED for status indication
- Optional: 3 additional switches

### Receiver
- 4x relay modules (active-low control)
- Piezo buzzer on pin 13
- Onboard LED (pin 13)

## Pin Configuration

### Receiver (RX)
```
Pin 13: Onboard LED & Buzzer
Pin 14: Relay 1 (E-Stop)
Pin 15: Relay 2 (Run)
Pin 16: Relay 3 (Pulse output)
Pin 17: Relay 4 (Toggle indicator)
Pin 12: Radio M0
Pin 11: Radio M1
Pin 2:  Radio AUX
Serial1: E32 Radio communication (9600 baud)
```

### Transmitter (TX)
```
Pin 14: Emergency Stop Button
Pin 15: Switch 2 (optional)
Pin 16: Switch 3 (optional)
Pin 17: Switch 4 (optional)
Pin 22: Switch 1 (optional)
Pin 23: E-Stop Button LED Control
Pin 4:  Radio M0
Pin 3:  Radio M1
Pin 2:  Radio AUX
Serial1: E32 Radio communication (9600 baud)
```

## Installation

1. Clone this repository:
```bash
git clone https://github.com/yourusername/wireless-estop-system.git
```

2. Install required libraries:
   - [EasyTransfer](https://github.com/madsci1016/Arduino-EasyTransfer)
   - [Chrono](https://github.com/SofaPirate/Chrono)
   - EEPROM (included with Arduino IDE)

3. Open the appropriate sketch in Arduino IDE:
   - `estop_tx.ino` for transmitter
   - `estop_rx.ino` for receiver

4. Configure the `#define` directives at the top of each file to match your hardware

5. Upload to your respective Arduino boards

## Configuration

### Compile-Time Options
Edit the `#define` directives in each sketch:

**Receiver:**
```cpp
#define ESTOP_TIMEOUT 8                    // Timeout in seconds before triggering failsafe
#define ESTOP_MIN_RUN_PACKETS_LIMIT 20     // Consecutive RUN packets needed to release relays
#define RELAY_PULSE_TIME 1                 // Relay pulse duration in seconds
#define BUZZER_TIMEOUT 1000                // Buzzer delay in milliseconds
```

**Testing Modes:**
```cpp
#define RELAY_TESTING  // Uncomment to test relay cycling
#define SILENT         // Uncomment to disable buzzer
```

### Runtime Configuration
Future versions will support runtime configuration via the transaction protocol defined in the code.

## Operation

### Normal Operation
1. Power on receiver unit - LED will be solid ON
2. Power on transmitter unit
3. Receiver LED will begin blinking with each heartbeat
4. Release emergency stop button on transmitter
5. After receiving 20 consecutive RUN packets, relays will activate

### Emergency Stop
1. Press emergency stop button on transmitter
2. Receiver immediately opens safety relays
3. Transmitter LED flashes to indicate STOP state

### Communication Loss
1. If no heartbeat received for 1 second, buzzer activates
2. If no heartbeat for 8 seconds, relays open (failsafe)
3. Relay 4 toggles every second to indicate fault condition
4. Buzzer continues until unit is power cycled

## Safety Warnings

⚠️ **CRITICAL SAFETY INFORMATION** ⚠️

- This system is intended for safety-critical applications
- Always implement redundant safety systems
- Test thoroughly before deploying in any application
- Wireless systems can be subject to interference
- This code is provided AS-IS without warranty
- Consult with qualified safety engineers before using in industrial applications
- Comply with all local safety regulations and standards

## Protocol

The system uses the EasyTransfer library for packet-based communication. Future versions will implement the full transaction protocol defined in the code for runtime configuration.

## Contributing

Contributions are welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly (especially safety-critical features)
5. Submit a pull request

## License
GPL

## Author

Scott McLeslie Fathom Robotics

## Acknowledgments

- EasyTransfer library by Bill Porter
- Chrono library by SofaPirate
- Arduino community

## Version History

- **v1.1** - Added buzzer timeout, heartbeat LED, improved safety features
- **v1.0** - Initial release

## Support

For issues, questions, or contributions, please open an issue on GitHub.

---

**Disclaimer**: This system is provided for educational and experimental purposes. Always ensure compliance with applicable safety standards and regulations for your specific use case.
