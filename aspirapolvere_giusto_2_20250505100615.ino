// Definizione dei pin
#define ON_PIN 13
#define OFF_PIN 12
#define POWERUP_PIN 11
#define POWERDOWN_PIN 10
#define SENSORE_PIN A0
#define MOTORE_PIN 6
#define MOTORE2_PIN 5

// Livelli di potenza corrispondenti ai valori PWM
const int potenzaLivelli[] = {65, 130, 195, 255};

// Variabili di stato
int stato = 0; // 0 = spento, 1 = acceso
int livello = 1; // da 1 a 4
int potenza = potenzaLivelli[livello - 1]; // Impostazione iniziale della potenza
unsigned long ultimoRipristino = 0;
unsigned long intervalloRipristino = 1000; // 1 secondo

// Variabili per il debouncing
bool last_onState = LOW;
bool last_offState = LOW;
bool last_upState = LOW;
bool last_downState = LOW;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

void setup() {
  // Impostazione dei pin
  pinMode(ON_PIN, INPUT);
  pinMode(OFF_PIN, INPUT);
  pinMode(POWERUP_PIN, INPUT);
  pinMode(POWERDOWN_PIN, INPUT);
  pinMode(SENSORE_PIN, INPUT);
  pinMode(MOTORE_PIN, OUTPUT);
  pinMode(MOTORE2_PIN, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  // Lettura dello stato dei pulsanti
  bool onPressed = digitalRead(ON_PIN);
  bool offPressed = digitalRead(OFF_PIN);
  bool upPressed = digitalRead(POWERUP_PIN);
  bool downPressed = digitalRead(POWERDOWN_PIN);

  unsigned long currentTime = millis();

  if ((currentTime - lastDebounceTime) > debounceDelay) {
    // Accensione
    if (onPressed == HIGH && last_onState == LOW && stato == 0) {
      stato = 1;
      livello = 1;
      potenza = potenzaLivelli[livello - 1];
      analogWrite(MOTORE_PIN, potenza);
      analogWrite(MOTORE2_PIN, potenza);
      last_onState = HIGH;
      Serial.println("Macchina accesa (Livello iniziale: 1)");
    }

    // Spegnimento
    if (offPressed == HIGH && last_offState == LOW && stato == 1) {
      stato = 0;
      analogWrite(MOTORE_PIN, 0);
      analogWrite(MOTORE2_PIN, 0);
      last_offState = HIGH;
      Serial.println("Macchina spenta");
    }

    if (stato == 1) {
      // Aumento livello
      if (upPressed == HIGH && last_upState == LOW && livello < 4) {
        livello++;
        potenza = potenzaLivelli[livello - 1];
        analogWrite(MOTORE_PIN, potenza);
        analogWrite(MOTORE2_PIN, potenza);
        last_upState = HIGH;
        Serial.print("Livello potenza aumentato a: ");
        Serial.println(livello);
      }

      // Diminuzione livello
      if (downPressed == HIGH && last_downState == LOW && livello > 1) {
        livello--;
        potenza = potenzaLivelli[livello - 1];
        analogWrite(MOTORE_PIN, potenza);
        analogWrite(MOTORE2_PIN, potenza);
        last_downState = HIGH;
        Serial.print("Livello potenza diminuito a: ");
        Serial.println(livello);
      }

      // Lettura del sensore
      int valoreSensore = analogRead(SENSORE_PIN);
      Serial.print("Livello: ");
      Serial.print(livello);
      Serial.print(" | Potenza: ");
      Serial.print(potenza);
      Serial.print(" | Sensore: ");
      Serial.println(valoreSensore);

      // Controllo caduta di potenza
      if (valoreSensore < 1023 && (currentTime - ultimoRipristino) > intervalloRipristino) {
        Serial.println("Caduta di potenza rilevata! Ripristino PWM...");
        analogWrite(MOTORE_PIN, potenza);
        analogWrite(MOTORE2_PIN, potenza);
        ultimoRipristino = currentTime;
      }
    }

    // Reset stati dei pulsanti
    if (onPressed == LOW) last_onState = LOW;
    if (offPressed == LOW) last_offState = LOW;
    if (upPressed == LOW) last_upState = LOW;
    if (downPressed == LOW) last_downState = LOW;

    lastDebounceTime = currentTime;
  }
}







