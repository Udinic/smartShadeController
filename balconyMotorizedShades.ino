#include <ESP32Servo.h>
#include <Adafruit_PN532.h>

#include "config.h"

// PIN definitions
#define PN532_IRQ   (2)
#define PN532_RESET (14)
const uint8_t PIN_SERVO = 13;
const uint8_t PIN_UP_BTN = 5;
const uint8_t PIN_DOWN_BTN = 15;

// Config
const boolean NFC_DISABLED = false;

// Supported NFC tags
const uint32_t CARDID_TOP = 3952824665;
const uint32_t CARDID_BOTTOM = 913822226;

// State
boolean connectingInProgress = false;
bool movingUp = false;
bool movingDown = false;
bool listeningToNFC = false;
uint8_t btnDownCurr;
uint8_t btnDownPrev;
uint8_t btnUpCurr;
uint8_t btnUpPrev;
uint8_t irqCurr;
uint8_t irqPrev;

// Init the MQTT feeds
AdafruitIO_Feed *balconyShade = io.feed("shade-open");
AdafruitIO_Feed *shadeLogs = io.feed("shade-logs");

// Init the object that controls the PN532 NFC chip
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

Servo myServo;

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

void setTargetShadeLevel(int percentOpen) {
  Serial.print("setTargetShadeLevel percentOpen[");Serial.print(percentOpen);Serial.println("]");
  if (percentOpen == 100) {
    rotateCounterClockwise();
  } else if (percentOpen == 0) {
    rotateClockwise();
  }
  startListeningToNFC();
}

void stopShade() {
  stopRotating();
  stopListeningToNFC();
}

int shadeLevel = 100;
void setShadeLevel(int percentOpen) {
  shadeLevel = percentOpen;
}

void handleNFCDetected() {
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
        // Shade is fully closed
        if (movingDown) {
          Serial.print("FOUND TOP CARD while moving down! Stopping... cardId["); Serial.print(cardId); Serial.println("].");
          stopShade();
          setShadeLevel(0);
        } else if (movingUp) {
          Serial.print("Found top card while moving up, ignoring.. cardId["); Serial.print(cardId); Serial.println("].");          
        } else {
          Serial.print("Found top card while NOT MOVING, ignoring.. cardId["); Serial.print(cardId); Serial.println("].");          
        }
      } else if (cardId == CARDID_BOTTOM) {
        // Shade is fully open
        if (movingUp) {
          Serial.print("FOUND BOTTOM CARD while moving up! Stopping... cardId["); Serial.print(cardId); Serial.println("].");
          stopShade();
          setShadeLevel(100);
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
}

void subscribeMqttFeeds() {
  balconyShade->onMessage(handleShadeLevelMessage);
}

void handleShadeLevelMessage(AdafruitIO_Data *data) {
  int shadeOpenPercent = atoi(data->value());
  Serial.print("[MQTT] Received shade level message with the value[");
  Serial.print(shadeOpenPercent);
  Serial.println("]");
  setTargetShadeLevel(shadeOpenPercent);
}

void handleConnectivity() {
  aio_status_t ioStatus = io.run(400, true);

  if (ioStatus < AIO_CONNECTED && !connectingInProgress) {
    Serial.println("Need to connect, starting..");
    connectingInProgress = true;
    io.connect();       
  } else if (ioStatus >= AIO_CONNECTED && connectingInProgress) {
    connectingInProgress = false;
    Serial.println("Connected to Adafruit IO!");
    subscribeMqttFeeds();
  }
}

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

void loop() {
  handleConnectivity();
  
  btnUpCurr = digitalRead(PIN_UP_BTN);
  btnDownCurr = digitalRead(PIN_DOWN_BTN);
  irqCurr = digitalRead(PN532_IRQ);
  
  if (btnUpCurr == LOW && btnUpPrev == HIGH) {
    Serial.println("Pressed UP button");
    if (movingUp || movingDown) {
      stopShade();
    } else {
      setTargetShadeLevel(100);
    }
  }  

  if (btnDownCurr == LOW && btnDownPrev == HIGH) {
    Serial.println("Pressed DOWN button");
    if (movingUp || movingDown) {
      stopShade();
    } else {
      setTargetShadeLevel(0);
    }
  }

  if (listeningToNFC && irqCurr == LOW && irqPrev == HIGH) {
     handleNFCDetected(); 
  } else if (irqCurr == LOW && irqPrev == HIGH) {
    Serial.println("##### Got IRQ while not listening..");  
  }

  irqPrev = irqCurr;
  btnUpPrev = btnUpCurr;
  btnDownPrev = btnDownCurr;
}
