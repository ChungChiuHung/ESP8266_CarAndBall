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
const char* ssid = "Chung-WiFi";
const char* password = "037620929";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Set your static IP address
IPAddress staticIP(192,168,50,200);
// Set your Gateway IP address
IPAddress gateway(192,168,50,1);
IPAddress subnet(255,255,255,0);



// Timing and state management
unsigned long previousMillis = 0;
unsigned long lastOnTime = 0; // Last time the "on" command was received
unsigned long delayStartTime = 0; // Start time for delay
const unsigned long delayDuration = 100; // Duration for delay when changing direction
unsigned long interval = 8000; // Interval between actions within go()
const unsigned long timeoutDuration  = 180000; //30 seconds timeout

// System state
bool motorState = false;

enum MotionMode { MOVING_SAME_DIRECTION, 
                  MOVING_DIFF_DIRECTION,
                  MOVING_STOP_FOR_FEW_SEC,
                  MOVING_ONE_MOTOR_RANDOMLY};

MotionMode currentMode = MOVING_SAME_DIRECTION;

int speedA, speedB;
bool directionA, directionB;
int activeMotor;

int generateRandomSpeed(){
  return random(100, 256); // Generate a random speed between 200, 500 
}

bool generateRandomDirection(){
  return random(0, 2); // Randomly generate 0 or 1
}

void stopMotors(){
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
  digitalWrite(DA, LOW);
  digitalWrite(DB, LOW);
}

void activateMotors(int speedA, bool directionA, int speedB, bool directionB)
{
  analogWrite(PWMA, speedA);
  analogWrite(PWMB, speedB);
  digitalWrite(DA, directionA ? HIGH : LOW);
  digitalWrite(DB, directionB ? HIGH : LOW);
}

void activateSingleMotor(int motor, int speed, bool direction){
  if(motor == 0){ // Right motor
    analogWrite(PWMA, speed);
    digitalWrite(DA, direction ? HIGH : LOW);
    analogWrite(PWMB, 0);
    digitalWrite(DB, LOW);
  }else { // Left motor
    analogWrite(PWMB, speed);
    digitalWrite(DB, direction ? HIGH : LOW);
    analogWrite(PWMA, 0);
    digitalWrite(DA, LOW);
  }
}

MotionMode selectNextMode(){
  int randValue = random(0, 10); // Adjust the range to set the weight
  if(randValue<2){
    return MOVING_SAME_DIRECTION;
  } else if (randValue < 4){
    return MOVING_DIFF_DIRECTION;
  } else if (randValue < 6){
    return MOVING_STOP_FOR_FEW_SEC;
  } else {
    return MOVING_ONE_MOTOR_RANDOMLY;
  }
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

void go() {
  unsigned long currentMillis = millis();
  static unsigned long modeStartTime = 0;
  static unsigned long modeDuration = 0;

  // Check if the motors have been running for more than timeoutDuration
  if (currentMillis - lastOnTime >= timeoutDuration){
    motorState = false;
    stopMotors();
    Serial.println("Motor stopped due to timeout");
    return;
  }

  if(currentMillis - modeStartTime >= modeDuration){
    modeStartTime = currentMillis;
    modeDuration = random(2000, interval); // Random duration for current mode
    currentMode = selectNextMode(); // Randomly select next mode with weighted probability

    // Generate random values for speed and direction based on the mode
    switch (currentMode) {
      case MOVING_SAME_DIRECTION:
        {
        directionA = directionB = generateRandomDirection();
        speedA = 255;
        speedB = 255;
        modeDuration = random(2000,5000); // Force modeDuration to 1~2 seconds
        break;
        }
      case MOVING_DIFF_DIRECTION:
        directionA = generateRandomDirection();
        directionB = generateRandomDirection();
        speedA = generateRandomSpeed();
        speedB = generateRandomSpeed();
        modeDuration = random(1000,2000); // Force modeDuration to 1~2 seconds
        break;
      case MOVING_STOP_FOR_FEW_SEC:
        stopMotors();
        modeDuration = random(1000,1500); // Force modeDuration to 1.5 seconds
        break;
      case MOVING_ONE_MOTOR_RANDOMLY:
        activeMotor = random(0,2); // Randomly select motor 0 or 1
        if (activeMotor == 0){ // Right motor
          speedA = random(220,256);
          directionA = generateRandomDirection();
        }else { // Left motor
          speedB = random(220,256);
          directionB = generateRandomDirection();
        }
        modeDuration = random(5000,8000);
        break;
    }
  }

  // Perform actions based on the current mode
  switch (currentMode){
    case MOVING_SAME_DIRECTION:
      activateMotors(speedA, directionA, speedB, directionB);
      break;
    case MOVING_DIFF_DIRECTION:
      activateMotors(speedA, directionA, speedB, directionB);
      break;
    case MOVING_STOP_FOR_FEW_SEC:
      stopMotors();
      break;
    case MOVING_ONE_MOTOR_RANDOMLY:
      if (activeMotor == 0){ //Right motor
        activateSingleMotor(0, speedA, directionA);
      } else {
        activateSingleMotor(1, speedB, directionB);
      }
      break;
  }

}

void loop() {
  if(motorState){
    go();
  }
}
