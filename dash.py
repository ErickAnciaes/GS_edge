# app.py - Bridge MQTT -> WebSocket (corrigido: remove broadcast kwarg)
# Use: python app.py
import json
import time
import threading
from flask import Flask, render_template, jsonify
from flask_socketio import SocketIO, emit
import paho.mqtt.client as mqtt

# -------- CONFIGURAÇÃO --------
MQTT_BROKER = "44.223.43.74"
MQTT_PORT = 1883
MQTT_TOPICS = [("workwell/monitoramento", 0), ("workwell/alerts", 0)]
MQTT_PUB_COMMAND_TOPIC = "workwell/command"

# Flask + SocketIO usando threading (mais robusto no Windows)
app = Flask(__name__)
app.config['SECRET_KEY'] = 'workwell-secret'
socketio = SocketIO(app, cors_allowed_origins="*", async_mode='threading')

# -------- MQTT cliente (subscriber) --------
mqtt_client = mqtt.Client(client_id=f"workwell-bridge-subscriber-{int(time.time())}")

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print(f"[MQTT] Conectado ao broker {MQTT_BROKER}:{MQTT_PORT} (rc={rc})")
        for topic, qos in MQTT_TOPICS:
            client.subscribe(topic, qos)
            print(f"[MQTT] Subscribed -> {topic}")
    else:
        print(f"[MQTT] Falha na conexão, rc={rc}")

def on_message(client, userdata, msg):
    try:
        raw = msg.payload.decode('utf-8', errors='replace')
    except Exception:
        raw = str(msg.payload)
    payload = None
    try:
        payload = json.loads(raw)
    except Exception:
        payload = raw

    event = {
        "topic": msg.topic,
        "payload": payload,
        "raw": raw,
        "ts": int(time.time() * 1000)
    }

    print(f"[MQTT] msg chegada: topic={msg.topic} payload_preview={str(raw)[:400]}")
    # grava log simples
    try:
        with open("last_msg.log", "a", encoding="utf-8") as f:
            f.write(f"{time.strftime('%Y-%m-%d %H:%M:%S')} topic={msg.topic} payload={raw[:400]}\n")
    except Exception as e:
        print("[LOG] falha gravar last_msg.log:", e)

    # Emite via Socket.IO para todos clientes (sem usar broadcast kwarg)
    try:
        socketio.emit('mqtt_message', event)  # envia para todos por padrão
        # pequeno yield para o loop do socketio (seguro em threading)
        socketio.sleep(0)
        print("[BRIDGE] emitido mqtt_message via Socket.IO")
    except Exception as e:
        print("[BRIDGE] ERRO emit socketio:", e)

def mqtt_loop_thread():
    # tenta conectar com reconexões automáticas
    while True:
        try:
            mqtt_client.on_connect = on_connect
            mqtt_client.on_message = on_message
            print(f"[MQTT] Tentando conectar ao broker {MQTT_BROKER}:{MQTT_PORT} ...")
            mqtt_client.connect(MQTT_BROKER, MQTT_PORT, keepalive=60)
            mqtt_client.loop_forever()  # bloqueia dentro da thread
        except Exception as e:
            print("[MQTT] Erro na conexão/loop:", e)
            print("[MQTT] Tentando reconectar em 5s...")
            time.sleep(5)

# publisher (cliente separado)
pub_client = mqtt.Client(client_id=f"workwell-publisher-{int(time.time())}")
def start_publisher():
    try:
        pub_client.connect(MQTT_BROKER, MQTT_PORT, 60)
        pub_client.loop_start()
        print("[PUB] Publisher MQTT conectado e loop iniciado")
    except Exception as e:
        print("[PUB] Falha ao conectar publisher:", e)

def publish_command(cmd_str):
    try:
        res = pub_client.publish(MQTT_PUB_COMMAND_TOPIC, cmd_str)
        print(f"[PUB] publish to {MQTT_PUB_COMMAND_TOPIC}: '{cmd_str}' -> rc={res.rc}")
        return True
    except Exception as e:
        print("[PUB] falha publish:", e)
        return False

# -------- Flask routes / SocketIO handlers --------
@app.route('/')
def index():
    return render_template('index.html')

@app.route('/health')
def health():
    return jsonify({
        "mqtt_connected": mqtt_client.is_connected(),
        "pub_connected": pub_client.is_connected()
    })

@socketio.on('connect')
def on_ws_connect():
    print("[WS] cliente conectado")
    emit('connected', {'ok': True})

@socketio.on('disconnect')
def on_ws_disconnect():
    print("[WS] cliente desconectado")

@socketio.on('send_command')
def handle_send_command(data):
    print("[WS] send_command recebido:", data)
    cmd = None
    if isinstance(data, dict):
        cmd = data.get('cmd')
    elif isinstance(data, str):
        cmd = data
    if cmd:
        ok = publish_command(cmd)
        emit('command_result', {'cmd': cmd, 'ok': ok})

# -------- startup --------
if __name__ == '__main__':
    # inicia o publisher
    start_publisher()

    # inicia thread MQTT subscriber (loop_forever dentro da thread)
    t = threading.Thread(target=mqtt_loop_thread, daemon=True)
    t.start()

    # inicia Flask-SocketIO com threading (mais confiável no Windows)
    print("[BRIDGE] Iniciando servidor web em http://127.0.0.1:5000")
    socketio.run(app, host='127.0.0.1', port=5000)
