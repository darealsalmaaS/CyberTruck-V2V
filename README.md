# Cyber Truck — Système de communication V2V à échelle réduite

Projet de module — **ENSA Marrakech**
Encadrant : M. Hatim Anas
Équipe : Lachab Fayza · Lafraoui Kaoutar · Laguida Omar · Sadgal Salma · Ghayate Oualid

---

## Présentation

Cyber Truck est une maquette de communication **Vehicle-to-Vehicle (V2V)** validée à échelle
réduite. Deux véhicules échangent sans fil via **ESP-NOW**, tandis qu'à l'intérieur du
véhicule B, deux microcontrôleurs communiquent par **bus CAN**. Un module de **vision par
ordinateur** (détection de panneaux) complète l'ensemble côté PC.

Le scénario principal (Situation 1 — freinage d'urgence) : le véhicule A, placé devant
le véhicule B mais hors de portée de son capteur, déclenche un freinage. Le signal est
transmis en ESP-NOW à B, qui s'arrête à son tour.

## Architecture

```
   VEHICULE A                         VEHICULE B
 ┌────────────┐                ┌──────────────────────────────────────────┐
 │  ESP8266   │   ESP-NOW      │  ESP CAPTEUR  ──CAN 0x200──►  ESP MOTEUR  │
 │  bouton +  │ ─────────────► │  HC-SR04                       L298N      │
 │  potentio. │   (V2V)        │  FC-03 (vitesse)               2 moteurs  │
 └────────────┘                │  OLED                          buzzer/LED │
                               │                                dashboard  │
                               └──────────────────────────────────────────┘

   PC (webcam) ── OpenCV ──► détection panneaux (STOP / CÉDEZ / PIÉTONS)
```

- **Inter-véhicule** : ESP-NOW exclusivement (aucun fil entre A et B). C'est l'**ESP moteur**
  de B qui reçoit le signal de freinage de A (voir `espB_moteur_v2v`).
- **Intra-véhicule B** : bus CAN (MCP2515 + TJA1050) entre les deux ESP8266.
  ⚠️ Le CAN ne sort jamais du véhicule : CANH/CANL ne relient jamais A et B.
- **Vision** : exécutée sur PC (l'ESP8266 n'a pas la puissance pour l'inférence).

## Deux versions de l'ESP moteur

Le dépôt contient volontairement deux variantes du firmware moteur :

| Dossier | Contenu | Usage |
|---------|---------|-------|
| `espB_moteur` | Version complète : dashboard web, mode self-driven, réception CAN 0x200 | Démo dashboard / conduite autonome |
| `espB_moteur_v2v` | Moteur + réception **ESP-NOW** (freinage V2V depuis le véhicule A) | Scénario V2V (Situation 1) |

> La MAC du récepteur ESP-NOW (`DC:4F:22:58:CE:71`) est celle de l'**ESP moteur**.
> Elle est renseignée dans le code du véhicule A (`macESP_B[]`).

## Contenu du dépôt

```
CyberTruck-V2V/
├── firmware/
│   ├── VehiculeA/
│   │   └── vehiculeA_v2v/        Bouton BRAKE + potentiomètre (vitesse) → ESP-NOW
│   └── VehiculeB/
│       ├── espB_capteur/         HC-SR04 + FC-03 + OLED → CAN 0x200
│       ├── espB_moteur/          Version complète : CAN + L298N + dashboard web + self-driven
│       └── espB_moteur_v2v/      Version V2V : moteur + réception ESP-NOW (freinage depuis A)
├── ai/
│   └── detection_panneaux/       Détection de panneaux (OpenCV) — tourne sur PC
└── dashboard/                    Tableau de bord React/Vite (vitrine, données simulées)
```

## Matériel

| Véhicule | Composants |
|----------|------------|
| A | 1× ESP8266, potentiomètre, bouton poussoir |
| B | 2× ESP8266, 2× MCP2515+TJA1050, HC-SR04, FC-03, OLED SSD1306, L298N, 2 moteurs DC, buzzer |
| PC | webcam (module vision OpenCV) |

## Bus CAN — trame 0x200 (capteur → moteur, 6 octets)

| Octet | Contenu |
|-------|---------|
| 0 | obstacle (0/1) |
| 1 | airbag (0/1) |
| 2-3 | distance (cm, 16 bits) |
| 4-5 | vitesse (km/h, 16 bits) |

## Mise en route

### Firmware (Arduino IDE)
1. Installer le support **ESP8266** (Boards Manager).
2. Installer les bibliothèques : `mcp_can` (coryjfowler), `Adafruit GFX`, `Adafruit SSD1306`.
3. Carte : **NodeMCU 1.0 (ESP-12E)**.
4. Flasher chaque `.ino` sur l'ESP correspondant.
   - ⚠️ ESP-NOW exige le **même canal WiFi** sur A et B (canal 1).
   - ⚠️ La MAC du récepteur ESP-NOW doit être renseignée dans le code A.
   - ⚠️ Débrancher les modules sur TX/RX pendant l'upload si nécessaire.

### Module AI (PC)
```bash
cd ai/detection_panneaux
pip install -r requirements.txt
python detection_panneaux.py
```
Détecte STOP, CÉDEZ LE PASSAGE et PASSAGE PIÉTONS par couleur + forme.
Touches : `Q` quitter · `D` masques debug · `S` screenshot.

### Dashboard React
```bash
cd dashboard
npm install
npm run dev
```
> Note : ce dashboard utilise des **données simulées**. Le tableau de bord
> réellement connecté au véhicule est celui **embarqué dans l'ESP moteur**
> (accessible via http://192.168.4.1, code PIN 1234).

## Accès au véhicule
Se connecter au point d'accès WiFi **VOITURE_WIFI** (mot de passe `12345678`),
puis ouvrir `http://192.168.4.1` — code PIN : `1234`.
