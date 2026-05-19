#!/usr/bin/env python3
"""
TCP Server - Compatible con ESP32 (trama binaria)
==================================================
Protocolo big-endian definido en el proyecto:

  TRAMA REQUEST (ESP32 -> Servidor / Servidor -> ESP32):
  ┌─────────────┬──────────┬──────────┬─────────┬──────────┬────────────────┐
  │ Identificador│ Longitud │ Usuario  │ Acción  │ Recurso  │  <valor>       │
  │  2 bytes     │  1 byte  │  4 bytes │  4 bits │  4 bits  │  0..32 bytes   │
  │  0xCAFE      │          │          │   MSB   │   LSB    │                │
  └─────────────┴──────────┴──────────┴─────────┴──────────┴────────────────┘
  Acción+Recurso comparten 1 byte: (accion << 4) | recurso

  TRAMA ACK (Servidor -> ESP32):
  ┌─────────────┬──────────┬────────────────┐
  │ Identificador│ Longitud │  <valor>       │
  │  2 bytes     │  1 byte  │  0..32 bytes   │
  │  0x3501      │          │                │
  └─────────────┴──────────┴────────────────┘

  NACK: id=0x3501, len=0xFF (sin valor)

Tabla 1 - Acciones:
  0x0 = -           0x1 = Lectura (R)   0x2 = Escritura (W)
  0x3 = Cancelar(C) 0x4 = Acceso (L)    0x5 = Activo (K)

Tabla 2 - Recursos:
  0x0 = RESET   0x1 = LED (L)   0x2 = ADC (A)
  0x3 = PWM (P) 0xF = Servidor (S)

Uso:
  python tcp_server.py              # escucha en 0.0.0.0:50007
  python tcp_server.py -p 5000      # puerto personalizado
  python tcp_server.py --host 127.0.0.1 -p 5000
"""

import socket
import struct
import threading
import argparse
import time
import logging
from dataclasses import dataclass, field
from typing import Optional

# ─── Configuración de logging con colores ────────────────────────────────────

RESET  = "\033[0m"
RED    = "\033[31m"
GREEN  = "\033[32m"
YELLOW = "\033[33m"
BLUE   = "\033[34m"
MAGENTA= "\033[35m"
CYAN   = "\033[36m"
WHITE  = "\033[37m"

class ColorFormatter(logging.Formatter):
    COLORS = {
        logging.DEBUG:    CYAN,
        logging.INFO:     GREEN,
        logging.WARNING:  YELLOW,
        logging.ERROR:    RED,
        logging.CRITICAL: MAGENTA,
    }
    def format(self, record):
        color = self.COLORS.get(record.levelno, RESET)
        record.msg = f"{color}{record.msg}{RESET}"
        return super().format(record)

handler = logging.StreamHandler()
handler.setFormatter(ColorFormatter("[%(asctime)s] %(levelname)s | %(message)s", "%H:%M:%S"))
log = logging.getLogger("TCP_SERVER")
log.addHandler(handler)
log.setLevel(logging.DEBUG)

# ─── Constantes del protocolo ─────────────────────────────────────────────────

HEADER = 0xCAFE   # identificador de trama normal
ACK_ID = 0x3501   # identificador de ACK/NACK

# Acciones (nibble alto del byte acción+recurso)
ACTION = {
    0x0: "none",
    0x1: "read",
    0x2: "write",
    0x3: "cancel",
    0x4: "access",   # login
    0x5: "keepalive",
}
ACTION_REV = {v: k for k, v in ACTION.items()}

# Recursos (nibble bajo del byte acción+recurso)
RESOURCE = {
    0x0: "reset",
    0x1: "led",
    0x2: "adc",
    0x3: "pwm",
    0xF: "server",
}
RESOURCE_REV = {v: k for k, v in RESOURCE.items()}

# ─── Estado simulado de los ESP32 conectados ─────────────────────────────────

@dataclass
class ESPState:
    """Estado virtual de un ESP32 conectado."""
    user: int = 0
    logged_in: bool = False
    led: int = 0           # 0 o 1
    adc: int = 2048        # valor simulado 0-4095
    pwm: int = 0           # duty 0-8191
    reset_timer: int = 0   # segundos para reset (0 = no programado)
    addr: tuple = field(default_factory=tuple)

# Usuarios registrados: matrícula (uint32) -> nombre
KNOWN_USERS = {
    0x001377d7: "a1275863",   # usuario del ESP32 de ejemplo
    0xDEADBEEF: "test_user",
}

# ─── Utilidades de protocolo ──────────────────────────────────────────────────

def build_ack(value: bytes = b"") -> bytes:
    """Construye una trama ACK con valor opcional."""
    assert len(value) <= 32, "value no puede superar 32 bytes"
    return struct.pack(">HB", ACK_ID, len(value)) + value

def build_nack() -> bytes:
    """Construye una trama NACK."""
    return struct.pack(">HB", ACK_ID, 0xFF)

def build_request(user: int, action: int, resource: int, value: bytes = b"") -> bytes:
    """
    Construye una trama de request/comando (0xCAFE) para enviar al ESP32.
    Longitud = 4 (user) + 1 (accion+recurso) + len(value)
    """
    assert len(value) <= 32
    length = 5 + len(value)   # user(4) + accion_recurso(1) + valor
    ar_byte = ((action & 0x0F) << 4) | (resource & 0x0F)
    frame = struct.pack(">HBIb", HEADER, length, user, ar_byte) + value
    # Rehacer sin signo para ar_byte
    frame = struct.pack(">HBI", HEADER, length, user)
    frame += bytes([ar_byte])
    frame += value
    return frame

def parse_frame(data: bytes):
    """
    Parsea los bytes recibidos y devuelve un dict con los campos del frame,
    o None si el frame es inválido.
    """
    if len(data) < 3:
        return None

    frame_id = struct.unpack_from(">H", data, 0)[0]

    # ── ACK / NACK ────────────────────────────────────────────────────────────
    if frame_id == ACK_ID:
        length = data[2]
        result = {"id": frame_id, "len": length, "type": "nack" if length == 0xFF else "ack"}
        if length != 0xFF and length > 0 and len(data) >= 3 + length:
            result["value"] = data[3:3+length]
        else:
            result["value"] = b""
        return result

    # ── Trama normal (CAFE) ───────────────────────────────────────────────────
    if frame_id == HEADER:
        if len(data) < 8:   # mínimo: id(2)+len(1)+user(4)+ar(1)
            return None
        length  = data[2]
        user    = struct.unpack_from(">I", data, 3)[0]
        ar_byte = data[7]
        action_nibble   = (ar_byte >> 4) & 0x0F
        resource_nibble = ar_byte & 0x0F

        value_len = length - 5 if length > 5 else 0
        value_len = min(value_len, 32)
        value = data[8:8+value_len] if value_len > 0 else b""

        return {
            "id":       frame_id,
            "type":     "request",
            "len":      length,
            "user":     user,
            "action":   action_nibble,
            "resource": resource_nibble,
            "value":    value,
            "action_name":   ACTION.get(action_nibble, "?"),
            "resource_name": RESOURCE.get(resource_nibble, "?"),
        }

    return None   # identificador desconocido

def hex_dump(data: bytes) -> str:
    return " ".join(f"{b:02X}" for b in data)

# ─── Hilo por cliente ─────────────────────────────────────────────────────────

class ClientHandler(threading.Thread):
    def __init__(self, conn: socket.socket, addr, server: "TCPServer"):
        super().__init__(daemon=True)
        self.conn     = conn
        self.addr     = addr
        self.server   = server
        self.state    = ESPState(addr=addr)
        self.running  = True
        self.last_cmd = {"action": None, "resource": None}  # último comando enviado

    def send(self, data: bytes, action_str=None, resource_str=None):
        if action_str:
            self.last_cmd = {"action": action_str, "resource": resource_str}
        log.debug(f"[{self.addr}] TX → {hex_dump(data)}")
        try:
            self.conn.sendall(data)
        except Exception as e:
            log.error(f"[{self.addr}] Error al enviar: {e}")

    def recv_all(self) -> Optional[bytes]:
        """Lee hasta 40 bytes del socket."""
        try:
            data = self.conn.recv(40)
            return data if data else None
        except Exception:
            return None

    def handle_frame(self, frame: dict):
        ftype = frame.get("type")

        # ── ACK / NACK del ESP32 ──────────────────────────────────────────────
        if ftype in ("ack", "nack"):
            label = "ACK" if ftype == "ack" else "NACK"
            val  = frame.get("value", b"")
            res  = self.last_cmd.get("resource")
            act  = self.last_cmd.get("action")

            if not val:
                val_str = "(sin valor)"

            elif res == "led" and len(val) >= 1:
                estado = "ON" if val[0] else "OFF"
                val_str = f"LED = {estado} ({val[0]})"

            elif res == "pwm" and act == "read" and len(val) >= 1:
                # ESP32 ahora devuelve porcentaje en 1 byte
                val_str = f"PWM = {val[0]}%"

            elif res == "adc" and len(val) == 2:
                raw = struct.unpack_from(">H", val)[0]
                voltaje = round(raw * 3.3 / 4095, 3)
                val_str = f"ADC = {raw}  ({voltaje} V)"

            elif res == "reset" and act == "read" and len(val) >= 1:
                val_str = f"RESET timer = {val[0]}s"

            elif len(val) == 1:
                val_str = f"0x{val[0]:02X} = {val[0]}"

            elif len(val) == 2:
                num = struct.unpack_from(">H", val)[0]
                val_str = f"0x{val[0]:02X}{val[1]:02X} = {num}"

            else:
                val_str = hex_dump(val)

            log.info(f"[{self.addr}] ESP32 respondió {label}  →  {val_str}")
            self.last_cmd = {"action": None, "resource": None}  # limpiar
            return

        # ── Trama CAFE ────────────────────────────────────────────────────────
        if ftype != "request":
            log.warning(f"[{self.addr}] Frame desconocido, enviando NACK")
            self.send(build_nack())
            return

        action   = frame["action"]
        resource = frame["resource"]
        user     = frame["user"]
        value    = frame["value"]

        log.info(
            f"[{self.addr}] RX ← user=0x{user:08X}  "
            f"acción={ACTION.get(action,'?')}(0x{action:X})  "
            f"recurso={RESOURCE.get(resource,'?')}(0x{resource:X})  "
            f"valor={hex_dump(value) if value else '(vacío)'}"
        )

        # ── LOGIN (access, server) ────────────────────────────────────────────
        if action == 0x4 and resource == 0xF:
            if user in KNOWN_USERS:
                self.state.user = user
                self.state.logged_in = True
                log.info(f"[{self.addr}] LOGIN OK → {KNOWN_USERS[user]} (0x{user:08X})")
                self.send(build_ack())
            else:
                log.warning(f"[{self.addr}] LOGIN FAIL → usuario 0x{user:08X} desconocido")
                self.send(build_nack())
            return

        # ── KEEP-ALIVE ────────────────────────────────────────────────────────
        if action == 0x5 and resource == 0xF:
            log.info(f"[{self.addr}] KEEP-ALIVE de 0x{user:08X}")
            self.send(build_ack())
            return

        # Para cualquier otro comando, el ESP32 debe estar logueado
        if not self.state.logged_in:
            log.warning(f"[{self.addr}] Comando sin login, enviando NACK")
            self.send(build_nack())
            return

        # ── LECTURA ───────────────────────────────────────────────────────────
        if action == 0x1:
            if resource == 0x1:   # LED
                resp = bytes([self.state.led])
                log.info(f"[{self.addr}] READ LED → {self.state.led}")
                self.send(build_ack(resp))

            elif resource == 0x2:   # ADC
                adc_val = struct.pack(">H", self.state.adc)
                log.info(f"[{self.addr}] READ ADC → {self.state.adc}")
                self.send(build_ack(adc_val))

            elif resource == 0x3:   # PWM (1 byte de porcentaje 0-100)
                pwm_val = bytes([self.state.pwm & 0xFF])
                log.info(f"[{self.addr}] READ PWM → {self.state.pwm}%")
                self.send(build_ack(pwm_val))

            elif resource == 0x0:   # RESET timer
                resp = bytes([self.state.reset_timer])
                log.info(f"[{self.addr}] READ RESET timer → {self.state.reset_timer}s")
                self.send(build_ack(resp))

            else:
                log.warning(f"[{self.addr}] READ recurso desconocido 0x{resource:X}")
                self.send(build_nack())

        # ── ESCRITURA ─────────────────────────────────────────────────────────
        elif action == 0x2:
            if resource == 0x1:   # LED
                if len(value) >= 1:
                    self.state.led = value[0] & 0x1
                    log.info(f"[{self.addr}] WRITE LED ← {self.state.led}")
                    self.send(build_ack(bytes([self.state.led])))
                else:
                    self.send(build_nack())

            elif resource == 0x3:   # PWM (porcentaje 0-100)
                if len(value) >= 1:
                    pct = min(value[0], 100)
                    self.state.pwm = pct   # guardamos porcentaje directamente
                    log.info(f"[{self.addr}] WRITE PWM ← {pct}%")
                    self.send(build_ack())
                else:
                    self.send(build_nack())

            elif resource == 0x2:   # ADC (solo lectura)
                log.warning(f"[{self.addr}] WRITE ADC no permitido → NACK")
                self.send(build_nack())

            elif resource == 0x0:   # RESET con timer (1 byte = segundos)
                if len(value) >= 1:
                    secs = value[0]
                    self.state.reset_timer = secs
                    log.info(f"[{self.addr}] WRITE RESET ← en {secs}s")
                    self.send(build_ack())
                    # Simular reset después de N segundos (solo log)
                    def _simulate_reset(s):
                        time.sleep(s)
                        log.warning(f"[{self.addr}] >>> RESET simulado del ESP32 <<<")
                        self.state.reset_timer = 0
                        self.state.logged_in = False
                    threading.Thread(target=_simulate_reset, args=(secs,), daemon=True).start()
                else:
                    self.send(build_nack())

            else:
                log.warning(f"[{self.addr}] WRITE recurso desconocido 0x{resource:X}")
                self.send(build_nack())

        # ── CANCELAR ──────────────────────────────────────────────────────────
        elif action == 0x3:
            if resource == 0x0:   # Cancelar RESET
                self.state.reset_timer = 0
                log.info(f"[{self.addr}] CANCEL RESET")
                self.send(build_ack())
            else:
                log.warning(f"[{self.addr}] CANCEL recurso desconocido 0x{resource:X}")
                self.send(build_nack())

        else:
            log.warning(f"[{self.addr}] Acción desconocida 0x{action:X} → NACK")
            self.send(build_nack())

    def run(self):
        log.info(f"[{self.addr}] Cliente conectado")
        try:
            while self.running:
                data = self.recv_all()
                if not data:
                    log.info(f"[{self.addr}] Cliente desconectado")
                    break
                log.debug(f"[{self.addr}] RX raw: {hex_dump(data)}")
                frame = parse_frame(data)
                if frame is None:
                    log.warning(f"[{self.addr}] Frame inválido → NACK")
                    self.send(build_nack())
                else:
                    self.handle_frame(frame)
        finally:
            self.conn.close()
            self.server.remove_client(self)

# ─── Consola de comandos (enviar tramas al ESP32) ─────────────────────────────

class CommandConsole(threading.Thread):
    """
    Consola con menú numerado para no interferir con los logs del keep-alive.
    Se selecciona primero la operación, luego el cliente si hay más de uno.
    """
    def __init__(self, server: "TCPServer"):
        super().__init__(daemon=True)
        self.server = server

    # ── Menú principal ────────────────────────────────────────────────────────
    MENU = [
        # (etiqueta,          accion_str,  recurso_str, necesita_valor)
        ("Leer LED",          "read",      "led",        False),
        ("Leer ADC",          "read",      "adc",        False),
        ("Leer PWM",          "read",      "pwm",        False),
        ("Leer timer RESET",  "read",      "reset",      False),
        ("Escribir LED",      "write",     "led",        True ),
        ("Escribir PWM (%)",  "write",     "pwm",        True ),
        ("Programar RESET",   "write",     "reset",      True ),
        ("Cancelar RESET",    "cancel",    "reset",      False),
        ("Listar clientes",   None,        None,         False),
        ("Salir",             None,        None,         False),
    ]

    def print_menu(self):
        print(f"\n{CYAN}{'─'*42}")
        print(f"  MENÚ DE OPERACIONES")
        print(f"{'─'*42}{RESET}")
        for i, (label, *_) in enumerate(self.MENU, 1):
            print(f"  {MAGENTA}{i:2}{RESET}. {label}")
        print(f"{CYAN}{'─'*42}{RESET}")

    def pick_client(self):
        """Devuelve un ClientHandler o None."""
        clients = self.server.clients
        if not clients:
            print(f"{YELLOW}  No hay clientes conectados.{RESET}")
            return None
        if len(clients) == 1:
            c = clients[0]
            status = f"{GREEN}logueado{RESET}" if c.state.logged_in else f"{YELLOW}sin login{RESET}"
            print(f"  Cliente: {c.addr}  user=0x{c.state.user:08X}  {status}")
            return c
        # Más de uno → elegir
        print(f"\n{CYAN}  Clientes conectados:{RESET}")
        for i, c in enumerate(clients, 1):
            status = f"{GREEN}logueado{RESET}" if c.state.logged_in else f"{YELLOW}sin login{RESET}"
            print(f"  {MAGENTA}{i}{RESET}. {c.addr}  user=0x{c.state.user:08X}  {status}")
        try:
            idx = int(input(f"  Elige cliente (1-{len(clients)}): ").strip()) - 1
            return clients[idx]
        except (ValueError, IndexError):
            print(f"{RED}  Índice inválido.{RESET}")
            return None

    def ask_value(self, resource_str: str) -> Optional[bytes]:
        hints = {"led": "0 o 1", "pwm": "0-100 (%)", "reset": "segundos (0-255)"}
        hint  = hints.get(resource_str, "valor numérico")
        try:
            raw = input(f"  Valor ({hint}): ").strip()
            val = int(raw)
            return bytes([val & 0xFF])
        except ValueError:
            print(f"{RED}  Valor inválido.{RESET}")
            return None

    def run(self):
        time.sleep(0.5)
        while True:
            self.print_menu()
            try:
                raw = input(f"  Opción: ").strip()
            except (EOFError, KeyboardInterrupt):
                self.server.stop()
                break

            if not raw.isdigit():
                continue

            idx = int(raw) - 1
            if idx < 0 or idx >= len(self.MENU):
                print(f"{RED}  Opción fuera de rango.{RESET}")
                continue

            label, action_str, resource_str, needs_val = self.MENU[idx]

            # ── Listar ────────────────────────────────────────────────────────
            if label == "Listar clientes":
                clients = self.server.clients
                if not clients:
                    print(f"{YELLOW}  No hay clientes conectados.{RESET}")
                else:
                    for i, c in enumerate(clients, 1):
                        status = f"{GREEN}logueado{RESET}" if c.state.logged_in else f"{YELLOW}sin login{RESET}"
                        print(f"  {i}. {c.addr}  user=0x{c.state.user:08X}  {status}")
                continue

            # ── Salir ─────────────────────────────────────────────────────────
            if label == "Salir":
                self.server.stop()
                break

            # ── Operación → elegir cliente ────────────────────────────────────
            client = self.pick_client()
            if client is None:
                continue

            if not client.state.logged_in:
                print(f"{YELLOW}  Advertencia: el cliente no ha hecho login.{RESET}")

            # ── Valor si es escritura ─────────────────────────────────────────
            value = b""
            if needs_val:
                value = self.ask_value(resource_str)
                if value is None:
                    continue

            # ── Construir y enviar ────────────────────────────────────────────
            action   = ACTION_REV[action_str]
            resource = RESOURCE_REV[resource_str]
            frame    = build_request(client.state.user, action, resource, value)

            val_str = hex_dump(value) if value else "(sin valor)"
            print(f"{GREEN}  → Enviando: {label}  {val_str}{RESET}")
            client.send(frame, action_str=action_str, resource_str=resource_str)

# ─── Servidor TCP ─────────────────────────────────────────────────────────────

class TCPServer:
    def __init__(self, host: str = "0.0.0.0", port: int = 50007):
        self.host    = host
        self.port    = port
        self.clients: list[ClientHandler] = []
        self.lock    = threading.Lock()
        self._stop   = threading.Event()

    def remove_client(self, client: ClientHandler):
        with self.lock:
            if client in self.clients:
                self.clients.remove(client)
                log.info(f"[{client.addr}] Eliminado de la lista de clientes")

    def stop(self):
        self._stop.set()
        log.info("Deteniendo servidor...")

    def run(self):
        srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        srv.bind((self.host, self.port))
        srv.listen(5)
        srv.settimeout(1.0)

        log.info(f"Servidor escuchando en {self.host}:{self.port}")
        log.info(f"Usuarios registrados: { {hex(k):v for k,v in KNOWN_USERS.items()} }")

        console = CommandConsole(self)
        console.start()

        try:
            while not self._stop.is_set():
                try:
                    conn, addr = srv.accept()
                except socket.timeout:
                    continue
                handler = ClientHandler(conn, addr, self)
                with self.lock:
                    self.clients.append(handler)
                handler.start()
        finally:
            srv.close()
            log.info("Servidor cerrado.")

# ─── Entry point ─────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(description="TCP Server para ESP32 (trama binaria)")
    parser.add_argument("--host", default="0.0.0.0", help="Dirección de escucha (default: 0.0.0.0)")
    parser.add_argument("-p", "--port", type=int, default=50007, help="Puerto (default: 50007)")
    parser.add_argument("--add-user", metavar="HEX:NAME", action="append",
                        help="Agregar usuario: --add-user 0xABCD1234:nombre")
    args = parser.parse_args()

    if args.add_user:
        for entry in args.add_user:
            try:
                hex_val, name = entry.split(":", 1)
                KNOWN_USERS[int(hex_val, 16)] = name
                log.info(f"Usuario agregado: {name} → {hex_val}")
            except Exception as e:
                log.warning(f"Formato incorrecto para --add-user '{entry}': {e}")

    server = TCPServer(host=args.host, port=args.port)
    try:
        server.run()
    except KeyboardInterrupt:
        server.stop()

if __name__ == "__main__":
    main()