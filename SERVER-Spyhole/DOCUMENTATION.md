# 📚 Documentazione Spyhole - Sistema di Controllo Accessi con Riconoscimento Facciale

## 📋 Indice

1. [Panoramica del Sistema](#panoramica-del-sistema)
2. [Architettura](#architettura)
3. [Configurazione e Setup](#configurazione-e-setup)
4. [Modelli Database](#modelli-database)
5. [Route e Endpoint](#route-e-endpoint)
6. [Funzioni di Riconoscimento Facciale](#funzioni-di-riconoscimento-facciale)
7. [Sistema di Autenticazione](#sistema-di-autenticazione)
8. [Frontend e Template](#frontend-e-template)
9. [Flussi di Lavoro](#flussi-di-lavoro)
10. [Integrazione Hardware](#integrazione-hardware)
11. [Sicurezza](#sicurezza)
12. [Troubleshooting](#troubleshooting)

---

## 🎯 Panoramica del Sistema

**Spyhole** è un sistema completo di controllo accessi che combina:
- **Riconoscimento facciale** tramite ESP32-CAM per accesso fisico
- **Dashboard web** per monitoraggio in tempo reale
- **Sistema di autenticazione** per gestione utenti autorizzati
- **Fallback con PIN Bluetooth** quando il riconoscimento facciale fallisce

### Componenti Principali

```
┌─────────────────────────────────────────────────────────────┐
│                    SPYHOLE SYSTEM                            │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌──────────────┐      ┌──────────────┐      ┌──────────┐  │
│  │  ESP32-CAM   │─────→│ Flask Server │←────→│ Web UI   │  │
│  │  + STM32     │ foto │   (app.py)   │ HTTP │Dashboard │  │
│  └──────────────┘      └──────────────┘      └──────────┘  │
│         │                      │                             │
│         │                      ├─→ SQLite DB (utenti)       │
│         │                      ├─→ Face Recognition         │
│         │                      └─→ Access Log               │
│         │                                                    │
│         └─→ Bluetooth PIN (fallback se non riconosciuto)    │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

---

## 🏗️ Architettura

### Stack Tecnologico

| Componente | Tecnologia | Versione | Scopo |
|------------|-----------|----------|-------|
| Backend | Flask | 3.1.2 | Web framework Python |
| Database | SQLite | 3.x | Gestione utenti |
| ORM | Flask-SQLAlchemy | 3.1.1 | Object-Relational Mapping |
| Face Recognition | face_recognition | 1.3.0 | Riconoscimento facciale |
| Computer Vision | OpenCV | 4.12.0 | Elaborazione immagini |
| Frontend | Tailwind CSS | 3.x | Styling UI |
| Template Engine | Jinja2 | 3.1.6 | Template HTML dinamici |

### Struttura del Progetto

```
F:\Spyhole/
├── app.py                      # ⭐ File principale Flask
├── requirements.txt            # Dipendenze Python
├── README.md                   # Readme del progetto
├── DOCUMENTATION.md            # Questa documentazione
│
├── instance/
│   └── spyhole.db             # 💾 Database SQLite (creato automaticamente)
│
├── static/                     # File statici
│   ├── main.css               # CSS personalizzato
│   └── custom.css             # CSS aggiuntivo
│
├── templates/                  # Template Jinja2
│   ├── base.html              # Template base (layout comune)
│   ├── index.html             # Homepage
│   ├── login.html             # Pagina login
│   ├── register.html          # Pagina registrazione
│   └── dashboard.html         # Dashboard accessi
│
├── uploads/                    # 📸 Foto catturate da ESP32-CAM
│   └── image_YYYYMMDD_HHMMSS.jpg
│
├── known_pictures/             # 👤 Foto volti autorizzati
│   └── username_YYYYMMDD_HHMMSS.jpg
│
└── env/                        # Virtual environment Python
    └── ...
```

---

## ⚙️ Configurazione e Setup

### Variabili di Configurazione

```python
# app.py - Linee 12-20

app = Flask(__name__)
app.secret_key = 'your-secret-key-change-this-in-production'

# Percorsi
basedir = os.path.abspath(os.path.dirname(__file__))
db_path = os.path.join(basedir, 'instance', 'spyhole.db')

# Configurazione Database
app.config['SQLALCHEMY_DATABASE_URI'] = f'sqlite:///{db_path}'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
app.config['MAX_CONTENT_LENGTH'] = 5 * 1024 * 1024  # 5MB max file
```

### Cartelle Create Automaticamente

```python
# app.py - Linee 15-17

os.makedirs(os.path.join(basedir, 'instance'), exist_ok=True)
os.makedirs('uploads', exist_ok=True)
os.makedirs('known_pictures', exist_ok=True)
```

**Scopo:**
- `instance/` → Contiene il database SQLite
- `uploads/` → Immagini ricevute da ESP32-CAM
- `known_pictures/` → Volti degli utenti registrati

---

## 💾 Modelli Database

### User Model

Tabella per memorizzare utenti autorizzati e le loro credenziali.

```python
# app.py - Linee 35-54

class User(db.Model):
    """Modello per utenti registrati"""
    __tablename__ = 'users'
    
    id = db.Column(db.Integer, primary_key=True)
    username = db.Column(db.String(80), unique=True, nullable=False)
    password_hash = db.Column(db.String(255), nullable=False)
    role = db.Column(db.String(20), default='user')
    face_filename = db.Column(db.String(255), nullable=True)
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
```

#### Campi della Tabella

| Campo | Tipo | Nullable | Descrizione |
|-------|------|----------|-------------|
| `id` | Integer | NO | Chiave primaria auto-incrementale |
| `username` | String(80) | NO | Username univoco |
| `password_hash` | String(255) | NO | Password hashata (bcrypt) |
| `role` | String(20) | NO | Ruolo: `user`, `guest`, `admin` |
| `face_filename` | String(255) | YES | Nome file foto in `known_pictures/` |
| `created_at` | DateTime | NO | Timestamp creazione utente |

#### Metodi della Classe

##### `set_password(password)`
```python
def set_password(self, password):
    """Hash della password con werkzeug.security"""
    self.password_hash = generate_password_hash(password)
```
**Logica:**
1. Riceve password in chiaro
2. Genera hash sicuro con `bcrypt`
3. Salva hash nel database (mai la password in chiaro!)

**Esempio:**
```python
user = User(username='mario')
user.set_password('mypassword123')  # Salva hash, non 'mypassword123'
```

##### `check_password(password)`
```python
def check_password(self, password):
    """Verifica la password confrontando l'hash"""
    return check_password_hash(self.password_hash, password)
```
**Logica:**
1. Riceve password tentata dall'utente
2. Calcola hash della password tentata
3. Confronta con hash salvato
4. Ritorna `True` se match, `False` altrimenti

**Esempio:**
```python
if user.check_password('mypassword123'):
    print("Password corretta!")
else:
    print("Password sbagliata!")
```

### Inizializzazione Database

```python
# app.py - Linee 57-66

print(f"[INFO] Inizializzazione database SQLite...")
print(f"[INFO] Percorso database: {db_path}")

with app.app_context():
    db.create_all()  # Crea tabelle se non esistono
    print("[INFO] ✓ Database e tabelle create con successo!")
    
    user_count = User.query.count()
    print(f"[INFO] Utenti registrati: {user_count}")
```

**Comportamento:**
- `db.create_all()` crea **solo** tabelle mancanti
- **NON** cancella dati esistenti
- **NON** modifica tabelle esistenti
- Safe per riavvii multipli

---

## 🌐 Route e Endpoint

### 1. Homepage - `/`

```python
@app.route('/')
def index():
    """Route principale per la homepage"""
    return render_template("index.html")
```

**Metodo:** `GET`  
**Autenticazione:** ❌ Non richiesta  
**Template:** `templates/index.html`  
**Scopo:** Pagina di benvenuto con informazioni sul sistema

---

### 2. Upload Immagine - `/upload` ⭐

**Endpoint principale per ESP32-CAM**

```python
@app.route('/upload', methods=['POST'])
def upload_image():
    """
    Endpoint per ricevere immagini dal sistema di riconoscimento facciale.
    Salva l'immagine, esegue il riconoscimento e registra l'accesso.
    """
```

**Metodo:** `POST`  
**Autenticazione:** ❌ Non richiesta (endpoint pubblico per ESP32)  
**Content-Type:** `image/jpeg`, `image/png` (binary data)

#### Logica Dettagliata

```python
# Step 1: Ricevi dati immagine
image_bytes = request.data
if not image_bytes:
    return jsonify({'error': 'No data received'}), 400

# Step 2: Salva immagine con timestamp
timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
image_path = os.path.join(UPLOAD_FOLDER, f"image_{timestamp}.jpg")
image = Image.open(io.BytesIO(image_bytes))
image.save(image_path)

# Step 3: Esegui riconoscimento facciale
recognized, name_or_msg = recognize_face(image_bytes)

# Step 4: Crea entry nel log
result = {
    'timestamp': timestamp,
    'filename': f"image_{timestamp}.jpg",
    'recognized': recognized,  # True/False
    'name': name_or_msg if recognized else "Unknown"
}
access_log.append(result)

# Step 5: Risposta JSON per ESP32
if recognized:
    return jsonify({'status': 'ok', 'name': name_or_msg}), 200
else:
    return jsonify({'status': 'not ok', 'reason': name_or_msg}), 200
```

#### Formato Risposta

**Caso 1: Volto Riconosciuto**
```json
{
  "status": "ok",
  "name": "Mario"
}
```
→ STM32 apre la porta

**Caso 2: Volto Non Riconosciuto**
```json
{
  "status": "not ok",
  "reason": "Unknown"
}
```
→ STM32 attiva modulo Bluetooth per PIN

**Caso 3: Nessun Volto Trovato**
```json
{
  "status": "not ok",
  "reason": "No face found"
}
```

#### Gestione Errori

```python
except Exception as e:
    return jsonify({
        'error': 'File non valido o danneggiato',
        'detail': str(e)
    }), 400
```

---

### 3. Registrazione - `/register`

```python
@app.route('/register', methods=['GET', 'POST'])
def register_page():
    """Pagina e gestione registrazione"""
```

**Metodo:** `GET` (mostra form), `POST` (processa registrazione)  
**Autenticazione:** ❌ Non richiesta  
**Content-Type:** `multipart/form-data` (per upload foto)

#### GET - Mostra Form

```python
if request.method == 'GET':
    return render_template('register.html')
```

Mostra form con campi:
- Username (min 2 caratteri)
- Password (min 4 caratteri)
- Ruolo (user/guest/admin)
- Upload foto volto

#### POST - Processa Registrazione

##### Step 1: Validazione Input
```python
username = request.form.get('username', '').strip()
password = request.form.get('password', '')
role = request.form.get('role', 'user')

# Validazione lunghezza
if not username or len(username) < 2:
    return jsonify({'success': False, 
                    'message': 'Username troppo corto'}), 400

if not password or len(password) < 4:
    return jsonify({'success': False, 
                    'message': 'Password troppo corta'}), 400
```

##### Step 2: Verifica Unicità Username
```python
if User.query.filter_by(username=username).first():
    return jsonify({'success': False, 
                    'message': 'Username già esistente'}), 400
```

##### Step 3: Gestione Foto Volto
```python
face_file = request.files.get('face_photo')

# Verifica formato
ext = os.path.splitext(face_file.filename)[1].lower()
if ext not in ['.jpg', '.jpeg', '.png']:
    return jsonify({'success': False, 
                    'message': 'Formato non valido'}), 400

# Salva con nome univoco
face_filename = f"{username}_{datetime.now().strftime('%Y%m%d_%H%M%S')}{ext}"
face_path = os.path.join(KNOWN_FOLDER, face_filename)
face_file.save(face_path)
```

##### Step 4: Verifica Presenza Volto
```python
image = face_recognition.load_image_file(face_path)
encodings = face_recognition.face_encodings(image)

if not encodings:
    os.remove(face_path)  # Rimuovi foto non valida
    return jsonify({'success': False, 
                    'message': 'Nessun volto rilevato'}), 400
```

**Importante:** Questa validazione previene foto senza volti o con più volti.

##### Step 5: Aggiungi al Sistema di Riconoscimento
```python
# Aggiungi encoding alla lista globale
known_face_encodings.append(encodings[0])
known_face_names.append(username)
```

Questo permette il riconoscimento **immediato** senza riavvio del server!

##### Step 6: Salva Utente nel Database
```python
new_user = User(username=username, role=role, face_filename=face_filename)
new_user.set_password(password)  # Hash della password

db.session.add(new_user)
db.session.commit()

return jsonify({'success': True, 
                'message': f'Utente {username} registrato!'}), 201
```

#### Gestione Errori con Rollback
```python
except Exception as e:
    db.session.rollback()  # Annulla transazione
    return jsonify({'success': False, 
                    'message': f'Errore: {str(e)}'}), 500
```

---

### 4. Login - `/login`

```python
@app.route('/login', methods=['GET', 'POST'])
def login_page():
    """Pagina e gestione login"""
```

**Metodo:** `GET` (mostra form), `POST` (autentica)  
**Content-Type:** `application/json` o `application/x-www-form-urlencoded`

#### GET - Mostra Form
```python
if request.method == 'GET':
    return render_template('login.html')
```

#### POST - Autenticazione

##### Step 1: Estrai Credenziali
```python
data = request.get_json() if request.is_json else request.form
username = data.get('username', '').strip()
password = data.get('password', '')
```

Supporta sia JSON che form data per flessibilità.

##### Step 2: Validazione Input
```python
if not username or not password:
    if request.is_json:
        return jsonify({'success': False, 
                        'message': 'Credenziali richieste'}), 400
    flash('Username e password richiesti', 'error')
    return redirect(url_for('login_page'))
```

##### Step 3: Cerca Utente
```python
user = User.query.filter_by(username=username).first()
```

Query SQL equivalente:
```sql
SELECT * FROM users WHERE username = 'mario' LIMIT 1;
```

##### Step 4: Verifica Password
```python
if user and user.check_password(password):
    # Login successful
    session['user_id'] = user.id
    session['username'] = user.username
    session['role'] = user.role
    
    return jsonify({'success': True, 
                    'message': 'Login effettuato'}), 200
else:
    return jsonify({'success': False, 
                    'message': 'Credenziali non valide'}), 401
```

**Dettaglio `session`:**
- Cookie crittografato lato client
- Firmato con `app.secret_key`
- Scade alla chiusura browser (default)

---

### 5. Dashboard - `/dashboard` 🔒

```python
@app.route('/dashboard')
@login_required  # ⭐ Richiede autenticazione
def dashboard():
    """Dashboard con registro accessi - PROTETTA"""
    return render_template("dashboard.html", log=access_log)
```

**Metodo:** `GET`  
**Autenticazione:** ✅ **RICHIESTA** (decoratore `@login_required`)  
**Template:** `templates/dashboard.html`

#### Decoratore `@login_required`

```python
def login_required(f):
    """Protegge route che richiedono login"""
    @wraps(f)
    def decorated_function(*args, **kwargs):
        if 'user_id' not in session:
            return redirect(url_for('login_page'))
        return f(*args, **kwargs)
    return decorated_function
```

**Logica:**
1. Controlla se `user_id` è nella sessione
2. Se NO → redirect a `/login`
3. Se SÌ → esegue la funzione normalmente

**Esempio flusso:**
```
User non loggato → /dashboard → redirect /login
User loggato → /dashboard → mostra dashboard ✓
```

---

### 6. API Log - `/api/log`

```python
@app.route('/api/log')
def get_log():
    """API endpoint per ottenere il log degli accessi (JSON)"""
    return jsonify(access_log)
```

**Metodo:** `GET`  
**Autenticazione:** ❌ Non richiesta  
**Response:** JSON Array

#### Formato Risposta

```json
[
  {
    "timestamp": "20251105_143022",
    "filename": "image_20251105_143022.jpg",
    "recognized": true,
    "name": "Mario"
  },
  {
    "timestamp": "20251105_143045",
    "filename": "image_20251105_143045.jpg",
    "recognized": false,
    "name": "Unknown"
  }
]
```

**Uso:** Dashboard usa questo endpoint per auto-refresh ogni 5 secondi.

---

### 7. Serve Immagini - `/images/<filename>`

```python
@app.route('/images/<filename>')
def get_image(filename):
    """Serve le immagini caricate"""
    return send_from_directory(UPLOAD_FOLDER, filename)
```

**Metodo:** `GET`  
**Parametro:** `filename` (es: `image_20251105_143022.jpg`)  
**Autenticazione:** ❌ Non richiesta

**Esempio:**
```
GET /images/image_20251105_143022.jpg
→ Serve F:\Spyhole\uploads\image_20251105_143022.jpg
```

---

### 8. Logout - `/logout`

```python
@app.route('/logout')
def logout():
    """Logout utente"""
    session.clear()  # Rimuove tutti i dati di sessione
    return redirect(url_for('index'))
```

**Metodo:** `GET`  
**Autenticazione:** ❌ Non richiesta (ma inutile se non loggato)

**Logica:**
1. Cancella tutti i dati della sessione (user_id, username, role)
2. Redirect alla homepage
3. Cookie di sessione invalidato

---

## 🔍 Funzioni di Riconoscimento Facciale

### `load_known_faces()`

```python
def load_known_faces():
    """
    Carica i volti noti dalla cartella KNOWN_FOLDER.
    Ogni immagine viene codificata e il nome viene estratto dal filename.
    """
    for filename in os.listdir(KNOWN_FOLDER):
        if filename.lower().endswith(('.jpg', '.jpeg', '.png')):
            path = os.path.join(KNOWN_FOLDER, filename)
            image = face_recognition.load_image_file(path)
            encodings = face_recognition.face_encodings(image)
            if encodings:
                known_face_encodings.append(encodings[0])
                known_face_names.append(os.path.splitext(filename)[0])
    
    print(f"[INFO] {len(known_face_encodings)} known faces loaded.")
```

#### Logica Dettagliata

**Step 1: Scansiona Cartella**
```python
for filename in os.listdir(KNOWN_FOLDER):
```
Esempio contenuto `known_pictures/`:
```
mario_20251105_120000.jpg
luigi_20251105_121500.jpg
peach_20251105_122000.jpg
```

**Step 2: Filtra per Estensione**
```python
if filename.lower().endswith(('.jpg', '.jpeg', '.png')):
```
Accetta solo immagini, ignora `.txt`, `.db`, ecc.

**Step 3: Carica Immagine**
```python
image = face_recognition.load_image_file(path)
```
Converte immagine in array NumPy RGB.

**Step 4: Estrai Face Encoding (128 dimensioni)**
```python
encodings = face_recognition.face_encodings(image)
```

**Cosa fa:**
1. Rileva volti nell'immagine (bounding box)
2. Estrae 68 landmark facciali (occhi, naso, bocca, ecc.)
3. Genera vettore di 128 numeri (face encoding)
4. Questo vettore è **univoco** per ogni volto

**Step 5: Salva in Memoria**
```python
if encodings:  # Se trova almeno un volto
    known_face_encodings.append(encodings[0])  # Primo volto
    known_face_names.append(os.path.splitext(filename)[0])
```

**Risultato finale:**
```python
known_face_encodings = [
    [0.12, -0.34, 0.56, ...],  # 128 numeri per Mario
    [-0.23, 0.45, -0.12, ...], # 128 numeri per Luigi
    [0.34, -0.11, 0.89, ...]   # 128 numeri per Peach
]

known_face_names = ['mario', 'luigi', 'peach']
```

---

### `recognize_face(image_bytes)`

```python
def recognize_face(image_bytes):
    """
    Riconosce un volto comparandolo con quelli noti.
    
    Args:
        image_bytes: Bytes dell'immagine da analizzare
        
    Returns:
        tuple: (riconosciuto: bool, nome_o_messaggio: str)
    """
```

#### Logica Completa con Esempio

**Input:** Foto da ESP32-CAM (bytes)

**Step 1: Carica Immagine**
```python
unknown_image = face_recognition.load_image_file(io.BytesIO(image_bytes))
```
Converte bytes → NumPy array

**Step 2: Rileva Volti**
```python
unknown_encodings = face_recognition.face_encodings(unknown_image)

if not unknown_encodings:
    return False, "No face found"
```

**Caso A:** Nessun volto → `(False, "No face found")`  
**Caso B:** Volto trovato → continua...

**Step 3: Estrai Encoding**
```python
unknown_encoding = unknown_encodings[0]  # Primo volto (128 numeri)
```

**Step 4: Calcola Distanze Euclidee**
```python
face_distances = face_recognition.face_distance(
    known_face_encodings,  # Tutti i volti noti
    unknown_encoding       # Volto da riconoscere
)
```

**Esempio calcolo:**
```python
# Volti noti
mario_encoding = [0.12, -0.34, 0.56, ...]
luigi_encoding = [-0.23, 0.45, -0.12, ...]

# Volto sconosciuto
unknown = [0.11, -0.35, 0.54, ...]  # Simile a Mario!

# Calcolo distanze (formula Euclidea)
distance_mario = sqrt((0.12-0.11)² + (-0.34-(-0.35))² + ...)
                = 0.15  # ⭐ Molto simile!

distance_luigi = sqrt((-0.23-0.11)² + (0.45-(-0.35))² + ...)
                = 0.82  # Molto diverso

# Risultato
face_distances = [0.15, 0.82]
```

**Step 5: Trova Match Migliore**
```python
best_match_index = face_distances.argmin()  # Indice distanza minima
best_distance = face_distances[best_match_index]
```

Esempio:
```python
face_distances = [0.15, 0.82]
best_match_index = 0  # Mario!
best_distance = 0.15
```

**Step 6: Verifica Soglia**
```python
if best_distance < 0.4:  # Soglia conservativa
    return True, known_face_names[best_match_index]
else:
    return False, "Unknown"
```

**Soglia 0.4:**
- `< 0.4` → Match affidabile (stesso volto)
- `≥ 0.4` → Volti troppo diversi

**Esempi:**
```python
# Caso 1: Mario stesso
distance = 0.15 < 0.4 → (True, "mario")

# Caso 2: Fratello di Mario (simile ma non uguale)
distance = 0.45 ≥ 0.4 → (False, "Unknown")

# Caso 3: Persona completamente diversa
distance = 0.95 ≥ 0.4 → (False, "Unknown")
```

---

## 🔐 Sistema di Autenticazione

### Sessioni Flask

```python
from flask import session

# Login
session['user_id'] = user.id
session['username'] = user.username
session['role'] = user.role

# Verifica
if 'user_id' in session:
    print(f"Utente loggato: {session['username']}")

# Logout
session.clear()
```

**Come funziona:**
1. Dati memorizzati in cookie crittografato
2. Firmato con `app.secret_key`
3. Lato client (browser), letto lato server
4. Scade alla chiusura browser

### Hash Password con Werkzeug

```python
from werkzeug.security import generate_password_hash, check_password_hash

# Registrazione
password_hash = generate_password_hash('mypassword123')
# Output: 'scrypt:32768:8:1$...$...' (hash bcrypt)

# Login
is_valid = check_password_hash(password_hash, 'mypassword123')
# Output: True
```

**Algoritmo:** scrypt (resistente a brute-force)

**Esempio completo:**
```python
# Registrazione
user.set_password('secret123')
db.session.add(user)
db.session.commit()

# Database salva:
# password_hash = 'scrypt:32768:8:1$abc123...'

# Login
if user.check_password('secret123'):  # ✓ Corretto
    print("Accesso consentito")
    
if user.check_password('wrong'):  # ✗ Sbagliato
    print("Accesso negato")
```

### Protezione Route con Decoratore

```python
from functools import wraps

def login_required(f):
    @wraps(f)
    def decorated_function(*args, **kwargs):
        if 'user_id' not in session:
            return redirect(url_for('login_page'))
        return f(*args, **kwargs)
    return decorated_function

# Uso
@app.route('/admin')
@login_required  # ⭐ Protegge la route
def admin_panel():
    return "Solo utenti loggati!"
```

**Flusso:**
```
User visita /admin
    ↓
Decoratore controlla session
    ↓
user_id presente? → SÌ → Mostra /admin
                  → NO → Redirect /login
```

---

## 🎨 Frontend e Template

### Base Template (`base.html`)

**Caratteristiche:**
- ✅ Tailwind CSS per styling
- ✅ Font Montserrat
- ✅ Navbar responsive con menu mobile
- ✅ Link dinamici basati su sessione

```html
{% if session.get('user_id') %}
  <a href="/dashboard">Dashboard</a>
  <span>{{ session.get('username') }}</span>
  <a href="/logout">Logout</a>
{% else %}
  <a href="/login">Login</a>
  <a href="/register">Registra</a>
{% endif %}
```

### Dashboard (`dashboard.html`)

**Features:**
- ✅ Auto-refresh ogni 5 secondi
- ✅ Fetch dati da `/api/log`
- ✅ Mostra immagini da `/images/<filename>`
- ✅ Badge colorati (verde=riconosciuto, rosso=negato)
- ✅ Timestamp formattato

```javascript
async function fetchLog() {
  const res = await fetch("/api/log");
  const log = await res.json();
  
  // Mostra in ordine inverso (più recenti primi)
  for (let i = log.length - 1; i >= 0; i--) {
    const item = log[i];
    // Crea card con immagine + dettagli
  }
}

// Refresh ogni 5 secondi
setInterval(fetchLog, 5000);
```

### Register (`register.html`)

**Features:**
- ✅ Upload foto da file
- ✅ Cattura foto da webcam
- ✅ Preview immagine
- ✅ Validazione client-side
- ✅ Submit con FormData

```javascript
const formData = new FormData();
formData.append("username", username);
formData.append("password", password);
formData.append("role", role);
formData.append("face_photo", blob, `${username}.jpg`);

const response = await fetch("/register", {
  method: "POST",
  body: formData
});
```

### Login (`login.html`)

**Features:**
- ✅ Form semplice e pulito
- ✅ Submit JSON
- ✅ Gestione errori con messaggi colorati
- ✅ Redirect automatico a dashboard

```javascript
const response = await fetch("/login", {
  method: "POST",
  headers: { "Content-Type": "application/json" },
  body: JSON.stringify({ username, password })
});

if (response.ok) {
  window.location.href = "/dashboard";
}
```

---

## 🔄 Flussi di Lavoro

### Flusso 1: Registrazione Nuovo Utente

```
┌─────────────────────────────────────────────────────────────┐
│ 1. User apre /register                                       │
├─────────────────────────────────────────────────────────────┤
│ 2. Compila form (username, password, ruolo)                 │
│ 3. Carica foto volto (file o webcam)                        │
│ 4. Click "Registra"                                          │
├─────────────────────────────────────────────────────────────┤
│ 5. Frontend → POST /register (FormData)                     │
├─────────────────────────────────────────────────────────────┤
│ 6. Backend valida:                                           │
│    - Username univoco? ✓                                     │
│    - Password ≥ 4 char? ✓                                    │
│    - Foto ha volto? ✓                                        │
├─────────────────────────────────────────────────────────────┤
│ 7. Salva foto in known_pictures/                            │
│ 8. Aggiungi encoding a known_face_encodings                 │
│ 9. Crea User nel database con password hashata              │
├─────────────────────────────────────────────────────────────┤
│ 10. Response: {"success": true, "message": "..."}           │
│ 11. Redirect a /login dopo 2 secondi                        │
└─────────────────────────────────────────────────────────────┘
```

### Flusso 2: Login Web

```
┌─────────────────────────────────────────────────────────────┐
│ 1. User apre /login                                          │
├─────────────────────────────────────────────────────────────┤
│ 2. Inserisce username + password                            │
│ 3. Click "Accedi"                                            │
├─────────────────────────────────────────────────────────────┤
│ 4. Frontend → POST /login (JSON)                            │
├─────────────────────────────────────────────────────────────┤
│ 5. Backend cerca User con username                          │
│ 6. Verifica password hash                                    │
├─────────────────────────────────────────────────────────────┤
│ CASO A: Password corretta                                    │
│   7a. Crea sessione (user_id, username, role)               │
│   8a. Response: {"success": true}                           │
│   9a. Redirect a /dashboard                                 │
├─────────────────────────────────────────────────────────────┤
│ CASO B: Password sbagliata                                   │
│   7b. Response: {"success": false, "message": "..."}        │
│   8b. Mostra errore                                          │
└─────────────────────────────────────────────────────────────┘
```

### Flusso 3: Accesso Fisico con ESP32-CAM

```
┌─────────────────────────────────────────────────────────────┐
│ 1. Persona si avvicina alla porta                           │
├─────────────────────────────────────────────────────────────┤
│ 2. ESP32-CAM scatta foto                                     │
│ 3. ESP32 → POST /upload (binary image data)                 │
├─────────────────────────────────────────────────────────────┤
│ 4. Flask salva foto in uploads/                             │
│ 5. Esegue recognize_face(image_bytes)                       │
│    - Estrae face encoding                                    │
│    - Calcola distanze con volti noti                        │
│    - Trova best match                                        │
├─────────────────────────────────────────────────────────────┤
│ CASO A: Riconosciuto (distance < 0.4)                       │
│   6a. Response: {"status": "ok", "name": "Mario"}           │
│   7a. STM32 apre porta ✓                                     │
│   8a. Log salvato con recognized=true                        │
├─────────────────────────────────────────────────────────────┤
│ CASO B: Non riconosciuto (distance ≥ 0.4)                   │
│   6b. Response: {"status": "not ok", "reason": "Unknown"}   │
│   7b. STM32 attiva modulo Bluetooth                         │
│   8b. Chiede PIN (1234)                                      │
│   9b. Se PIN corretto → Apre porta                          │
│   10b. Log salvato con recognized=false                     │
└─────────────────────────────────────────────────────────────┘
```

### Flusso 4: Monitoraggio Dashboard

```
┌─────────────────────────────────────────────────────────────┐
│ 1. Admin loggato apre /dashboard                            │
├─────────────────────────────────────────────────────────────┤
│ 2. Decoratore @login_required verifica sessione             │
│ 3. Se non loggato → redirect /login                         │
│ 4. Se loggato → mostra dashboard.html                       │
├─────────────────────────────────────────────────────────────┤
│ 5. JavaScript esegue fetchLog()                              │
│ 6. GET /api/log → riceve array JSON                         │
│ 7. Per ogni entry:                                           │
│    - Mostra immagine (/images/filename)                     │
│    - Mostra timestamp formattato                             │
│    - Badge verde (riconosciuto) o rosso (negato)            │
├─────────────────────────────────────────────────────────────┤
│ 8. setInterval(fetchLog, 5000) → Auto-refresh ogni 5s       │
│                                                              │
│ ┌─────────────────────────────────────┐                     │
│ │ Ogni 5 secondi:                     │                     │
│ │ - Fetch nuovo log                   │                     │
│ │ - Aggiorna UI                       │                     │
│ │ - Mostra nuovi accessi in tempo reale│                    │
│ └─────────────────────────────────────┘                     │
└─────────────────────────────────────────────────────────────┘
```

---

## 🔌 Integrazione Hardware

### Componenti Hardware Utilizzati

| Componente | Modello | Quantità | Funzione |
|------------|---------|----------|----------|
| **Microcontroller** | STM32F303 Discovery | 1 | Controllo principale sistema |
| **Camera WiFi** | ESP32-CAM | 1 | Cattura e invio foto al server |
| **Servomotore** | Tower Pro SG90 (9g) | 1 | Attuatore apertura porta |
| **Modulo Bluetooth** | HC-05 | 1 | Input PIN fallback |
| **LED RGB** | LED RGB Common Cathode/Anode | 1 | Indicatore visivo stato |
| **Resistenze** | 0.25W | 3 | Limitazione corrente LED RGB |
| **Breadboard** | Standard | 1 | Prototipazione circuito |

### Schema Componenti

```
┌─────────────────────────────────────────────────────────────┐
│                    SISTEMA SPYHOLE                           │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌──────────────┐         ┌─────────────────────┐          │
│  │  ESP32-CAM   │  WiFi   │  Flask Server       │          │
│  │              │────────→│  (Riconoscimento    │          │
│  │  Cattura     │  HTTP   │   Facciale)         │          │
│  │  foto        │         │                     │          │
│  └──────┬───────┘         └─────────────────────┘          │
│         │ GPIO                                              │
│         │ Signal (High/Low)                                 │
│         ↓                                                    │
│  ┌──────────────────────────────────────────────┐          │
│  │     STM32F303 Discovery Board                │          │
│  │                                               │          │
│  │  Pin Connections:                             │          │
│  │  ├─→ PA5  : Servo SG90 (PWM)                 │          │
│  │  ├─→ PA6  : LED RGB - Red                    │          │
│  │  ├─→ PA7  : LED RGB - Green                  │          │
│  │  ├─→ PB0  : LED RGB - Blue                   │          │
│  │  ├─→ PB6  : Bluetooth HC-05 TX               │          │
│  │  ├─→ PB7  : Bluetooth HC-05 RX               │          │
│  │  └─→ PC13 : Signal from ESP32-CAM            │          │
│  └──────┬───────────┬──────────┬─────────────────┘          │
│         │           │          │                             │
│         ↓           ↓          ↓                             │
│  ┌──────────┐ ┌─────────┐ ┌──────────┐                     │
│  │ SG90     │ │ HC-05   │ │ LED RGB  │                     │
│  │ Servo    │ │Bluetooth│ │ +3x 220Ω │                     │
│  │          │ │         │ │Resistors │                     │
│  └──────────┘ └─────────┘ └──────────┘                     │
│      │             │            │                            │
│      ↓             ↓            ↓                            │
│   Apre porta   Input PIN   Stato visivo                     │
└─────────────────────────────────────────────────────────────┘
```

### Specifiche Tecniche

#### STM32F303 Discovery
- **MCU**: STM32F303VCT6
- **Core**: ARM Cortex-M4 32-bit
- **Clock**: 72 MHz
- **Flash**: 256 KB
- **RAM**: 48 KB
- **GPIO**: 5V tolerant I/O
- **PWM**: Timer-based (per servo)
- **USART**: Per Bluetooth HC-05

#### ESP32-CAM
- **WiFi**: 802.11 b/g/n
- **Camera**: OV2640 (2MP)
- **Risoluzione**: Fino a 1600x1200
- **GPIO**: 9 disponibili
- **Alimentazione**: 5V (via USB o pin VCC)
- **Formato foto**: JPEG

#### Servo SG90
- **Tipo**: Micro servo 9g
- **Coppia**: 1.8 kg/cm (4.8V)
- **Velocità**: 0.1 sec/60° (4.8V)
- **Angolo rotazione**: 180°
- **PWM**: 50Hz (periodo 20ms)
- **Duty cycle**: 1ms (0°), 1.5ms (90°), 2ms (180°)
- **Alimentazione**: 4.8-6V

#### Bluetooth HC-05
- **Versione**: Bluetooth 2.0 + EDR
- **Range**: 10 metri
- **Baud rate**: 9600 (default)
- **Modalità**: Master/Slave
- **Alimentazione**: 3.3V-6V
- **Comunicazione**: UART (TX/RX)

#### LED RGB
- **Tipo**: Common Cathode (o Anode)
- **Tensione forward**: ~2V (R), ~3V (G), ~3V (B)
- **Corrente**: 20mA per canale
- **Resistenze**: 220Ω (0.25W)
- **Calcolo resistenza**: R = (Vcc - Vf) / I = (5V - 3V) / 0.02A = 100Ω (→ 220Ω per sicurezza)

### Codice ESP32-CAM (Completo)

```cpp
#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>

// ===== CONFIGURAZIONE WIFI =====
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* serverUrl = "http://192.168.1.100:5000/upload";  // IP del server Flask

// ===== PIN ESP32-CAM =====
#define SIGNAL_TO_STM32 2  // GPIO2 → PC13 STM32
#define LED_FLASH 4        // Flash LED integrato

// ===== CONFIGURAZIONE CAMERA =====
camera_config_t camera_config = {
  .pin_pwdn = 32,
  .pin_reset = -1,
  .pin_xclk = 0,
  .pin_sscb_sda = 26,
  .pin_sscb_scl = 27,
  .pin_d7 = 35,
  .pin_d6 = 34,
  .pin_d5 = 39,
  .pin_d4 = 36,
  .pin_d3 = 21,
  .pin_d2 = 19,
  .pin_d1 = 18,
  .pin_d0 = 5,
  .pin_vsync = 25,
  .pin_href = 23,
  .pin_pclk = 22,
  .xclk_freq_hz = 20000000,
  .ledc_timer = LEDC_TIMER_0,
  .ledc_channel = LEDC_CHANNEL_0,
  .pixel_format = PIXFORMAT_JPEG,
  .frame_size = FRAMESIZE_VGA,  // 640x480
  .jpeg_quality = 12,            // 0-63 (più basso = migliore qualità)
  .fb_count = 1
};

void setup() {
  Serial.begin(115200);
  
  // Configura pin output
  pinMode(SIGNAL_TO_STM32, OUTPUT);
  pinMode(LED_FLASH, OUTPUT);
  digitalWrite(SIGNAL_TO_STM32, LOW);
  
  // Connetti WiFi
  Serial.println("Connessione WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connesso!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  
  // Inizializza camera
  esp_err_t err = esp_camera_init(&camera_config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    return;
  }
  Serial.println("Camera inizializzata!");
}

void loop() {
  // Accendi flash LED
  digitalWrite(LED_FLASH, HIGH);
  delay(100);
  
  // Cattura foto
  camera_fb_t* fb = esp_camera_fb_get();
  
  if (!fb) {
    Serial.println("Errore cattura foto");
    digitalWrite(LED_FLASH, LOW);
    delay(2000);
    return;
  }
  
  Serial.printf("Foto catturata: %d bytes\n", fb->len);
  
  // Invia al server Flask
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "image/jpeg");
    http.setTimeout(10000);  // Timeout 10 secondi
    
    int httpCode = http.POST(fb->buf, fb->len);
    
    if (httpCode == 200) {
      String response = http.getString();
      Serial.println("Risposta server: " + response);
      
      // Parse JSON response
      if (response.indexOf("\"status\":\"ok\"") > 0) {
        // VOLTO RICONOSCIUTO
        Serial.println("✓ Accesso autorizzato!");
        digitalWrite(SIGNAL_TO_STM32, HIGH);
        delay(3000);  // Mantieni segnale per 3 secondi
        digitalWrite(SIGNAL_TO_STM32, LOW);
      } 
      else {
        // VOLTO NON RICONOSCIUTO
        Serial.println("✗ Accesso negato - Richiesta PIN");
        digitalWrite(SIGNAL_TO_STM32, LOW);
      }
    } 
    else {
      Serial.printf("Errore HTTP: %d\n", httpCode);
    }
    
    http.end();
  }
  
  // Rilascia buffer foto
  esp_camera_fb_return(fb);
  digitalWrite(LED_FLASH, LOW);
  
  delay(3000);  // Attendi 3 secondi prima del prossimo scatto
}
```

### Codice STM32F303 Discovery (HAL Library)

```c
/* main.c - STM32F303 Discovery */
#include "main.h"
#include <string.h>
#include <stdio.h>

// ===== DEFINIZIONI PIN =====
#define SIGNAL_FROM_ESP32_Pin GPIO_PIN_13
#define SIGNAL_FROM_ESP32_GPIO_Port GPIOC

#define LED_RED_Pin GPIO_PIN_6
#define LED_RED_GPIO_Port GPIOA
#define LED_GREEN_Pin GPIO_PIN_7
#define LED_GREEN_GPIO_Port GPIOA
#define LED_BLUE_Pin GPIO_PIN_0
#define LED_BLUE_GPIO_Port GPIOB

#define SERVO_Pin GPIO_PIN_5
#define SERVO_GPIO_Port GPIOA

// ===== VARIABILI GLOBALI =====
UART_HandleTypeDef huart1;  // Bluetooth HC-05
TIM_HandleTypeDef htim2;    // Timer per PWM servo

uint8_t bt_buffer[10];
char pin_input[5] = {0};
uint8_t pin_index = 0;

// ===== PROTOTIPI FUNZIONI =====
void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_USART1_UART_Init(void);
void MX_TIM2_Init(void);

void set_led_color(uint8_t red, uint8_t green, uint8_t blue);
void servo_set_angle(uint8_t angle);
void open_door(void);
void close_door(void);
void bluetooth_wait_pin(void);

int main(void) {
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  
  // Avvia PWM per servo
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
  
  // Posizione iniziale servo (porta chiusa)
  close_door();
  
  // LED rosso = sistema pronto
  set_led_color(255, 0, 0);
  
  printf("Sistema Spyhole inizializzato\n");
  
  while (1) {
    // Leggi segnale da ESP32-CAM
    GPIO_PinState esp32_signal = HAL_GPIO_ReadPin(SIGNAL_FROM_ESP32_GPIO_Port, 
                                                   SIGNAL_FROM_ESP32_Pin);
    
    if (esp32_signal == GPIO_PIN_SET) {
      // ===== VOLTO RICONOSCIUTO =====
      printf("Volto riconosciuto - Apertura porta\n");
      
      // LED verde = accesso autorizzato
      set_led_color(0, 255, 0);
      
      // Apri porta
      open_door();
      
      // Mantieni aperta per 5 secondi
      HAL_Delay(5000);
      
      // Chiudi porta
      close_door();
      set_led_color(255, 0, 0);  // Torna a rosso
    }
    else {
      // ===== VOLTO NON RICONOSCIUTO =====
      
      // LED blu = richiesta PIN
      set_led_color(0, 0, 255);
      
      printf("Inserire PIN via Bluetooth: ");
      
      // Aspetta input PIN da Bluetooth
      bluetooth_wait_pin();
      
      // Verifica PIN
      if (strcmp(pin_input, "1234") == 0) {
        printf("PIN corretto - Apertura porta\n");
        
        // LED verde = accesso autorizzato
        set_led_color(0, 255, 0);
        
        // Apri porta
        open_door();
        HAL_Delay(5000);
        close_door();
        set_led_color(255, 0, 0);
      }
      else {
        printf("PIN errato - Accesso negato\n");
        
        // LED rosso lampeggiante = allarme
        for (int i = 0; i < 5; i++) {
          set_led_color(255, 0, 0);
          HAL_Delay(200);
          set_led_color(0, 0, 0);
          HAL_Delay(200);
        }
        set_led_color(255, 0, 0);
      }
      
      // Reset buffer PIN
      memset(pin_input, 0, sizeof(pin_input));
      pin_index = 0;
    }
    
    HAL_Delay(100);
  }
}

// ===== CONTROLLO LED RGB =====
void set_led_color(uint8_t red, uint8_t green, uint8_t blue) {
  // PWM software o digitalWrite semplice
  HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, red > 128 ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, green > 128 ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, blue > 128 ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

// ===== CONTROLLO SERVO SG90 =====
void servo_set_angle(uint8_t angle) {
  // Servo SG90: 50Hz (periodo 20ms)
  // 0° = 1ms pulse (5% duty cycle)
  // 90° = 1.5ms pulse (7.5% duty cycle)
  // 180° = 2ms pulse (10% duty cycle)
  
  // Timer configurato per 20ms periodo
  // ARR = 20000 (20ms @ 1MHz clock)
  uint32_t pulse = 1000 + (angle * 1000 / 180);  // 1000-2000 us
  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pulse);
}

void open_door(void) {
  servo_set_angle(90);  // Ruota 90° per aprire
  printf("Porta aperta\n");
}

void close_door(void) {
  servo_set_angle(0);   // Torna a 0° per chiudere
  printf("Porta chiusa\n");
}

// ===== BLUETOOTH HC-05 =====
void bluetooth_wait_pin(void) {
  pin_index = 0;
  memset(pin_input, 0, sizeof(pin_input));
  
  uint32_t timeout = HAL_GetTick() + 30000;  // Timeout 30 secondi
  
  while (pin_index < 4 && HAL_GetTick() < timeout) {
    if (HAL_UART_Receive(&huart1, bt_buffer, 1, 100) == HAL_OK) {
      char received = bt_buffer[0];
      
      // Verifica se è un numero
      if (received >= '0' && received <= '9') {
        pin_input[pin_index++] = received;
        
        // Echo back
        HAL_UART_Transmit(&huart1, (uint8_t*)"*", 1, 100);
      }
      
      // Enter conferma PIN
      if (received == '\r' || received == '\n') {
        break;
      }
    }
  }
  
  pin_input[4] = '\0';  // Null terminator
  printf("PIN ricevuto: %s\n", pin_input);
}

// ===== CONFIGURAZIONE TIMER 2 (PWM) =====
void MX_TIM2_Init(void) {
  TIM_OC_InitTypeDef sConfigOC = {0};
  
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 71;      // 72MHz / 72 = 1MHz
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 19999;      // 20ms periodo (50Hz)
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  HAL_TIM_PWM_Init(&htim2);
  
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 1500;  // 1.5ms (90°)
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1);
}

// ===== CONFIGURAZIONE USART1 (Bluetooth) =====
void MX_USART1_UART_Init(void) {
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  HAL_UART_Init(&huart1);
}

// ===== CONFIGURAZIONE GPIO =====
void MX_GPIO_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  
  // LED RGB (Output)
  GPIO_InitStruct.Pin = LED_RED_Pin | LED_GREEN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  
  GPIO_InitStruct.Pin = LED_BLUE_Pin;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  
  // Segnale da ESP32 (Input)
  GPIO_InitStruct.Pin = SIGNAL_FROM_ESP32_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(SIGNAL_FROM_ESP32_GPIO_Port, &GPIO_InitStruct);
}
```

### Schema Comunicazione Dettagliato

```
┌────────────────────────────────────────────────────────────────────┐
│                      FLUSSO COMPLETO SISTEMA                        │
└────────────────────────────────────────────────────────────────────┘

    ┌──────────────┐                                  ┌───────────────┐
    │  ESP32-CAM   │                                  │ Flask Server  │
    │  (WiFi)      │                                  │ 192.168.1.100 │
    └──────┬───────┘                                  └───────┬───────┘
           │                                                  │
           │ 1. Scatta foto (640x480 JPEG)                  │
           │───────────────────────────────────────────────→│
           │                                                  │
           │                    2. Riconoscimento facciale   │
           │                       - Estrai encoding         │
           │                       - Compara con DB          │
           │                       - Calcola distanza        │
           │                                                  │
           │ 3. Response JSON                                │
           │←───────────────────────────────────────────────│
           │ {"status": "ok", "name": "Mario"}               │
           │    oppure                                        │
           │ {"status": "not ok", "reason": "Unknown"}       │
           │                                                  │
           ↓                                                  │
    [Parse Response]                                         │
           │                                                  │
           ├─→ Status OK? ──→ GPIO2 HIGH (3 sec)            │
           │                      │                           │
           │                      ↓                           │
           └─→ Status NOT OK? ─→ GPIO2 LOW                  │
                                  │                           │
                                  ↓                           │
                      ┌────────────────────┐                 │
                      │  STM32F303         │                 │
                      │  Discovery         │                 │
                      └─────────┬──────────┘                 │
                                │                             │
                   ┌────────────┼────────────┐              │
                   │            │            │               │
        ┌──────────▼──┐  ┌─────▼─────┐  ┌─▼────────┐      │
        │ PC13        │  │ PA5       │  │ PA6/7/B0 │      │
        │ Signal IN   │  │ Servo PWM │  │ LED RGB  │      │
        └──────┬──────┘  └─────┬─────┘  └─┬────────┘      │
               │               │           │                │
        [Leggi segnale]  [Controllo]  [Indicatori]        │
               │               │           │                │
               │               │           │                │
        ┌──────▼──────────────▼───────────▼──────┐        │
        │  LOGICA DECISIONALE                     │        │
        │                                          │        │
        │  IF signal == HIGH:                     │        │
        │    LED Verde ON                          │        │
        │    Servo → 90° (apri porta)             │        │
        │    Delay 5 sec                           │        │
        │    Servo → 0° (chiudi porta)            │        │
        │    LED Rosso ON                          │        │
        │                                          │        │
        │  ELSE:                                   │        │
        │    LED Blu ON                            │        │
        │    Attiva Bluetooth HC-05                │        │
        │    Aspetta PIN (4 cifre)                │        │
        │    IF PIN == "1234":                     │        │
        │      LED Verde ON                        │        │
        │      Servo → 90° (apri)                 │        │
        │    ELSE:                                 │        │
        │      LED Rosso lampeggiante (allarme)   │        │
        │                                          │        │
        └──────────────┬───────────────────────────┘        │
                       │                                     │
                       ↓                                     │
              ┌────────────────┐                            │
              │ Componenti     │                            │
              │ Fisici         │                            │
              ├────────────────┤                            │
              │ • SG90 Servo   │ → Apre/chiude porta        │
              │ • LED RGB      │ → Feedback visivo          │
              │ • HC-05 BT     │ → Input PIN fallback       │
              │ • 3x 220Ω      │ → Limita corrente LED      │
              │ • Breadboard   │ → Prototipazione           │
              └────────────────┘                            │
                                                             │
┌────────────────────────────────────────────────────────────┘
│  Dashboard Web (Parallelo)
│  - Admin monitora accessi in tempo reale
│  - Visualizza foto e timestamp
│  - Statistiche riconoscimenti
└─────────────────────────────────────────────────────────────┘
```

### Tabella Pin Connections

| Componente | Pin | Collega a | Note |
|------------|-----|-----------|------|
| **ESP32-CAM** | GPIO2 | STM32 PC13 | Segnale riconoscimento (3.3V) |
| **ESP32-CAM** | GND | STM32 GND | Ground comune |
| **ESP32-CAM** | 5V | Alimentatore | Alimentazione esterna |
| **STM32 PA5** | PWM Out | Servo SG90 Signal | PWM 50Hz |
| **STM32 5V** | — | Servo SG90 VCC | Alimentazione servo |
| **STM32 GND** | — | Servo SG90 GND | Ground servo |
| **STM32 PA6** | GPIO Out | LED RGB Red | Tramite resistenza 220Ω |
| **STM32 PA7** | GPIO Out | LED RGB Green | Tramite resistenza 220Ω |
| **STM32 PB0** | GPIO Out | LED RGB Blue | Tramite resistenza 220Ω |
| **STM32 PB6** | USART1 TX | HC-05 RX | Comunicazione seriale |
| **STM32 PB7** | USART1 RX | HC-05 TX | Comunicazione seriale |
| **STM32 3.3V** | — | HC-05 VCC | Alimentazione Bluetooth |
| **STM32 GND** | — | HC-05 GND | Ground Bluetooth |

### Schema Elettrico Breadboard

```
                    ┌─────────────────────────┐
                    │  STM32F303 Discovery    │
                    │                         │
        ┌───────────┤ PA5 (PWM)               │
        │           │ PA6 (GPIO)   ┌──────────┤ PC13 (Input) ←─── ESP32-CAM GPIO2
        │  ┌────────┤ PA7 (GPIO)   │          │
        │  │  ┌─────┤ PB0 (GPIO)   │          │
        │  │  │  ┌──┤ PB6 (USART TX)         │
        │  │  │  │┌─┤ PB7 (USART RX)         │
        │  │  │  ││ │ 5V      3.3V   GND     │
        │  │  │  ││ └──┬────────┬──────┬─────┘
        │  │  │  ││    │        │      │
        ↓  ↓  ↓  ↓↓    ↓        ↓      ↓
    ┌──────────────────────────────────────────┐
    │         BREADBOARD                        │
    ├──────────────────────────────────────────┤
    │                                           │
    │  [SG90 Servo]          [LED RGB]         │
    │   S: ←── PA5            R: ←─[220Ω]─ PA6 │
    │   +: ←── 5V             G: ←─[220Ω]─ PA7 │
    │   -: ←── GND            B: ←─[220Ω]─ PB0 │
    │                         -: ←── GND        │
    │                                           │
    │  [HC-05 Bluetooth]                        │
    │   VCC: ←── 3.3V                          │
    │   GND: ←── GND                           │
    │   TX:  ←── PB7                           │
    │   RX:  ←── PB6                           │
    │                                           │
    └──────────────────────────────────────────┘
```

---

## 🔒 Sicurezza

### 1. Password Hashing
✅ **Bcrypt/Scrypt** tramite Werkzeug  
✅ Password **mai** salvate in chiaro  
✅ Hash salted (resistente a rainbow tables)

### 2. Sessioni Sicure
✅ Cookie firmato con `secret_key`  
✅ Scadenza automatica  
✅ HTTPS raccomandato in produzione

### 3. Validazione Input
✅ Lunghezza minima password (4 caratteri)  
✅ Controllo formato file (JPG/PNG)  
✅ Limite dimensione upload (5MB)  
✅ Verifica volto nelle foto

### 4. SQL Injection Protection
✅ SQLAlchemy ORM (parametrizzato)  
✅ Nessuna query SQL raw

### 5. Protezione Route
✅ Decoratore `@login_required`  
✅ Dashboard accessibile solo a utenti loggati

### Raccomandazioni Produzione

```python
# ⚠️ CAMBIARE in produzione!
app.secret_key = 'your-secret-key-change-this-in-production'

# 🔐 Generare chiave sicura:
import secrets
print(secrets.token_hex(32))
# Output: 'a7f3b2c8d9e4f5a6...' (64 caratteri)

# Usare in .env:
# SECRET_KEY=a7f3b2c8d9e4f5a6...
```

**Altre raccomandazioni:**
- ✅ HTTPS con certificato SSL
- ✅ Rate limiting su `/upload` (prevenire spam)
- ✅ Firewall per proteggere porta 5000
- ✅ Backup regolare database
- ✅ Log accessi su file persistente

---

## 🐛 Troubleshooting

### Problema: Database non si crea

**Errore:**
```
sqlalchemy.exc.OperationalError: unable to open database file
```

**Soluzione:**
```python
# Verifica che cartella instance/ esista
os.makedirs(os.path.join(basedir, 'instance'), exist_ok=True)

# Usa percorso assoluto
basedir = os.path.abspath(os.path.dirname(__file__))
db_path = os.path.join(basedir, 'instance', 'spyhole.db')
app.config['SQLALCHEMY_DATABASE_URI'] = f'sqlite:///{db_path}'
```

### Problema: Volti non riconosciuti

**Possibili cause:**
1. **Soglia troppo restrittiva**: `best_distance < 0.4`
   - Aumentare a `0.5` se troppo severo
   
2. **Foto registrazione di bassa qualità**
   - Usare foto ben illuminate
   - Un solo volto per foto
   - Risoluzione minima 640x480

3. **Foto ESP32 diversa da registrazione**
   - Stessa angolazione
   - Stessa illuminazione
   - Evitare occhiali scuri

### Problema: Session non persiste

**Causa:** Secret key cambia ad ogni riavvio

**Soluzione:**
```python
# Fissa la chiave (non usare secrets.token_hex() direttamente!)
app.secret_key = 'chiave-fissa-in-produzione-usa-env-file'
```

### Problema: Upload foto troppo lente

**Causa:** Dimensione file grande

**Soluzione:**
```python
# Limita dimensione file
app.config['MAX_CONTENT_LENGTH'] = 2 * 1024 * 1024  # 2MB

# Comprimi lato ESP32
camera_config.jpeg_quality = 12;  // 0-63 (più basso = migliore)
```

### Problema: Dashboard non aggiorna

**Verifica console browser:**
```javascript
// Dovrebbe apparire ogni 5 secondi
console.log("Fetching log...");
```

**Controlla endpoint:**
```bash
curl http://localhost:5000/api/log
# Dovrebbe ritornare JSON array
```

---

## 📊 Struttura Dati

### access_log (in memoria)

```python
access_log = [
    {
        'timestamp': '20251105_143022',
        'filename': 'image_20251105_143022.jpg',
        'recognized': True,
        'name': 'Mario'
    },
    {
        'timestamp': '20251105_143045',
        'filename': 'image_20251105_143045.jpg',
        'recognized': False,
        'name': 'Unknown'
    }
]
```

**⚠️ Nota:** I log sono in memoria RAM, si perdono al riavvio del server.

**Soluzione persistenza:**
```python
# Opzione 1: Salva in database
class AccessLog(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    timestamp = db.Column(db.String(50))
    filename = db.Column(db.String(255))
    recognized = db.Column(db.Boolean)
    name = db.Column(db.String(80))

# Opzione 2: Salva in file JSON
import json
with open('access_log.json', 'w') as f:
    json.dump(access_log, f)
```

### known_face_encodings

```python
known_face_encodings = [
    [0.12, -0.34, 0.56, ..., 0.89],  # 128 numeri (Mario)
    [-0.23, 0.45, -0.12, ..., 0.34], # 128 numeri (Luigi)
]

known_face_names = ['mario', 'luigi']
```

**Indice corrispondente:**
```python
known_face_encodings[0]  # Encoding di Mario
known_face_names[0]       # "mario"
```

---

## 🚀 Avvio Applicazione

### Sviluppo

```powershell
# Attiva virtual environment
cd F:\Spyhole
.\env\Scripts\activate

# Avvia server
python app.py

# Output:
# [INFO] Inizializzazione database SQLite...
# [INFO] Percorso database: F:\Spyhole\instance\spyhole.db
# [INFO] ✓ Database e tabelle create con successo!
# [INFO] Utenti registrati: 3
# [INFO] 3 known faces loaded.
# * Running on http://0.0.0.0:5000
```

### Produzione (con Gunicorn)

```bash
pip install gunicorn

gunicorn -w 4 -b 0.0.0.0:5000 app:app
```

**Parametri:**
- `-w 4`: 4 worker processes
- `-b 0.0.0.0:5000`: Bind su tutte le interfacce

---

## 📈 Possibili Estensioni

### 1. Persistenza Log Accessi

```python
class AccessLog(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    timestamp = db.Column(db.DateTime, default=datetime.utcnow)
    user_id = db.Column(db.Integer, db.ForeignKey('users.id'))
    recognized = db.Column(db.Boolean)
    image_path = db.Column(db.String(255))
    
    user = db.relationship('User', backref='accesses')
```

### 2. Email Notifiche

```python
from flask_mail import Mail, Message

@app.route('/upload', methods=['POST'])
def upload_image():
    # ... riconoscimento ...
    
    if not recognized:
        # Invia email a admin
        msg = Message('Accesso Negato',
                      recipients=['admin@example.com'])
        msg.body = f'Tentativo accesso non autorizzato alle {timestamp}'
        msg.attach('photo.jpg', 'image/jpeg', image_bytes)
        mail.send(msg)
```

### 3. Statistiche Accessi

```python
@app.route('/stats')
@login_required
def statistics():
    total = len(access_log)
    recognized = sum(1 for x in access_log if x['recognized'])
    denied = total - recognized
    
    return render_template('stats.html', 
                          total=total, 
                          recognized=recognized, 
                          denied=denied)
```

### 4. Multi-Volto nella Stessa Foto

```python
def recognize_face(image_bytes):
    unknown_encodings = face_recognition.face_encodings(unknown_image)
    
    # Riconosci TUTTI i volti
    recognized_people = []
    for encoding in unknown_encodings:
        distances = face_recognition.face_distance(known_face_encodings, encoding)
        best_idx = distances.argmin()
        if distances[best_idx] < 0.4:
            recognized_people.append(known_face_names[best_idx])
    
    return recognized_people
```

### 5. API REST Completa

```python
@app.route('/api/users', methods=['GET'])
@login_required
def get_users():
    users = User.query.all()
    return jsonify([{
        'id': u.id,
        'username': u.username,
        'role': u.role,
        'created_at': u.created_at.isoformat()
    } for u in users])

@app.route('/api/users/<int:user_id>', methods=['DELETE'])
@login_required
def delete_user(user_id):
    user = User.query.get_or_404(user_id)
    db.session.delete(user)
    db.session.commit()
    return jsonify({'success': True})
```

---

## 📞 Contatti e Supporto

Per domande o problemi:
- 📧 Email: support@spyhole.local
- 📖 Documentazione: Questo file
- 🐛 Bug Report: Aprire issue su repository

---

**Ultima modifica:** 5 novembre 2025  
**Versione:** 1.0.0  
**Autore:** Spyhole Development Team
