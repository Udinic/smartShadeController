#include <ESP32Servo.h>
#include <Adafruit_PN532.h>

#define PN532_IRQ   (2)
#define PN532_RESET (14)  // Not connected by default on the NFC Shield

Servo myServo;  // create servo object to control a servo

const boolean NFC_DISABLED = true;

const uint8_t PIN_SERVO = 13;
const uint8_t PIN_UP_BTN = 5;
const uint8_t PIN_DOWN_BTN = 15;

const uint32_t CARDID_TOP = 3952824665;
const uint32_t CARDID_BOTTOM = 3591963054;

bool movingUp = false;
bool movingDown = false;
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

  if (!NFC_DISABLED) {
    Serial.println("Initializing NFC chip...");
    nfc.begin();
  
    uint32_t versiondata = nfc.getFirmwareVersion();
    if (! versiondata) {
      Serial.print("Didn't find PN53x board");
      while (1); // halt
    }
  
    // configure board to read RFID tags
    nfc.SAMConfig();
  
    // Setting the NFC IRQ pin.
    pinMode(PN532_IRQ, INPUT_PULLUP);
  }

  //Setting up the buttong
  pinMode(PIN_UP_BTN, INPUT_PULLUP);
  btnUpPrev = digitalRead(PIN_UP_BTN);
  
  pinMode(PIN_DOWN_BTN, INPUT_PULLUP);
  btnDownPrev = digitalRead(PIN_DOWN_BTN);
    
  Serial.println("Started listening to input..");

}

void startListeningToNFC() {
  if (NFC_DISABLED) {
    return;
  }
  listeningToNFC = true;
  irqCurr = digitalRead(PN532_IRQ);
  irqPrev = irqCurr;
  
  Serial.println("START listening to NFC tags..");
  nfc.startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A);
}

void stopListeningToNFC() {
  if (NFC_DISABLED) {
    return;
  }
  listeningToNFC = false;
  Serial.println("STOP listening to NFC tags..");
  digitalWrite(PN532_RESET, HIGH);
  
}

void rotateClockwise() {
  myServo.attach(PIN_SERVO);
  myServo.writeMicroseconds(850);  
  movingDown = true;
  movingUp = false;
}

void rotateCounterClockwise() {
  myServo.attach(PIN_SERVO);
  myServo.writeMicroseconds(2100);  
  movingUp = true;
  movingDown = false;
}

void stopRotating() {
  Serial.println("Stopping..");
  movingUp = false;
  movingDown = false;
  myServo.detach();
}

uint32_t getCardId(uint8_t uid[], uint8_t uidLength) {
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
      return cardid;
    } else {
      return -1;
    }
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
    if (movingUp || movingDown) {
      stopRotating();
      stopListeningToNFC();
    } else {
      rotateCounterClockwise();
      startListeningToNFC();
    }
  }  

  if (btnDownCurr == LOW && btnDownPrev == HIGH) {
    Serial.println("Pressed DOWN button");
    if (movingUp || movingDown) {
      stopRotating();
      stopListeningToNFC();
    } else {
      rotateClockwise();
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
      uint32_t cardId = getCardId(uid, uidLength);
      if (cardId == CARDID_TOP) {
        if (movingDown) {
          Serial.print("FOUND TOP CARD while moving down! Stopping... cardId["); Serial.print(cardId); Serial.println("].");
          stopRotating();
          stopListeningToNFC();
        } else if (movingUp) {
          Serial.print("Found top card while moving up, ignoring.. cardId["); Serial.print(cardId); Serial.println("].");          
        } else {
          Serial.print("Found top card while NOT MOVING, ignoring.. cardId["); Serial.print(cardId); Serial.println("].");          
        }
      } else if (cardId == CARDID_BOTTOM) {
        if (movingUp) {
          Serial.print("FOUND BOTTOM CARD while moving up! Stopping... cardId["); Serial.print(cardId); Serial.println("].");
          stopRotating();
          stopListeningToNFC();
        } else if (movingDown) {
          Serial.print("Found bottom card while moving down, ignoring.. cardId["); Serial.print(cardId); Serial.println("].");          
        } else {
          Serial.print("Found bottom card while NOT MOVING, ignoring.. cardId["); Serial.print(cardId); Serial.println("].");          
        }        
      } else {
        Serial.print("Found unidentified card["); Serial.print(cardId); Serial.println("]. Ignoring..");
      }
    }

    if (listeningToNFC) {
      delay(500);
      Serial.println("Start listening for cards again");
      nfc.startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A);
    }
  } else if (irqCurr == LOW && irqPrev == HIGH) {
    Serial.println("##### Got IRQ while not listening..");  
  }

  irqPrev = irqCurr;
  btnUpPrev = btnUpCurr;
  btnDownPrev = btnDownCurr;
}
