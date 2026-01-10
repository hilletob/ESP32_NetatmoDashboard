# ESP32 Wetter-Dashboard - Anleitung für Windows 10

Diese Anleitung hilft dir, die Firmware für das ESP32 Wetter-Dashboard zu kompilieren und auf den ESP32 hochzuladen.

## Inhaltsverzeichnis
1. [Benötigte Software installieren](#1-benötigte-software-installieren)
2. [Projekt herunterladen](#2-projekt-herunterladen)
3. [Konfigurationsdatei erstellen](#3-konfigurationsdatei-erstellen)
4. [Firmware kompilieren und hochladen](#4-firmware-kompilieren-und-hochladen)
5. [Fehlersuche](#5-fehlersuche)

---

## 1. Benötigte Software installieren

### 1.1 Git installieren

Git wird benötigt, um den Programmcode herunterzuladen.

1. Öffne deinen Webbrowser (z.B. Chrome, Firefox, Edge)
2. Gehe auf die Webseite: https://git-scm.com/download/win
3. Der Download sollte automatisch starten. Falls nicht, klicke auf **"Click here to download manually"**
4. Öffne die heruntergeladene Datei (z.B. `Git-2.xx.x-64-bit.exe`)
5. Klicke dich durch die Installation:
   - **"Next"** bei allen Schritten (die Standard-Einstellungen sind gut)
   - Bei **"Adjusting your PATH environment"** wähle: **"Git from the command line and also from 3rd-party software"**
   - Klicke am Ende auf **"Install"**
6. Nach der Installation klicke auf **"Finish"**

### 1.2 Visual Studio Code installieren

VS Code ist das Programm, mit dem du den Code bearbeiten und die Firmware kompilieren kannst.

1. Gehe auf: https://code.visualstudio.com/
2. Klicke auf **"Download for Windows"** (der große blaue Button)
3. Öffne die heruntergeladene Datei (z.B. `VSCodeUserSetup-x64-x.xx.x.exe`)
4. Akzeptiere die Lizenzvereinbarung und klicke auf **"Weiter"**
5. **Wichtig**: Setze folgende Häkchen:
   - ✅ **"Add 'Open with Code' action to Windows Explorer file context menu"**
   - ✅ **"Add 'Open with Code' action to Windows Explorer directory context menu"**
   - ✅ **"Add to PATH"**
6. Klicke auf **"Weiter"** und dann **"Installieren"**
7. Nach der Installation: Setze das Häkchen bei **"Launch Visual Studio Code"** und klicke auf **"Fertigstellen"**

### 1.3 PlatformIO Extension installieren

PlatformIO ist eine Erweiterung für VS Code, die das Kompilieren für den ESP32 ermöglicht.

1. VS Code sollte jetzt geöffnet sein
2. Klicke auf das **Extensions-Symbol** in der linken Seitenleiste (oder drücke `Strg+Shift+X`)
   - Das Symbol sieht aus wie 4 Quadrate, wobei eines etwas abgesetzt ist
3. Gib in das Suchfeld ein: `platformio`
4. Klicke bei **"PlatformIO IDE"** (von PlatformIO) auf den grünen Button **"Install"**
5. Die Installation dauert 2-5 Minuten. Warte, bis unten rechts steht: **"PlatformIO: Installation completed"**
6. Klicke unten rechts auf **"Reload Now"** um VS Code neu zu starten

---

## 2. Projekt herunterladen

### 2.1 Git Repository klonen

1. Erstelle einen Ordner auf deinem Computer, z.B.:
   - Öffne den **Windows Explorer**
   - Gehe zu `C:\Users\DeinBenutzername\Dokumente`
   - Rechtsklick → **"Neu"** → **"Ordner"**
   - Nenne den Ordner z.B. `ESP32-Projekte`

2. Öffne eine **Eingabeaufforderung** in diesem Ordner:
   - Rechtsklick auf den Ordner `ESP32-Projekte`
   - Wähle **"Git Bash Here"** (falls vorhanden)
   - **ODER** wähle **"In Terminal öffnen"**

3. Gib folgenden Befehl ein und drücke **Enter**:
   ```bash
   git clone https://github.com/DEIN-USERNAME/esp32NetatmoSeeed.git
   ```
   *(Ersetze die URL mit der richtigen Git-URL des Projekts)*

4. Das Projekt wird heruntergeladen. Warte bis der Vorgang abgeschlossen ist.

### 2.2 Projekt in VS Code öffnen

1. Öffne VS Code
2. Klicke auf **"File"** → **"Open Folder..."**
3. Navigiere zu `C:\Users\DeinBenutzername\Dokumente\ESP32-Projekte\esp32NetatmoSeeed`
4. Klicke auf **"Ordner auswählen"**
5. PlatformIO sollte jetzt automatisch das Projekt laden (unten in der blauen Leiste siehst du PlatformIO-Symbole)

---

## 3. Konfigurationsdatei erstellen

Die Konfigurationsdatei enthält deine persönlichen Zugangsdaten (WLAN-Passwort, API-Keys etc.).

### 3.1 Template kopieren

1. In VS Code: Klicke in der linken Seitenleiste auf **"Explorer"** (Ordner-Symbol ganz oben)
2. Navigiere zu `src/config.local.h.template`
3. Rechtsklick auf `config.local.h.template` → **"Copy"**
4. Rechtsklick auf den `src` Ordner → **"Paste"**
5. Benenne die neue Datei um:
   - Rechtsklick auf `config.local.h.template copy` → **"Rename"**
   - Ändere den Namen zu: `config.local.h`

### 3.2 Konfiguration ausfüllen

1. Öffne die Datei `src/config.local.h` mit einem Doppelklick
2. Fülle folgende Werte aus (frage deinen Bruder nach den genauen Daten):

```cpp
// WLAN-Zugangsdaten
#define WIFI_SSID "DeinWLAN-Name"           // Name deines WLANs
#define WIFI_PASSWORD "DeinWLAN-Passwort"   // WLAN-Passwort

// Netatmo API
#define NETATMO_CLIENT_ID "..."             // Von Netatmo
#define NETATMO_CLIENT_SECRET "..."         // Von Netatmo
#define NETATMO_REFRESH_TOKEN "..."         // Von Netatmo
#define NETATMO_DEVICE_ID "..."             // Deine Wetterstation ID

// Gemini AI API
#define GEMINI_API_KEY "..."                // Von Google AI Studio

// Standort
#define LOCATION_NAME "Davos"               // Dein Ort
#define LOCATION_LAT 46.8019                // Breitengrad
#define LOCATION_LON 9.8367                 // Längengrad
#define LOCATION_ALTITUDE 1560              // Höhe in Metern
```

3. Speichere die Datei: `Strg+S`

---

## 4. Firmware kompilieren und hochladen

### 4.1 ESP32 anschließen

1. Verbinde den ESP32 mit einem **USB-Kabel** mit deinem Computer
2. Windows installiert automatisch die Treiber (kann 1-2 Minuten dauern)
3. Überprüfe die Verbindung:
   - Öffne den **Geräte-Manager** (Windows-Taste drücken, dann "Geräte-Manager" eingeben)
   - Klappe **"Anschlüsse (COM & LPT)"** auf
   - Du solltest etwas sehen wie: **"USB-SERIAL CH340 (COM3)"** oder ähnlich
   - Merke dir die **COM-Nummer** (z.B. COM3, COM4, COM5...)

### 4.2 Build durchführen (Kompilieren)

1. In VS Code unten in der blauen Statusleiste findest du mehrere Symbole
2. Klicke auf das **Häkchen-Symbol** ✓ (Build)
3. Unten öffnet sich ein Terminal-Fenster
4. Der Build-Prozess startet:
   - Beim ersten Mal dauert das ca. 5-10 Minuten (Downloads von Bibliotheken)
   - Bei späteren Builds: ca. 30-60 Sekunden
5. Warte bis ganz unten steht: **"SUCCESS"** und grüne Zahlen erscheinen
6. Falls ein Fehler auftritt, siehe [Fehlersuche](#5-fehlersuche)

### 4.3 Upload durchführen

1. Stelle sicher, dass der ESP32 angeschlossen ist
2. Klicke in der blauen Statusleiste auf das **Pfeil-Symbol** → (Upload)
3. Der Code wird kompiliert (falls nötig) und dann hochgeladen
4. Du siehst im Terminal:
   ```
   Writing at 0x00010000... (10 %)
   Writing at 0x00020000... (20 %)
   ...
   Writing at 0x00100000... (100 %)
   Wrote 123456 bytes...
   ```
5. Warte bis steht: **"SUCCESS"**
6. Der ESP32 startet automatisch neu

### 4.4 Monitor (Ausgabe anschauen)

1. Klicke in der blauen Statusleiste auf das **Stecker-Symbol** 🔌 (Serial Monitor)
2. Ein Terminal öffnet sich mit der seriellen Ausgabe
3. Du siehst nun die Debug-Ausgaben des ESP32:
   ```
   [wifi] Connecting to WiFi...
   [wifi] Connected! IP: 192.168.1.123
   [netatmo] Fetching station data...
   [main] Indoor: 21.5°C, 65% rH, 850 ppm CO2
   [display] Updating display...
   [sleep] Going to sleep for 11 minutes
   ```

4. **Monitor beenden**:
   - Klicke unten rechts auf das **Mülleimer-Symbol** im Terminal
   - **ODER** drücke `Strg+C`

---

## 5. Fehlersuche

### Problem: "No such file or directory: config.local.h"

**Lösung**: Du hast vergessen, die Konfigurationsdatei zu erstellen.
- Gehe zurück zu [3. Konfigurationsdatei erstellen](#3-konfigurationsdatei-erstellen)

---

### Problem: "A fatal error occurred: Could not open COM3"

**Ursache**: Der USB-Port ist nicht korrekt erkannt oder bereits in Benutzung.

**Lösung**:
1. Schließe den Serial Monitor (falls offen)
2. Ziehe das USB-Kabel ab und stecke es wieder ein
3. Warte 10 Sekunden
4. Versuche den Upload erneut
5. Falls es nicht funktioniert:
   - Öffne `platformio.ini` im Projekt-Root
   - Füge hinzu:
     ```ini
     upload_port = COM3
     monitor_port = COM3
     ```
   - Ersetze `COM3` mit deinem tatsächlichen Port (siehe Geräte-Manager)

---

### Problem: "error: espcomm_upload_mem failed"

**Ursache**: Der ESP32 ist nicht im Upload-Modus.

**Lösung**:
1. Halte den **BOOT-Button** am ESP32 gedrückt
2. Klicke in VS Code auf Upload
3. Warte bis "Connecting..." erscheint
4. Lasse den BOOT-Button los

---

### Problem: Build dauert ewig oder hängt

**Lösung**:
1. Schließe VS Code komplett
2. Lösche den Ordner `.pio` im Projektverzeichnis:
   - Im Windows Explorer: `C:\Users\DeinBenutzername\Dokumente\ESP32-Projekte\esp32NetatmoSeeed\.pio`
   - Ordner löschen (Rechtsklick → Löschen)
3. Öffne VS Code wieder
4. Starte einen neuen Build

---

### Problem: "No module named 'serial'"

**Ursache**: Python-Abhängigkeiten fehlen.

**Lösung**:
1. Öffne ein **neues Terminal** in VS Code: `Strg+Shift+ö`
2. Führe aus:
   ```bash
   pip install pyserial
   ```

---

### Problem: Bildschirm zeigt nichts an

**Checkliste**:
1. ✅ Ist der ePaper-Display richtig angeschlossen?
2. ✅ Sind die WLAN-Zugangsdaten korrekt?
3. ✅ Hat der ESP32 sich mit dem WLAN verbunden? (Prüfe im Serial Monitor)
4. ✅ Sind die Netatmo API-Daten korrekt?
5. ✅ Warte mindestens 2 Minuten nach dem ersten Start (erste Aktualisierung dauert länger)

---

## Häufig verwendete Befehle

| Aktion | Button in VS Code | Tastenkombination |
|--------|-------------------|-------------------|
| **Build** (Kompilieren) | ✓ Häkchen-Symbol | `Strg+Alt+B` |
| **Upload** (Hochladen) | → Pfeil-Symbol | `Strg+Alt+U` |
| **Monitor** (Ausgabe) | 🔌 Stecker-Symbol | `Strg+Alt+S` |
| **Clean** (Aufräumen) | 🗑️ Mülleimer-Symbol | `Strg+Alt+C` |

---

## Tipps für fortgeschrittene Nutzer

### Projekt aktualisieren (Updates vom Git holen)

1. Öffne ein Terminal in VS Code: `Strg+Shift+ö`
2. Führe aus:
   ```bash
   git pull
   ```
3. Falls Änderungen heruntergeladen wurden, führe einen neuen Build + Upload durch

### PlatformIO Befehle im Terminal

Alternativ zu den Buttons kannst du auch Befehle im Terminal eingeben:

```bash
# Nur kompilieren
pio run

# Kompilieren und hochladen
pio run --target upload

# Serial Monitor öffnen
pio device monitor

# Alles löschen und neu bauen
pio run --target clean
```

---

## Kontakt

Falls du Probleme hast, die hier nicht gelöst werden, frage deinen Bruder oder erstelle ein Issue auf GitHub.

**Viel Erfolg!** 🚀
