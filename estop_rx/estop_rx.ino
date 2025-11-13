/*
 * E-Stop Receiver (RX) Code
 * Version 1.1
 *
 * Wireless emergency stop receiver using E32 LoRa radio module.
 * Controls 4 safety relays based on heartbeat packets from transmitter.
 *
 * Features:
 * - Heartbeat monitoring with 8-second timeout
 * - 4 independent relay control (active-low)
 * - 20-packet safety threshold before enabling RUN state
 * - 1-second buzzer warning before failsafe
 * - Override button to force RUN state (bypasses E-stop and timeout)
 * - Override LED indication (blinking on heartbeat, solid when active)
 * - Periodic buzzer in override mode (2 buzzes every 5 seconds)
 * - Failsafe defaults to STOP state on power-up or timeout
 * - SILENT mode support (disable all buzzer sounds)
 *
 * Hardware: Teensy 4.0 with E32 LoRa module
 * Pin 13: Shared between onboard LED and buzzer (buzzer takes priority when active)
 */

#include <EasyTransfer.h>
#include <Chrono.h>

//create object
EasyTransfer ET; 
Chrono cwdt;
Chrono rlytog;
Chrono buzzerTimer;  // NEW: Timer for 1000ms buzzer timeout
Chrono overrideBuzzerTimer;  // NEW: Timer for 5-second periodic buzzer in override mode

//#define RELAY_TESTING
#define ESTOP_RX
//#define ESTOP_TX
//#define SILENT

#define ESTOP_TIMEOUT 8   // Timeout in seconds before triggering failsafe
#define ESTOP_MIN_RUN_PACKETS_LIMIT 20   //minimum number of consecutive RUN/GO packets to receive before allowing the relays to reset
#define ESTOP_STOP 0
#define ESTOP_RUN 1

#define RELAY_PULSE_TIME 1
#define BUZZER_TIMEOUT 1000  // NEW: 1000ms before buzzer starts


struct SEND_DATA_STRUCTURE {
  uint8_t relays;
};



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

// NEW: E-stop override pins from schematic
#define ESTOP_OVERRIDE_PIN 23
#define ESTOP_OVERRIDE_LED_PIN 22

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
bool ononce = false; //Matt

volatile unsigned long time_now;
volatile unsigned long time_prev;

// NEW: Variables for new features
bool buzzerActive = false;
int overrideBuzzerCount = 0;  // Track number of buzzes in override mode


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
  
#ifdef ESTOP_RX
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);
  pinMode(RELAY4, OUTPUT);

  digitalWrite(RELAY1, RELAY_OFF);
  digitalWrite(RELAY2, RELAY_OFF);
  digitalWrite(RELAY3, RELAY_OFF);
  digitalWrite(RELAY4, RELAY_OFF);
  
  // NEW: Setup E-stop override
  pinMode(ESTOP_OVERRIDE_PIN, INPUT_PULLUP);  // Pull-up so open = HIGH
  pinMode(ESTOP_OVERRIDE_LED_PIN, OUTPUT);
  digitalWrite(ESTOP_OVERRIDE_LED_PIN, LOW);
#endif //ESTOP_RX

#ifdef ESTOP_TX
  pinMode(SW1, INPUT);
  pinMode(SW2, INPUT);
  pinMode(SW3, INPUT);
  pinMode(SW4, INPUT);
  pinMode(SW_ESTOP, INPUT);
  pinMode(ESTOP_BTN_LED_CTL, OUTPUT);
  pinMode(13, HIGH); // ?
  //read inputs and set state
  estop.relays = 1;
#endif  //ESTOP_TX

  // NEW: Start the buzzer timers
  buzzerTimer.restart();
  overrideBuzzerTimer.restart();
}

void loop() {

  #if defined(RELAY_TESTING) && defined(ESTOP_RX)
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

#if !defined(RELAY_TESTING)
  
  if (ET.receiveData()) {
    cwdt.restart();  //restart counter
    buzzerTimer.restart();  // NEW: Restart buzzer timer on every valid packet

    #ifdef ESTOP_RX
    Serial.println(estop.relays);

    // NEW: Check if E-stop override is active
    bool overrideActive = (digitalRead(ESTOP_OVERRIDE_PIN) == HIGH);  // Active HIGH (open pins)

    // If override is active, force RUN state and ignore E-stop button
    if (overrideActive) {
      estopState = ESTOP_RUN;
      digitalWrite(RELAY1, RELAY_OFF);
      digitalWrite(RELAY2, RELAY_ON);
      digitalWrite(RELAY3, RELAY_OFF);
      digitalWrite(RELAY4, RELAY_OFF);
      noTone(13);
      buzzerActive = false;
      // LED stays solid (handled in main loop section)
    }
    // Normal operation (no override)
    else {
      // Blink E-stop override LED on heartbeat (if override not active)
      digitalWrite(ESTOP_OVERRIDE_LED_PIN, !digitalRead(ESTOP_OVERRIDE_LED_PIN));

      if (estop.relays & 1) {  // Big red button got pushed
      estopState = ESTOP_STOP; 
      time_now = millis();
      runPackets = 0;
      ononce = false;
      digitalWrite(RELAY1, RELAY_ON);
      digitalWrite(RELAY2, RELAY_OFF);
      digitalWrite(RELAY3, RELAY_ON); //Matt
      if (!ononce && (time_now - time_prev) > RELAY_PULSE_TIME * 1000) {
          digitalWrite(RELAY3, RELAY_OFF);
        } //Matt
      if ((time_now - time_prev) > RELAY_PULSE_TIME * 1000) {
        time_prev = time_now;
        ononce = true; //Matt
        digitalWrite(RELAY4, !digitalRead(RELAY4));
        #if !defined(SILENT)
        tone(13, 4000);
        #endif
        }
    }
    else {  // Big red button is up!
      if (runPackets <= ESTOP_MIN_RUN_PACKETS_LIMIT) {
        runPackets++;
        // Stop alarm buzzer since communication is restored (even if not in full RUN yet)
        if (runPackets == 1) {
          noTone(13);
          buzzerActive = false;
        }
      }
      else {   // Now in valid RUN/GO state
        estopState = ESTOP_RUN;
        digitalWrite(RELAY1, RELAY_OFF);
        digitalWrite(RELAY2, RELAY_ON);
        digitalWrite(RELAY3, RELAY_OFF);
        digitalWrite(RELAY4, RELAY_OFF);
        // NEW: Stop buzzer when returning to RUN state
        noTone(13);
        buzzerActive = false;
      }

    }
    }  // End of else (normal operation, no override)

    #endif  //ESTOP_RX
  }
  
 
  #ifdef ESTOP_RX
  delay(50);
  
  // NEW: E-stop override LED control and buzzer
  bool overrideActive = (digitalRead(ESTOP_OVERRIDE_PIN) == HIGH);  // Active HIGH (open pins)
  if (overrideActive) {
    // Override active - LED solid ON
    digitalWrite(ESTOP_OVERRIDE_LED_PIN, HIGH);
    
    // Buzz every 5 seconds twice at half volume (2000Hz instead of 4000Hz)
    if (overrideBuzzerTimer.hasPassed(5000)) {
      overrideBuzzerTimer.restart();
      overrideBuzzerCount = 0;  // Reset buzz counter
    }
    
    // Generate two short buzzes
    unsigned long elapsed = overrideBuzzerTimer.elapsed();
    if (elapsed < 100 || (elapsed >= 200 && elapsed < 300)) {
      // First buzz (0-100ms) or second buzz (200-300ms)
      #if !defined(SILENT)
      tone(13, 2000);  // Half the frequency = less loud
      #endif
    } else {
      noTone(13);
    }
  } else {
    // If override not active, LED blinks on heartbeat (handled in receiveData section above)
    overrideBuzzerTimer.restart();  // Reset timer when not in override
  }
  
  // NEW: Check if buzzer should activate after 1000ms without heartbeat (only if not in override)
  if (!overrideActive && buzzerTimer.hasPassed(BUZZER_TIMEOUT) && !buzzerActive) {
    buzzerActive = true;
    #if !defined(SILENT)
    tone(13, 4000);
    #endif
  }

  // Check for timeout - but NOT if override is active
  bool overrideActiveTimeout = (digitalRead(ESTOP_OVERRIDE_PIN) == HIGH);
  if (!overrideActiveTimeout && cwdt.hasPassed(ESTOP_TIMEOUT * 1000)) {  //ESTOP_TIMEOUT seconds have passed since a valid packet
      time_now = millis();
      runPackets = 0;
      digitalWrite(RELAY1, RELAY_ON);
      digitalWrite(RELAY2, RELAY_OFF);
      if ((time_now - time_prev) > RELAY_PULSE_TIME * 1000) {
        time_prev = time_now;
        digitalWrite(RELAY4, !digitalRead(RELAY4));
        #if !defined(SILENT)
          tone(13, 4000);
        #endif
      }
  }
  #endif  //ESTOP_RX


  
  
  #ifdef ESTOP_TX
  estop.relays = 0;  //clear all states before setting them
  // Will prob ignore the other SWx events and only act on SW_ESTOP events
  if (digitalRead(SW_ESTOP) == 1) {
    estop.relays = estop.relays | 1;
    //Flash the LED to alert the user that the system is in STOP mode
    time_now = millis();
    if (abs(time_now - time_prev) > 500) {
      time_prev = time_now;
      digitalWrite(ESTOP_BTN_LED_CTL, !digitalRead(ESTOP_BTN_LED_CTL));
    }
  }
  else
     digitalWrite(ESTOP_BTN_LED_CTL, HIGH);  //ESTOP is in RUN/GO mode. LED is solid for the user to see quickly
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
  
#endif //not testing
}
