#include <ESP32Servo.h>

Servo myServo;  // create servo object to control a servo

const uint8_t PIN_SERVO = 23;
const uint8_t PIN_UP_BTN = 5;
const uint8_t PIN_DOWN_BTN = 18;

bool moving = false;

uint8_t btnDownCurr;
uint8_t btnDownPrev;

uint8_t btnUpCurr;
uint8_t btnUpPrev;


void setup() {
  Serial.begin(9600);

  Serial.println("");
  Serial.println("");
  Serial.println("");
  Serial.println("***************************************");
  Serial.println("  Balcony motorized shades circuit");
  Serial.println("");
  Serial.println("  Written by: Udi Cohen");
  Serial.println("***************************************");

  pinMode(PIN_UP_BTN, INPUT_PULLUP);
  btnUpPrev = digitalRead(PIN_UP_BTN);
  
  pinMode(PIN_DOWN_BTN, INPUT_PULLUP);
  btnDownPrev = digitalRead(PIN_DOWN_BTN);
    
  Serial.println("Started listening to input..");
}

void rotateClockwise() {
  myServo.attach(PIN_SERVO);
  myServo.writeMicroseconds(850);  
}

void rotateCounterClockwise() {
  myServo.attach(PIN_SERVO);
  myServo.writeMicroseconds(2100);  
}

void stopRotating() {
  Serial.println("Stopping..");
  myServo.detach();
}


void loop() {
  btnUpCurr = digitalRead(PIN_UP_BTN);
  btnDownCurr = digitalRead(PIN_DOWN_BTN);
  
  if (btnUpCurr == LOW && btnUpPrev == HIGH) {
    Serial.println("Pressed UP button");
    if (moving) {
      stopRotating();
      moving = false;
    } else {
      rotateCounterClockwise();
      moving = true;
    }
  }  

  if (btnDownCurr == LOW && btnDownPrev == HIGH) {
    Serial.println("Pressed DOWN button");
    if (moving) {
      stopRotating();
      moving = false;
    } else {
      rotateClockwise();
      moving = true;
    }
  }

  btnUpPrev = btnUpCurr;
  btnDownPrev = btnDownCurr;
}
