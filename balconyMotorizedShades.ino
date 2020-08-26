#include <ESP32Servo.h>

Servo myServo;  // create servo object to control a servo

uint8_t servoPin = 23;
uint8_t redBtnPin = 5;
uint8_t greenBtnPin  = 18;

bool moving = false;

int greenPresses = 0;
int redPresses = 0;

uint8_t btnPrevGreen;
uint8_t btnPrevRed;

void setup() {
  Serial.begin(9600);
  
  pinMode(redBtnPin, INPUT_PULLUP);
  btnPrevRed = digitalRead(redBtnPin);
//  digitalWrite(redBtnPin, HIGH);
  
  pinMode(greenBtnPin, INPUT_PULLUP);
  btnPrevGreen = digitalRead(greenBtnPin);
//  digitalWrite(greenBtnPin, HIGH);
    
  Serial.println("Servo udinic starting");

  myServo.attach(servoPin);
  Serial.println("Servo attached");
}

void loop() {
  if (digitalRead(redBtnPin) == LOW && btnPrevRed == HIGH) {
      redPresses++;
      if (moving) {
        Serial.println("Stopping..");
        myServo.detach();
        moving = false;
      } else {
        Serial.print("[");
        Serial.print(redPresses);
        Serial.print("]");
        Serial.println("Pressing red button");
        myServo.attach(servoPin);
        myServo.writeMicroseconds(2100);   
        moving = true;   
      }
  }  

  if (digitalRead(greenBtnPin) == LOW && btnPrevGreen == HIGH) {
      greenPresses++;
      if (moving) {
        Serial.println("Stopping..");
        myServo.detach();
        moving = false;
      } else {
        Serial.print("[");
        Serial.print(greenPresses);
        Serial.print("]");
        Serial.println("Pressing green button");
        myServo.attach(servoPin);
        myServo.writeMicroseconds(850);  
        moving = true; 
      }
  }

  btnPrevRed = digitalRead(redBtnPin);
  btnPrevGreen = digitalRead(greenBtnPin);
}
