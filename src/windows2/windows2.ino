// Open or close all 8 green house windows when internal temperature is higher
// or lower than a fixed threshold.

// History:
// 2016-04-01 Yoann Aubineau - Initial program
// 2016-08-16 Yoann Aubineau - Code cleanup before pushing to Github
// 2019-12-27 Nicolas Leroy - Humidity and state readings added

// Todo:
// - Make board LED blink when temperature is between thredsholds.
// - This code is pretty low-lever and tedious to follow. Introducing a higher
//   level `Window` class could improve readability substantially.

#include "DHT.h"

// ****************************************************************************
// WINDOWS                                     N1, N2, N3, N4, S1, S2, S3, S4

//const int window_count = 8;
const int window_count = 8;
#define FOR_EACH_WINDOW for (int i = 0; i < window_count; i++)


const int win_direction_pins[window_count] = { 22, 24, 26, 28, 30, 32, 34, 36 };
const int win_action_pins[window_count]    = { 38, 40, 42, 44, 46, 48, 50, 52 };
const int win_opened_pins[window_count]    = { 39, 41, 43, 45, 47, 49, 51, 53 };
const int win_closed_pins[window_count]    = { 23, 25, 27, 29, 31, 33, 35, 37 };
int win_status[window_count]               = {  0,  0,  0,  0,  0,  0,  0,  0 };
unsigned long win_started_at[window_count] = {  0,  0,  0,  0,  0,  0,  0,  0 };
/*
const int win_direction_pins[window_count] = { 10, 10, 10, 10, 10, 10, 10, 10  };
const int win_action_pins[window_count]    = { 11, 11, 11, 11, 11, 11, 11, 11 };
const int win_opened_pins[window_count]    = { 12, 12, 12, 12, 12, 12, 12, 12 };
const int win_closed_pins[window_count]    = { 13, 13, 13, 13, 13, 13, 13, 13 };
int win_status[window_count]               = {  0, 0, 0, 0, 0, 0, 0, 0 };
unsigned long win_started_at[window_count] = {  0, 0, 0, 0, 0, 0, 0, 0 };
*/
const unsigned long win_max_running_time = 30000; // 30s

// ****************************************************************************
// MESSAGE buffer used to delay writing to serial, which is slow

String messages[window_count];
String commande;
const String NULL_STRING = String("NULL_STRING");

// ****************************************************************************
// TEMPERATURE

const int SENSOR_PIN = A0;
const int SENSOR_TYPE = DHT22;
DHT dht(SENSOR_PIN, SENSOR_TYPE);

const float temperature_threshold = 25.0;
const float threshold_margin = 0.5;
const float high_temperature_threshold = temperature_threshold + threshold_margin;
const float low_temperature_threshold = temperature_threshold - threshold_margin;

unsigned long temperature_last_read_at = 0;
const int temperature_read_interval = 2000;

// ****************************************************************************
// BOARD LED

#define BOARD_LED_INIT digitalWrite(13, LOW); pinMode(13, OUTPUT);
#define BOARD_LED_ON digitalWrite(13, HIGH);
#define BOARD_LED_OFF digitalWrite(13, LOW);

// ****************************************************************************
void setup() {

  BOARD_LED_INIT
  Serial.begin(9600);
  Serial.flush();
  dht.begin();

  FOR_EACH_WINDOW {
    digitalWrite(win_direction_pins[i], LOW);
    digitalWrite(win_action_pins[i], LOW);
    pinMode(win_direction_pins[i], OUTPUT);
    pinMode(win_action_pins[i], OUTPUT);
    pinMode(win_opened_pins[i], INPUT_PULLUP);
    pinMode(win_closed_pins[i], INPUT_PULLUP);
    win_status[i] = 0;
  }
}

// ****************************************************************************
void loop() {


    
  // --------------------------------------------------------------------------
  // Stop motors after timeout

  FOR_EACH_WINDOW {
    messages[i] = NULL_STRING;
  }
  FOR_EACH_WINDOW {
    if (win_status[i] != 0 &&
        digitalRead(win_action_pins[i]) == HIGH &&
        win_started_at[i] + win_max_running_time <= millis()) {
      messages[i] = "msg : Window " + String(i+1) + " timeout --> stopping motor.";
      digitalWrite(win_action_pins[i], LOW);
      digitalWrite(win_direction_pins[i], LOW);
    }
  }
  FOR_EACH_WINDOW {
    if (messages[i] != NULL_STRING) {
      Serial.println(messages[i]);
    }
  }

  // --------------------------------------------------------------------------
  // Stop motors when windows are fully opened

  FOR_EACH_WINDOW {
    messages[i] = NULL_STRING;
  }
  FOR_EACH_WINDOW {
    if (win_status[i] == 1 &&
        digitalRead(win_action_pins[i]) == HIGH &&
        digitalRead(win_direction_pins[i]) == LOW &&
        digitalRead(win_opened_pins[i]) == LOW) {
      messages[i] = "msg : Window " + String(i+1) + " fully opened --> stopping motor.";
      digitalWrite(win_action_pins[i], LOW);
      digitalWrite(win_direction_pins[i], LOW);
    }
  }
  FOR_EACH_WINDOW {
    if (messages[i] != NULL_STRING) {
      Serial.println(messages[i]);
    }
  }

  // --------------------------------------------------------------------------
  // Stop motors when windows are fully closed

  FOR_EACH_WINDOW {
    messages[i] = NULL_STRING;
  }
  FOR_EACH_WINDOW {
    if (win_status[i] == -1 &&
        digitalRead(win_action_pins[i]) == HIGH &&
        digitalRead(win_direction_pins[i]) == HIGH &&
        digitalRead(win_closed_pins[i]) == LOW) {
      messages[i] = "msg : Window " + String(i+1) + " fully closed --> stopping motor.";
      digitalWrite(win_action_pins[i], LOW);
      digitalWrite(win_direction_pins[i], LOW);
    }
  }
  FOR_EACH_WINDOW {
    if (messages[i] != NULL_STRING) {
      Serial.println(messages[i]);
    }
  }

  // --------------------------------------------------------------------------
  // Read temperature at fixed interval

  unsigned long current_timestamp = millis();
  if (temperature_last_read_at + temperature_read_interval > current_timestamp) {
    return;
  }
  float current_temperature = dht.readTemperature();
  float current_humidity = dht.readHumidity();

  //float current_temperature = 23;
  //float current_humidity = 75;  

  temperature_last_read_at = current_timestamp;
  if (isnan(current_temperature)) {
    Serial.println("msg : Failed to read from DHT sensor!");
    return;
  }

  if(Serial.available()){
      commande = Serial.readStringUntil('\n'); 
      Serial.println("Commande : " + commande);
  }
  
  if (commande.equals("test")) {
    Serial.println("Test ok !!");
  }
  
  if (commande.equals("ouvert")) {
    Serial.println("Ouverture");
    current_temperature = 26;
  }
  
  if (commande.equals("ferme")) {
    Serial.println("Fermeture");
    current_temperature = 24;
  } 
  
  Serial.print("state : " + String(current_temperature) + " " + String(current_humidity));
  FOR_EACH_WINDOW {
      Serial.print(" "+ String(i+1)+":"+String(digitalRead(win_action_pins[i]))+String(digitalRead(win_direction_pins[i]))+String(digitalRead(win_opened_pins[i]))+String(digitalRead(win_closed_pins[i])));
  }

  Serial.print("\n");



  // --------------------------------------------------------------------------
  // Open windows when temperature is high

  if (current_temperature > high_temperature_threshold) {
    BOARD_LED_ON
    FOR_EACH_WINDOW {
      messages[i] = NULL_STRING;
    }
    FOR_EACH_WINDOW {
      if (win_status[i] != 1 && digitalRead(win_opened_pins[i]) != LOW) {
        win_status[i] = 1;
        messages[i] = "msg : OPENING window " + String(i+1);
        digitalWrite(win_direction_pins[i], LOW);
        digitalWrite(win_action_pins[i], HIGH);
        win_started_at[i] = millis();
        delay(500);
      }
    }
    FOR_EACH_WINDOW {
      if (messages[i] != NULL_STRING) {
        Serial.println(messages[i]);
      }
    }
    return;
  }

  // --------------------------------------------------------------------------
  // Close windows when temperature is low

  if (current_temperature < low_temperature_threshold) {
    BOARD_LED_OFF
    FOR_EACH_WINDOW {
      messages[i] = NULL_STRING;
    }
    FOR_EACH_WINDOW {
      if (win_status[i] != -1 && digitalRead(win_closed_pins[i]) != LOW) {
        win_status[i] = -1;
        messages[i] = "msg : CLOSING window " + String(i+1);
        digitalWrite(win_direction_pins[i], HIGH);
        digitalWrite(win_action_pins[i], HIGH);
        win_started_at[i] = millis();
        delay(500);
      }
    }
    FOR_EACH_WINDOW {
      if (messages[i] != NULL_STRING) {
        Serial.println(messages[i]);
      }
    }
    return;
  }

  // --------------------------------------------------------------------------
  // Stop windows otherwise

  BOARD_LED_OFF
  FOR_EACH_WINDOW {
    messages[i] = NULL_STRING;
  }
  FOR_EACH_WINDOW {
    if (win_status[i] != 0) {
      win_status[i] = 0;
      messages[i] = "msg : STOPPING window " + String(i+1);
      digitalWrite(win_action_pins[i], LOW);
      digitalWrite(win_direction_pins[i], LOW);
    }
  }
  FOR_EACH_WINDOW {
    if (messages[i] != NULL_STRING) {
      Serial.println(messages[i]);
    }
  }
  return;
}
