import socket
import threading
import sys
import signal

HOST = '0.0.0.0'
PORT = 5000

stop_event = threading.Event()

def handle_client(conn, addr):
    print(f"\n[+] Nueva conexión desde {addr}")
    buffer = b""
    try:
        while not stop_event.is_set():
            # Configuramos timeout para poder salir del hilo
            conn.settimeout(1.0)
            try:
                data = conn.recv(1024)
            except socket.timeout:
                continue
            if not data:
                break
            # Mostrar representación exacta (bytes)
            print(f"[RECV {addr}] {data!r}")
            # Responder OK para que el ESP no se bloquee (aunque no lea)
            conn.sendall(b"OK\n")
    except Exception as e:
        print(f"[!] Error con {addr}: {e}")
    finally:
        conn.close()
        print(f"[-] Desconectado {addr}")

def main():
    # Capturar Ctrl+C para detener limpiamente
    def signal_handler(sig, frame):
        print("\n[!] Deteniendo servidor...")
        stop_event.set()
        sys.exit(0)
    signal.signal(signal.SIGINT, signal_handler)

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind((HOST, PORT))
        s.listen(5)
        s.settimeout(1.0)  # Para poder evaluar stop_event
        print(f"Servidor DEBUG escuchando en {HOST}:{PORT}")
        print("Presiona Ctrl+C para salir.\n")
        try:
            while not stop_event.is_set():
                try:
                    conn, addr = s.accept()
                except socket.timeout:
                    continue
                t = threading.Thread(target=handle_client, args=(conn, addr), daemon=True)
                t.start()
        except KeyboardInterrupt:
            pass
        finally:
            print("Servidor detenido.")

if __name__ == "__main__":
    main()