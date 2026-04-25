#!/usr/bin/env python3
"""
RackIQ TCP Server - Protocolo UABC
Universidad Autónoma de Baja California
Usuario: a1275863
Puerto: 5000
"""

import socket
import threading
import time
import random
from datetime import datetime

# ─────────────────────────────────────────────
#  Configuración
# ─────────────────────────────────────────────
HOST = "0.0.0.0"   # Escucha en todas las interfaces (incluye 192.168.1.66)
PORT = 5000
USUARIO = "a1275863"

# ─────────────────────────────────────────────
#  Estado simulado de los recursos del ESP32
# ─────────────────────────────────────────────
estado = {
    "led": 0,          # 0 o 1
    "pwm": 0,          # 0-100
    "adc": 0,          # solo lectura, se simula aleatoriamente
    "conectado": False,
    "autenticado": False,
    "ultimo_keepalive": None,
}

# Socket del cliente activo (hilo receptor lo asigna)
cliente_socket = None
cliente_lock = threading.Lock()

# ─────────────────────────────────────────────
#  Utilidades de log
# ─────────────────────────────────────────────
def ts():
    return datetime.now().strftime("%H:%M:%S")

def log_rx(msg):
    print(f"\n  [{ts()}] ← ESP32 : {msg.strip()}")

def log_tx(msg):
    print(f"  [{ts()}] → SERVER : {msg.strip()}")

def log_info(msg):
    print(f"  [{ts()}]   INFO   : {msg}")

def log_err(msg):
    print(f"  [{ts()}]  ERROR   : {msg}")

def prompt():
    print("\n> ", end="", flush=True)

# ─────────────────────────────────────────────
#  Enviar respuesta al cliente
# ─────────────────────────────────────────────
def enviar(sock, mensaje):
    try:
        data = (mensaje + "\n").encode()
        sock.sendall(data)
        log_tx(mensaje)
    except Exception as e:
        log_err(f"No se pudo enviar: {e}")

# ─────────────────────────────────────────────
#  Parser del protocolo UABC
#  Formato: UABC:<usuario>:<operacion>:<recurso>:<valor>:<comentario>
# ─────────────────────────────────────────────
def parsear_paquete(raw: str):
    """
    Devuelve un dict con los campos del paquete, o None si el formato es inválido.
    """
    raw = raw.strip()
    partes = raw.split(":")
    if len(partes) < 5:
        return None
    if partes[0] != "UABC":
        return None
    return {
        "raw":        raw,
        "usuario":    partes[1],
        "operacion":  partes[2].upper(),
        "recurso":    partes[3].upper(),
        "valor":      partes[4].strip() if len(partes) > 4 else "",
        "comentario": ":".join(partes[5:]).strip() if len(partes) > 5 else "",
    }

# ─────────────────────────────────────────────
#  Lógica de respuesta automática del servidor
# ─────────────────────────────────────────────
def procesar_paquete(sock, raw: str):
    global estado
	
    if raw.startswith("ACK"):
        log_info(f"ACK del ESP32: {raw}")
        return  # no responder nada
    if raw.startswith("NACK"):
        log_info(f"NACK del ESP32: {raw}")
        return  # no responder nada
	
    pkt = parsear_paquete(raw)

    if pkt is None:
        enviar(sock, "NACK:Formato invalido")
        return

    if pkt["usuario"] != USUARIO:
        enviar(sock, "NACK:Usuario no reconocido")
        return

    op  = pkt["operacion"]
    rec = pkt["recurso"]
    val = pkt["valor"]

    # ── Login ────────────────────────────────
    if op == "L" and rec == "S":
        estado["autenticado"] = True
        log_info("ESP32 autenticado correctamente.")
        enviar(sock, "ACK:Login OK")
        return

    # ── Keep-Alive ───────────────────────────
    if op == "K" and rec == "S":
        estado["ultimo_keepalive"] = time.time()
        log_info("Keep-Alive recibido.")
        enviar(sock, "ACK:Keep-Alive OK")
        return

    # ── Validar autenticación para cualquier otra operación ──
    if not estado["autenticado"]:
        enviar(sock, "NACK:No autenticado, realice Login primero")
        return

    # ── Escritura LED ────────────────────────
    if op == "W" and rec == "L":
        if val not in ("0", "1"):
            enviar(sock, "NACK:Valor invalido para LED (use 0 o 1)")
            return
        estado["led"] = int(val)
        enviar(sock, f"ACK:{estado['led']}")
        return

    # ── Lectura LED ──────────────────────────
    if op == "R" and rec == "L":
        enviar(sock, f"ACK:{estado['led']}")
        return

    # ── Lectura ADC ──────────────────────────
    if op == "R" and rec == "A":
        estado["adc"] = random.randint(0, 4095)   # simula lectura 12-bit
        enviar(sock, f"ACK:{estado['adc']}")
        return

    if op == "W" and rec == "A":
        enviar(sock, "NACK:Operacion W no permitida en ADC")
        return

    # ── Lectura PWM ──────────────────────────
    if op == "R" and rec == "P":
        enviar(sock, f"ACK:{estado['pwm']}")
        return

    # ── Escritura PWM ────────────────────────
    if op == "W" and rec == "P":
        try:
            v = int(val)
            if not (0 <= v <= 100):
                raise ValueError
        except ValueError:
            enviar(sock, "NACK:Valor invalido para PWM (0-100)")
            return
        estado["pwm"] = v
        enviar(sock, f"ACK:{estado['pwm']}")
        return

    # ── Comando no reconocido ─────────────────
    enviar(sock, "NACK:Comando no reconocido")

# ─────────────────────────────────────────────
#  Hilo receptor — corre en background
# ─────────────────────────────────────────────
def hilo_receptor(sock, addr):
    global cliente_socket, estado
    log_info(f"Conexión establecida desde {addr}")
    estado["conectado"] = True
    estado["autenticado"] = False

    try:
        buf = ""
        while True:
            data = sock.recv(1024)
            if not data:
                break
            buf += data.decode(errors="replace")
            # Procesar líneas completas (terminadas en \n)
            while "\n" in buf:
                linea, buf = buf.split("\n", 1)
                linea = linea.strip()
                if linea:
                    log_rx(linea)
                    procesar_paquete(sock, linea)
                    prompt()
    except ConnectionResetError:
        log_info("ESP32 cerró la conexión.")
    except Exception as e:
        log_err(f"Error en receptor: {e}")
    finally:
        sock.close()
        with cliente_lock:
            cliente_socket = None
        estado["conectado"] = False
        estado["autenticado"] = False
        log_info("Cliente desconectado.")
        prompt()

# ─────────────────────────────────────────────
#  Hilo aceptador — espera nuevas conexiones
# ─────────────────────────────────────────────
def hilo_aceptador(server_sock):
    global cliente_socket
    while True:
        try:
            conn, addr = server_sock.accept()
            with cliente_lock:
                if cliente_socket is not None:
                    log_info("Ya hay un cliente conectado. Rechazando nueva conexión.")
                    conn.close()
                    continue
                cliente_socket = conn
            t = threading.Thread(target=hilo_receptor, args=(conn, addr), daemon=True)
            t.start()
        except Exception as e:
            log_err(f"Error en aceptador: {e}")
            break

# ─────────────────────────────────────────────
#  Construcción de paquete saliente
# ─────────────────────────────────────────────
def construir_paquete(op, rec, valor="", comentario=""):
    return f"UABC:{USUARIO}:{op}:{rec}:{valor}:{comentario}"

# ─────────────────────────────────────────────
#  Enviar comando desde el menú
# ─────────────────────────────────────────────
def cmd_enviar(paquete):
    with cliente_lock:
        sock = cliente_socket
    if sock is None:
        log_err("No hay ningún ESP32 conectado.")
        return
    enviar(sock, paquete)

# ─────────────────────────────────────────────
#  Menú interactivo
# ─────────────────────────────────────────────
MENU = """
╔══════════════════════════════════════════════╗
║        RackIQ TCP Server  —  UABC            ║
║  Host: 0.0.0.0   Puerto: 5000                ║
║  Usuario: a1275863                           ║
╠══════════════════════════════════════════════╣
║  Comandos al ESP32:                          ║
║   1. Encender LED  (W:L:1)                   ║
║   2. Apagar LED    (W:L:0)                   ║
║   3. Leer LED      (R:L)                     ║
║   4. Leer ADC      (R:A)                     ║
║   5. Leer PWM      (R:P)                     ║
║   6. Escribir PWM  (W:P:<valor>)             ║
║   7. Paquete personalizado                   ║
╠══════════════════════════════════════════════╣
║   s. Ver estado simulado                     ║
║   q. Salir                                   ║
╚══════════════════════════════════════════════╝"""

def mostrar_estado():
    print(f"""
  ┌─ Estado del ESP32 ───────────────────────┐
  │  Conectado   : {'SÍ' if estado['conectado'] else 'NO'}
  │  Autenticado : {'SÍ' if estado['autenticado'] else 'NO'}
  │  LED         : {estado['led']}
  │  PWM         : {estado['pwm']}%
  │  ADC (último): {estado['adc']}
  │  Último KA   : {datetime.fromtimestamp(estado['ultimo_keepalive']).strftime('%H:%M:%S') if estado['ultimo_keepalive'] else 'nunca'}
  └──────────────────────────────────────────┘""")

def menu():
    print(MENU)
    while True:
        try:
            opc = input("\n> ").strip().lower()
        except (EOFError, KeyboardInterrupt):
            print("\nCerrando servidor...")
            break

        if opc == "1":
            cmd_enviar(construir_paquete("W", "L", "1", "Encender LED"))

        elif opc == "2":
            cmd_enviar(construir_paquete("W", "L", "0", "Apagar LED"))

        elif opc == "3":
            cmd_enviar(construir_paquete("R", "L", "", "Leer LED"))

        elif opc == "4":
            cmd_enviar(construir_paquete("R", "A", "", "Leer ADC"))

        elif opc == "5":
            cmd_enviar(construir_paquete("R", "P", "", "Leer PWM"))

        elif opc == "6":
            v = input("  Valor PWM (0-100): ").strip()
            try:
                vi = int(v)
                if not (0 <= vi <= 100):
                    raise ValueError
                cmd_enviar(construir_paquete("W", "P", str(vi), "Escribir PWM"))
            except ValueError:
                log_err("Valor inválido.")

        elif opc == "7":
            raw = input("  Paquete completo (UABC:...): ").strip()
            if raw:
                cmd_enviar(raw)

        elif opc == "s":
            mostrar_estado()

        elif opc == "q":
            print("Cerrando servidor...")
            break

        else:
            print("  Opción no válida. Usa 1-7, s o q.")

# ─────────────────────────────────────────────
#  Main
# ─────────────────────────────────────────────
def main():
    print(f"\n  Iniciando servidor TCP en {HOST}:{PORT} ...")
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    try:
        server.bind((HOST, PORT))
    except OSError as e:
        print(f"  Error al hacer bind en {HOST}:{PORT}: {e}")
        return

    server.listen(1)
    print(f"  Escuchando en 0.0.0.0:{PORT}  (tu IP: 192.168.1.66:{PORT})")
    print("  Esperando conexión del ESP32...\n")

    # Hilo aceptador en background
    t_acc = threading.Thread(target=hilo_aceptador, args=(server,), daemon=True)
    t_acc.start()

    # Menú en el hilo principal
    try:
        menu()
    finally:
        server.close()
        print("  Servidor cerrado.\n")

if __name__ == "__main__":
    main()