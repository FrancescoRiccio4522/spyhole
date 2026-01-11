# 🔐 SPYHOLE - Sistema di Controllo Accessi con Riconoscimento Facciale

**Progetto di Architettura e Progetto di Calcolatori (APC)**

👨‍💼 **Autori**: Andrea Esposito (M63001650) | Francesco Riccio (M63001646)  
👨‍🏫 **Relatore**: Prof. Nicola Mazzocca

---

## 📋 Panoramica del Sistema

**Spyhole** è un sistema di controllo accessi a **due fattori** che combina:
- 🎥 **Riconoscimento facciale** tramite ESP32-CAM e AI
- 🔐 **Codice PIN Bluetooth** come fallback sicuro
- 📊 **Dashboard web** per monitorare accessi in tempo reale
- 🔑 **Gestione utenti** con registrazione e autenticazione

L'intero sistema è gestito da una **dashboard web** che permette di visualizzare gli accessi in tempo reale e gestire gli utenti registrati.

---

## 🏗️ Architettura del Sistema

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

## ⚙️ Componenti Hardware

| Componente | Modello | Ruolo |
|-----------|---------|-------|
| **Microcontrollore** | STM32F303 Discovery | Controller principale, gestisce flusso logico |
| **Fotocamera** | ESP32-CAM (OV2640) | Acquisisce immagini per riconoscimento facciale |
| **Comunicazione** | Modulo Bluetooth HC-05 | Riceve comandi PIN dallo smartphone |
| **Sblocco** | Servomotore SG90 | Aziona la serratura (180° di rotazione) |
| **Feedback** | LED RGB + Resistenze 330Ω | Indica lo stato del sistema |

---

## 📱 Flusso di Lavoro

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

## 📡 Comunicazione UART & Bluetooth

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

## 🔊 Moduli Chiave

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

## ⚡ Sistema di Interruzioni

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

## 🚀 Struttura dei Progetti

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

## 📊 Stack Tecnologico

| Layer | Tecnologia | Versione | Utilizzo |
|-------|-----------|----------|----------|
| **Backend** | Flask | 3.1.2 | Web framework REST API |
| **ORM** | Flask-SQLAlchemy | 3.1.1 | Object-Relational Mapping |
| **Face Recognition** | face_recognition | 1.3.0 | Riconoscimento facciale (dlib) |
| **Computer Vision** | OpenCV | 4.12.0 | Elaborazione immagini |
| **Frontend** | Tailwind CSS | 3.x | Styling responsivo |
| **Template Engine** | Jinja2 | 3.1.6 | Rendering HTML dinamico |
| **Database** | SQLite | 3.x | Persistenza dati |
| **Microcontrollore** | STM32F303 | ARM Cortex-M4 | Controller principale |
| **Fotocamera** | ESP32-CAM | Dual-core 240MHz | Acquisizione immagini |

---

## 🔐 Sicurezza

- ✅ **Face encoding** salvato nel database (non foto original)
- ✅ **Hashing password** con werkzeug security
- ✅ **Lockout progressivo** dopo tentativi falliti
- ✅ **Comunicazione UART** (locale, non pubblica)
- ✅ **PIN Bluetooth** come secondo fattore
- ✅ **Token session** per dashboard web

---

## 📥 Installazione & Setup

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

## 🎯 Caso d'Uso: Accesso Completo

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

## 📝 API REST

### POST `/upload`
Invia immagine per riconoscimento facciale

**Request**: raw JPEG bytes o multipart/form-data  
**Response**:
```json
{
  "status": "ok",
  "name": "Mario",
  "filename": "image_20251104_123456.jpg"
}
```

### GET `/api/log`
Recupera log accessi

**Response**:
```json
[
  {
    "timestamp": "20251104_123456",
    "filename": "image.jpg",
    "recognized": true,
    "name": "Mario"
  }
]
```

---

## 📚 Documentazione Aggiuntiva

Per dettagli completi su:
- **Database schema**: vedi [DOCUMENTATION.md](SERVER-Spyhole/DOCUMENTATION.md)
- **Setup server**: vedi [SERVER-Spyhole/README.md](SERVER-Spyhole/README.md)
- **Configurazione hardware**: vedi [Spyhole Presentation.pdf](Spyhole%20Presentation.pdf)

---

## 🎓 Progetto Accademico

**Università**: Università degli Studi di Napoli Federico II  
**Corso**: Architettura e Progetto di Calcolatori (APC)  
**Anno Accademico**: 2024/2025  
**Docente**: Prof. Nicola Mazzocca

---

## 📸 Media

- 📹 **Video Demo**: [VIDEO-PROGETTO.MOV](VIDEO-PROGETTO.MOV)
- 🎞️ **Presentazione**: [Spyhole Presentation.pdf](Spyhole%20Presentation.pdf) / [PowerPoint](Spyhole%20Presentation.pptx)

---

## 🤝 Autori

| Nome | Matricola | Ruolo |
|------|-----------|-------|
| **Andrea Esposito** | M63001650 | Firmware STM32 + Hardware |
| **Francesco Riccio** | M63001646 | Backend Flask + Face Recognition |

---

## 📄 Licenza

Progetto universitario - Uso libero per scopi educativi.

---

**Last Updated**: Gennaio 2026  
**Versione**: 1.0.0

