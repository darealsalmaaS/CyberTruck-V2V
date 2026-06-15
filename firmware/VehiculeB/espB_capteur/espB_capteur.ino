/*
 * ============================================================
 *  CYBER TRUCK - Vehicule B - ESP CAPTEUR (perception)
 * ============================================================
 *  Role :
 *    - Mesure distance (HC-SR04)
 *    - Mesure vitesse roue (capteur FC-03 a fourche optique)
 *    - Affichage local OLED SSD1306
 *    - Emet la telemetrie sur le bus CAN (ID 0x200) vers l'ESP moteur
 *
 *  Trame CAN 0x200 (6 octets) :
 *    [0] obstacleAlerte (0/1)
 *    [1] airbagAlerte   (0/1)
 *    [2] distance HIGH byte
 *    [3] distance LOW byte
 *    [4] vitesse  HIGH byte
 *    [5] vitesse  LOW byte
 *
 *  Branchements :
 *    HC-SR04 : TRIG=D1(GPIO5), ECHO=D2(GPIO4)
 *    FC-03   : OUT=D0(GPIO16)
 *    OLED    : SDA=D3(GPIO0), SCL=D4(GPIO2)
 *    MCP2515 : CS=D8(GPIO15), SPI standard
 * ============================================================
 */

#include <SPI.h>
#include <mcp_can.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ================= ULTRASON =================
#define TRIG_PIN 5   // D1 / GPIO5
#define ECHO_PIN 4   // D2 / GPIO4

// ================= FC03 SUR D0 =================
#define FC03_PIN 16  // D0 / GPIO16
#define NB_TROUS 20
#define DIAMETRE_ROUE_CM 6.5
unsigned long pulseCount = 0;
int lastFC03State = HIGH;
unsigned long lastSpeedTime = 0;
float rpm = 0;
float vitesse_kmh = 0;

// ================= OLED =================
#define OLED_SDA 0   // D3 / GPIO0
#define OLED_SCL 2   // D4 / GPIO2
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ================= CAN MCP2515 =================
#define CAN_CS 15    // D8
MCP_CAN CAN(CAN_CS);

// ================= VARIABLES =================
long distance = 999;
byte obstacleAlerte = 0;
byte airbagAlerte = 0;

// ================= FC03 POLLING =================
void updateFC03() {
  int state = digitalRead(FC03_PIN);
  if (lastFC03State == HIGH && state == LOW) {
    pulseCount++;
  }
  lastFC03State = state;
}

// ================= ULTRASON =================
long lireDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 12000);
  if (duration == 0) return 999;
  long d = duration * 0.034 / 2;
  if (d > 300) return 999;
  if (d <= 1) return 0;
  return d;
}

// ================= CALCUL VITESSE =================
void calculerVitesse() {
  unsigned long now = millis();
  if (now - lastSpeedTime >= 500) {
    unsigned long pulses = pulseCount;
    pulseCount = 0;
    float dt = (now - lastSpeedTime) / 1000.0;
    lastSpeedTime = now;
    float tours = pulses / (float)NB_TROUS;
    float tours_par_sec = tours / dt;
    rpm = tours_par_sec * 60.0;
    float perimetre_cm = 3.1416 * DIAMETRE_ROUE_CM;
    float vitesse_cm_s = tours_par_sec * perimetre_cm;
    float vitesse_m_s = vitesse_cm_s / 100.0;
    vitesse_kmh = vitesse_m_s * 3.6;
  }
}

// ================= OLED =================
void afficherOLED() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("SMART E-CAR ESP1");
  display.setCursor(0, 14);
  display.print("Distance : ");
  if (distance == 999) display.println("--- cm");
  else {
    display.print(distance);
    display.println(" cm");
  }
  display.setCursor(0, 28);
  display.print("Vitesse  : ");
  display.print(vitesse_kmh, 1);
  display.println(" km/h");
  display.setCursor(0, 42);
  display.print("RPM      : ");
  display.println(rpm, 0);
  display.setCursor(0, 56);
  if (airbagAlerte) display.println("AIRBAG ACTIVE !");
  else if (obstacleAlerte) display.println("OBSTACLE DETECTE !");
  else display.println("Etat : OK");
  display.display();
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(FC03_PIN, INPUT);
  lastFC03State = digitalRead(FC03_PIN);
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Erreur OLED !");
  } else {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("OLED OK");
    display.display();
    delay(1000);
  }
  while (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) != CAN_OK) {
    Serial.println("Erreur init MCP2515...");
    delay(500);
  }
  CAN.setMode(MCP_NORMAL);
  Serial.println("CAN ESP1 pret");
  lastSpeedTime = millis();
}

// ================= LOOP =================
void loop() {
  updateFC03();
  calculerVitesse();
  static unsigned long lastUltrasonTime = 0;
  static unsigned long lastCANTime = 0;
  static unsigned long lastOLEDTime = 0;
  if (millis() - lastUltrasonTime >= 120) {
    lastUltrasonTime = millis();
    distance = lireDistance();
    if (distance == 0) {
      airbagAlerte = 1;
      obstacleAlerte = 1;
    }
    else if (distance < 15) {
      airbagAlerte = 0;
      obstacleAlerte = 1;
    }
    else {
      airbagAlerte = 0;
      obstacleAlerte = 0;
    }
  }
  if (millis() - lastCANTime >= 100) {
    lastCANTime = millis();
    uint16_t distanceCAN = constrain(distance, 0, 999);
    uint16_t vitesseCAN  = constrain((int)vitesse_kmh, 0, 65535);
    byte dataCAN[6];
    dataCAN[0] = obstacleAlerte;
    dataCAN[1] = airbagAlerte;
    dataCAN[2] = highByte(distanceCAN);
    dataCAN[3] = lowByte(distanceCAN);
    dataCAN[4] = highByte(vitesseCAN);
    dataCAN[5] = lowByte(vitesseCAN);
    byte sndStat = CAN.sendMsgBuf(0x200, 0, 6, dataCAN);
    if (sndStat == CAN_OK) {
      Serial.print("CAN OK | D=");
      Serial.print(distanceCAN);
      Serial.print(" cm | V=");
      Serial.print(vitesseCAN);
      Serial.print(" km/h | Pulses=");
      Serial.println(pulseCount);
    } else {
      Serial.println("Erreur envoi CAN");
    }
  }
  if (millis() - lastOLEDTime >= 250) {
    lastOLEDTime = millis();
    afficherOLED();
  }
}
