/*
 * ============================================================
 *  CYBER TRUCK - Vehicule B - ESP MOTEUR (actionnement + dashboard)
 * ============================================================
 *  Role :
 *    - Recoit la telemetrie de l'ESP capteur via CAN (ID 0x200)
 *    - Pilote les 2 moteurs DC via L298N
 *    - Sert un dashboard web (point d'acces WiFi VOITURE_WIFI)
 *    - Gere le mode manuel (W/X/A/D), le claxon, la LED airbag
 *    - Gere un mode Self-Driven (parcours 50m -> virage -> 40m)
 *
 *  Acces dashboard : se connecter au WiFi "VOITURE_WIFI" (mdp 12345678)
 *                    puis ouvrir http://192.168.4.1  (code PIN : 1234)
 *
 *  Branchements :
 *    L298N   : IN1=D1(GPIO5) IN2=D2(GPIO4) IN3=D3(GPIO0) IN4=D4(GPIO2)
 *              ENA+ENB = D0 (GPIO16)
 *    Buzzer  : TX (GPIO1)
 *    LED airbag : RX (GPIO3)
 *    MCP2515 : CS=D8(GPIO15), SPI standard
 * ============================================================
 */

#include <ESP8266WiFi.h>
#include <SPI.h>
#include <mcp_can.h>
#include <pgmspace.h>

// ================= CAN =================
#define CAN_CS 15   // D8
MCP_CAN CAN(CAN_CS);

// ================= WIFI =================
const char* ssid = "VOITURE_WIFI";
const char* password = "12345678";

IPAddress local_IP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

WiFiServer server(80);

// ================= MOTEURS =================
#define IN1 5       // D1
#define IN2 4       // D2
#define IN3 0       // D3
#define IN4 2       // D4
#define ENA_ENB 16  // D0

// ================= BUZZER + LED =================
#define BUZZER_PIN 1      // TX
#define AIRBAG_LED_PIN 3  // RX

// ================= ETATS MANUEL =================
bool forwardState = false;
bool backwardState = false;
bool leftState = false;
bool rightState = false;

// ================= DONNEES CAN =================
bool obstacle = false;
bool airbag = false;

uint16_t distanceRecu = 999;
uint16_t vitesseRecu = 0;

unsigned long lastCanReceive = 0;

// ================= SELF DRIVEN =================
enum AutoState {
  AUTO_IDLE,
  AUTO_FORWARD_50,
  AUTO_TURN_RIGHT,
  AUTO_FORWARD_40,
  AUTO_DONE,
  AUTO_SAFETY_STOP
};

AutoState autoState = AUTO_IDLE;

bool autoMode = false;
float autoDistance = 0.0;
unsigned long lastAutoUpdate = 0;
unsigned long autoTurnStart = 0;

const float AUTO_DISTANCE_1 = 50.0;   // 50 m
const float AUTO_DISTANCE_2 = 40.0;   // 40 m
const unsigned long AUTO_TURN_TIME = 1200; // ms, a regler selon ton vehicule

// =====================================================
// DASHBOARD HTML
// =====================================================
const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0">
<title>SMART E-CAR</title>

<style>
*{box-sizing:border-box}
body{
  margin:0;
  font-family:Arial,Helvetica,sans-serif;
  background:linear-gradient(135deg,#020617,#0f172a,#020617);
  color:white;
}
.login{
  min-height:100vh;
  display:flex;
  justify-content:center;
  align-items:center;
}
.login-card{
  width:330px;
  padding:25px;
  border-radius:24px;
  background:#111827;
  border:1px solid #0ea5e9;
  box-shadow:0 0 35px #0ea5e955;
  text-align:center;
}
.logo{font-size:45px;margin-bottom:10px}
h1{color:#38bdf8;letter-spacing:2px}
input{
  width:100%;
  height:50px;
  font-size:25px;
  text-align:center;
  border-radius:12px;
  border:1px solid #38bdf8;
  background:#020617;
  color:white;
  margin:15px 0;
}
button{
  border:0;
  border-radius:12px;
  font-weight:bold;
  cursor:pointer;
}
.unlock{
  width:100%;
  height:50px;
  background:#22c55e;
  color:#020617;
  font-size:18px;
}
.error{color:#ef4444;height:20px;margin-top:10px}
.app{display:none;padding:15px}
.top{
  display:flex;
  justify-content:space-between;
  align-items:center;
  background:#111827;
  border:1px solid #1e40af;
  border-radius:20px;
  padding:15px;
  margin-bottom:15px;
}
.title{color:#38bdf8;font-weight:bold;letter-spacing:2px}
.status{display:flex;align-items:center;gap:8px}
.dot{
  width:12px;
  height:12px;
  border-radius:50%;
  background:#22c55e;
  box-shadow:0 0 15px #22c55e;
}
.dot.off{background:#ef4444;box-shadow:0 0 15px #ef4444}
.grid{
  display:grid;
  grid-template-columns:1fr 1fr;
  gap:15px;
}
.card{
  background:#111827;
  border:1px solid #334155;
  border-radius:22px;
  padding:20px;
}
.big{grid-column:span 2}
.label{
  color:#94a3b8;
  font-size:13px;
  letter-spacing:2px;
}
.value{
  font-size:42px;
  font-weight:bold;
  margin-top:10px;
}
.green{color:#22c55e}
.red{color:#ef4444}
.blue{color:#38bdf8}
.orange{color:#f59e0b}
.speed{text-align:center}
.speed .num{
  font-size:90px;
  color:#22c55e;
  font-family:monospace;
}
.alert{
  grid-column:span 2;
  font-size:25px;
  font-weight:bold;
  padding:25px;
  border-radius:22px;
  background:#020617;
  border:1px solid #334155;
}
.alert.danger{
  color:#ef4444;
  border-color:#ef4444;
  animation:pulse 1s infinite;
}
.alert.safe{color:#22c55e}
.control{
  grid-column:span 2;
  display:grid;
  grid-template-columns:1fr 220px 1fr;
  gap:15px;
  align-items:center;
}
.wheel{
  width:230px;
  height:230px;
  margin:auto;
  border-radius:50%;
  border:22px solid #334155;
  background:#020617;
  position:relative;
  transform:rotate(var(--rot));
  transition:.2s;
}
.wheel:before{
  content:"";
  position:absolute;
  width:70px;
  height:70px;
  border-radius:50%;
  background:#1e293b;
  left:50%;
  top:50%;
  transform:translate(-50%,-50%);
}
.wheel:after{
  content:"E-CAR";
  position:absolute;
  color:#38bdf8;
  font-weight:bold;
  left:50%;
  top:50%;
  transform:translate(-50%,-50%);
}
.dpad{
  display:grid;
  grid-template-columns:repeat(3,65px);
  grid-template-rows:repeat(3,65px);
  gap:6px;
  justify-content:center;
}
.dpad button{
  font-size:28px;
  background:#1e293b;
  color:white;
}
.dpad button.active{background:#2563eb}
.stop{
  background:#ef4444;
  color:white;
  height:60px;
  width:100%;
  font-size:20px;
}
.horn{
  background:#f59e0b;
  color:#020617;
  height:60px;
  width:100%;
  font-size:20px;
  margin-bottom:10px;
}
.auto{
  background:#22c55e;
  color:#020617;
  height:60px;
  width:100%;
  font-size:20px;
  margin-bottom:10px;
}
.autoStop{
  background:#7c2d12;
  color:white;
  height:55px;
  width:100%;
  font-size:18px;
  margin-bottom:10px;
}
@keyframes pulse{
  0%,100%{opacity:1}
  50%{opacity:.4}
}
@media(max-width:800px){
  .grid,.control{grid-template-columns:1fr}
  .big,.alert,.control{grid-column:span 1}
}
</style>
</head>

<body>

<div class="login" id="login">
  <div class="login-card">
    <div class="logo">CAR</div>
    <h1>SMART E-CAR</h1>
    <p>Code d'acces</p>
    <input id="code" type="password" maxlength="4" placeholder="1234">
    <button class="unlock" onclick="unlock()">UNLOCK</button>
    <div class="error" id="err"></div>
  </div>
</div>

<div class="app" id="app">
  <div class="top">
    <div>
      <div class="title">SMART E-CAR DASHBOARD</div>
      <div style="color:#94a3b8;font-size:12px">ESP8266 WiFi + CAN realtime + Self Driven</div>
    </div>
    <div class="status">
      <div class="dot" id="wifiDot"></div>
      <span id="wifiText">ESP OK</span>
    </div>
  </div>

  <div class="grid">
    <div class="card big speed">
      <div class="label">VEHICLE SPEED</div>
      <div><span class="num" id="speed">0</span> <span>KM/H</span></div>
    </div>

    <div class="card">
      <div class="label">FRONT DISTANCE</div>
      <div class="value blue"><span id="distance">999</span> cm</div>
    </div>

    <div class="card">
      <div class="label">CAN LINK</div>
      <div class="value" id="canAlive">OFF</div>
    </div>

    <div class="card">
      <div class="label">OBSTACLE</div>
      <div class="value" id="obstacle">CLEAR</div>
    </div>

    <div class="card">
      <div class="label">AIRBAG LED</div>
      <div class="value" id="airbag">OFF</div>
    </div>

    <div class="card">
      <div class="label">SELF DRIVEN</div>
      <div class="value" id="autoState">IDLE</div>
    </div>

    <div class="card">
      <div class="label">AUTO DISTANCE</div>
      <div class="value blue"><span id="autoDistance">0</span> m</div>
    </div>

    <div class="alert safe" id="alert">ROAD CLEAR - SYSTEM READY</div>

    <div class="card control">
      <div>
        <div class="wheel" id="wheel" style="--rot:0deg"></div>
      </div>

      <div class="dpad">
        <div></div>
        <button id="btnW" onmousedown="pressKey('W')" onmouseup="releaseKey('W')" ontouchstart="pressKey('W')" ontouchend="releaseKey('W')">^</button>
        <div></div>

        <button id="btnA" onmousedown="pressKey('A')" onmouseup="releaseKey('A')" ontouchstart="pressKey('A')" ontouchend="releaseKey('A')">&lt;</button>
        <div></div>
        <button id="btnD" onmousedown="pressKey('D')" onmouseup="releaseKey('D')" ontouchstart="pressKey('D')" ontouchend="releaseKey('D')">&gt;</button>

        <div></div>
        <button id="btnX" onmousedown="pressKey('X')" onmouseup="releaseKey('X')" ontouchstart="pressKey('X')" ontouchend="releaseKey('X')">v</button>
        <div></div>
      </div>

      <div>
        <button class="auto" onclick="autoStart()">SELF DRIVEN</button>
        <button class="autoStop" onclick="autoStop()">STOP AUTO</button>
        <button class="horn" onmousedown="hornOn()" onmouseup="hornOff()" ontouchstart="hornOn()" ontouchend="hornOff()">CLAXON C</button>
        <button class="stop" onclick="sendStop()">STOP</button>
      </div>
    </div>
  </div>
</div>

<script>
let unlocked=false;
let keys={W:false,X:false,A:false,D:false};
let lastCmd="";
let horn=false;

function unlock(){
  let c=document.getElementById("code").value;
  if(c==="1234"){
    unlocked=true;
    document.getElementById("login").style.display="none";
    document.getElementById("app").style.display="block";
    startDashboard();
  }else{
    document.getElementById("err").innerText="Code incorrect";
  }
}

function send(path){
  fetch(path+"?t="+Date.now(),{cache:"no-store"}).catch(()=>{});
}

function autoStart(){
  send("/AUTO_START");
}

function autoStop(){
  send("/AUTO_STOP");
}

function buildCmd(){
  let cmd="";
  if(keys.W)cmd+="/W";
  if(keys.X)cmd+="/X";
  if(keys.A)cmd+="/A";
  if(keys.D)cmd+="/D";
  if(cmd==="")cmd="/STOP";
  return cmd;
}

function updateUIButtons(){
  document.getElementById("btnW").classList.toggle("active",keys.W);
  document.getElementById("btnX").classList.toggle("active",keys.X);
  document.getElementById("btnA").classList.toggle("active",keys.A);
  document.getElementById("btnD").classList.toggle("active",keys.D);

  let rot=0;
  if(keys.A)rot=-40;
  if(keys.D)rot=40;
  document.getElementById("wheel").style.setProperty("--rot",rot+"deg");
}

function sendDrive(){
  let cmd=buildCmd();
  if(cmd!==lastCmd){
    lastCmd=cmd;
    send(cmd);
  }
  updateUIButtons();
}

function pressKey(k){
  keys[k]=true;
  sendDrive();
}

function releaseKey(k){
  keys[k]=false;
  sendDrive();
}

function sendStop(){
  keys={W:false,X:false,A:false,D:false};
  lastCmd="";
  send("/STOP");
  updateUIButtons();
}

function hornOn(){
  if(!horn){
    horn=true;
    send("/HORN_ON");
  }
}

function hornOff(){
  if(horn){
    horn=false;
    send("/HORN_OFF");
  }
}

document.addEventListener("keydown",e=>{
  if(!unlocked)return;
  if(e.repeat)return;

  if(e.key==="ArrowUp"){e.preventDefault();pressKey("W")}
  else if(e.key==="ArrowDown"){e.preventDefault();pressKey("X")}
  else if(e.key==="ArrowLeft"){e.preventDefault();pressKey("A")}
  else if(e.key==="ArrowRight"){e.preventDefault();pressKey("D")}
  else if(e.key.toLowerCase()==="c"){hornOn()}
});

document.addEventListener("keyup",e=>{
  if(!unlocked)return;

  if(e.key==="ArrowUp"){e.preventDefault();releaseKey("W")}
  else if(e.key==="ArrowDown"){e.preventDefault();releaseKey("X")}
  else if(e.key==="ArrowLeft"){e.preventDefault();releaseKey("A")}
  else if(e.key==="ArrowRight"){e.preventDefault();releaseKey("D")}
  else if(e.key.toLowerCase()==="c"){hornOff()}
});

function setWifi(ok){
  document.getElementById("wifiDot").classList.toggle("off",!ok);
  document.getElementById("wifiText").innerText=ok?"ESP OK":"ESP OFF";
}

function updateStatus(d){
  document.getElementById("speed").innerText=d.vitesse;
  document.getElementById("distance").innerText=d.distance;

  let can=document.getElementById("canAlive");
  can.innerText=d.canAlive?"ON":"OFF";
  can.className=d.canAlive?"value green":"value red";

  let obs=document.getElementById("obstacle");
  obs.innerText=d.obstacle?"DETECTED":"CLEAR";
  obs.className=d.obstacle?"value red":"value green";

  let air=document.getElementById("airbag");
  air.innerText=d.airbag?"ACTIVE":"OFF";
  air.className=d.airbag?"value red":"value green";

  let auto=document.getElementById("autoState");
  auto.innerText=d.autoState;
  auto.className=d.autoMode?"value orange":"value green";

  document.getElementById("autoDistance").innerText=d.autoDistance;

  let alert=document.getElementById("alert");

  if(!d.canAlive){
    alert.innerText="CAN CONNECTION LOST - ESP1 DATA NOT RECEIVED";
    alert.className="alert danger";
  }
  else if(d.airbag){
    alert.innerText="AIRBAG ACTIVE - VEHICLE STOPPED";
    alert.className="alert danger";
  }
  else if(d.obstacle){
    alert.innerText="OBSTACLE DETECTED - AUTOMATIC BRAKING";
    alert.className="alert danger";
  }
  else if(d.autoMode){
    alert.innerText="SELF DRIVEN ACTIVE - " + d.autoState;
    alert.className="alert safe";
  }
  else{
    alert.innerText="ROAD CLEAR - SYSTEM READY";
    alert.className="alert safe";
  }
}

function pollStatus(){
  fetch("/status?t="+Date.now(),{cache:"no-store"})
  .then(r=>r.json())
  .then(d=>{
    setWifi(true);
    updateStatus(d);
  })
  .catch(()=>{
    setWifi(false);
  });
}

function startDashboard(){
  pollStatus();
  setInterval(pollStatus,700);
}
</script>

</body>
</html>
)rawliteral";

// =====================================================
// HTTP
// =====================================================
void sendDashboard(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Cache-Control: no-store");
  client.println("Connection: close");
  client.println();
  client.print(FPSTR(DASHBOARD_HTML));
}

void sendSimpleResponse(WiFiClient &client, const char* text) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.println("Cache-Control: no-store");
  client.println("Connection: close");
  client.println();
  client.println(text);
}

String autoStateText() {
  if (autoState == AUTO_IDLE) return "IDLE";
  if (autoState == AUTO_FORWARD_50) return "FORWARD 50M";
  if (autoState == AUTO_TURN_RIGHT) return "TURN RIGHT";
  if (autoState == AUTO_FORWARD_40) return "FORWARD 40M";
  if (autoState == AUTO_DONE) return "DONE";
  if (autoState == AUTO_SAFETY_STOP) return "SAFETY STOP";
  return "UNKNOWN";
}

void sendStatus(WiFiClient &client) {
  unsigned long lastCanMs = millis() - lastCanReceive;
  bool canAlive = (lastCanReceive > 0 && lastCanMs < 1500);

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: application/json");
  client.println("Cache-Control: no-store, no-cache, must-revalidate, max-age=0");
  client.println("Pragma: no-cache");
  client.println("Expires: 0");
  client.println("Connection: close");
  client.println();

  client.print("{");

  client.print("\"obstacle\":");
  client.print(obstacle ? "true" : "false");

  client.print(",\"airbag\":");
  client.print(airbag ? "true" : "false");

  client.print(",\"distance\":");
  client.print(distanceRecu);

  client.print(",\"vitesse\":");
  client.print(vitesseRecu);

  client.print(",\"canAlive\":");
  client.print(canAlive ? "true" : "false");

  client.print(",\"lastCanMs\":");
  client.print(lastCanMs);

  client.print(",\"autoMode\":");
  client.print(autoMode ? "true" : "false");

  client.print(",\"autoState\":\"");
  client.print(autoStateText());
  client.print("\"");

  client.print(",\"autoDistance\":");
  client.print(autoDistance, 1);

  client.print("}");
}

// =====================================================
// MOTEURS
// =====================================================
void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  digitalWrite(ENA_ENB, LOW);
}

void updateMotors() {
  if (obstacle || airbag) {
    stopMotors();
    return;
  }

  int leftSpeed = 0;
  int rightSpeed = 0;

  if (forwardState) {
    leftSpeed += 1;
    rightSpeed += 1;
  }

  if (backwardState) {
    leftSpeed -= 1;
    rightSpeed -= 1;
  }

  if (rightState) {
    leftSpeed += 1;
    rightSpeed -= 1;
  }

  if (leftState) {
    leftSpeed -= 1;
    rightSpeed += 1;
  }

  if (leftSpeed == 0 && rightSpeed == 0) {
    stopMotors();
    return;
  }

  digitalWrite(ENA_ENB, HIGH);

  if (leftSpeed > 0) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  } else if (leftSpeed < 0) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
  }

  if (rightSpeed > 0) {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
  } else if (rightSpeed < 0) {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
  } else {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
  }
}

// =====================================================
// SELF DRIVEN
// =====================================================
void startAutoDrive() {
  autoMode = true;
  autoState = AUTO_FORWARD_50;
  autoDistance = 0.0;
  lastAutoUpdate = millis();
  autoTurnStart = 0;

  forwardState = true;
  backwardState = false;
  leftState = false;
  rightState = false;
}

void stopAutoDrive() {
  autoMode = false;
  autoState = AUTO_IDLE;
  autoDistance = 0.0;

  forwardState = false;
  backwardState = false;
  leftState = false;
  rightState = false;

  stopMotors();
}

void updateAutoDrive() {
  if (!autoMode) return;

  if (obstacle || airbag) {
    autoState = AUTO_SAFETY_STOP;
    forwardState = false;
    backwardState = false;
    leftState = false;
    rightState = false;
    stopMotors();
    return;
  }

  unsigned long now = millis();
  float dt = (now - lastAutoUpdate) / 1000.0;
  lastAutoUpdate = now;

  float vitesse_ms = vitesseRecu / 3.6;

  if (autoState == AUTO_FORWARD_50) {
    autoDistance += vitesse_ms * dt;

    forwardState = true;
    backwardState = false;
    leftState = false;
    rightState = false;

    if (autoDistance >= AUTO_DISTANCE_1) {
      autoState = AUTO_TURN_RIGHT;
      autoTurnStart = now;
      autoDistance = 0.0;
    }
  }

  else if (autoState == AUTO_TURN_RIGHT) {
    forwardState = false;
    backwardState = false;
    leftState = false;
    rightState = true;

    if (now - autoTurnStart >= AUTO_TURN_TIME) {
      autoState = AUTO_FORWARD_40;
      autoDistance = 0.0;
    }
  }

  else if (autoState == AUTO_FORWARD_40) {
    autoDistance += vitesse_ms * dt;

    forwardState = true;
    backwardState = false;
    leftState = false;
    rightState = false;

    if (autoDistance >= AUTO_DISTANCE_2) {
      autoState = AUTO_DONE;
      autoMode = false;

      forwardState = false;
      backwardState = false;
      leftState = false;
      rightState = false;

      stopMotors();
    }
  }
}

// =====================================================
// CAN
// =====================================================
void readCAN() {
  if (CAN_MSGAVAIL == CAN.checkReceive()) {
    long unsigned int rxId;
    byte len = 0;
    byte buf[8];

    CAN.readMsgBuf(&rxId, &len, buf);

    if (rxId == 0x200 && len >= 6) {
      lastCanReceive = millis();

      obstacle = (buf[0] == 1);
      airbag   = (buf[1] == 1);

      distanceRecu = word(buf[2], buf[3]);
      vitesseRecu  = word(buf[4], buf[5]);

      digitalWrite(AIRBAG_LED_PIN, airbag ? HIGH : LOW);
    }
  }
}

// =====================================================
// CLIENT HTTP
// =====================================================
void handleClient() {
  WiFiClient client = server.available();
  if (!client) return;

  client.setTimeout(300);

  String req = client.readStringUntil('\n');
  client.flush();

  if (req.indexOf("GET / ") != -1 || req.indexOf("GET /index.html") != -1) {
    sendDashboard(client);
    client.stop();
    return;
  }

  if (req.indexOf("GET /status") != -1) {
    sendStatus(client);
    client.stop();
    return;
  }

  if (req.indexOf("GET /AUTO_START") != -1) {
    startAutoDrive();
    sendSimpleResponse(client, "AUTO START");
    client.stop();
    return;
  }

  if (req.indexOf("GET /AUTO_STOP") != -1) {
    stopAutoDrive();
    sendSimpleResponse(client, "AUTO STOP");
    client.stop();
    return;
  }

  if (req.indexOf("GET /HORN_ON") != -1) {
    digitalWrite(BUZZER_PIN, HIGH);
    sendSimpleResponse(client, "HORN ON");
    client.stop();
    return;
  }

  if (req.indexOf("GET /HORN_OFF") != -1) {
    digitalWrite(BUZZER_PIN, LOW);
    sendSimpleResponse(client, "HORN OFF");
    client.stop();
    return;
  }

  if (req.indexOf("GET /STOP") != -1) {
    stopAutoDrive();
    sendSimpleResponse(client, "STOP");
    client.stop();
    return;
  }

  if (!autoMode) {
    forwardState = false;
    backwardState = false;
    leftState = false;
    rightState = false;

    if (!obstacle && !airbag) {
      if (req.indexOf("/W") != -1) forwardState = true;
      if (req.indexOf("/X") != -1) backwardState = true;
      if (req.indexOf("/A") != -1) leftState = true;
      if (req.indexOf("/D") != -1) rightState = true;
    }
  }

  updateMotors();
  sendSimpleResponse(client, "CMD OK");
  client.stop();
}

// =====================================================
// SETUP
// =====================================================
void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA_ENB, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(AIRBAG_LED_PIN, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(AIRBAG_LED_PIN, LOW);

  stopMotors();

  while (CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) != CAN_OK) {
    delay(500);
  }

  CAN.setMode(MCP_NORMAL);

  WiFi.mode(WIFI_AP);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, password, 1, false, 4);

  server.begin();
}

// =====================================================
// LOOP
// =====================================================
void loop() {
  readCAN();

  updateAutoDrive();

  updateMotors();

  handleClient();

  delay(1);
}
