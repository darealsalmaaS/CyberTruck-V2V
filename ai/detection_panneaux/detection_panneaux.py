"""
Détecteur de panneaux routiers — OpenCV
========================================
STOP          : octogone rouge + texte blanc
CÉDEZ         : triangle POINTE EN BAS, rouge plein, triangle blanc intérieur
PASSAGE PIÉTON: triangle POINTE EN HAUT, bordure rouge, intérieur blanc + noir

pip install opencv-python numpy
python detection_panneaux.py
"""

import cv2
import numpy as np
import time

# ─────────────────────────────────────────────
#  PLAGES HSV
# ─────────────────────────────────────────────
ROUGE_BAS_1  = np.array([0,   130,  80])
ROUGE_HAUT_1 = np.array([10,  255, 255])
ROUGE_BAS_2  = np.array([165, 130,  80])
ROUGE_HAUT_2 = np.array([180, 255, 255])

NOIR_BAS  = np.array([0,   0,   0])
NOIR_HAUT = np.array([180, 60, 80])

BLANC_BAS  = np.array([0,  0,  200])
BLANC_HAUT = np.array([180, 35, 255])

AIRE_MIN = 2000
AIRE_MAX = 150000


# ─────────────────────────────────────────────
#  UTILITAIRES
# ─────────────────────────────────────────────
def masque_rouge(hsv):
    m1 = cv2.inRange(hsv, ROUGE_BAS_1, ROUGE_HAUT_1)
    m2 = cv2.inRange(hsv, ROUGE_BAS_2, ROUGE_HAUT_2)
    return cv2.bitwise_or(m1, m2)

def nettoyer(masque, er=1, di=2):
    k = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (5, 5))
    masque = cv2.erode(masque, k, iterations=er)
    masque = cv2.dilate(masque, k, iterations=di)
    return masque

def ratio_couleur(hsv_roi, bas, haut):
    h, w = hsv_roi.shape[:2]
    if h == 0 or w == 0:
        return 0.0
    m = cv2.inRange(hsv_roi, bas, haut)
    return np.count_nonzero(m) / (h * w)

def approx_poly(contour, eps=0.04):
    e = eps * cv2.arcLength(contour, True)
    return cv2.approxPolyDP(contour, e, True)


# ─────────────────────────────────────────────
#  SCORES DE FORME
# ─────────────────────────────────────────────
def score_octogone(contour):
    approx = approx_poly(contour, 0.04)
    n = len(approx)
    if n < 6 or n > 10:
        return 0.0
    aire = cv2.contourArea(contour)
    perim = cv2.arcLength(contour, True)
    compacite = 4 * np.pi * aire / (perim ** 2) if perim > 0 else 0
    score_n = 1.0 - abs(n - 8) / 8.0
    score_c = max(0.0, 1.0 - abs(compacite - 0.90) * 3)
    return 0.5 * score_n + 0.5 * score_c


def analyser_triangle(contour):
    """
    Retourne (score_forme, pointe_en_bas)
    pointe_en_bas=True  → Cédez le passage  (triangle inversé ▽)
    pointe_en_bas=False → Passage piétons   (triangle normal  △)
    """
    approx = approx_poly(contour, 0.06)
    n = len(approx)
    if n < 3 or n > 6:
        return 0.0, False

    score_n = 1.0 - abs(n - 3) / 3.0
    aire = cv2.contourArea(contour)
    perim = cv2.arcLength(contour, True)
    compacite = 4 * np.pi * aire / (perim ** 2) if perim > 0 else 0
    score_c = max(0.0, 1.0 - abs(compacite - 0.60) * 4)

    # Orientation : position du centroïde dans la bounding box
    M = cv2.moments(contour)
    x, y, w, h = cv2.boundingRect(contour)
    pointe_en_bas = False
    if M["m00"] > 0 and h > 0:
        cy = M["m01"] / M["m00"]
        ratio_pos = (cy - y) / h   # 0=haut 1=bas
        # Triangle ▽ (pointe bas) → centroïde dans le tiers HAUT (ratio < 0.45)
        pointe_en_bas = ratio_pos < 0.45

    score = 0.4 * score_n + 0.6 * score_c
    return score, pointe_en_bas


# ─────────────────────────────────────────────
#  CLASSIFICATION
# ─────────────────────────────────────────────
def classifier(contour, hsv_full):
    aire = cv2.contourArea(contour)
    if aire < AIRE_MIN or aire > AIRE_MAX:
        return None, 0

    x, y, w, h = cv2.boundingRect(contour)
    if w == 0 or h == 0:
        return None, 0

    # Ratio d'aspect : écarter les formes très allongées
    ratio_aspect = min(w, h) / max(w, h)
    if ratio_aspect < 0.40:
        return None, 0

    roi = hsv_full[y:y+h, x:x+w]

    r_rouge = ratio_couleur(roi, ROUGE_BAS_1, ROUGE_HAUT_1) + \
              ratio_couleur(roi, ROUGE_BAS_2, ROUGE_HAUT_2)
    r_blanc = ratio_couleur(roi, BLANC_BAS,  BLANC_HAUT)
    r_noir  = ratio_couleur(roi, NOIR_BAS,   NOIR_HAUT)

    s_oct = score_octogone(contour)
    s_tri, pointe_bas = analyser_triangle(contour)

    # ── STOP ────────────────────────────────────────────────────────
    # Octogone rouge avec texte blanc, peu de noir
    score_stop = 0.0
    if s_oct > 0.35:
        score_stop += s_oct * 4.0
        score_stop += r_rouge * 3.0
        score_stop += r_blanc * 1.5
        score_stop += 1.5 if r_rouge > 0.30 else 0.0
        score_stop += 1.0 if r_noir  < 0.10 else 0.0

    # ── CÉDEZ LE PASSAGE ────────────────────────────────────────────
    # Triangle ▽ POINTE EN BAS, rouge dominant + blanc intérieur, pas de noir
    score_cedez = 0.0
    if s_tri > 0.35 and pointe_bas:
        score_cedez += s_tri  * 4.0
        score_cedez += 3.0                          # bonus orientation correcte
        score_cedez += r_rouge * 2.5
        score_cedez += r_blanc * 2.0
        score_cedez += 1.5 if r_rouge > 0.25 else 0.0
        score_cedez += 1.0 if r_noir  < 0.08 else 0.0

    # ── PASSAGE PIÉTONS ─────────────────────────────────────────────
    # Triangle △ POINTE EN HAUT, bordure rouge + silhouette NOIRE intérieure
    score_pieton = 0.0
    if s_tri > 0.35 and not pointe_bas:
        score_pieton += s_tri  * 4.0
        score_pieton += 3.0                         # bonus orientation correcte
        score_pieton += r_rouge * 1.5
        score_pieton += r_blanc * 1.5
        score_pieton += r_noir  * 4.0              # silhouette noire = clé
        score_pieton += 2.0 if r_noir > 0.05 else 0.0

    scores = {
        "STOP":             score_stop,
        "CEDEZ LE PASSAGE": score_cedez,
        "PASSAGE PIETONS":  score_pieton,
    }

    meilleur = max(scores, key=scores.get)
    val = scores[meilleur]

    # Seuils stricts — évite les faux positifs
    SEUILS = {
        "STOP":             6.0,
        "CEDEZ LE PASSAGE": 8.0,
        "PASSAGE PIETONS":  8.0,
    }

    if val < SEUILS[meilleur]:
        return None, 0

    conf = min(99, int((val / 18.0) * 100))
    return meilleur, conf


# ─────────────────────────────────────────────
#  DESSIN
# ─────────────────────────────────────────────
COULEURS = {
    "STOP":             (0,  30, 210),
    "CEDEZ LE PASSAGE": (0,  30, 210),
    "PASSAGE PIETONS":  (30, 30, 210),
}

def dessiner(frame, contour, label, conf):
    x, y, w, h = cv2.boundingRect(contour)
    col = COULEURS.get(label, (0, 200, 0))
    cv2.rectangle(frame, (x, y), (x+w, y+h), col, 2)
    cv2.drawContours(frame, [contour], -1, col, 2)
    texte = f"{label}  {conf}%"
    (tw, th), bl = cv2.getTextSize(texte, cv2.FONT_HERSHEY_SIMPLEX, 0.56, 1)
    ty = max(y - 8, th + 4)
    cv2.rectangle(frame, (x, ty-th-4), (x+tw+6, ty+bl), col, -1)
    cv2.putText(frame, texte, (x+3, ty-2),
                cv2.FONT_HERSHEY_SIMPLEX, 0.56, (255,255,255), 1, cv2.LINE_AA)

def hud(frame, detections, fps):
    H, W = frame.shape[:2]
    ov = frame.copy()
    cv2.rectangle(ov, (0,0), (W, 38), (30,30,30), -1)
    cv2.addWeighted(ov, 0.6, frame, 0.4, 0, frame)
    cv2.putText(frame, "DETECTION PANNEAUX ROUTIERS", (10, 24),
                cv2.FONT_HERSHEY_SIMPLEX, 0.65, (255,255,255), 1, cv2.LINE_AA)
    cv2.putText(frame, f"FPS {fps:.1f}", (W-85, 24),
                cv2.FONT_HERSHEY_SIMPLEX, 0.55, (180,220,180), 1, cv2.LINE_AA)
    if detections:
        for i, (lbl, cf) in enumerate(detections):
            col = COULEURS.get(lbl, (0,200,0))
            yt = H - 20 - i*28
            cv2.rectangle(frame, (8, yt-18), (300, yt+6), (20,20,20), -1)
            cv2.rectangle(frame, (8, yt-18), (300, yt+6), col, 1)
            cv2.putText(frame, f"[OK] {lbl}  {cf}%", (14, yt),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.52, col, 1, cv2.LINE_AA)
    else:
        yt = H - 20
        cv2.rectangle(frame, (8, yt-18), (220, yt+6), (20,20,20), -1)
        cv2.putText(frame, "Aucun panneau detecte", (14, yt),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.48, (120,120,120), 1, cv2.LINE_AA)
    cv2.putText(frame, "Q=quitter  D=debug  S=screenshot", (W-315, H-10),
                cv2.FONT_HERSHEY_SIMPLEX, 0.42, (100,100,100), 1, cv2.LINE_AA)


# ─────────────────────────────────────────────
#  BOUCLE PRINCIPALE
# ─────────────────────────────────────────────
def main():
    print("=" * 55)
    print("  Détecteur de panneaux — OpenCV (sans YOLO)")
    print("  STOP | CÉDEZ LE PASSAGE | PASSAGE PIÉTONS")
    print("=" * 55)
    print("  Panneaux attendus :")
    print("  STOP          : octogone rouge")
    print("  CÉDEZ         : triangle ▽ pointe en BAS, rouge+blanc")
    print("  PASSAGE PIÉT. : triangle △ pointe en HAUT, rouge+noir")
    print()
    print("  Q=quitter  D=debug masques  S=screenshot\n")

    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("[ERREUR] Caméra introuvable.")
        return

    cap.set(cv2.CAP_PROP_FRAME_WIDTH,  1280)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)
    cap.set(cv2.CAP_PROP_FPS, 30)

    mode_debug = False
    screenshots = 0
    t_prev = time.time()
    fps = 0.0

    print("[OK] Caméra ouverte — montrez un panneau à la caméra !\n")

    while True:
        ret, frame = cap.read()
        if not ret:
            break

        now = time.time()
        fps = 0.9 * fps + 0.1 / max(now - t_prev, 1e-6)
        t_prev = now

        flou = cv2.GaussianBlur(frame, (5, 5), 0)
        hsv  = cv2.cvtColor(flou, cv2.COLOR_BGR2HSV)

        # On travaille uniquement sur les zones rouges
        m_rouge = nettoyer(masque_rouge(hsv), er=1, di=2)

        contours, _ = cv2.findContours(
            m_rouge, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

        detections = []
        for cnt in contours:
            label, conf = classifier(cnt, hsv)
            if label:
                dessiner(frame, cnt, label, conf)
                detections.append((label, conf))

        hud(frame, detections, fps)
        cv2.imshow("Detection Panneaux", frame)

        if mode_debug:
            m_noir  = nettoyer(cv2.inRange(hsv, NOIR_BAS,  NOIR_HAUT),  1, 1)
            m_blanc = nettoyer(cv2.inRange(hsv, BLANC_BAS, BLANC_HAUT), 1, 1)
            debug = np.zeros_like(frame)
            debug[:,:,2] = m_rouge   # canal R = rouge
            debug[:,:,0] = m_blanc   # canal B = blanc
            debug[:,:,1] = m_noir    # canal G = noir
            cv2.imshow("Debug  R=rouge  B=blanc  G=noir", debug)

        k = cv2.waitKey(1) & 0xFF
        if k == ord('q'):
            print("\n[Fin]")
            break
        elif k == ord('d'):
            mode_debug = not mode_debug
            if not mode_debug:
                cv2.destroyWindow("Debug  R=rouge  B=blanc  G=noir")
            print(f"[Debug] {'ON' if mode_debug else 'OFF'}")
        elif k == ord('s'):
            nom = f"capture_{screenshots:03d}.jpg"
            cv2.imwrite(nom, frame)
            screenshots += 1
            print(f"[Screenshot] {nom}")

    cap.release()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
