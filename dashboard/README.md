# E-Car Remote

Interface web React pour piloter et superviser une voiture electrique miniature ou un prototype embarque. Le projet simule un tableau de bord complet avec telemetrie, capteurs de proximite, camera, controle manuel, auto-parking, alertes et parametres de securite.

Ce depot est organise pour presenter le projet dans un contexte de candidature de stage : code source clair, dependances verrouillees, build reproductible et documentation rapide a lire.

## Apercu

- Authentification demo par PIN (`1234`)
- Dashboard temps reel : batterie, signal, temperature moteur, vitesse et etat des capteurs
- Controle manuel : direction, vitesse, mode de conduite et arret d'urgence
- Visualisation des capteurs : distances avant, arriere, gauche et droite
- Module auto-park avec progression animee
- Flux d'alertes pour obstacles, batterie et signal
- Interface responsive pour desktop et mobile

## Stack

- React
- Vite
- Tailwind CSS
- Framer Motion
- Lucide React

## Installation

```bash
npm install
npm run dev
```

Puis ouvrir l'URL affichee par Vite, generalement :

```text
http://127.0.0.1:5173
```

## Scripts

```bash
npm run dev      # Lance le serveur local
npm run build    # Genere la version production
npm run preview  # Previsualise le build
```

## Structure

```text
src/
  App.jsx              # Application principale et vues
  main.jsx             # Point d'entree React
  styles.css           # Styles globaux et composants visuels
  assets/
    steering-wheel.png # Asset de l'interface de controle
```

## Notes de demonstration

Les donnees capteurs et la telemetrie sont simulees cote frontend afin de demontrer l'ergonomie de l'interface. Le projet peut ensuite etre connecte a une API, un microcontroleur ou un broker temps reel selon le materiel utilise.
