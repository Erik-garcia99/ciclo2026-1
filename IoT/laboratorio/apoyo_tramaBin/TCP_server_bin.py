"""
Servidor TCP - Protocolo Binario ESP32
======================================
Trama comando:
  [0xCA][0xFE][LEN][USER 4B][ACCION:4|RECURSO:4][VALOR 0-32B]

Trama ACK:
  [0x35][0x01][LEN][VALOR 0-32B]

Trama NACK:
  [0x35][0x01][0xFF]
"""

import socket
import struct
import threading
import time

# ─── Protocolo ────────────────────────────────────────────────────────────────
HEADER      = 0xCAFE
ACK_HEADER  = 0x3501

# Acciones (nibble alto)
ACTION = {
    "none"       : 0x0,
    "read"       : 0x1,
    "write"      : 0x2,
    "login"      : 0x4,
    "keep_alive" : 0x5,
}

# Recursos (nibble bajo)
RESOURCE = {
    "none"    : 0x0,
    "led"     : 0x1,
    "adc"     : 0x2,
    "pwm"     : 0x3,
    "server"  : 0xF,
}

USER = 0b100110111011110110111  # 1275863 - se actualiza cuando el cliente hace login

# ─── Construcción de tramas ───────────────────────────────────────────────────

def build_frame(action: str, resource: str, user: int, value: bytes = b"") -> bytes:
    """Construye una trama de comando."""
    action_res = (ACTION[action] << 4) | RESOURCE[resource]
    # LEN = usuario(4) + accion_recurso(1) + valor(N)
    length = 4 + 1 + len(value)
    frame = struct.pack(">HBIb", HEADER, length, user, action_res)
    frame += value
    return frame

def build_ack(value: bytes = b"") -> bytes:
    """Construye una trama ACK con valor opcional."""
    length = len(value)
    return struct.pack(">HB", ACK_HEADER, length) + value

def build_nack() -> bytes:
    """Construye una trama NACK."""
    return struct.pack(">HB", ACK_HEADER, 0xFF)

# ─── Parseo de trama recibida ─────────────────────────────────────────────────

def parse_frame(data: bytes) -> dict | None:
    if len(data) < 3:
        return None

    header = struct.unpack_from(">H", data, 0)[0]

    if header == HEADER:
        if len(data) < 8:
            return None
        length   = data[2]
        user     = struct.unpack_from(">I", data, 3)[0]
        ar_byte  = data[7]
        action   = (ar_byte >> 4) & 0x0F
        resource = ar_byte & 0x0F
        value_len = length - 5 if length > 5 else 0
        value    = data[8: 8 + value_len] if value_len > 0 else b""
        return {
            "type"    : "CMD",
            "header"  : f"0x{header:04X}",
            "length"  : length,
            "user"    : user,
            "action"  : action,
            "resource": resource,
            "value"   : value,
        }

    elif header == ACK_HEADER:
        length = data[2]
        if length == 0xFF:
            return {"type": "NACK"}
        value = data[3: 3 + length] if length > 0 else b""
        return {"type": "ACK", "value": value}

    return None

# ─── Nombres para debug ───────────────────────────────────────────────────────

ACTION_NAME   = {v: k for k, v in ACTION.items()}
RESOURCE_NAME = {v: k for k, v in RESOURCE.items()}

def print_frame(f: dict):
    if f["type"] == "CMD":
        a = ACTION_NAME.get(f["action"],   f"0x{f['action']:X}")
        r = RESOURCE_NAME.get(f["resource"], f"0x{f['resource']:X}")
        val_hex = f["value"].hex(" ").upper() if f["value"] else "(vacío)"
        print(f"\n  ┌── FRAME RECIBIDO ──────────────────")
        print(f"  │ header  : {f['header']}")
        print(f"  │ len     : {f['length']}")
        print(f"  │ user    : {f['user']}")
        print(f"  │ action  : {a}")
        print(f"  │ resource: {r}")
        print(f"  │ value   : {val_hex}")
        print(f"  └────────────────────────────────────")
    elif f["type"] == "ACK":
        val_hex = f["value"].hex(" ").upper() if f["value"] else "(vacío)"
        print(f"\n  [ACK] valor: {val_hex}")
    elif f["type"] == "NACK":
        print(f"\n  [NACK]")

# ─── Menú de operaciones ──────────────────────────────────────────────────────

MENU = """
╔══════════════════════════════════════════╗
║       SERVIDOR TCP - ESP32 LAB           ║
╠══════════════════════════════════════════╣
║  LECTURAS                                ║
║  1. Leer LED                             ║
║  2. Leer ADC                             ║
║  3. Leer PWM                             ║
╠══════════════════════════════════════════╣
║  ESCRITURAS                              ║
║  4. Escribir LED  (0 / 1)               ║
║  5. Escribir PWM  (0 - 8191)            ║
╠══════════════════════════════════════════╣
║  SERVIDOR                                ║
║  6. Enviar ACK                           ║
║  7. Enviar NACK                          ║
║  8. Enviar ACK con valor                 ║
╠══════════════════════════════════════════╣
║  0. Salir                                ║
╚══════════════════════════════════════════╝
"""

def handle_menu(conn, user: int):
    while True:
        print(MENU)
        op = input("  Opción: ").strip()

        if op == "0":
            break

        elif op == "1":
            frame = build_frame("read", "led", user)
            conn.sendall(frame)
            print(f"  → Enviado: {frame.hex(' ').upper()}")

        elif op == "2":
            frame = build_frame("read", "adc", user)
            conn.sendall(frame)
            print(f"  → Enviado: {frame.hex(' ').upper()}")

        elif op == "3":
            frame = build_frame("read", "pwm", user)
            conn.sendall(frame)
            print(f"  → Enviado: {frame.hex(' ').upper()}")

        elif op == "4":
            val = input("  Valor LED (0/1): ").strip()
            if val in ("0", "1"):
                frame = build_frame("write", "led", user, bytes([int(val)]))
                conn.sendall(frame)
                print(f"  → Enviado: {frame.hex(' ').upper()}")
            else:
                print("  ⚠ Valor inválido")

        elif op == "5":
            val = input("  Valor PWM (0-8191): ").strip()
            try:
                v = int(val)
                if 0 <= v <= 8191:
                    value_bytes = struct.pack(">H", v)  # big-endian 2 bytes
                    frame = build_frame("write", "pwm", user, value_bytes)
                    conn.sendall(frame)
                    print(f"  → Enviado: {frame.hex(' ').upper()}")
                else:
                    print("  ⚠ Fuera de rango")
            except ValueError:
                print("  ⚠ Valor inválido")

        elif op == "6":
            ack = build_ack()
            conn.sendall(ack)
            print(f"  → ACK enviado: {ack.hex(' ').upper()}")

        elif op == "7":
            nack = build_nack()
            conn.sendall(nack)
            print(f"  → NACK enviado: {nack.hex(' ').upper()}")

        elif op == "8":
            val = input("  Valor hex a enviar (ej: 01 FF A3): ").strip()
            try:
                value_bytes = bytes.fromhex(val.replace(" ", ""))
                ack = build_ack(value_bytes)
                conn.sendall(ack)
                print(f"  → ACK+valor enviado: {ack.hex(' ').upper()}")
            except ValueError:
                print("  ⚠ Hex inválido")

        else:
            print("  ⚠ Opción no reconocida")

# ─── Hilo receptor ────────────────────────────────────────────────────────────

def recv_loop(conn):
    global USER
    while True:
        try:
            data = conn.recv(256)
            if not data:
                print("\n  [!] Cliente desconectado")
                break

            raw_hex = data.hex(" ").upper()
            print(f"\n  RAW ← {raw_hex}")

            frame = parse_frame(data)
            if frame:
                print_frame(frame)
                # guardar user si es login
                if frame.get("type") == "CMD" and frame.get("action") == ACTION["login"]:
                    USER = frame["user"]
                    print(f"  [*] Usuario registrado: {USER}")
            else:
                print("  [!] Frame no reconocido")

        except Exception as e:
            print(f"\n  [!] Error recv: {e}")
            break

# ─── Main ─────────────────────────────────────────────────────────────────────

HOST = "0.0.0.0"
PORT = 5000

def main():
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind((HOST, PORT))
    srv.listen(1)
    print(f"\n  Servidor escuchando en {HOST}:{PORT} ...")

    while True:
        conn, addr = srv.accept()
        print(f"\n  [+] Conexión de {addr[0]}:{addr[1]}")

        # hilo que solo escucha
        t = threading.Thread(target=recv_loop, args=(conn,), daemon=True)
        t.start()

        # menú en el hilo principal
        try:
            handle_menu(conn, USER)
        except (BrokenPipeError, ConnectionResetError):
            print("\n  [!] Conexión perdida")
        finally:
            conn.close()
            print("\n  Esperando nueva conexión...")

if __name__ == "__main__":
    main()