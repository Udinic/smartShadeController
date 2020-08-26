#include <ESP32Servo.h>
#include <Adafruit_PN532.h>

#define PN532_IRQ   (2)
#define PN532_RESET (3)  // Not connected by default on the NFC Shield

Servo myServo;  // create servo object to control a servo

const uint8_t PIN_SERVO = 23;
const uint8_t PIN_UP_BTN = 5;
const uint8_t PIN_DOWN_BTN = 18;

bool moving = false;
bool listeningToNFC = false;

uint8_t btnDownCurr;
uint8_t btnDownPrev;

uint8_t btnUpCurr;
uint8_t btnUpPrev;

uint8_t irqCurr;
uint8_t irqPrev;


// Init the object that controls the PN532 NFC chip
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);


void setup() {
  Serial.begin(115200);

  Serial.println("");
  Serial.println("");
  Serial.println("");
  Serial.println("***************************************");
  Serial.println("  Balcony motorized shades circuit");
  Serial.println("");
  Serial.println("  Written by: Udi Cohen");
  Serial.println("***************************************");

  Serial.println("Initializing NFC chip..");
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }

  // configure board to read RFID tags
  nfc.SAMConfig();

  // Setting the interrupt to be triggered from the IRQ pin.
  pinMode(PN532_IRQ, INPUT_PULLUP);

  //Setting up the buttong
  pinMode(PIN_UP_BTN, INPUT_PULLUP);
  btnUpPrev = digitalRead(PIN_UP_BTN);
  
  pinMode(PIN_DOWN_BTN, INPUT_PULLUP);
  btnDownPrev = digitalRead(PIN_DOWN_BTN);
    
  Serial.println("Started listening to input..");
}

void startListeningToNFC() {
  listeningToNFC = true;
  irqCurr = digitalRead(PN532_IRQ);
  irqPrev = irqCurr;
  
  Serial.println("START listening to NFC tags..");
  nfc.startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A);
}

void stopListeningToNFC() {
  listeningToNFC = false;
  Serial.println("STOP listening to NFC tags..");
  
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

void printShitAboutCard(uint8_t uid[], uint8_t uidLength) {
    Serial.println("***********************");
    Serial.println("Found an ISO14443A card !!");
    Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    
    if (uidLength == 4)
    {
      // We probably have a Mifare Classic card ... 
      uint32_t cardid = uid[0];
      cardid <<= 8;
      cardid |= uid[1];
      cardid <<= 8;
      cardid |= uid[2];  
      cardid <<= 8;
      cardid |= uid[3]; 
      Serial.print("Seems to be a Mifare Classic card #");
      Serial.println(cardid);
    }
    Serial.println("");
    Serial.println("***********************");
}

void loop() {
  btnUpCurr = digitalRead(PIN_UP_BTN);
  btnDownCurr = digitalRead(PIN_DOWN_BTN);
  irqCurr = digitalRead(PN532_IRQ);
  
  if (btnUpCurr == LOW && btnUpPrev == HIGH) {
    Serial.println("Pressed UP button");
    if (moving) {
      stopRotating();
      moving = false;
      stopListeningToNFC();
    } else {
      rotateCounterClockwise();
      moving = true;
      startListeningToNFC();
    }
  }  

  if (btnDownCurr == LOW && btnDownPrev == HIGH) {
    Serial.println("Pressed DOWN button");
    if (moving) {
      stopRotating();
      moving = false;
      stopListeningToNFC();
    } else {
      rotateClockwise();
      moving = true;
      startListeningToNFC();
    }
  }

  if (listeningToNFC && irqCurr == LOW && irqPrev == HIGH) {
      
    Serial.println("**********");
    Serial.println("Got NFC IRQ");  
    Serial.println("**********");

    uint8_t success = false;
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
    uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

    // read the NFC tag's info
    success = nfc.readDetectedPassiveTargetID(uid, &uidLength);
    Serial.println(success ? "Read successful" : "Read failed (not a card?)");

    if (success) {
      printShitAboutCard(uid, uidLength);
    }

    delay(500);
    Serial.println("Start listening for cards again");
    nfc.startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A);
  }

  irqPrev = irqCurr;
  btnUpPrev = btnUpCurr;
  btnDownPrev = btnDownCurr;
}
