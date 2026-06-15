#include <ESP8266WiFi.h>
#include <espnow.h>          // ★ AJOUT ESP-NOW
#include <SPI.h>
#include <mcp_can.h>

// ===== CAN =====
#define CAN_CS 15   // D8
MCP_CAN CAN(CAN_CS);

// ===== WIFI =====
const char* ssid = "VOITURE_WIFI";
const char* password = "12345678";
#define WIFI_CHANNEL 1       // ★ AJOUT : doit etre identique sur Vehicule A
WiFiServer server(80);

// ===== PINS MOTEUR =====
#define IN1 5    // D1
#define IN2 4    // D2
#define IN3 0    // D3
#define IN4 2    // D4
#define ENA 16   // D0
#define ENB 16   // D7

int SPEED = 100;

// ===== STATES =====
bool forward = false;
bool backward = false;
bool left = false;
bool right = false;

bool obstacle = false;   // 🚨 sécurité

// ===== ★ AJOUT ESP-NOW V2V =====
bool brakeV2V = false;                     // freinage reçu de Vehicule A
unsigned long lastBrakeTime = 0;
const unsigned long BRAKE_TIMEOUT = 800;   // ms : si rien reçu -> relache
int vitesseRecue = 0;                      // vitesse simulee reçue de A

typedef struct {
  char type[10];   // "BRAKE" ou "SPEED"
  int  value;      // BRAKE : 1/0
  int  vitesse;    // 0-120 km/h
} Message;

void onReceive(uint8_t *mac, uint8_t *data, uint8_t len) {
  Message msg;
  memcpy(&msg, data, sizeof(msg));

  if (strcmp(msg.type, "BRAKE") == 0) {
    if (msg.value == 1) {
      brakeV2V = true;
      lastBrakeTime = millis();
      Serial.println("🚨 BRAKE V2V recu de A -> STOP");
    } else {
      brakeV2V = false;
      Serial.println("✅ BRAKE V2V relache");
    }
  }
  else if (strcmp(msg.type, "SPEED") == 0) {
    vitesseRecue = msg.vitesse;
    Serial.print("Vitesse A recue : ");
    Serial.println(vitesseRecue);
  }
}
// ===== ★ FIN AJOUT ESP-NOW =====

// ===== MOTOR CONTROL =====
void updateMotors() {

  // 🚨 STOP si obstacle OU freinage V2V         // ★ AJOUT : || brakeV2V
  if (obstacle || brakeV2V) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    analogWrite(ENA, 0);
    analogWrite(ENB, 0);
    return;
  }

  int leftSpeed = 0;
  int rightSpeed = 0;

  if (forward) {
    leftSpeed += SPEED;
    rightSpeed += SPEED;
  }

  if (backward) {
    leftSpeed -= SPEED;
    rightSpeed -= SPEED;
  }

  if (right) {
    leftSpeed += SPEED;
    rightSpeed -= SPEED;
  }

  if (left) {
    leftSpeed -= SPEED;
    rightSpeed += SPEED;
  }

  leftSpeed = constrain(leftSpeed, -1023, 1023);
  rightSpeed = constrain(rightSpeed, -1023, 1023);

  // LEFT MOTOR
  if (leftSpeed > 0) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, leftSpeed);
  } else if (leftSpeed < 0) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    analogWrite(ENA, -leftSpeed);
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, 0);
  }

  // RIGHT MOTOR
  if (rightSpeed > 0) {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    analogWrite(ENB, rightSpeed);
  } else if (rightSpeed < 0) {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    analogWrite(ENB, -rightSpeed);
  } else {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    analogWrite(ENB, 0);
  }
}

// ===== SETUP =====
void setup() {

  Serial.begin(115200);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);

  // ===== CAN INIT =====
  while (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) != CAN_OK) {
    Serial.println("CAN INIT FAIL...");
    delay(100);
  }
  CAN.setMode(MCP_NORMAL);
  Serial.println("CAN READY");

  // ===== WIFI =====
  WiFi.mode(WIFI_AP_STA);                          // ★ AJOUT : STA requis pour ESP-NOW
  WiFi.softAP(ssid, password, WIFI_CHANNEL);       // ★ MODIF : canal fixé
  server.begin();

  Serial.print("MAC STA : ");                      // ★ AJOUT : pour la mettre dans A
  Serial.println(WiFi.macAddress());
  Serial.println("READY: http://192.168.4.1");

  // ===== ★ AJOUT ESP-NOW INIT =====
  if (esp_now_init() != 0) {
    Serial.println("❌ ESP-NOW INIT FAIL");
    return;
  }
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(onReceive);
  Serial.println("✅ ESP-NOW READY (recepteur BRAKE + SPEED)");
}

// ===== LOOP =====
void loop() {

  // ===== ★ AJOUT : TIMEOUT BRAKE V2V =====
  if (brakeV2V && (millis() - lastBrakeTime > BRAKE_TIMEOUT)) {
    brakeV2V = false;
    Serial.println("⏱️ BRAKE V2V timeout -> relache");
  }

  // ===== CAN RECEIVE =====
  if (CAN_MSGAVAIL == CAN.checkReceive()) {

    long unsigned int rxId;
    byte len = 0;
    byte buf[8];

    CAN.readMsgBuf(&rxId, &len, buf);

    // ESP capteur envoie 1 ou 0
    if (buf[0] == 1) {
      obstacle = true;
      Serial.println("🚨 OBSTACLE DETECTED");
    } else {
      obstacle = false;
    }
  }

  // ===== ★ AJOUT : appliquer l'etat moteur en continu =====
  // (sinon le STOP V2V n'agit que quand un client web envoie une requete)
  updateMotors();

  // ===== WIFI CLIENT =====
  WiFiClient client = server.available();
  if (!client) return;

  String req = client.readStringUntil('\n');
  client.flush();

  // RESET STATES
  forward = false;
  backward = false;
  left = false;
  right = false;

  // 🚨 BLOQUER SI OBSTACLE OU BRAKE V2V          // ★ MODIF : && !brakeV2V
  if (!obstacle && !brakeV2V) {
    if (req.indexOf("/W") != -1) forward = true;
    if (req.indexOf("/X") != -1) backward = true;
    if (req.indexOf("/A") != -1) left = true;
    if (req.indexOf("/D") != -1) right = true;
  }

  updateMotors();

  // ===== WEB PAGE =====
   client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println();

  client.println(R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<title>SMART CAR</title>

<style>
body{
  text-align:center;
  font-family:Arial;
  background:#111;
  color:white;
}
h1{color:#00ffcc;}

#joystick{
  width:200px;
  height:200px;
  background:#333;
  border-radius:50%;
  margin:30px auto;
  position:relative;
}
#stick{
  width:60px;
  height:60px;
  background:#00ffcc;
  border-radius:50%;
  position:absolute;
  top:70px;
  left:70px;
}

button{
  width:150px;
  height:60px;
  font-size:20px;
  border-radius:10px;
}
.stop{
  background:red;
  color:white;
}
</style>
</head>

<body>

<h1>🚗 WIFI CAR</h1>
<p>W/X/A/D clavier ou joystick</p>

<div id="joystick">
  <div id="stick"></div>
</div>

<button class="stop" onclick="stopAll()">STOP</button>

<script>
let keys={};

// KEYBOARD
document.onkeydown = e => {
  keys[e.key.toLowerCase()] = true;
  send();
};

document.onkeyup = e => {
  keys[e.key.toLowerCase()] = false;
  send();
};

// 🔥 FIXED SEND FUNCTION
function send(){

  let cmd="";

  if(keys['w']) cmd+="/W";
  if(keys['x']) cmd+="/X";
  if(keys['a']) cmd+="/A";
  if(keys['d']) cmd+="/D";

  if(cmd=="") cmd="/STOP";

  fetch(cmd);
}

function stopAll(){
  fetch("/STOP");
}

// JOYSTICK
let joy = document.getElementById("joystick");
let stick = document.getElementById("stick");

joy.addEventListener("touchmove", function(e){
  let rect = joy.getBoundingClientRect();
  let x = e.touches[0].clientX - rect.left;
  let y = e.touches[0].clientY - rect.top;

  let dx = x - 100;
  let dy = y - 100;

  stick.style.left = (x-30)+"px";
  stick.style.top  = (y-30)+"px";

  let cmd="";

  if(dy < -30) cmd+="/W";
  if(dy > 30) cmd+="/X";
  if(dx < -30) cmd+="/A";
  if(dx > 30) cmd+="/D";

  if(cmd=="") cmd="/STOP";

  fetch(cmd);
});

joy.addEventListener("touchend", function(){
  stick.style.left="70px";
  stick.style.top="70px";
  fetch("/STOP");
});
</script>

</body>
</html>
)rawliteral");

  delay(1);
}
