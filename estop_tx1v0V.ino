/* Modified Estop Reciever code for override/bypass button feature.
   Emits 300 Hz tone while button depressed. The code for the tones
   got funky with the different states, and trying to get the relays
   back into a run state when bypassed pressed but no signal meant
   adding extra if statement at top of loop().
   Code and states are bit muddied and redundant now, but is functional.
   Code will require major refactor before more functionality is added.
*/


#include <EasyTransfer.h>
#include <Chrono.h>
#include <EEPROM.h>

//create object
EasyTransfer ET;
Chrono cwdt;
Chrono rlytog;

//#define RELAY_TESTING
//#define ESTOP_RX
#define ESTOP_TX
//#define SILENT

#define ESTOP_TIMEOUT 8                 //should be read from EEPROM
#define ESTOP_MIN_RUN_PACKETS_LIMIT 20  //minimum number of consecutive RUN/GO packets to receive before allowing the relays to reset
#define ESTOP_STOP 0
#define ESTOP_RUN 1

#define RELAY_PULSE_TIME 1


struct SEND_DATA_STRUCTURE {
  uint8_t relays;
  uint8_t cmd;       //bit 8: 0, read; 1, write -- bit 7-0: action command
  uint8_t trans_id;  //transaction id (optional)
  uint8_t data;      //data for cmd. ignored if the cmd is a read command
};

//struct EEPROM_STRUCTURE {
//  int estop_timeout;
//  int estop_min_run_packets_limit;
//  int relay_pulse_time;
//}

#define TRANS_SET_ESTOP_TIMEOUT 0x81     //Timeout in seconds with no valid run command before opening the estop relay
#define TRANS_SET_PAUSE_TIMEOUT 0x82     //Timeout in seconds with no valid run command before opening the pause relay
#define TRANS_SET_RUN_COUNT_ENABLE 0x83  //Amount of valid RUN commands to receive before closing the estop and pause relay
#define TRANS_SET_ESTOP_RELAY 0x88       //Command to close/open the estop relay. Close/open is determined by data byte (0 - open; 1 - close)
#define TRANS_SET_PAUSE_RELAY 0x89       //Command to close/open the estop relay. Close/open is determined by data byte

#define TRANS_WRITE_TO_EEPROM 0xA0  //Write the value of the current running value of the variable contained in data to EEPROM \
                                    //0x01  ESTOP_TIMEOUT \
                                    //0x02  PAUSE_TIMEOUT \
                                    //0x03  RUN_COUNT_ENABLE

#define TRANS_GET_ESTOP_TIMEOUT 0x01
#define TRANS_GET_PAUSE_TIMEOUT 0x02
#define TRANS_GET_RUN_COUNT_ENABLE 0x03
#define TRANS_GET_ESTOP_RELAY 0x08
#define TRANS_GET_PAUSE_RELAY 0x09



SEND_DATA_STRUCTURE estop;

#ifdef ESTOP_RX
#define RELAY1 14
#define RELAY2 15
#define RELAY3 16
#define RELAY4 17

#define RELAY_OFF HIGH
#define RELAY_ON LOW

#define RAD_M0 12
#define RAD_M1 11
#define RAD_AUX 2

#define OVER_BUT 23
#define OVER_LED 22

#endif  //ESTOP_RX

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
bool overridePressed = false;

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

  //pinMode(OVER_BUT, INPUT_PULLUP);
  //pinMode(OVER_LED, OUTPUT);

  digitalWrite(RAD_M0, LOW);  // Both M0 and M1 should be low for normal operation
  digitalWrite(RAD_M1, LOW);

#ifdef ESTOP_RX
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);
  pinMode(RELAY4, OUTPUT);

  digitalWrite(RELAY1, RELAY_OFF);
  digitalWrite(RELAY2, RELAY_OFF);
  digitalWrite(RELAY3, RELAY_OFF);
  digitalWrite(RELAY4, RELAY_OFF);
#endif  //ESTOP_RX

#ifdef ESTOP_TX
  pinMode(SW1, INPUT);
  pinMode(SW2, INPUT);
  pinMode(SW3, INPUT);
  pinMode(SW4, INPUT);
  pinMode(SW_ESTOP, INPUT);
  pinMode(ESTOP_BTN_LED_CTL, OUTPUT);
  //pinMode(13, HIGH); // ?
  //read inputs and set state
  digitalWrite(13, HIGH);  // <-- This is the corrected line
  estop.relays = 1;
#endif  //ESTOP_TX
}

void loop() {

#if defined(RELAY_TESTING) && defined(ESTOP_RX)
  // ... (This testing code remains unchanged) ...
  digitalWrite(RELAY1, HIGH);
  delay(1000);
  digitalWrite(RELAY1, LOW);
  delay(1000);
  digitalWrite(RELAY2, HIGH);
  delay(1000);
  digitalWrite(RELAY2, LOW);
  delay(1000);
  digitalWrite(RELAY3, HIGH);
  delay(1000);
  digitalWrite(RELAY3, LOW);
  delay(1000);
  digitalWrite(RELAY4, HIGH);
  delay(1000);
  digitalWrite(RELAY4, LOW);
  delay(1000);
#endif

  // ... (Your commented-out override code remains unchanged) ...
  //overridePressed = !digitalRead(OVER_BUT);
  //if(overridePressed){
  //...
  //}

#if !defined(RELAY_TESTING)

  if (ET.receiveData()) {
    cwdt.restart();  //restart counter
#ifdef ESTOP_RX
    Serial.println(estop.relays);

    // ... (All of your ESTOP_RX logic remains unchanged) ...
    if ((estop.relays & 1) && !overridePressed) {  // Big red button got pushed & override not pressed
      // ...
    } else {  // Big red button is up!
      // ...
    }

    // ...

#endif  //ESTOP_RX
  }


#ifdef ESTOP_RX
  delay(50);

  if (cwdt.hasPassed(ESTOP_TIMEOUT * 1000) && !overridePressed) {
    // ... (This ESTOP_RX timeout logic remains unchanged) ...
  }

#endif  //ESTOP_RX




#ifdef ESTOP_TX
  // --- START: Added logic for pulsing onboard LED (Pin 13) ---

  // Static variables retain their value between loop() calls
  static unsigned long pulse_timer_prev = 0;
  static int pulse_brightness = 0;  // Current brightness (0-255)
  static int pulse_direction = 5;   // Amount to change brightness each step

  // How often to update the pulse (milliseconds). Smaller = faster pulse.
  const int PULSE_STEP_INTERVAL = 20;

  // --- END: Added logic for pulsing ---


  estop.relays = 0;  //clear all states before setting them

  if (digitalRead(SW_ESTOP) == 1) {
    // --- STOP STATE ---
    estop.relays = estop.relays | 1;

    // --- Set onboard LED (Pin 13) to SOLID ON ---
    analogWrite(13, 255);
    // Reset pulse variables so it starts from 0 next time
    pulse_brightness = 0;
    pulse_direction = 5;
    // ---


    //Flash the EXTERNAL LED (ESTOP_BTN_LED_CTL) to alert the user
    time_now = millis();
    if ((time_now - time_prev) > 500) {
      time_prev = time_now;
      digitalWrite(ESTOP_BTN_LED_CTL, !digitalRead(ESTOP_BTN_LED_CTL));
    }
  } else {
    // --- RUN STATE ---

    //Set EXTERNAL LED (ESTOP_BTN_LED_CTL) to solid ON
    digitalWrite(ESTOP_BTN_LED_CTL, HIGH);

    // --- Set onboard LED (Pin 13) to SLOW PULSE ---
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
    // ---
  }

  // --- (Rest of your TX logic is unchanged) ---
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