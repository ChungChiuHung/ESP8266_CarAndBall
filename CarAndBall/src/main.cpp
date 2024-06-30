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
// Motor control pins
const int PWMA=D1;//Right side speed control
const int PWMB=D2;//Left side speed control
const int DA=D3;//Right direction control 
const int DB=D4;//Left direction control

// Replace with your network credentials
const char* ssid = "playground_luo";
const char* password = "luckyhousepro";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Set your static IP address
IPAddress staticIP(192,168,0,103);
// Set your Gateway IP address
IPAddress gateway(192,168,0,1);
IPAddress subnet(255,255,255,0);



// Timing and state management
unsigned long previousMillis = 0;
unsigned long lastOnTime = 0; // Last time the "on" command was received
unsigned long delayStartTime = 0; // Start time for delay
const unsigned long delayDuration = 100; // Duration for delay when changing direction
unsigned long interval = 8000; // Interval between actions within go()
const unsigned long timeoutDuration  = 180000; //30 seconds timeout
enum State {WAIT_FOR_TIME, CHANGE_STATE, BOTH_MOTORS_SAME_DIRECTION, APPLY_DELAY, CHECK_NEXT};
State goState = WAIT_FOR_TIME;
int step = 0; // Track the current step

// System state
bool motorState = false;

// Last direction states
int lastDirectionA = -1;
int lastDirectionB = -1;

// Last active motor
int lastActiveMotor = 0; // 0 for none, 1 for A, 2 for B

int generateRandomSpeed(){
  return 500;
}

void stopMotors(){
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
  digitalWrite(DA, LOW);
  digitalWrite(DB, LOW);
}

void setup(void) {
  Serial.begin(115200);
  Serial.println("Starting device...");

  // Configure static IP
  WiFi.config(staticIP, gateway, subnet);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP Local IP Address
  Serial.println("Connected to WiFi");
  Serial.println("IP address: " + WiFi.localIP().toString());

  // Define server routes
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "hello from esp8266!\r\n");
  });

  server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request){
    if(!motorState){// Only update if the motor was off
      motorState = true;
      lastOnTime = millis(); // Reset the last "on" time
    }
    request->send(200, "text/plain", "Motor is on and the process restarted");
  });

  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request){
    motorState = false;
    stopMotors();
    request->send(200, "text/plain", "Motor is off");
  });

  // Start server
  server.begin();
  Serial.println("HTTP server started");

  pinMode(PWMA, OUTPUT); 
  pinMode(PWMB, OUTPUT); 
  pinMode(DA, OUTPUT); 
  pinMode(DB, OUTPUT); 
    Serial.println("Motor SHield 12E Initialized");
}


void activateMotor(int pwmPin, int dirPin, int newDirection, int* lastDirection){
  // First, ensure the other motor is turned off
  if(pwmPin == PWMA){
    // Stop Motor B before starting motor A
    analogWrite(PWMB, 0);
    digitalWrite(DB, LOW);
  }else{
    // Stop motor A before starting Motor B
    analogWrite(PWMA, 0);
    digitalWrite(DA, LOW);
  }

  int randomSpeed = generateRandomSpeed();

  // Check if the direction needs to change with delay
  if(newDirection != *lastDirection){
    delayStartTime = millis();
    goState = APPLY_DELAY;
  }else{
    analogWrite(pwmPin, randomSpeed);
    digitalWrite(dirPin, newDirection);
    *lastDirection = newDirection;
    goState = CHECK_NEXT;
  }
}

void applyDelayedChange(){
  int randomSpeed = generateRandomSpeed();

  if (lastActiveMotor == 1){
    // Stop Motor B
    analogWrite(PWMB, 0);
    digitalWrite(DB, LOW);

    // Start Motor A with new settings
    analogWrite(PWMA, randomSpeed);
    digitalWrite(DA, !lastDirectionA);
    lastDirectionA = !lastDirectionA;
  } else{
    // Stop Motor A
    analogWrite(PWMA, 0);
    digitalWrite(DA, LOW);

    // Start Motor B with new settings
    analogWrite(PWMB, randomSpeed);
    digitalWrite(DB, !lastDirectionB);
    lastDirectionB = !lastDirectionB;
  }
  goState = CHECK_NEXT;
}

void go() {
  unsigned long currentMillis = millis();
  switch (goState)
  {
    case WAIT_FOR_TIME:
      if(currentMillis - previousMillis >= interval){
        previousMillis = currentMillis;
        goState = CHANGE_STATE;
      }
      break;
    case CHANGE_STATE:
    {
      // Randomly choose a motor, ensure it's not the same as last time
      int selectedMotor = (lastActiveMotor == 1) ? 2:1;
      int newDirection = random(2);
      if(selectedMotor == 1)
      {
        activateMotor(PWMA, DA, newDirection, &lastDirectionA);
        lastActiveMotor = 1;
      } else{
        activateMotor(PWMB, DB, newDirection, &lastDirectionB);
        lastActiveMotor = 2;
      }
      break;
    }
    case BOTH_MOTORS_SAME_DIRECTION:
    {
        int direction = random(2); // Same direction for both motors
        //int speed = generateRandomSpeed();
        activateMotor(PWMA, DA, direction, &lastDirectionA);
        activateMotor(PWMB, DB, direction, &lastDirectionB);
        goState = CHECK_NEXT;
        break;
    }
    case APPLY_DELAY:
      if(currentMillis - delayStartTime >= delayDuration){
        // Delay completed, perform the state change
        applyDelayedChange();
      }
      break;
    case CHECK_NEXT:
      goState = WAIT_FOR_TIME;
      break;
    default:
      Serial.println("Unhandled state in go()");
      break;
  }
}



void loop() {
  unsigned long currentMillis = millis();

  if (motorState)
  {
    go();
    if(currentMillis - lastOnTime > timeoutDuration){
      motorState = false;
      stopMotors();
      Serial.println("Motor stopped due to timeout");
    }
  }
  else{
    stopMotors();
  }
}
