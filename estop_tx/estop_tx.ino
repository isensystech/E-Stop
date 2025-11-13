/*
 * E-Stop Transmitter (TX) Code
 * Version 1.0
 *
 * Wireless emergency stop transmitter using E32 LoRa radio module.
 * Continuously sends heartbeat packets with E-stop button state.
 *
 * Features:
 * - Emergency stop button monitoring
 * - Continuous heartbeat transmission (100ms interval)
 * - External LED status indication (flashing in STOP, solid in RUN)
 * - Onboard LED status (solid in STOP, pulsing in RUN)
 * - Support for additional switches (SW1-4, currently unused)
 *
 * Hardware: Teensy 4.0 with E32 LoRa module
 */


#include <EasyTransfer.h>
#include <Chrono.h>

//create object
EasyTransfer ET;
Chrono cwdt;
Chrono rlytog;

//#define RELAY_TESTING
//#define ESTOP_RX
#define ESTOP_TX
//#define SILENT

#define ESTOP_TIMEOUT 8                 // Timeout in seconds before triggering failsafe
#define ESTOP_MIN_RUN_PACKETS_LIMIT 20  //minimum number of consecutive RUN/GO packets to receive before allowing the relays to reset
#define ESTOP_STOP 0
#define ESTOP_RUN 1

#define RELAY_PULSE_TIME 1


struct SEND_DATA_STRUCTURE {
  uint8_t relays;
};



SEND_DATA_STRUCTURE estop;


#ifdef ESTOP_TX
#define SW1 22
#define SW2 15
#define SW3 16
#define SW4 17
#define SW_ESTOP 14

#define RAD_M0 4
#define RAD_M1 3
#define RAD_AUX 2

#define ESTOP_BTN_LED_CTL 23
#endif  //ESTOP_TX

int runPackets = 0;
int estopState = ESTOP_STOP;
bool ononce = false;  //Matt

volatile unsigned long time_now;
volatile unsigned long time_prev;

void setup() {
  // Debug terminal (USB)
  Serial.begin(9600);
  Serial.println("Starting up....");

  // ES32 radio
  Serial1.begin(9600);
  ET.begin(details(estop), &Serial1);


  pinMode(13, OUTPUT);
  pinMode(RAD_M0, OUTPUT);
  pinMode(RAD_M1, OUTPUT);
  pinMode(RAD_AUX, INPUT);

  digitalWrite(RAD_M0, LOW);  // Both M0 and M1 should be low for normal operation
  digitalWrite(RAD_M1, LOW);

#ifdef ESTOP_TX
  pinMode(SW1, INPUT);
  pinMode(SW2, INPUT);
  pinMode(SW3, INPUT);
  pinMode(SW4, INPUT);
  pinMode(SW_ESTOP, INPUT);
  pinMode(ESTOP_BTN_LED_CTL, OUTPUT);
  digitalWrite(13, HIGH);  // Set onboard LED on
  estop.relays = 1;
#endif  //ESTOP_TX
}

void loop() {

#if !defined(RELAY_TESTING)




#ifdef ESTOP_TX
  // Static variables for pulsing onboard LED (Pin 13)
  static unsigned long pulse_timer_prev = 0;
  static int pulse_brightness = 0;  // Current brightness (0-255)
  static int pulse_direction = 5;   // Amount to change brightness each step
  const int PULSE_STEP_INTERVAL = 20;  // How often to update the pulse (milliseconds)

  estop.relays = 0;  //clear all states before setting them

  if (digitalRead(SW_ESTOP) == 1) {
    // STOP STATE: E-stop button pressed
    estop.relays = estop.relays | 1;

    // Set onboard LED (Pin 13) to SOLID ON
    analogWrite(13, 255);
    // Reset pulse variables so it starts from 0 next time
    pulse_brightness = 0;
    pulse_direction = 5;

    // Flash the EXTERNAL LED (ESTOP_BTN_LED_CTL) to alert the user
    time_now = millis();
    if ((time_now - time_prev) > 500) {
      time_prev = time_now;
      digitalWrite(ESTOP_BTN_LED_CTL, !digitalRead(ESTOP_BTN_LED_CTL));
    }
  } else {
    // RUN STATE: E-stop button released

    // Set EXTERNAL LED (ESTOP_BTN_LED_CTL) to solid ON
    digitalWrite(ESTOP_BTN_LED_CTL, HIGH);

    // Set onboard LED (Pin 13) to SLOW PULSE
    unsigned long pulse_time_now = millis();
    if (pulse_time_now - pulse_timer_prev > PULSE_STEP_INTERVAL) {
      pulse_timer_prev = pulse_time_now;  // Reset timer for next step

      pulse_brightness += pulse_direction;  // Change brightness

      // Reverse direction when it hits the top or bottom
      if (pulse_brightness <= 0) {
        pulse_brightness = 0;
        pulse_direction = 5;  // Start going up
      } else if (pulse_brightness >= 255) {
        pulse_brightness = 255;
        pulse_direction = -5;  // Start going down
      }

      analogWrite(13, pulse_brightness);  // Apply the new brightness
    }
  }

  // Read additional switches (currently not used in protocol)
  if (digitalRead(SW2) == 1)
    estop.relays = estop.relays | 2;
  if (digitalRead(SW3) == 1)
    estop.relays = estop.relays | 4;
  if (digitalRead(SW4) == 1)
    estop.relays = estop.relays | 8;

  Serial.println(estop.relays);


  ET.sendData();
  delay(100);
#endif  //ESTOP_TX

#endif  //not testing
}