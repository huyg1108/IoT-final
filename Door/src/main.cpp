#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// button pin and parameters
int open_door_pin = 12;
int requestOpenState = LOW;
int lastButtonState = LOW; 
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// ultrasonic
int trig_pin = 22;
int echo_pin = 23;

// buzzer and led pin
int buz_pin = 25;
int g_led_pin = 5;
int r_led_pin = 18;
int warn_led_pin = 19;

// relay
int relay_pin = 14;

// parameters for security
int thresholdSeconds = 10;
int spamDurationSeconds = 5;
int thresholdDistance = 10;
int cameraResetTime = 10;
static unsigned long intrusionDuration = 0;
bool intrusionPublished = false;

// state of the door
int openState = LOW; 
int detectCheck = HIGH;

const char* ssid = "Wokwi-GUEST";
const char* password = "";

// const char* mqttServer = "broker.emqx.io"; 
const char* mqttServer = "broker.hivemq.com"; 
int port = 1883;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

void wifiConnect() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) 
{
  String payloadStr = "";

  for (int i = 0; i < length; i++) {
    payloadStr += (char)payload[i];
  }
  if (strcmp(topic, "/TimeThreshold") == 0) {
    thresholdSeconds = payloadStr.toInt();
    Serial.print("Threshold Seconds: ");
    Serial.println(thresholdSeconds);
  }
  else if (strcmp(topic, "/DistanceThreshold") == 0) {
    thresholdDistance = payloadStr.toInt();
    Serial.print("Threshold Distance: ");
    Serial.println(thresholdDistance);
  }
  else if (strcmp(topic, "/TimeStopSpam") == 0) {
    spamDurationSeconds = payloadStr.toInt();
    Serial.print("Spam Duration Seconds: ");
    Serial.println(spamDurationSeconds);
  }
  else if (strcmp(topic, "/CameraResetTime") == 0) {
    cameraResetTime = payloadStr.toInt();
    Serial.print("Camera Reset Time: ");
    Serial.println(cameraResetTime);
  }
  else if (strcmp(topic, "/DoorOpen") == 0) {
    if (payloadStr != "Stranger" && payloadStr != "") {
      openState = HIGH;
    }
    else {
      detectCheck = LOW;
    }
  }
}

void mqttConnect() 
{ 
  while(!mqttClient.connected()) 
  {
      Serial.println("Attempting MQTT connection...");

      if(mqttClient.connect("ESPClient")) 
      {
          Serial.println("Connected");
          mqttClient.subscribe("/TimeThreshold");
          mqttClient.subscribe("/DistanceThreshold");
          mqttClient.subscribe("/TimeStopSpam");
          mqttClient.subscribe("/DoorOpen");
          mqttClient.subscribe("/CameraResetTime");
      }
      else 
      {
          Serial.print("Failed, rc=");
          Serial.print(mqttClient.state());
          Serial.println("Try again in 5 seconds");
          delay(5000);
      }
    }
}


long getDist() {
  digitalWrite(trig_pin, LOW);
  delayMicroseconds(2);
  digitalWrite(trig_pin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig_pin, LOW);

  long duration = pulseIn(echo_pin, HIGH);
  long distCm = duration * 0.034 / 2;

  return distCm;
}

void mode1(int delay_time) {
  digitalWrite(relay_pin, HIGH);
  digitalWrite(g_led_pin, HIGH);
  tone(buz_pin, 800, 150);
  delay(200);
  tone(buz_pin, 1000, 150);
  delay(200);
  tone(buz_pin, 1200, 150);
  delay(200);
  delay(delay_time);
  delay(200);
  digitalWrite(relay_pin, LOW);
  digitalWrite(g_led_pin, LOW);
}

void mode2() {
  digitalWrite(r_led_pin, HIGH);
  tone(buz_pin, 400, 500);
  delay(500);
}

void mode3(int dist) {
  int blink_delay = map(dist, 0, 50, 50, 300);

  digitalWrite(warn_led_pin, HIGH);
  delay(blink_delay);
  digitalWrite(warn_led_pin, LOW);
  delay(blink_delay);

  tone(buz_pin, 1000);
  delay(blink_delay);
  noTone(buz_pin);
  delay(blink_delay);
}

void setup() {
  Serial.begin(115200);
  esp_log_level_set("ledc", ESP_LOG_NONE);

  // pinMode(button_pin, INPUT);
  pinMode(trig_pin, OUTPUT);
  pinMode(echo_pin, INPUT);
  pinMode(buz_pin, OUTPUT);
  pinMode(g_led_pin, OUTPUT);
  pinMode(r_led_pin, OUTPUT);
  pinMode(warn_led_pin, OUTPUT);
  pinMode(relay_pin, OUTPUT);
  digitalWrite(relay_pin, LOW);  // turn off relay initially
  Serial.println("Connecting to WiFi");
  wifiConnect();
  mqttClient.setServer(mqttServer, port);
  mqttClient.setCallback(callback);
  mqttClient.setKeepAlive(90);
}

void publishDoorOpenRequest() {
  String payload = "{\"action\": \"request_open\"}";
  
  if (mqttClient.publish("/DoorOpenRequest", payload.c_str())) {
    Serial.println("Published Door Open Request successfully!");
  } else {
    Serial.println("Failed to publish Door Open Request.");
  }
}

void publishIntruderTime(unsigned long intrusionTime) {
  String payload = "{\"duration\": " + String(intrusionTime) + "}";
  
  if (mqttClient.publish("/IntruderTime", payload.c_str())) {
    Serial.println("Published Intruder Time successfully!");
  } else {
    Serial.println("Failed to publish Intruder Time.");
  }
}

void loop() {
  if(!mqttClient.connected()) { 
    mqttConnect(); 
  }
  mqttClient.loop();  

  int reading = digitalRead(open_door_pin);
  int distCm = getDist();
  static unsigned long startTime = 0;
  static unsigned long spamEndTime = 0;
  static bool counting = false;
  static bool spamming = false;

  // for button
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != requestOpenState) {
      requestOpenState = reading;

      if (requestOpenState == LOW) {
        digitalWrite(g_led_pin, HIGH);
        
        tone(buz_pin, 800, 100);
        
        publishDoorOpenRequest();
        delay(100);
        
        digitalWrite(g_led_pin, LOW);
      }
    }
  }
  lastButtonState = reading;

  // control verify case
  if (openState == HIGH) {
    mode1(cameraResetTime* 1000);
    counting = false;
    spamming = false;
    openState = LOW;
  }
  else {
    if (distCm < thresholdDistance) {
        if (!counting) {
            startTime = millis();
            counting = true;
        } else {
            unsigned long elapsedTime = (millis() - startTime) / 1000;
            if (elapsedTime >= thresholdSeconds) {
                spamming = true;
                intrusionDuration = elapsedTime;
                spamEndTime = millis() + (spamDurationSeconds * 1000);
            }
        }
    } else {
        counting = false;
    }

    if (spamming) {
        if (millis() >= spamEndTime) {
            if (!intrusionPublished) {
                publishIntruderTime(intrusionDuration);
                intrusionPublished = true;
            }
            spamming = false;
        } else {
            
            mode3(distCm);
        }
    } else {
        intrusionPublished = false;
    }
  }

  // detect fail
  if (detectCheck == LOW) {
    mode2();
    detectCheck = HIGH;
    digitalWrite(r_led_pin, LOW);
  }
}