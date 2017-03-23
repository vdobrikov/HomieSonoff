/*
 *  Homie switch (Itead Sonoff board) node
 *  
 *  MCU:
 *   - Sonoff (ESP8266)
 *  
 *  Integration type: 
 *   - Homie (MQTT)
 *   
 *  MQTT topics:
 *   state_topic: devices/device_id/switch0/on ON|OFF
 *   command_topic: "devices/device_id/switch0/on/set ON|OFF"   
 *   
 *  Homie version:
 *   1.5.0
 *  
 *  Sonoff pinout (button top):
 *     Button (GPIO 0)
 *      VCC (3.3)
 *      RX
 *      TX
 *      GND
 *      GPIO 14 (not on all boards)
 *      
 *  Flashing notes:
 *   - Board ESP 8266 (generic)
 *   - Flash size: 1M (64K SPIFFS)
 *   - Hold button to boot in flash mode
 *   
 *  Author:
 *   Vladimir Dobrikov <hedin.mail@gmail.com>
 */
 
#include <Homie.h>
#include <Bounce2.h>
#include <AsyncDelay.h>

#define FW_NAME "itead-sonoff"
#define FW_VERSION "1.0.2"

#define PIN_RELAY 12
#define PIN_LED 13
#define PIN_BUTTON 0

#define TIMEOUT_MILLIS_STATUS 60000 // 1 min
#define TIMEOUT_MILLIS_DISCONNECTED 60000 // 1 min

HomieNode switchNode("switch0", "switch");

volatile bool swSentStatus = false;

Bounce debouncer = Bounce(); 
AsyncDelay delayStatus;

bool disconnected = false;
unsigned long disconnectedTime = 0;

void setup() {
  Serial.begin(115200);
  Serial.print(FW_NAME);
  Serial.print(" ");
  Serial.println(FW_VERSION);
  Serial.println();
  
  pinMode(PIN_BUTTON,INPUT_PULLUP);
  debouncer.attach(PIN_BUTTON);
  debouncer.interval(5); // interval in ms

  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, LOW);  

  Homie.setLedPin(PIN_LED, LOW);
  Homie.setFirmware(FW_NAME, FW_VERSION);  
  Homie.registerNode(switchNode);
  switchNode.subscribe("on", switchOnHandler);
  Homie.setSetupFunction(setupHandler);
  Homie.setLoopFunction(loopHandler);
  Homie.setup();
}

void loop() {
  debouncer.update();
  
  if (debouncer.fell()) {
    toggleSwitch();
  } 

  Homie.loop();

  if (!Homie.isReadyToOperate()) {
    if (disconnected && millis() - disconnectedTime > TIMEOUT_MILLIS_DISCONNECTED) {
      ESP.restart();
    }
    if (!disconnected) {
      disconnected = true;  
      disconnectedTime = millis();
    }
  } else {
    disconnected = false;  
  }
}

///////// Handlers //////////
void setupHandler() {
  delayStatus.start(TIMEOUT_MILLIS_STATUS, AsyncDelay::MILLIS);
}

void loopHandler() {
  if (!swSentStatus || delayStatus.isExpired()) {
    Serial.println("Need to send status");
    swSentStatus = sendStatus();
    delayStatus.repeat();
  }
}

bool switchOnHandler(String value) {
  if (value == "ON") {
    digitalWrite(PIN_RELAY, HIGH);
  } else if (value == "OFF") {
    digitalWrite(PIN_RELAY, LOW);
  } else {
    return false;
  }
  swSentStatus = false;
  Serial.print("Remote event: ");
  Serial.println(value);

  return true;
}

bool sendStatus() {
  if (Homie.isReadyToOperate()) {
    bool curStatus = digitalRead(PIN_RELAY);
    Homie.setNodeProperty(switchNode, "on", curStatus ? "ON" : "OFF", true);
    Serial.print("Status sent successfully (");
    Serial.print(curStatus ? "ON" : "OFF");
    Serial.println(")");
    return true;
  } else {
    Serial.println("Status cannot be sent");
    return false;
  }
}

void toggleSwitch() {
  bool curStatus = digitalRead(PIN_RELAY);
  digitalWrite(PIN_RELAY, !curStatus);
  swSentStatus = false;
  Serial.print("Local event: ");
  Serial.println(!curStatus ? "ON" : "OFF");
}


