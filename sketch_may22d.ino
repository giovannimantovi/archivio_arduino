#include <ArduinoBLE.h>

// Pin definizione
#define ON_PIN 13
#define OFF_PIN 12
#define POWERUP_PIN 11
#define POWERDOWN_PIN 10
#define SENSORE_PIN A0
#define MOTORE_PIN 6
#define MOTORE2_PIN 5

// Servizio e caratteristiche BLE
BLEService chatService("19B10000-E8F2-537E-4F6C-D104768A1214");

BLECharacteristic rxCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214",
                                   BLEWrite | BLEWriteWithoutResponse, 100);
BLECharacteristic txCharacteristic("19B10002-E8F2-537E-4F6C-D104768A1214",
                                   BLERead | BLENotify, 100);

// Livelli di potenza (PWM)
const int potenzaLivelli[] = {65, 130, 195, 255};
int stato = 0;         // 0 = spento, 1 = acceso
int livello = 1;
int x = 1;             // Livello di potenza originale
int potenza = potenzaLivelli[livello - 1];

// Debounce
bool last_onState = LOW, last_offState = LOW;
bool last_upState = LOW, last_downState = LOW;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

// Sensore
int sogliaCaduta = 923;
int sogliaRipristino = 973;
bool cadutaRilevata = false;
unsigned long attesa = 0;
int tempo_check = 1000;

void setup() {
  pinMode(ON_PIN, INPUT);
  pinMode(OFF_PIN, INPUT);
  pinMode(POWERUP_PIN, INPUT);
  pinMode(POWERDOWN_PIN, INPUT);
  pinMode(SENSORE_PIN, INPUT);
  pinMode(MOTORE_PIN, OUTPUT);
  pinMode(MOTORE2_PIN, OUTPUT);

  Serial.begin(115200);
  while (!Serial);

  if (!BLE.begin()) {
    Serial.println("Errore BLE");
    while (1);
  }

  BLE.setLocalName("GIGA-Chat");
  BLE.setAdvertisedService(chatService);
  chatService.addCharacteristic(rxCharacteristic);
  chatService.addCharacteristic(txCharacteristic);
  BLE.addService(chatService);
  BLE.advertise();

  Serial.println("In attesa di connessione BLE...");
  attesa = millis();
}

void loop() {
  unsigned long currentTime = millis();
  BLE.poll(); // necessario per ricevere dati BLE

  // === COMANDI BLE ===
  if (rxCharacteristic.written()) {
    int length = rxCharacteristic.valueLength();
    uint8_t buffer[100];
    rxCharacteristic.readValue(buffer, length);

    String receivedMessage = "";
    for (int i = 0; i < length; i++) {
      receivedMessage += (char)buffer[i];
    }

    Serial.print("Messaggio BLE ricevuto: ");
    Serial.println(receivedMessage);

    if (receivedMessage == "1" && stato == 0) {
      accendiMacchina();
      Serial.println("Accensione via BLE");
    } else if (receivedMessage == "0" && stato == 1) {
      spegniMacchina();
      Serial.println("Spegnimento via BLE");
    } else if (receivedMessage == "3" && stato == 1 && livello < 4) {
      livello++;
      x = livello;
      aggiornaPotenza();
      Serial.print("Livello aumentato via BLE: ");
      Serial.println(livello);
    } else if (receivedMessage == "4" && stato == 1 && livello > 1) {
      livello--;
      x = livello;
      aggiornaPotenza();
      Serial.print("Livello diminuito via BLE: ");
      Serial.println(livello);
    }

    String reply = "Comando ricevuto: " + receivedMessage;
    txCharacteristic.writeValue(reply.c_str());
  }

  // === PULSANTI FISICI ===
  bool onPressed = digitalRead(ON_PIN);
  bool offPressed = digitalRead(OFF_PIN);
  bool upPressed = digitalRead(POWERUP_PIN);
  bool downPressed = digitalRead(POWERDOWN_PIN);

  if ((currentTime - lastDebounceTime) > debounceDelay) {
    if (onPressed == HIGH && last_onState == LOW && stato == 0) {
      accendiMacchina();
      Serial.println("Accensione manuale");
    }

    if (offPressed == HIGH && last_offState == LOW && stato == 1) {
      spegniMacchina();
      Serial.println("Spegnimento manuale");
    }

    if (stato == 1) {
      if (upPressed == HIGH && last_upState == LOW && livello < 4) {
        livello++;
        x = livello;
        aggiornaPotenza();
        Serial.print("Livello aumentato manualmente: ");
        Serial.println(livello);
      }

      if (downPressed == HIGH && last_downState == LOW && livello > 1) {
        livello--;
        x = livello;
        aggiornaPotenza();
        Serial.print("Livello diminuito manualmente: ");
        Serial.println(livello);
      }
    }

    last_onState = onPressed;
    last_offState = offPressed;
    last_upState = upPressed;
    last_downState = downPressed;
    lastDebounceTime = currentTime;
  }

  // === LETTURA SENSORE ===
  if (stato == 1) {
    int valoreSensore = analogRead(SENSORE_PIN);

    Serial.print("Livello: ");
    Serial.print(livello);
    Serial.print(" | Potenza: ");
    Serial.print(potenza);
    Serial.print(" | Sensore: ");
    Serial.println(valoreSensore);

    if (valoreSensore < sogliaCaduta) {
      if ((millis() - attesa) >= tempo_check && livello < 4) {
        livello++;
        aggiornaPotenza();
        Serial.print("Caduta rilevata, aumento livello a: ");
        Serial.println(livello);
        cadutaRilevata = true;
        attesa = millis();
      }
    }

    if (valoreSensore > sogliaRipristino && cadutaRilevata) {
      livello = x;
      aggiornaPotenza();
      Serial.print("Ripristino potenza. Livello: ");
      Serial.println(livello);
      cadutaRilevata = false;
    }
  }
}

// === FUNZIONI DI SUPPORTO ===

void accendiMacchina() {
  stato = 1;
  livello = 1;
  x = livello;
  aggiornaPotenza();
}

void spegniMacchina() {
  stato = 0;
  analogWrite(MOTORE_PIN, 0);
  analogWrite(MOTORE2_PIN, 0);
}

void aggiornaPotenza() {
  potenza = potenzaLivelli[livello - 1];
  analogWrite(MOTORE_PIN, potenza);
  analogWrite(MOTORE2_PIN, potenza);
}
