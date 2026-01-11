# 🇮🇹 SPYHOLE - Sistema di Controllo Accessi con Riconoscimento Facciale

**Progetto di Architettura e Progetto di Calcolatori (APC)**

**Autori**: Andrea Esposito (M63001650) | Francesco Riccio (M63001646)  
**Professore**: Prof. Nicola Mazzocca

---

## Panoramica del Sistema

**Spyhole** è un sistema di controllo accessi a **due fattori** che combina:
- **Riconoscimento facciale** tramite ESP32-CAM e AI
- **Codice PIN Bluetooth** come fallback sicuro
- **Dashboard web** per monitorare accessi in tempo reale
- **Gestione utenti** con registrazione e autenticazione

L'intero sistema è gestito da una **dashboard web** che permette di visualizzare gli accessi in tempo reale e gestire gli utenti registrati.

---

## Architettura del Sistema

```
┌─────────────────────────────────────────────────────┐
│           SPYHOLE - Sistema Completo               │
├─────────────────────────────────────────────────────┤
│                                                     │
│  Hardware:                                         │
│  ├─ ESP32-CAM (riconoscimento facciale)           │
│  ├─ STM32F303 Discovery (controller principale)    │
│  ├─ Modulo Bluetooth HC-05 (comandi PIN)          │
│  ├─ Servomotore SG90 (sblocco serratura)          │
│  └─ LED RGB + Resistenze (feedback visivo)        │
│                                                     │
│  Backend:                                          │
│  ├─ Flask (API REST + Dashboard)                  │
│  ├─ SQLite (database utenti & face encodings)     │
│  └─ face_recognition library (dlib)               │
│                                                     │
│  Frontend:                                         │
│  ├─ Tailwind CSS (styling)                        │
│  └─ Jinja2 (template engine)                      │
│                                                     │
└─────────────────────────────────────────────────────┘
```

---

## Flusso di Lavoro

### 1️⃣ **Registrazione/Login**
L'utente si autentica nella webapp o viene registrato per la prima volta.
- Email + Password → credenziali
- Foto del volto → face encoding (vettore 128D con dlib)

### 2️⃣ **Richiesta di Accesso**
L'utente invia il comando **"Access"** tramite interfaccia Bluetooth dal proprio smartphone.

### 3️⃣ **Riconoscimento Facciale**
- ESP32-CAM cattura l'immagine del volto
- Confronta con i face encodings salvati nel database
- Se match > 0.6 (soglia): ✅ **ACCESSO CONCESSO**
- Se match < 0.6: ❌ **ACCESSO NEGATO**

### 4️⃣ **Sblocco Serratura**
Se il riconoscimento è positivo:
- Servomotore SG90 si attiva (PWM 50 Hz)
- Serratura si sblocca per 3-5 secondi
- LED RGB acceso (verde/blu)

### 5️⃣ **Lockout di Sicurezza**
In caso di troppi errori (> 3 tentativi):
- Sistema entra in lockout per 10 secondi
- LED rosso lampeggiante
- Ulteriori tentativi bloccati fino al ripristino

---

## 🔄 Macchina a Stati Finiti

```
┌──────────────┐
│   IDLE       │◄─── Reset / Lockout scaduto
└──────┬───────┘
       │ Comando "Access" ricevuto
       ▼
┌──────────────┐
│   ACQUIRING  │ Richiede foto a ESP32-CAM
└──────┬───────┘
       │
       ▼
┌──────────────────────┐
│ FACE_RECOGNITION     │ Confronta con database
└──────┬───────────────┘
       │
   ┌───┴────────────────────┐
   │ MATCH?                 │
   └───┬─────────┬──────────┘
       │ YES     │ NO
       ▼         ▼
    ┌──────┐  ┌─────────────┐
    │UNLOCK│  │ERROR_RETRY  │ (tentativi < 3)
    └──────┘  └──────┬──────┘
       │             │
       │      ┌──────┴─────────────┐
       │      │ Tentativi >= 3?    │
       │      └──────┬──────┬──────┘
       │             │YES   │NO
       │             ▼      ▼
       │          ┌──────┐  IDLE
       │          │LOCKOUT│ (attendi)
       │          └──────┘
       │             │
       ▼             ▼
    ┌────────────────────┐
    │ IDLE (ripristino)  │
    └────────────────────┘
```

---

## Comunicazione UART & Bluetooth

### Configurazione Hardware
- **STM32 ↔ ESP32-CAM**: UART @ **115200 baud** (immagini ad alta velocità)
- **STM32 ↔ HC-05**: UART @ **9600 baud** (comandi Bluetooth)
- **GND**: Comune a tutti i dispositivi
- **TX/RX logici**: 3.3V (compatibili nativamente)
- **Alimentazione**: 5V da STM32 → regolatori interni

### Flusso di Comunicazione
```
Smartphone (Bluetooth)
    │
    ▼
HC-05 (UART @ 9600 baud)
    │
    ├─────────────→ STM32F303 ←─────────────┐
    │              (Controller)              │
    │                  │                     │
    └──────────────────┼─────────────────────┘
                       │
                       ▼
            ESP32-CAM (UART @ 115200 baud)
                       │
                       ▼
                  📸 Foto face
                       │
                       ▼
                  💻 Server Flask
                  (face_recognition)
                       │
                       ▼
                   Match Y/N
```

---

## Moduli Chiave

### **Bluetooth HC-05**
- Riceve comandi via Serial Port Profile (SPP)
- Comportamento: porta seriale wireless
- Baud rate: 9600 bps (stabile)
- Alimentazione: 5V, logica 3.3V
- Configurabile tramite AT commands

### **ESP32-CAM**
- **Processore**: Dual-core ESP32-S @ 240 MHz
- **Memoria**: 520 KB SRAM + 4 MB PSRAM + 4 MB Flash
- **Fotocamera**: OV2640 (1600×1200 px)
- **Formati**: JPEG, YUV422, RGB565
- **Connettività**: Wi-Fi 802.11 b/g/n + Bluetooth 4.2
- **Consumo**: 160-260 mA (trasmissione)
- **Programmazione**: USB-Seriale (senza USB nativo)

### **Servomotore SG90**
- **Tensione**: 4.8 - 6.0 V
- **Rotazione**: ~180° (90° per lato)
- **Controllo**: PWM @ 50 Hz
- **Impulso**: 500-2400 µs (1500 µs = centro)
- **Coppia**: ~1.8 kg·cm @ 4.8 V
- **Velocità**: ~0.1 s/60° (senza carico)
- **Consumo**: <10 mA a riposo, 100-250 mA in movimento

**PWM Control**:
```
Frequenza: 50 Hz (20 ms per ciclo)
├─ 1000 µs → Posizione sinistra (fine corsa)
├─ 1500 µs → Centro
└─ 2000 µs → Posizione destra (fine corsa)
```

---

## Sistema di Interruzioni

Il sistema utilizza interruzioni per gestire eventi in modo **efficiente** senza attese attive:

1. **UART RX Bluetooth** (`HAL_UART_RxCpltCallback()`)
   - Riceve comandi via BT (es. "Access")
   - Modifica lo stato della macchina a stati

2. **UART RX ESP32-CAM** (stessa callback)
   - Riceve risposta: 'Y' (riconosciuto) o 'N' (non riconosciuto)
   - Gestisce transizione di stato

3. **Timer PWM (Servo)**
   - Controlla durata impulso (prescaler 1 MHz)
   - Periodo: 20000 µs = 50 Hz
   - Registro di comparazione (CCR1) = durata impulso

4. **Timer Lockout**
   - Avviato dopo 3 fallimenti
   - Durata: 10 secondi
   - Monitorato da `HAL_GetTick()` nel main loop

---

## Struttura dei Progetti

```
📦 72_Esposito_Riccio/
│
├── 📁 SERVER-Spyhole/              # Backend Flask + Dashboard
│   ├── app.py                      # ⭐ Main Flask app
│   ├── requirements.txt            # Dipendenze Python
│   ├── README.md                   # Setup server
│   ├── DOCUMENTATION.md            # Documentazione completa
│   ├── instance/spyhole.db         # SQLite (auto-created)
│   ├── static/                     # CSS personalizzato
│   └── templates/                  # HTML Jinja2
│
├── 📁 PROGETTO_ESAME_APC/          # Firmware STM32F303
│   ├── Core/Inc/                   # Header files
│   ├── Core/Src/                   # Source files
│   ├── Drivers/                    # HAL drivers
│   └── Debug/                      # Build output
│
├── 📁 PROGETTO-CAM/                # Codice Arduino ESP32-CAM
│   └── PROGETTO-CAM.ino           # Sketch Arduino
│
├── 📄 Spyhole Presentation.pdf    # Slides della presentazione
├── 📹 VIDEO-PROGETTO.MOV          # Demo video
└── 📄 README.md                    # Questo file
```

---



## Installazione & Setup

### **Backend Server**

```bash
# 1. Clone e accedi alla cartella
cd SERVER-Spyhole

# 2. Crea virtual environment
python -m venv env
source env/bin/activate  # Linux/Mac
# oppure: env\Scripts\activate  # Windows

# 3. Installa dipendenze
pip install -r requirements.txt

# 4. Avvia il server
python app.py
```

Server disponibile su: **http://localhost:5000**

### **Firmware STM32**

1. Apri progetto in **STM32CubeIDE**
2. Configura fusioni UART (115200 baud per ESP32, 9600 per HC-05)
3. Compila e carica nel microcontrollore
4. Collega componenti hardware come da schema

### **Arduino ESP32-CAM**

1. Apri Arduino IDE
2. Installa board support: `esp32 by Espressif Systems`
3. Seleziona: `Tools > Board > ESP32 > AI Thinker ESP32-CAM`
4. Apri `PROGETTO-CAM.ino`
5. Carica sketch (necessario convertitore USB-Seriale)

---

## Caso d'Uso: Accesso Completo

```
1️⃣ Utente avvicina volto a ESP32-CAM
2️⃣ Smartphone → Bluetooth → "Access" command
3️⃣ STM32 riceve command via HC-05 (UART @ 9600)
4️⃣ STM32 ordina a ESP32-CAM di catturare foto (UART @ 115200)
5️⃣ ESP32-CAM invia JPEG al server Flask via Wi-Fi
6️⃣ Flask → face_recognition.compare_faces()
7️⃣ Se match > 0.6: invia 'Y' a ESP32-CAM
8️⃣ ESP32-CAM comunica 'Y' a STM32 via UART
9️⃣ STM32 attiva PWM servo (1500-2000 µs)
🔟 Servomotore ruota → Serratura si sblocca
1️⃣1️⃣ LED RGB acceso (verde) → Accesso concesso ✅
```

---
## Media

- 📹 **Video Demo**: [VIDEO-PROGETTO.MOV](VIDEO-PROGETTO.MOV)
- 🎞️ **Presentazione**: [Spyhole Presentation.pdf](Spyhole%20Presentation.pdf) / [PowerPoint](Spyhole%20Presentation.pptx)

---


## 📄 Licenza

Progetto universitario - Uso libero per scopi educativi.

---

**Last Updated**: Gennaio 2026  
**Versione**: 1.0.0

---

# 🇬🇧 SPYHOLE - Face Recognition Access Control System

**Computer Architecture and Design (APC) Project**

**Authors**: Andrea Esposito (M63001650) | Francesco Riccio (M63001646)  
**Professorr**: Prof. Nicola Mazzocca

---

## System Overview

**Spyhole** is a **two-factor** access control system that combines:
- **Face recognition** via ESP32-CAM and AI
- **Bluetooth PIN code** as secure fallback
- **Web dashboard** for real-time access monitoring
- **User management** with registration and authentication

The entire system is managed by a **web dashboard** that allows real-time access monitoring and registered user management.

---

## System Architecture

```
┌─────────────────────────────────────────────────────┐
│           SPYHOLE - Complete System                 │
├─────────────────────────────────────────────────────┤
│                                                     │
│  Hardware:                                         │
│  ├─ ESP32-CAM (face recognition)                  │
│  ├─ STM32F303 Discovery (main controller)         │
│  ├─ Bluetooth HC-05 Module (PIN commands)         │
│  ├─ SG90 Servo Motor (lock release)               │
│  └─ RGB LED + Resistors (visual feedback)         │
│                                                     │
│  Backend:                                          │
│  ├─ Flask (REST API + Dashboard)                  │
│  ├─ SQLite (users & face encodings database)      │
│  └─ face_recognition library (dlib)               │
│                                                     │
│  Frontend:                                         │
│  ├─ Tailwind CSS (styling)                        │
│  └─ Jinja2 (template engine)                      │
│                                                     │
└─────────────────────────────────────────────────────┘
```

---

## Hardware Components

| Component | Model | Role |
|-----------|---------|-------|
| **Microcontroller** | STM32F303 Discovery | Main controller, manages logic flow |
| **Camera** | ESP32-CAM (OV2640) | Captures images for face recognition |
| **Communication** | Bluetooth HC-05 Module | Receives PIN commands from smartphone |
| **Lock Release** | SG90 Servo Motor | Operates the lock (180° rotation) |
| **Feedback** | RGB LED + 330Ω Resistors | Indicates system status |

---

## Workflow

### 1️⃣ **Registration/Login**
User authenticates in the webapp or gets registered for the first time.
- Email + Password → credentials
- Face photo → face encoding (128D vector with dlib)

### 2️⃣ **Access Request**
User sends **"Access"** command via Bluetooth interface from their smartphone.

### 3️⃣ **Face Recognition**
- ESP32-CAM captures face image
- Compares with face encodings saved in database
- If match > 0.6 (threshold): ✅ **ACCESS GRANTED**
- If match < 0.6: ❌ **ACCESS DENIED**

### 4️⃣ **Lock Release**
If recognition is positive:
- SG90 Servo Motor activates (PWM 50 Hz)
- Lock releases for 3-5 seconds
- RGB LED on (green/blue)

### 5️⃣ **Security Lockout**
In case of too many errors (> 3 attempts):
- System enters lockout for 10 seconds
- Red LED flashing
- Further attempts blocked until reset

---

## 🔄 Finite State Machine

```
┌──────────────┐
│   IDLE       │◄─── Reset / Lockout expired
└──────┬───────┘
       │ "Access" command received
       ▼
┌──────────────┐
│   ACQUIRING  │ Request photo from ESP32-CAM
└──────┬───────┘
       │
       ▼
┌──────────────────────┐
│ FACE_RECOGNITION     │ Compare with database
└──────┬───────────────┘
       │
   ┌───┴────────────────────┐
   │ MATCH?                 │
   └───┬─────────┬──────────┘
       │ YES     │ NO
       ▼         ▼
    ┌──────┐  ┌─────────────┐
    │UNLOCK│  │ERROR_RETRY  │ (attempts < 3)
    └──────┘  └──────┬──────┘
       │             │
       │      ┌──────┴─────────────┐
       │      │ Attempts >= 3?     │
       │      └──────┬──────┬──────┘
       │             │YES   │NO
       │             ▼      ▼
       │          ┌──────┐  IDLE
       │          │LOCKOUT│ (wait)
       │          └──────┘
       │             │
       ▼             ▼
    ┌────────────────────┐
    │ IDLE (reset)       │
    └────────────────────┘
```

---

## UART & Bluetooth Communication

### Hardware Configuration
- **STM32 ↔ ESP32-CAM**: UART @ **115200 baud** (high-speed images)
- **STM32 ↔ HC-05**: UART @ **9600 baud** (Bluetooth commands)
- **GND**: Common to all devices
- **TX/RX logic**: 3.3V (natively compatible)
- **Power supply**: 5V from STM32 → internal regulators

### Communication Flow
```
Smartphone (Bluetooth)
    │
    ▼
HC-05 (UART @ 9600 baud)
    │
    ├─────────────→ STM32F303 ←─────────────┐
    │              (Controller)              │
    │                  │                     │
    └──────────────────┼─────────────────────┘
                       │
                       ▼
            ESP32-CAM (UART @ 115200 baud)
                       │
                       ▼
                  📸 Face photo
                       │
                       ▼
                  💻 Flask Server
                  (face_recognition)
                       │
                       ▼
                   Match Y/N
```

---

## Key Modules

### **Bluetooth HC-05**
- Receives commands via Serial Port Profile (SPP)
- Behavior: wireless serial port
- Baud rate: 9600 bps (stable)
- Power: 5V, logic 3.3V
- Configurable via AT commands

### **ESP32-CAM**
- **Processor**: Dual-core ESP32-S @ 240 MHz
- **Memory**: 520 KB SRAM + 4 MB PSRAM + 4 MB Flash
- **Camera**: OV2640 (1600×1200 px)
- **Formats**: JPEG, YUV422, RGB565
- **Connectivity**: Wi-Fi 802.11 b/g/n + Bluetooth 4.2
- **Power consumption**: 160-260 mA (transmission)
- **Programming**: USB-Serial (no native USB)

### **SG90 Servo Motor**
- **Voltage**: 4.8 - 6.0 V
- **Rotation**: ~180° (90° per side)
- **Control**: PWM @ 50 Hz
- **Pulse**: 500-2400 µs (1500 µs = center)
- **Torque**: ~1.8 kg·cm @ 4.8 V
- **Speed**: ~0.1 s/60° (no load)
- **Power consumption**: <10 mA at rest, 100-250 mA in motion

**PWM Control**:
```
Frequency: 50 Hz (20 ms per cycle)
├─ 1000 µs → Left position (end stop)
├─ 1500 µs → Center
└─ 2000 µs → Right position (end stop)
```

---

## ⚡ Interrupt System

The system uses interrupts to handle events **efficiently** without active waits:

1. **Bluetooth UART RX** (`HAL_UART_RxCpltCallback()`)
   - Receives commands via BT (e.g., "Access")
   - Modifies state machine status

2. **ESP32-CAM UART RX** (same callback)
   - Receives response: 'Y' (recognized) or 'N' (not recognized)
   - Handles state transition

3. **PWM Timer (Servo)**
   - Controls pulse duration (1 MHz prescaler)
   - Period: 20000 µs = 50 Hz
   - Compare register (CCR1) = pulse duration

4. **Lockout Timer**
   - Started after 3 failures
   - Duration: 10 seconds
   - Monitored by `HAL_GetTick()` in main loop

---

## Project Structure

```
📦 72_Esposito_Riccio/
│
├── 📁 SERVER-Spyhole/              # Flask Backend + Dashboard
│   ├── app.py                      # ⭐ Main Flask app
│   ├── requirements.txt            # Python dependencies
│   ├── README.md                   # Server setup
│   ├── DOCUMENTATION.md            # Complete documentation
│   ├── instance/spyhole.db         # SQLite (auto-created)
│   ├── static/                     # Custom CSS
│   └── templates/                  # Jinja2 HTML
│
├── 📁 PROGETTO_ESAME_APC/          # STM32F303 Firmware
│   ├── Core/Inc/                   # Header files
│   ├── Core/Src/                   # Source files
│   ├── Drivers/                    # HAL drivers
│   └── Debug/                      # Build output
│
├── 📁 PROGETTO-CAM/                # Arduino ESP32-CAM Code
│   └── PROGETTO-CAM.ino           # Arduino Sketch
│
├── 📄 Spyhole Presentation.pdf    # Presentation slides
├── 📹 VIDEO-PROGETTO.MOV          # Demo video
└── 📄 README.md                    # This file
```

---

## Installation & Setup

### **Backend Server**

```bash
# 1. Clone and access folder
cd SERVER-Spyhole

# 2. Create virtual environment
python -m venv env
source env/bin/activate  # Linux/Mac
# or: env\Scripts\activate  # Windows

# 3. Install dependencies
pip install -r requirements.txt

# 4. Start server
python app.py
```

## Use Case: Complete Access

```
1️⃣ User approaches ESP32-CAM with face
2️⃣ Smartphone → Bluetooth → "Access" command
3️⃣ STM32 receives command via HC-05 (UART @ 9600)
4️⃣ STM32 orders ESP32-CAM to capture photo (UART @ 115200)
5️⃣ ESP32-CAM sends JPEG to Flask server via Wi-Fi
6️⃣ Flask → face_recognition.compare_faces()
7️⃣ If match > 0.6: sends 'Y' to ESP32-CAM
8️⃣ ESP32-CAM communicates 'Y' to STM32 via UART
9️⃣ STM32 activates PWM servo (1500-2000 µs)
🔟 Servo motor rotates → Lock releases
1️⃣1️⃣ RGB LED on (green) → Access granted ✅
```


---

## Additional Documentation

For complete details on:
- **Database schema**: see [DOCUMENTATION.md](SERVER-Spyhole/DOCUMENTATION.md)
- **Server setup**: see [SERVER-Spyhole/README.md](SERVER-Spyhole/README.md)
- **Hardware configuration**: see [Spyhole Presentation.pdf](Spyhole%20Presentation.pdf)

---

## Media

- 📹 **Demo Video**: [VIDEO-PROGETTO.MOV](VIDEO-PROGETTO.MOV)
- 🎞️ **Presentation**: [Spyhole Presentation.pdf](Spyhole%20Presentation.pdf) / [PowerPoint](Spyhole%20Presentation.pptx)

---


## 📄 License

Academic project - Free use for educational purposes.

---

**Last Updated**: January 2026  
**Version**: 1.0.0

