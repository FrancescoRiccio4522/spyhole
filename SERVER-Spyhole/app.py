from flask import Flask, request, jsonify, send_from_directory, render_template, session, redirect, url_for, flash
from flask_sqlalchemy import SQLAlchemy
from werkzeug.security import generate_password_hash, check_password_hash
from werkzeug.utils import secure_filename
import face_recognition
import os
import io
from datetime import datetime
from PIL import Image
from functools import wraps

app = Flask(__name__)
app.secret_key = 'your-secret-key-change-this-in-production'  # Cambia questo in produzione!

# Crea le cartelle necessarie PRIMA di inizializzare il database
basedir = os.path.abspath(os.path.dirname(__file__))
os.makedirs(os.path.join(basedir, 'instance'), exist_ok=True)
os.makedirs('uploads', exist_ok=True)
os.makedirs('known_pictures', exist_ok=True)

# Configura il database con percorso assoluto
db_path = os.path.join(basedir, 'instance', 'spyhole.db')
app.config['SQLALCHEMY_DATABASE_URI'] = f'sqlite:///{db_path}'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
app.config['MAX_CONTENT_LENGTH'] = 5 * 1024 * 1024  # 5MB max file size

db = SQLAlchemy(app)

#MEMORIA TEMPORANEA PER TENERE TRACCIA DEGLI ACCESSI NELLA SESSIONE CORRENTE
access_log = []  # Lista di dizionari: [{'filename': ..., 'name': ...}]
UPLOAD_FOLDER = "uploads"
KNOWN_FOLDER = "known_pictures"

# ============================================
# MODELLI DATABASE
# ============================================

class User(db.Model):
    """Modello per utenti registrati"""
    __tablename__ = 'users'
    
    id = db.Column(db.Integer, primary_key=True)
    username = db.Column(db.String(80), unique=True, nullable=False)
    password_hash = db.Column(db.String(255), nullable=False)
    role = db.Column(db.String(20), default='user')  # user, guest, admin
    face_filename = db.Column(db.String(255), nullable=True)
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    
    def set_password(self, password):
        """Hash della password"""
        self.password_hash = generate_password_hash(password)
    
    def check_password(self, password):
        """Verifica la password"""
        return check_password_hash(self.password_hash, password)
    
    def __repr__(self):
        return f'<User {self.username}>'

# Crea le tabelle del database
print(f"[INFO] Inizializzazione database SQLite...")
print(f"[INFO] Percorso database: {db_path}")

with app.app_context():
    db.create_all()
    print("[INFO] ✓ Database e tabelle create con successo!")
    
    # Verifica se ci sono utenti registrati
    user_count = User.query.count()
    print(f"[INFO] Utenti registrati: {user_count}")
    if user_count == 0:
        print("[INFO] Nessun utente trovato. Usa /register per creare il primo account.")

# ============================================
# DECORATORI
# ============================================

def login_required(f):
    """Decoratore per proteggere le route che richiedono login"""
    @wraps(f)
    def decorated_function(*args, **kwargs):
        if 'user_id' not in session:
            return redirect(url_for('login_page'))
        return f(*args, **kwargs)
    return decorated_function

# ============================================
# RICONOSCIMENTO FACCIALE
# ============================================

# Carica volti noti all'avvio
known_face_encodings = []
known_face_names = []

def load_known_faces():
    """
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

load_known_faces()

def recognize_face(image_bytes):
    """
    Riconosce un volto comparandolo con quelli noti.
    """
    # Carica immagine ricevuta
    unknown_image = face_recognition.load_image_file(io.BytesIO(image_bytes))
    unknown_encodings = face_recognition.face_encodings(unknown_image)

    if not unknown_encodings:
        return False, "No face found"

    unknown_encoding = unknown_encodings[0]

    # Calcola la distanza tra volti (soglia: 0.4 per maggiore precisione)
    face_distances = face_recognition.face_distance(known_face_encodings, unknown_encoding)
    best_match_index = face_distances.argmin()
    best_distance = face_distances[best_match_index]

    # Soglia più severa per sicurezza
    if best_distance < 0.4:
        return True, known_face_names[best_match_index]
    else:
        return False, "Unknown"


@app.route('/upload', methods=['POST'])
def upload_image():
    """
    Endpoint per ricevere immagini dal sistema di riconoscimento facciale.
    Salva l'immagine, esegue il riconoscimento e registra l'accesso.
    Outputs JSON con status del riconoscimento.
    """
    try:
        image_bytes = request.data
        if not image_bytes:
            return jsonify({'error': 'No data received'}), 400

        # Salva immagine ricevuta con timestamp
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        image_path = os.path.join(UPLOAD_FOLDER, f"image_{timestamp}.jpg")
        image = Image.open(io.BytesIO(image_bytes))
        image.save(image_path)

        # Riconoscimento facciale
        recognized, name_or_msg = recognize_face(image_bytes)
        result = {
            'timestamp': timestamp,
            'filename': f"image_{timestamp}.jpg",
            'recognized': recognized,
            'name': name_or_msg if recognized else "Unknown"
        }
        access_log.append(result)

        if recognized:
            return jsonify({'status': 'ok', 'name': name_or_msg}), 200
        else:
            return jsonify({'status': 'not ok', 'reason': name_or_msg}), 200

    except Exception as e:
        return jsonify({'error': 'File non valido o danneggiato', 'detail': str(e)}), 400
    

@app.route('/')
def index():
    """Route principale per la homepage"""
    return render_template("index.html")


@app.route('/images/<filename>')
def get_image(filename):
    """Serve le immagini caricate"""
    return send_from_directory(UPLOAD_FOLDER, filename)


@app.route('/dashboard')
@login_required
def dashboard():
    """Dashboard con il registro degli accessi in tempo reale - RICHIEDE LOGIN"""
    return render_template("dashboard.html", log=access_log)


@app.route('/api/log')
def get_log():
    """API endpoint per ottenere il log degli accessi (JSON)"""
    return jsonify(access_log)

@app.route('/login', methods=['GET', 'POST'])
def login_page():
    """Pagina e gestione login"""
    if request.method == 'GET':
        return render_template('login.html')
    
    # POST: processa il login
    data = request.get_json() if request.is_json else request.form
    username = data.get('username', '').strip()
    password = data.get('password', '')
    
    if not username or not password:
        if request.is_json:
            return jsonify({'success': False, 'message': 'Username e password richiesti'}), 400
        flash('Username e password richiesti', 'error')
        return redirect(url_for('login_page'))
    
    user = User.query.filter_by(username=username).first()
    
    if user and user.check_password(password):
        # Login successful
        session['user_id'] = user.id
        session['username'] = user.username
        session['role'] = user.role
        
        if request.is_json:
            return jsonify({'success': True, 'message': 'Login effettuato con successo'}), 200
        return redirect(url_for('dashboard'))
    else:
        if request.is_json:
            return jsonify({'success': False, 'message': 'Username o password non validi'}), 401
        flash('Username o password non validi', 'error')
        return redirect(url_for('login_page'))


@app.route('/register', methods=['GET', 'POST'])
def register_page():
    """Pagina e gestione registrazione"""
    if request.method == 'GET':
        return render_template('register.html')
    
    try:
        username = request.form.get('username', '').strip()
        password = request.form.get('password', '')
        role = request.form.get('role', 'user')
        
        # Validazione
        if not username or len(username) < 2:
            return jsonify({'success': False, 'message': 'Username troppo corto (min 2 caratteri)'}), 400
        
        if not password or len(password) < 4:
            return jsonify({'success': False, 'message': 'Password troppo corta (min 4 caratteri)'}), 400
        
        # Verifica se l'utente esiste già
        if User.query.filter_by(username=username).first():
            return jsonify({'success': False, 'message': 'Username già esistente'}), 400
        
        # Gestione foto volto
        face_file = request.files.get('face_photo')
        face_filename = None
        
        if face_file and face_file.filename:
            ext = os.path.splitext(face_file.filename)[1].lower()
            if ext not in ['.jpg', '.jpeg', '.png']:
                return jsonify({'success': False, 'message': 'Formato file non valido (solo JPG, PNG)'}), 400
            
            # ✅ Salva con nome = username + estensione (NO timestamp)
            face_filename = f"{username}{ext}"
            face_path = os.path.join(KNOWN_FOLDER, face_filename)

            # Se esiste già, lo elimina
            if os.path.exists(face_path):
                os.remove(face_path)

            face_file.save(face_path)

            # Verifica che ci sia un volto nella foto
            try:
                image = face_recognition.load_image_file(face_path)
                encodings = face_recognition.face_encodings(image)
                if not encodings:
                    os.remove(face_path)
                    return jsonify({'success': False, 'message': 'Nessun volto rilevato nella foto'}), 400

                # Aggiorna subito i volti noti in memoria
                known_face_encodings.append(encodings[0])
                known_face_names.append(username)

            except Exception as e:
                if os.path.exists(face_path):
                    os.remove(face_path)
                return jsonify({'success': False, 'message': f'Errore nel processare la foto: {str(e)}'}), 400
        
        # Crea utente nel DB
        new_user = User(username=username, role=role, face_filename=face_filename)
        new_user.set_password(password)
        
        db.session.add(new_user)
        db.session.commit()
        
        return jsonify({'success': True, 'message': f'Utente {username} registrato con successo!'}), 201
        
    except Exception as e:
        db.session.rollback()
        return jsonify({'success': False, 'message': f'Errore durante la registrazione: {str(e)}'}), 500


@app.route('/logout')
def logout():
    """Logout utente"""
    session.clear()
    return redirect(url_for('index'))

if __name__ == '__main__':
    app.run(host='172.20.10.2', port=5000)
