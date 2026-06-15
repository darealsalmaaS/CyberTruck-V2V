# Module AI — Détection de panneaux routiers (OpenCV)

Détecteur de panneaux par **vision classique** (OpenCV, sans réseau de neurones).
Reconnaît trois panneaux à partir de la couleur (HSV) et de la forme du contour :

| Panneau | Critère |
|---------|---------|
| STOP | octogone rouge, texte blanc |
| CÉDEZ LE PASSAGE | triangle pointe en bas (▽), rouge + blanc |
| PASSAGE PIÉTONS | triangle pointe en haut (△), bordure rouge + silhouette noire |

## Où ça s'exécute

⚠️ Ce module tourne sur un **PC** (avec webcam), **pas sur l'ESP8266** : l'ESP8266
n'a pas la puissance de calcul pour l'inférence vision. Dans une intégration complète,
le PC ferait la détection puis enverrait une commande au véhicule (par WiFi), mais le
script actuel est autonome (affichage à l'écran uniquement).

## Installation

```bash
pip install -r requirements.txt
python detection_panneaux.py
```

## Commandes (fenêtre OpenCV)

- `Q` : quitter
- `D` : afficher/masquer les masques de debug (rouge / blanc / noir)
- `S` : capturer une image (`capture_XXX.jpg`)

## Réglages utiles

- `AIRE_MIN` / `AIRE_MAX` : taille des contours retenus (filtre le bruit et les objets trop gros).
- `SEUILS` : seuils de confiance par panneau — augmenter pour réduire les faux positifs.
- Plages HSV (`ROUGE_*`, `BLANC_*`, `NOIR_*`) : à ajuster selon l'éclairage de la salle.
