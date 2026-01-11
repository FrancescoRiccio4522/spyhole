# Spyhole - Sistema di Controllo Accessi con Riconoscimento Facciale

Sistema di riconoscimento facciale per controllo accessi con webapp integrata e database SQLite.

## Caratteristiche

- ✅ **Riconoscimento facciale** con `face_recognition` (dlib)
- ✅ **Database SQLite** per utenti e face encodings
- ✅ **Webapp moderna** con Tailwind CSS e Montserrat font
- ✅ **Dashboard in tempo reale** per monitorare accessi
- ✅ **API REST** compatibile con STM32 (raw bytes o multipart)
- ✅ **Log degli accessi** con anteprima immagini
- ✅ **Registrazione utenti** con upload immagine e webcam

## Installazione

### 1. Requisiti
- Python 3.8+
- Visual C++ Build Tools (per dlib su Windows)

### 2. Installa dipendenze
```powershell
pip install -r requirements.txt
```

**Nota**: Se l'installazione di `face_recognition` fallisce su Windows, installa prima dlib:
```powershell
pip install cmake
pip install dlib
pip install face_recognition
```

### 3. Avvia il server
```powershell
python app.py
```

Il server sarà disponibile su: **http://localhost:5000**

## Utilizzo

### Registrazione Utente
1. Vai su http://localhost:5000/register
2. Inserisci nome utente e ruolo
3. Carica una foto del volto (o usa la webcam)
4. Clicca "Registra"

Il sistema salverà:
- Immagine in `uploads/`
- Face encoding nel database SQLite
- Dati utente (username, role, timestamp)

### Test Riconoscimento
1. Vai su http://localhost:5000 (homepage)
2. Scorri fino alla sezione "Test Veloce Riconoscimento"
3. Carica una foto
4. Il sistema riconoscerà il volto e mostrerà il risultato

### Dashboard
1. Vai su http://localhost:5000/dashboard
2. Visualizza statistiche e log accessi in tempo reale
3. La dashboard si aggiorna automaticamente ogni 5 secondi
4. Clicca sulle anteprime per ingrandire le immagini

## API per STM32

### POST /upload
Invia immagine per riconoscimento.

**Input**: raw bytes (JPEG/PNG) o multipart/form-data

**Output JSON**:
```json
{
  "status": "ok",
  "name": "Mario",
  "filename": "image_20251104_123456_stm32_upload.jpg"
}
```

Se non riconosciuto:
```json
{
  "status": "not ok",
  "reason": "Unknown",
  "filename": "image_20251104_123456_stm32_upload.jpg"
}
```

### GET /api/log
Recupera il log completo degli accessi.

**Output JSON**:
```json
[
  {
    "timestamp": "20251104_123456",
    "filename": "image_20251104_123456.jpg",
    "recognized": true,
    "name": "Mario"
  }
]
```

### GET /api/reload-faces
Ricarica i volti noti dal database (utile dopo nuove registrazioni).

## Struttura File

```
Spyhole/
├── app.py                 # Backend Flask con face recognition
├── requirements.txt       # Dipendenze Python
├── spyhole.db            # Database SQLite (generato automaticamente)
├── uploads/              # Immagini caricate per riconoscimento
├── known_faces/          # (opzionale, non più usato - tutto in DB)
├── static/
│   ├── spylogo.png       # Logo del sito
│   ├── main.css          # CSS base
│   ├── custom.css        # CSS custom
│   ├── index.css         # CSS homepage
│   └── register.css      # CSS registrazione
└── templates/
    ├── base.html         # Template base (navbar, font)
    ├── index.html        # Homepage
    ├── register.html     # Form registrazione
    ├── login.html        # Form login (TODO)
    └── dashboard.html    # Dashboard accessi
```

## Logica Face Recognition

Il sistema usa la stessa logica del `server.py` originale:

1. **Registrazione**:
   - Carica immagine → estrae face encoding → salva in DB
   
2. **Riconoscimento**:
   - Carica volti noti dal DB
   - Estrae face encoding dall'immagine ricevuta
   - Calcola distanza euclidea con `face_recognition.face_distance()`
   - **Soglia**: distanza < 0.4 → riconosciuto, altrimenti "Unknown"

3. **Log**:
   - Ogni tentativo di accesso viene salvato in `access_log[]` (RAM)
   - Visibile in dashboard con anteprima immagine

## TODO

- [ ] Implementare pagina login funzionante
- [ ] Aggiungere autenticazione admin
- [ ] Persistenza log accessi nel database
- [ ] Integrazione Bluetooth fallback
- [ ] Controllo servo/LED/buzzer (GPIO)

## Note Tecniche

- **Font**: Montserrat (Google Fonts)
- **CSS**: Tailwind CSS via CDN
- **Face Recognition**: soglia 0.4 (severa come server.py)
- **Database**: SQLite con SQLAlchemy ORM
- **Encoding storage**: JSON array in colonna TEXT

## Troubleshooting

### Errore: "No face detected"
- Assicurati che la foto contenga un solo volto ben illuminato
- Niente occhiali scuri o maschere
- Volto frontale, non di profilo

### face_recognition non si installa
```powershell
# Installa Visual C++ Build Tools
# Poi:
pip install cmake
pip install dlib
pip install face_recognition
```

### Dashboard vuota
- Prima fai almeno un upload/test
- Il log è in memoria (si svuota al riavvio del server)

---

**Spyhole © 2025** - Sistema di controllo accessi con riconoscimento facciale
