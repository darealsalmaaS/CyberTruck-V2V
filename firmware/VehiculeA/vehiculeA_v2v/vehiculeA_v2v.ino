/*
 * ESP A — Véhicule A
 * - Bouton poussoir D2  -> envoi BRAKE via ESP-NOW
 * - Potentiometre  A0   -> envoi vitesse simulee 0-120 km/h via ESP-NOW
 *
 * Branchements :
 *  Bouton : une patte -> D2 (GPIO4), autre patte -> GND
 *  Pot    : gauche -> 3.3V , droite -> GND , curseur -> A0
 *
 * Note bouton : INPUT_PULLUP -> repos = HIGH (1), appuye = LOW (0)
 */

#include <ESP8266WiFi.h>
#include <espnow.h>

extern "C" {
  #include "user_interface.h"   // pour wifi_set_channel
}

// MAC de Vehicule B (ESP moteur)
uint8_t macESP_B[] = {0xDC, 0x4F, 0x22, 0x58, 0xCE, 0x71};

#define PIN_BUTTON   D2
#define WIFI_CHANNEL 1   // DOIT etre identique a Vehicule B

// ── Structure IDENTIQUE des deux cotes ──
typedef struct {
  char type[10];   // "BRAKE" ou "SPEED"
  int  value;      // BRAKE : 1 = freinage, 0 = relache
  int  vitesse;    // 0-120 km/h
} Message;

Message msgToSend;

// ── Bouton (anti-rebond robuste) ──
int  stableState     = HIGH;   // etat confirme (repos = HIGH)
int  lastReading     = HIGH;   // derniere lecture brute
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50;
bool brakeActive = false;      // freinage en cours

// ── Envoi periodique ──
unsigned long lastBrakeSend = 0;
const unsigned long BRAKE_INTERVAL = 200;  // ms tant que bouton maintenu
unsigned long lastSpeedSend = 0;
const unsigned long SPEED_INTERVAL = 300;  // ms envoi vitesse

void onSent(uint8_t *mac_addr, uint8_t status) {
  Serial.print("Envoi ESP-NOW : ");
  Serial.println(status == 0 ? "OK" : "ECHEC");
}

void sendBrake(int value) {
  strcpy(msgToSend.type, "BRAKE");
  msgToSend.value   = value;
  msgToSend.vitesse = 0;
  esp_now_send(macESP_B, (uint8_t *)&msgToSend, sizeof(msgToSend));
}

void sendSpeed() {
  int raw = analogRead(A0);              // 0-1023
  int kmh = map(raw, 0, 1023, 0, 120);   // 0-120 km/h
  strcpy(msgToSend.type, "SPEED");
  msgToSend.value   = 0;
  msgToSend.vitesse = kmh;
  esp_now_send(macESP_B, (uint8_t *)&msgToSend, sizeof(msgToSend));
  Serial.print("Vitesse envoyee : ");
  Serial.println(kmh);
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  wifi_set_channel(WIFI_CHANNEL);

  if (esp_now_init() != 0) {
    Serial.println("Erreur init ESP-NOW");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(onSent);
  esp_now_add_peer(macESP_B, ESP_NOW_ROLE_SLAVE, WIFI_CHANNEL, NULL, 0);

  Serial.println("ESP A pret (bouton BRAKE + pot vitesse)...");
}

void loop() {

  // ===== LECTURE BOUTON AVEC ANTI-REBOND =====
  int reading = digitalRead(PIN_BUTTON);

  if (reading != lastReading) {
    lastDebounceTime = millis();   // l'etat a bouge -> on redemarre le timer
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    // l'etat est stable depuis assez longtemps
    if (reading != stableState) {
      stableState = reading;

      if (stableState == LOW) {        // front : vient d'etre appuye
        brakeActive = true;
        Serial.println("Bouton appuye -> BRAKE ON");
      } else {                         // front : vient d'etre relache
        brakeActive = false;
        sendBrake(0);                  // dit a B de repartir
        Serial.println("Bouton relache -> BRAKE OFF");
      }
    }
  }
  lastReading = reading;

  // ===== ENVOI BRAKE=1 tant que maintenu =====
  if (brakeActive && (millis() - lastBrakeSend > BRAKE_INTERVAL)) {
    sendBrake(1);
    lastBrakeSend = millis();
  }

  // ===== POTENTIOMETRE -> VITESSE (pas pendant freinage) =====
  if (!brakeActive && (millis() - lastSpeedSend > SPEED_INTERVAL)) {
    sendSpeed();
    lastSpeedSend = millis();
  }
}
