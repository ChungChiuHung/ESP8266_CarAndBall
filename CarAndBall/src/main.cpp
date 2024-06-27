#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
/*
 		Board pin | NodeMCU GPIO | 	Arduino IDE
 					A- 										1 												5 or D1
 					A+ 										3 												0 or D3
 					B- 										2 												4 or D2
 					B+ 										4 												2 or D4
*/

#ifndef STASSID
#define STASSID "playground_luo"
#define STAPSK "luckyhousepro"
#endif


// Replace with your network credentials
const char* ssid = "playground_luo";
const char* password = "luckyhousepro";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);


bool state = 0;

const int PWMA=D1;//Right side 
const int PWMB=D2;//Left side 
const int DA=D3;//Right reverse 
const int DB=D4;//Left reverse 

unsigned long previousMillis = 0;
unsigned long interval = 0; // Interval for each step (in milliseconds)
int step = 0; // Track the current step

// Transition delay in milliseconds
const unsigned long transitionDelay = 500;

// Global variables to store the last state change times
unsigned long lastChangedTimeD1 = 0;
unsigned long lastChangedTimeD2 = 0;
unsigned long lastChangedTimeD3 = 0;
unsigned long lastChangedTimeD4 = 0;

void go() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // Update previousMillis for the next step
    previousMillis = currentMillis;

    // Set random interval
    interval = random(1000, 3000); // Random delay between 1 to 5 seconds

    // Set random movement
    int randomMovement = random(0, 3); // Generate a random number between 0 to 5

    // Set random speed
    // int randomSpeed = random(250, 250); // Random speed between 50% to 100%
    int randomSpeed = 500;

    // Apply PWM by Software for speed control
    analogWrite(PWMA, randomSpeed); // Apply PWM to simulate speed control
    analogWrite(PWMB, randomSpeed); // Apply PWM to simulate speed control

    // Execute the random movement
    switch (randomMovement) {
      case 0:
        controlledDigitalWrite(DA, LOW, transitionDelay);
        controlledDigitalWrite(DB, LOW, transitionDelay);
        break;
      case 1:
        controlledDigitalWrite(DA, HIGH, transitionDelay);
        controlledDigitalWrite(DB, LOW, transitionDelay);
        break;
      case 2:
        controlledDigitalWrite(DA, LOW, transitionDelay);
        controlledDigitalWrite(DB, HIGH, transitionDelay);
        break;
    }
  }
}


void stop(){
  digitalWrite(PWMA, 0); 
  digitalWrite(DA, LOW); 
     
  digitalWrite(PWMB, 0); 
  digitalWrite(DB, LOW);
}

void controlledDigitalWrite(int pin, int value, unsigned long delayTime)
{
  static int lastStateD1 = -1;
  static int lastStateD2 = -1;
  static int lastStateD3 = -1;
  static int lastStateD4 = -1;

  unsigned long* lastChangeTimePtr = nullptr;
  int* lastStatePtr = nullptr;

  // Assign pointers based on pin number
  switch (pin)
  {
  case D1:
    lastChangeTimePtr = &lastChangedTimeD1;
    lastStatePtr = &lastStateD1;
    break;
  case D2:
    lastChangeTimePtr = &lastChangedTimeD2;
    lastStatePtr = &lastStateD2;
    break;
  case D3:
    lastChangeTimePtr = &lastChangedTimeD3;
    lastStatePtr = &lastStateD3;
  case D4:
    lastChangeTimePtr = &lastChangedTimeD4;
    lastStatePtr = &lastStateD4;
    break;
  }
  // Check if the required delay time has passed
  if (lastStatePtr && *lastStatePtr != value && (millis() - *lastChangeTimePtr >= delayTime))
  {
    digitalWrite(pin, value); // Change the pin state
    *lastChangeTimePtr = millis();
    *lastStatePtr = value; // update the last state
  }
}


void setup(void) {
  Serial.begin(115200);
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", "hello from esp8266!\r\n");
  });

  server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request){
    state = 1;
    request->send_P(200, "text/plain", "on");
  });

  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request){
    state = 0;
    request->send_P(200, "text/plain", "off");
  });

  // Start server
  server.begin();

  pinMode(PWMA, OUTPUT); 
  pinMode(PWMB, OUTPUT); 
  pinMode(DA, OUTPUT); 
  pinMode(DB, OUTPUT); 
 	Serial.println("Motor SHield 12E Initialized");
}

void loop() {
  switch(state){
    case 0:
    stop();
    break;
    case 1:
    go();
    break;
    }
}
